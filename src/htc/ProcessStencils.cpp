/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "HtSageUtils.h"
#include "Normalize.h"
#include "HtcAttributes.h"
#include "HtdInfoAttribute.h"
#include "HtSCDecls.h"
#include "ProcessStencils.h"


#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 

extern bool htc_do_split;
 
struct fceInfo {
  SgFunctionCallExp *fce;
  std::string calleeName;
  std::vector<SgExpression *> *calleeArgs;
  SgType *stencilType;
  int stencilDimx;
  int stencilDimy;
  int stencilOrgx;
  int stencilOrgy;
  int pipeLen;
  bool useStreaming;
  bool genHostCode;
  SgClassDeclaration *CCoefDecl;
};

static SgFunctionDeclaration *buildEmptyFunction(fceInfo &fi);
static void buildStencilCode(fceInfo &fi,
                             SgFunctionDeclaration *newFD);


static void reportError(std::string msg, SgNode *n) {
  std::cerr << "HTC ERROR:: " << msg << std::endl;
  std::cerr << n->get_file_info()->get_filenameString() << " line " <<
      n->get_file_info()->get_line() << ":\n   ";
  std::cerr << n->unparseToString() << std::endl;
}


static int getIntVal(SgExpression *exp) {
  int retval = 0;
  SgIntVal *intExp = isSgIntVal(exp);
  if (!intExp) {
    reportError("parameter must be constant", exp);
    exit(2);
  } else {
    retval = intExp->get_value();
  }
  return retval;
}


 
//-------------------------------------------------------------------------
// Generate stencil functions corresponding to rhomp_stencil intrinsincs.
//
// This pass rewrites all rhomp_stencil intrinsic calls.  Each call will
// induce a stencil function to be "outlined" and corresponding stencil
// code for those particular parameters to be generated.  By default, an
// optimized streaming implementation will be generated.  Alternatively, if
// the user requests it, we will generate a generic stencil loop nest that
// can be used for testing or host code generation.
//
// Stencil intrinsics:
//   void rhomp_stencil_conv2d (void *grid_dst, void *grid_src,
//                              int grid_dimx, int grid_dimy,
//                              int stencil_dimx, int stencil_dimy, 
//                              int stencil_orgx, int stencil_orgy,
//                              void *kernel, int pipelen);
//   void rhomp_stencil_conv2ds (void *grid_dst, void *grid_src,
//                              int grid_dimx, int grid_dimy,
//                              int stencil_dim, void *kernel, 
//                              int pipelen);
//
// The grid dimensions include border elements (if any).
// pipelen < 4096: Generate optimized coproc streaming stencil code.
// pipelen = 4096: Generate generic coproc stencil loop nest code.
// pipelen = 8192: Generate generic host stencil loop nest code.
//-------------------------------------------------------------------------
void ProcessStencils(SgProject *project)
{
  class Visitor : public AstSimpleProcessing {
  public:
    void visit(SgNode *S) {
      if (SgFunctionCallExp *FCE = isSgFunctionCallExp(S)) {
        SgFunctionDeclaration *calleeFD = 
            FCE->getAssociatedFunctionDeclaration();
        if (calleeFD) {
          std::string calleeName = calleeFD->get_name().getString();
          if (calleeName == "rhomp_stencil_conv2d"
              || calleeName == "rhomp_stencil_conv2ds") {
            finfo.fce = FCE;
            finfo.calleeName = calleeName;
            finfo.calleeArgs = &FCE->get_args()->get_expressions();
            std::vector<SgExpression *> &vv = *finfo.calleeArgs;
#if 1
            // Determine base type of source.
            SgExpression *gridSrcExp = HtSageUtils::SkipCasting(vv[1]);
            SgType *srcBaseType = 0;
            if (isSgArrayType(gridSrcExp->get_type())) {
              srcBaseType = 
                  SageInterface::getArrayElementType(gridSrcExp->get_type());
            } else {
              srcBaseType = 
                  SageInterface::getElementType(gridSrcExp->get_type());
            }
            assert(SageInterface::isScalarType(srcBaseType));
            finfo.stencilType = srcBaseType;
            // TODO: Check that source, dest, and kernel base types match.
#endif
            if (calleeName == "rhomp_stencil_conv2d") {
              finfo.stencilDimx = getIntVal(vv[4]);
              finfo.stencilDimy = getIntVal(vv[5]);
              finfo.stencilOrgx = getIntVal(vv[6]);
              finfo.stencilOrgy = getIntVal(vv[7]);
              finfo.pipeLen = getIntVal(vv[9]);
            } else if (calleeName == "rhomp_stencil_conv2ds") {
              finfo.stencilDimx = getIntVal(vv[4]);
              if ((finfo.stencilDimx % 2) == 0) {
                reportError("simple stencil dimension must be odd", vv[4]);
                exit(2);
              }
              finfo.stencilDimy = finfo.stencilDimx;
              finfo.stencilOrgx = finfo.stencilDimx / 2;
              finfo.stencilOrgy = finfo.stencilOrgx;
              finfo.pipeLen = getIntVal(vv[6]);
            }
            finfo.genHostCode = (finfo.pipeLen == 8192);
            finfo.useStreaming = (!finfo.genHostCode && finfo.pipeLen != 4096);
            finfo.CCoefDecl = 0;
            if (htc_do_split && !finfo.genHostCode) {
              return;
            }
            processList.push_back(finfo);
          }
        } 
      }
    }
    fceInfo finfo;  
    std::vector<fceInfo> processList;
  } v;

  // Discover and record all rhomp_stencil calls.
  v.traverseInputFiles(project, postorder);

  // Generate stencil functions for each rhomp_stencil call, and rewrite
  // the old call to invoke the corresponding new stencil function.
  foreach (fceInfo fi, v.processList) {
    // Build the new outlined function skeleton.
    SgFunctionDeclaration *newFD = buildEmptyFunction(fi);

    // Build the function body with one of three code generation schemes.
    buildStencilCode(fi, newFD);

    // Rewrite the old call.
    SgFunctionDefinition *callerFdef =
        SageInterface::getEnclosingFunctionDefinition(fi.fce);
    SgBasicBlock *callerBody = callerFdef->get_body();
    std::vector<SgExpression *> &vv = *fi.calleeArgs;

    SgExpression *kernExp = 
        vv[fi.calleeName == "rhomp_stencil_conv2d" ? 8 : 5];
    // The streaming kernel wants cols/rows passed WITHOUT border widths.
    SgExpression *colsExp = 0, *rowsExp = 0;
    if (fi.useStreaming) {
      colsExp = SageBuilder::buildSubtractOp(
          SageInterface::deepCopy(vv[2]),
          SageBuilder::buildIntVal(fi.stencilDimx-1));
      rowsExp = SageBuilder::buildSubtractOp(
          SageInterface::deepCopy(vv[3]),
          SageBuilder::buildIntVal(fi.stencilDimy-1));
    } else {
      colsExp = SageInterface::deepCopy(vv[2]);
      rowsExp = SageInterface::deepCopy(vv[3]);
    }

    // The streaming kernel needs a CCoef struct instead of a kernel pointer.
    if (fi.useStreaming) {
      // Insert a local declaration of a CCoef struct followed by
      // assignments to copy every element of k to the struct.
      SgName sname = "__htc_coef";
      sname << ++SageInterface::gensym_counter;
      SgVariableDeclaration *localCoefDecl =
          SageBuilder::buildVariableDeclaration(
              sname, fi.CCoefDecl->get_type(), 0, callerFdef);
      SageInterface::insertStatementBefore(
          SageInterface::getEnclosingStatement(fi.fce), localCoefDecl);
      for (int y = 0; y < fi.stencilDimy; y++) {
        for (int x = 0; x < fi.stencilDimx; x++) {
          // __htc_coef.m_coef[y][x] = k[y*stencilDimx+x]; 
          SgCastExp *uuu = isSgCastExp(kernExp);
          assert(uuu);
          SgExpression *actualKern = 
              SageInterface::deepCopy(uuu->get_operand());
          SgPntrArrRefExp *kref = SageBuilder::buildPntrArrRefExp(
              actualKern,
              SageBuilder::buildAddOp(
                  SageBuilder::buildMultiplyOp(
                      SageBuilder::buildIntVal(y), 
                      SageBuilder::buildIntVal(fi.stencilDimx)),
                  SageBuilder::buildIntVal(x)));
          SgPntrArrRefExp *member = SageBuilder::buildPntrArrRefExp(
              SageBuilder::buildPntrArrRefExp(
                  SageBuilder::buildVarRefExp("m_coef",
                      isSgScopeStatement(fi.CCoefDecl->get_definition())),
                  SageBuilder::buildIntVal(y)), 
              SageBuilder::buildIntVal(x));
          SgExpression *coefRef = SageBuilder::buildDotExp(
              SageBuilder::buildVarRefExp(localCoefDecl), member);
          SgStatement *asg = SageBuilder::buildAssignStatement(coefRef, kref);
          SageInterface::insertStatementBefore(
              SageInterface::getEnclosingStatement(fi.fce), asg);
        }
      }
      kernExp = SageBuilder::buildVarRefExp(localCoefDecl);
    }

    SgFunctionCallExp *newFCE = SageBuilder::buildFunctionCallExp(
        newFD->get_name(),
        SageBuilder::buildVoidType(),
        SageBuilder::buildExprListExp(
            SageInterface::deepCopy(vv[0]),
            SageInterface::deepCopy(vv[1]),
            colsExp,
            rowsExp,
            kernExp), 
        callerBody);
    SageInterface::replaceExpression(fi.fce, newFCE, true /* keepOldExp */);
  }
}


// Build an empty stencil function skeleton.
static SgFunctionDeclaration *buildEmptyFunction(fceInfo &fi) {
  SgFunctionCallExp *fce = fi.fce;
  SgGlobal *gs = SageInterface::getGlobalScope(fce); 
  SgName funcName = (fi.useStreaming ? StencilStreamPrefix : StencilPrefix);
  funcName << ++SageInterface::gensym_counter;

  // Create formal parameter list for new function:
  //   (MemAddr_t wrAddr, MemAddr_t rdAddr, uint32_t stRows, uint32_t stCols);
  SgType *ptrType = 0;
  if (fi.useStreaming) {
    ptrType = SageBuilder::buildOpaqueType("MemAddr_t", gs);
  } else {
    ptrType = SageBuilder::buildPointerType(fi.stencilType);
  }
  SgType *uintType = SageBuilder::buildOpaqueType("uint32_t", gs);
#if 1
  // Build the CCoef struct decl.
  std::string structName = "CCoef" + funcName;
  SgClassDeclaration *structDecl = 
      SageBuilder::buildStructDeclaration(structName, gs);
  SgClassDefinition *structDef = 
      SageBuilder::buildClassDefinition(structDecl);
  SgType *atypeCols = SageBuilder::buildArrayType(
      fi.stencilType,
      SageBuilder::buildIntVal(fi.stencilDimx)); 
  SgType *atype = SageBuilder::buildArrayType(
      atypeCols,
      SageBuilder::buildIntVal(fi.stencilDimy)); 
  structDef->append_member(
      SageBuilder::buildVariableDeclaration("m_coef", atype, 0, structDef));
  SageInterface::prependStatement(structDecl, gs);
  structDecl->unsetOutputInCodeGeneration();
  fi.CCoefDecl = structDecl;
#endif

  SgFunctionParameterList *parms = SageBuilder::buildFunctionParameterList();
  SageInterface::appendArg(parms,
    SageBuilder::buildInitializedName("wrAddr", ptrType));
  SageInterface::appendArg(parms,
    SageBuilder::buildInitializedName("rdAddr", ptrType));
  SageInterface::appendArg(parms,
    SageBuilder::buildInitializedName("cols", uintType));
  SageInterface::appendArg(parms,
    SageBuilder::buildInitializedName("rows", uintType));
  if (fi.useStreaming) {
    SageInterface::appendArg(parms, 
        SageBuilder::buildInitializedName("coef", fi.CCoefDecl->get_type()));
  } else {
    SageInterface::appendArg(parms, 
        SageBuilder::buildInitializedName("k", ptrType));
  } 

  // Create a defining function declaration.
  SgFunctionDeclaration *fdecl = SageBuilder::buildDefiningFunctionDeclaration(
      funcName, SageBuilder::buildVoidType(), parms, gs);
  SgBasicBlock *funcBody = fdecl->get_definition()->get_body();

  // Insert new function into global scope.
  SageInterface::prependStatement(fdecl, gs);

  return fdecl;
}


static void buildStencilCode(fceInfo &fi, SgFunctionDeclaration *newFD)
{
  SgFunctionCallExp *fce = fi.fce;

  // Add an HtdInfoAttribute for the new function.
  SgFunctionDefinition *fdef = newFD->get_definition();
  HtdInfoAttribute *htd = new HtdInfoAttribute();
  fdef->addNewAttribute("htdinfo", htd);

  SgBasicBlock *funcBody = fdef->get_body(); 
  SgGlobal *gs = SageInterface::getGlobalScope(fce); 
  std::vector<SgExpression *> &vv = *fi.calleeArgs;

  int stencilDimx = fi.stencilDimx, stencilDimy = fi.stencilDimy,
      stencilOrgx = fi.stencilOrgx, stencilOrgy = fi.stencilOrgy, 
      pipeLen = fi.pipeLen;

  SgExpression *expGridDimx = SageBuilder::buildVarRefExp("cols", funcBody);
  SgExpression *expGridDimy = SageBuilder::buildVarRefExp("rows", funcBody);
  int borderRight = stencilDimx - 1 - stencilOrgx; 
  int borderBottom = stencilDimy - 1 - stencilOrgy; 

#if 1
  std::cout << "Generating stencil function for " << 
      fi.fce->unparseToString() << std::endl;
  std::cout << "Stencil parameters: " << std::endl;
  std::cout << "  data type   : " << fi.stencilType->unparseToString() << std::endl;
  std::cout << "  gridDimx    : " << vv[2]->unparseToString() << std::endl;
  std::cout << "  gridDimy    : " << vv[3]->unparseToString() << std::endl;
  std::cout << "  stencilDimx : " << stencilDimx << std::endl;
  std::cout << "  stencilDimy : " << stencilDimy << std::endl;
  std::cout << "  stencilOrgx : " << stencilOrgx << std::endl;
  std::cout << "  stencilOrgy : " << stencilOrgy << std::endl;
  std::cout << "  borderRight : " << borderRight << std::endl;
  std::cout << "  borderBottom: " << borderBottom << std::endl;
  std::cout << "  pipeLen     : " << pipeLen << std::endl;
#endif

  if (!fi.useStreaming) {
    // This is a special code generation mode that indicates we should
    // generate a standard C loop nest for the stencil rather than 
    // optimized HT streaming code.
    //   for (int y = stencilOrgy; y < gridDimy-borderBottom; y++) {
    //     for (int x = stencilOrgx; x < gridDimx-borderRight; x++) {
    //       res = 0;
    //       for (int ky = -stencilOrgy; ky < (-stencilOrgy+StencilDimy); ky++) {
    //         for (int kx = -stencilOrgx; kx < (-stencilOrgx+StencilDimx); kx++) {
    //           res = res + rdAddr[(y+ky)*cols+(x+kx)] * k[(ky+stencilOrgy)*stencilDimx+(kx+stencilOrgx)]; 
    //         }
    //       }
    //       wrAddr[y*cols+x] = res;
    //     }
    //   }
    //
    // Note: was __htc_t_y, __htc_t_x, __htc_t_ky, __htc_t_kx, __htc_t_res.
    SgVariableDeclaration *yDecl = SageBuilder::buildVariableDeclaration(
        "y", SageBuilder::buildIntType(), 0, funcBody);
    SgVariableDeclaration *xDecl = SageBuilder::buildVariableDeclaration(
        "x", SageBuilder::buildIntType(), 0, funcBody);
    SgVariableDeclaration *kyDecl = SageBuilder::buildVariableDeclaration(
        "ky", SageBuilder::buildIntType(), 0, funcBody);
    SgVariableDeclaration *kxDecl = SageBuilder::buildVariableDeclaration(
        "kx", SageBuilder::buildIntType(), 0, funcBody);
    SgVariableDeclaration *resDecl = SageBuilder::buildVariableDeclaration(
        "res", fi.stencilType, 0, funcBody);
    SageInterface::appendStatement(yDecl, funcBody);
    SageInterface::appendStatement(xDecl, funcBody);
    SageInterface::appendStatement(kyDecl, funcBody);
    SageInterface::appendStatement(kxDecl, funcBody);
    SageInterface::appendStatement(resDecl, funcBody);

    // Compute stencil.
    // rdAddr[(y+ky)*cols+(x+kx)]
    SgPntrArrRefExp *rdAddrRef = SageBuilder::buildPntrArrRefExp(
        SageBuilder::buildVarRefExp("rdAddr", funcBody),
        SageBuilder::buildAddOp(
            SageBuilder::buildMultiplyOp(
                SageBuilder::buildAddOp(
                    SageBuilder::buildVarRefExp(yDecl),
                    SageBuilder::buildVarRefExp(kyDecl)), 
                SageBuilder::buildVarRefExp("cols", funcBody)),
            SageBuilder::buildAddOp(
                SageBuilder::buildVarRefExp(xDecl),
                SageBuilder::buildVarRefExp(kxDecl))));
        
    // k[(ky+stencilOrgy)*stencilDimx+(kx+stencilOrgx)]; 
    SgPntrArrRefExp *kRef = SageBuilder::buildPntrArrRefExp(
        SageBuilder::buildVarRefExp("k", funcBody),
        SageBuilder::buildAddOp(
            SageBuilder::buildMultiplyOp(
                SageBuilder::buildAddOp(
                    SageBuilder::buildVarRefExp(kyDecl), 
                    SageBuilder::buildIntVal(stencilOrgy)),
                SageBuilder::buildIntVal(stencilDimx)),
            SageBuilder::buildAddOp(
                SageBuilder::buildVarRefExp(kxDecl), 
                SageBuilder::buildIntVal(stencilOrgx))));

    SgStatement *resultAsg = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(resDecl),
        SageBuilder::buildAddOp(
            SageBuilder::buildVarRefExp(resDecl),
            SageBuilder::buildMultiplyOp(rdAddrRef, kRef)));

    // Write result back to wrAddr.
    // wrAddr[y*cols+x] = res;
    SgPntrArrRefExp *wrAddrRef = SageBuilder::buildPntrArrRefExp(
        SageBuilder::buildVarRefExp("wrAddr", funcBody),
        SageBuilder::buildAddOp(
            SageBuilder::buildMultiplyOp(
                SageBuilder::buildVarRefExp(yDecl), 
                SageBuilder::buildVarRefExp("cols", funcBody)),
            SageBuilder::buildVarRefExp(xDecl)));
    SgStatement *wrAddrAsg = SageBuilder::buildAssignStatement(
        wrAddrRef,
        SageBuilder::buildVarRefExp(resDecl));
  
    // Inner kernel loop (kx).
    SgStatement *forInitStmt = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(kxDecl),
        SageBuilder::buildIntVal(-stencilOrgx));
    SgStatement *forTestStmt = SageBuilder::buildExprStatement(
        SageBuilder::buildLessThanOp(SageBuilder::buildVarRefExp(kxDecl),
        SageBuilder::buildIntVal(-stencilOrgx+stencilDimx)));
    SgExpression *forIncrExp = SageBuilder::buildPlusPlusOp(
        SageBuilder::buildVarRefExp(kxDecl), SgUnaryOp::postfix);
    SgScopeStatement *loopBB = SageBuilder::buildBasicBlock(resultAsg);
    SgForStatement *kxForStmt = SageBuilder::buildForStatement(forInitStmt,
        forTestStmt, forIncrExp, loopBB);

    // Outer kernel loop (ky).
    forInitStmt = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(kyDecl),
        SageBuilder::buildIntVal(-stencilOrgy));
    forTestStmt = SageBuilder::buildExprStatement(
        SageBuilder::buildLessThanOp(SageBuilder::buildVarRefExp(kyDecl),
        SageBuilder::buildIntVal(-stencilOrgy+stencilDimy)));
    forIncrExp = SageBuilder::buildPlusPlusOp(
        SageBuilder::buildVarRefExp(kyDecl), SgUnaryOp::postfix);
    SgForStatement *kyForStmt = SageBuilder::buildForStatement(forInitStmt,
        forTestStmt, forIncrExp, SageBuilder::buildBasicBlock(kxForStmt));

    // Inner sweep loop (x).
    forInitStmt = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(xDecl),
        SageBuilder::buildIntVal(stencilOrgx));
    forTestStmt = SageBuilder::buildExprStatement(
        SageBuilder::buildLessThanOp(SageBuilder::buildVarRefExp(xDecl),
        SageBuilder::buildSubtractOp(
            expGridDimx,
            SageBuilder::buildIntVal(borderRight))));
    forIncrExp = SageBuilder::buildPlusPlusOp(
        SageBuilder::buildVarRefExp(xDecl), SgUnaryOp::postfix);
    SgStatement *resultInit = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(resDecl),
        SageBuilder::buildIntVal(0));
    SgForStatement *xForStmt = SageBuilder::buildForStatement(forInitStmt,
        forTestStmt, forIncrExp, 
        SageBuilder::buildBasicBlock(resultInit, kyForStmt, wrAddrAsg));

    // Outer sweep loop (y).
    forInitStmt = SageBuilder::buildAssignStatement(
        SageBuilder::buildVarRefExp(yDecl),
        SageBuilder::buildIntVal(stencilOrgy));
    forTestStmt = SageBuilder::buildExprStatement(
        SageBuilder::buildLessThanOp(SageBuilder::buildVarRefExp(yDecl),
        SageBuilder::buildSubtractOp(
            expGridDimy,
            SageBuilder::buildIntVal(borderBottom))));
    forIncrExp = SageBuilder::buildPlusPlusOp(
        SageBuilder::buildVarRefExp(yDecl), SgUnaryOp::postfix);
    SgForStatement *yForStmt = SageBuilder::buildForStatement(forInitStmt,
        forTestStmt, forIncrExp, SageBuilder::buildBasicBlock(xForStmt));

    SageInterface::appendStatement(yForStmt, funcBody);

    if (fi.genHostCode) {
      SageInterface::addTextForUnparser(newFD, "\n#ifdef CNY_HTC_HOST",
          AstUnparseAttribute::e_before);
      SageInterface::addTextForUnparser(newFD, "\n#endif",
          AstUnparseAttribute::e_after);
    }
#if 0
    SageInterface::addTextForUnparser(yForStmt,
        "\n#pragma omp parallel for num_threads(32) reduction(+:res)",
        AstUnparseAttribute::e_before);
#endif

  } else {
    //
    // Generate the optimized streaming stencil case.  This generates a
    // canned HT sequence.
    //

    // Note that the module width must be 0 (currently we just do a hacky
    // check for the outlined stencil name in FSM rewriter-- later we should
    // have an attribute for transmitting module widths).
    //
    // Also, the module clock=1x. (Ht default seems to be 1x, so doing nothing
    // okay for now).

    // Add instructions.
    htd->appendInst("STENCIL_ENTER"); 
    htd->appendInst("STENCIL_START"); 
    htd->appendInst("STENCIL_WAIT"); 
    htd->appendInst("STENCIL_RETURN"); 
 
    // Add defines and typedefs.
    std::string stenFuncName = newFD->get_name().getString();
    std::string xsizeStr = stenFuncName + "_X_SIZE";
    std::string ysizeStr = stenFuncName + "_Y_SIZE";
    std::string xorgStr = stenFuncName + "_X_ORIGIN";
    std::string yorgStr = stenFuncName + "_Y_ORIGIN";
    std::string rdPortStr = stenFuncName + "_STENCIL_READ_MEM_PORT";
    std::string wrPortStr = stenFuncName + "_STENCIL_WRITE_MEM_PORT";
    htd->appendDefine(xsizeStr, boost::lexical_cast<std::string>(stencilDimx)); 
    htd->appendDefine(ysizeStr, boost::lexical_cast<std::string>(stencilDimy)); 
    htd->appendDefine(xorgStr, boost::lexical_cast<std::string>(stencilOrgx)); 
    htd->appendDefine(yorgStr, boost::lexical_cast<std::string>(stencilOrgy)); 
    htd->appendDefine(rdPortStr, "0"); 
    htd->appendDefine(wrPortStr, "1"); 
    htd->appendTypedef(fi.stencilType->unparseToString(), "StType_t");

    // Add CCoef:
    //   struct CCoef { StType_t m_coef[Y_SIZE][X_SIZE]; };
    htd->appendStructDecl(fi.CCoefDecl->unparseToString());

    // Add shared vars for shared part of FSM.  Some of these mirror the 
    // private vars in order to give access in the shared FSM.
    htd->appendSharedVar("bool", "bStart");
    htd->appendSharedVar("MemAddr_t", "rdAddr");
    htd->appendSharedVar("MemAddr_t", "wrAddr");
    htd->appendSharedVar("uint32_t", "rdRowIdx");
    htd->appendSharedVar("uint32_t", "wrRowIdx");
    htd->appendSharedVar("uint32_t", "cols");
    htd->appendSharedVar("uint32_t", "rows");
    htd->appendSharedVar("CCoef" + stenFuncName, "coef");

#if 1
    // Add some staging variables for the pipelined stencil equation.
    htd->appendArbitraryModuleString(
        "AddStage()\n"
        "    .AddVar(type=bool, name=bValid, range=1-3, reset=true)\n"
        "    .AddVar(type=StType_t, name=mult, dimen1=" + ysizeStr + ", dimen2=" + xsizeStr + ", range=1)\n"
        "    .AddVar(type=StType_t, name=ysum, dimen1=" + xsizeStr + ", range=2)\n"
        "    .AddVar(type=StType_t, name=rslt, range=3);\n");
#endif

    // Add unnamed read and write streams (using mostly default parms).
    htd->appendReadStream(""                      /* name */, 
                          "StType_t"              /* type */, 
                          ""                      /* streamCnt */,
                          ""                      /* elemCnt */,
                          ""                      /* closeStr */,
                          ""                      /* memSrc */,
                          rdPortStr               /* memPort */,
                          ""                      /* streamBw */);

    // The reserve parameter here must match the AddStage command above
    // (i.e., N-stage pipelined stencil equation ==> reserve=N).
    htd->appendWriteStream(""                       /* name */, 
                           "StType_t"               /* type */, 
                           ""                       /* streamCnt */,
                           ""                       /* elemCnt */,
                           ""                       /* closeStr */,
                           ""                       /* memDst */,
                           wrPortStr                /* memPort */,
                           "3"                      /* reserve */);

 
    // Add stencil buffer.
    // FIXME: gridSize is hard-coded for now.
    ++SageInterface::gensym_counter;
    std::string stbName = boost::lexical_cast<std::string>(stencilDimx) + "x"
        + boost::lexical_cast<std::string>(stencilDimy) + "o"
        + boost::lexical_cast<std::string>(stencilOrgx) + "x"
        + boost::lexical_cast<std::string>(stencilOrgy) + "_"
        + boost::lexical_cast<std::string>(SageInterface::gensym_counter);

    htd->appendArbitraryModuleString("AddStencilBuffer(name=" + stbName
        + ", type=StType_t, gridSize={1024, 1024}, "
        "stencilSize={" + xsizeStr + ", " + ysizeStr + "}, pipeLen="
        + boost::lexical_cast<std::string>(pipeLen) + ");");

    //
    // Now emit the canned streaming stencil code.
    //
    std::string stencilSequence = "\n"
    "	if (PR_htValid) {\n"
    "		switch (PR_htInst) {\n"
    "		case STENCIL_ENTER: {\n"
    "			// Split offset calculation from source stencil to destination location over\n"
    "			// two cycles for timing. OFF = ((Y_ORIGIN * (PR_cols + X_SIZE-1) + X_ORIGIN) * sizeof(StType_t);\n"
    "			//\n"
    "			uint32_t offset = " + yorgStr + " * (PR_cols + " + xsizeStr+ "-1);\n"
    "			S_rdAddr = PR_rdAddr;\n"
    "			S_wrAddr = offset;\n"
    "			S_rdRowIdx = 0;\n"
    "			S_wrRowIdx = " + yorgStr + ";\n"
    "			S_cols = PR_cols;\n"
    "			S_rows = PR_rows;\n"
    "			S_coef = PR_coef;\n"
    "			HtContinue(STENCIL_START);\n"
    "		}\n"
    "		break;\n"
    "		case STENCIL_START: {\n"
    "			S_bStart = true;\n"
    "\n"
    "			S_wrAddr = ((uint32_t)S_wrAddr + " + xorgStr + ") * sizeof(StType_t) + PR_wrAddr;\n"
    "\n"
    "			StencilBufferInit_" + stbName + "((ht_uint11)PR_cols, (ht_uint11)PR_rows);\n"
    "\n"
    "			HtContinue(STENCIL_WAIT);\n"
    "		}\n"
    "		break;\n"
    "		case STENCIL_WAIT: {\n"
    "			if (S_wrRowIdx == S_rows)\n"
    "				WriteStreamPause(STENCIL_RETURN);\n"
    "			else\n"
    "				HtContinue(STENCIL_WAIT);\n"
    "		}\n"
    "		break;\n"
    "		case STENCIL_RETURN: {\n"
    "			if (SendReturnBusy_" + stenFuncName + "()) {\n"
    "				HtRetry();\n"
    "				break;\n"
    "			}\n"
    "\n"
    "			SendReturn_" + stenFuncName + "();\n"
    "		}\n"
    "		break;\n"
    "		default:\n"
    "			assert(0);\n"
    "		}\n"
    "	}\n"
    "\n"
    "	// start read stream per row\n"
    "	if (SR_bStart && SR_rdRowIdx < SR_rows + " + ysizeStr + "-1 && !ReadStreamBusy()) {\n"
    "		ReadStreamOpen(SR_rdAddr, SR_cols + " + xsizeStr + "-1);\n"
    "		S_rdAddr += (SR_cols + " + xsizeStr + " -1) * sizeof(StType_t);\n"
    "		S_rdRowIdx += 1;\n"
    "	}\n"
    "\n"
    "	// start write stream per row\n"
    "	if (SR_bStart && SR_wrRowIdx < SR_rows + " + yorgStr + " && !WriteStreamBusy()) {\n"
    "		WriteStreamOpen(SR_wrAddr, SR_cols);\n"
    "		S_wrAddr += (SR_cols + " + xsizeStr + "-1) * sizeof(StType_t);\n"
    "		S_wrRowIdx += 1;\n"
    "	}\n"
    "\n"
    "	CStencilBufferIn_" + stbName + " stIn;\n"
    "	stIn.m_bValid = ReadStreamReady() && WriteStreamReady();\n"
    "	stIn.m_data = stIn.m_bValid ? ReadStream() : (StType_t)0;\n"
    "	\n"
    "	CStencilBufferOut_" + stbName + " stOut;\n"
    "\n"
    "	StencilBuffer_" + stbName + "(stIn, stOut);\n"
    "\n"
    "	//\n"
    "	// compute stencil\n"
    "	//\n"
#if 0
    "	if (stOut.m_bValid) {\n"
    "		StType_t rslt = 0;\n"
    "		for (uint32_t x = 0; x < X_SIZE; x += 1)\n"
    "			for (uint32_t y = 0; y < Y_SIZE; y += 1)\n"
    "				rslt = rslt + (StType_t)(stOut.m_data[y][x] * SR_coef.m_coef[y][x]);\n"
    "\n"
    "		WriteStream(rslt);\n"
    "	}\n"
#else   
    "	T1_bValid = stOut.m_bValid;\n"
    "	for (uint32_t x = 0; x < " + xsizeStr + "; x += 1)\n"
    "		for (uint32_t y = 0; y < " + ysizeStr + "; y += 1)\n"
    "			T1_mult[y][x] = (StType_t)(stOut.m_data[y][x] * SR_coef.m_coef[y][x]);\n"
    "\n"
    "	for (uint32_t x = 0; x < " + xsizeStr + "; x += 1) {\n"
    "		T2_ysum[x] = 0;\n"
    "		for (uint32_t y = 0; y < " + ysizeStr + "; y += 1)\n"
    "			T2_ysum[x] += T2_mult[y][x];\n"
    "	}\n"
    "\n"
    "	T3_rslt = 0;\n"
    "	for (uint32_t x = 0; x < " + xsizeStr + "; x += 1)\n"
    "		T3_rslt += T3_ysum[x];\n"
    "\n"
    "	if (T3_bValid)\n"
    "		WriteStream(T3_rslt);\n"
#endif
    "\n";

    SgStatement *stbStmt = SageBuilder::buildNullStatement();
    SageInterface::prependStatement(stbStmt, funcBody);
    SageInterface::addTextForUnparser(stbStmt, stencilSequence,
        AstUnparseAttribute::e_after);
  }

}


