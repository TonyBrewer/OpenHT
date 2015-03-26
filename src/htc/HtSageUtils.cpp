/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include "HtSageUtils.h"

extern bool debugHooks;

namespace HtSageUtils {


bool isHtlReservedIdent(std::string name)
{
  return (name == "PR_htValid" || name == "PR_htInst" || name == "PR_htId"
      || name == "SR_unitId");
}


SgLabelStatement *makeLabelStatement(SgName baseName, int num,
                                     SgScopeStatement *scope) {
  baseName << "__" << num;
  return SageBuilder::buildLabelStatement(baseName,
      SageBuilder::buildBasicBlock(), scope);
}


SgLabelStatement *makeLabelStatement(SgName baseName, SgScopeStatement *scope) {
  baseName << "__" << ++SageInterface::gensym_counter;
  return SageBuilder::buildLabelStatement(baseName,
      SageBuilder::buildBasicBlock(), scope);
}


SgStatement *findSafeInsertPoint(SgNode *node) {
    SgStatement *insertPoint = SageInterface::getEnclosingStatement(node);
    SgStatement *es = isSgExprStatement(insertPoint);
    SgStatement *esParent = es ? isSgStatement(es->get_parent()) : 0;
    if (es && (isSgSwitchStatement(esParent) || isSgIfStmt(esParent))) {
      // Make sure insertion point is outside of the condition of an if-stmt
      // or the selector of a switch-stmt.
      insertPoint = esParent; 
    } else {
      if (esParent) {
        assert(isSgBasicBlock(esParent));
      }
    }
    return insertPoint;
}


// This will attempt to infer a file name for translated Ast fragments
// by traversing parent pointers upwards until a SgFile node is found.
// For some cases in Htc, the translated Ast logically belongs to the
// SgFile which it is under.  A boolean return value will indicate if
// the file name info is directly from the node, or whether we inferred
// it.
std::pair<bool, std::string> getFileName(SgNode *n)
{
  std::pair<bool, std::string> retval;
  retval.first = true;
  std::string fname = n->get_file_info()->get_filenameString();
  if (fname == std::string("transformation")) {
    SgFile *spp = SageInterface::getEnclosingFileNode(n);
    retval.first = false;
    retval.second = spp->getFileName();
  } else {
    retval.second = fname;
  } 

  return retval;
}


//-------------------------------------------------------------------------
// Screen for constructs/idioms that violate htc/htv restrctions.
//
// - Statement expressions (GNU extension).
// - Calls through pointers.
// - TODO: Recursion/mutual recursion.
// - TODO: Filter stdio, stdlib, and system calls, etc.
//
//-------------------------------------------------------------------------
void ScreenForHtcRestrictions(SgProject *project)
{
  class FilterVisitor : public AstSimpleProcessing {
  public:
    FilterVisitor() : foundErrors(false) { }

    void visit(SgNode *n) {
      switch (n->variantT()) {
      case V_SgStatementExpression: 
        reportError("GNU extension 'statement expression' is not allowed.", n);
        break;
      case V_SgFunctionCallExp: {
        SgFunctionCallExp *FCE = isSgFunctionCallExp(n);
        SgFunctionDeclaration *calleeFD = 
            FCE->getAssociatedFunctionDeclaration();
        if (!calleeFD) {
            reportError("calls through function pointers are not allowed.", n);
        }
        break;
        }
      default:
        break;
      }
    }
 
    void reportError(std::string msg, SgNode *n) {
      std::cerr << "HTC RESTRICTION:: " << msg << std::endl;
      std::cerr << n->get_file_info()->get_filenameString() << " line " <<
          n->get_file_info()->get_line() << ":\n   ";
      std::cerr << n->unparseToString() << std::endl;
      foundErrors = true;
    }
 
    bool foundErrors;
  } v;

    v.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
  if (v.foundErrors) {
    std::cerr << "Htc exiting with errors." << std::endl;
    exit(2);
  }
}


void DeclareFunctionInScope(std::string name, 
                            SgType *type,
                            SgScopeStatement *scope) {
    // Return if already declared in scope
    SgName sname(name);
    if (scope->get_symbol_table()->find_any(sname, NULL, NULL)) {
        return;
    }
    
    SgFunctionParameterTypeList *parms = 
        SageBuilder::buildFunctionParameterTypeList();
    
    SgFunctionDeclaration *func = 
        SageBuilder::buildNondefiningFunctionDeclaration( 
            name,
            type,
            SageBuilder::buildFunctionParameterList(parms),
            scope);
    SageInterface::setExtern(func);
    
    if (!debugHooks) {
        func->unsetOutputInCodeGeneration();
    }
    
    return;
}


// Copied from sageInterface.C because it is static there.
SgExpression *SkipCasting(SgExpression *exp) {
  SgCastExp *cast_exp = isSgCastExp(exp);
  if (cast_exp) {
    SgExpression *operand = cast_exp->get_operand();
    return SkipCasting(operand);
  } else {
    return exp;
  }
}


} // namespace HtSageUtils

