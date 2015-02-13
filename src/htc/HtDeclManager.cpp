/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include "HtDeclManager.h"
#include "HtSCDecls.h"
#include "HtSageUtils.h"

extern scDeclarations *SCDecls;

extern bool debugHooks;

namespace HtDeclMgr {


struct declInfo {
  declInfo() {
    name2vdecl.clear();
    name2fdecl.clear();
  } 
  std::map<std::string, SgVariableDeclaration *> name2vdecl;
  std::map<std::string, SgFunctionDeclaration *> name2fdecl;
};
std::map<SgScopeStatement *, declInfo> g2d;


SgType *charToSgType(char ty, SgScopeStatement *GS = 0)
{
  SgType *vtype = 0;
  switch (ty) {
  case 'v':
    vtype = SageBuilder::buildVoidType();
    break;
  case 'b':
    vtype = SageBuilder::buildBoolType();
    break;
  case 'c':
    vtype = SageBuilder::buildUnsignedCharType();
    break;
  case 's':
    vtype = SageBuilder::buildUnsignedShortType();
    break;
  case 'i':
    vtype = SageBuilder::buildIntType();
    break;
  case 'L':
    vtype = SageBuilder::buildLongType();
    break;
  case 'u':
    vtype = SageBuilder::buildUnsignedIntType();
    break;
  case 'U':
    vtype = SageBuilder::buildUnsignedLongType();
    break;
  case 'f':
    vtype = SageBuilder::buildFloatType();
    break;
  case 'D':
    vtype = SageBuilder::buildDoubleType();
    break;
  case '*':
    vtype = SageBuilder::buildOpaqueType("omp_lock_t", GS);
    break;
  default:
    assert(0 && "unimplemented type");
    break;
  }
  return vtype;
}


SgFunctionParameterTypeList *stringToSgFunctionParamTypeList(std::string s,
                                                     SgScopeStatement *GS = 0)
{
  SgFunctionParameterTypeList *ptl = 
      SageBuilder::buildFunctionParameterTypeList();
  for (int i = 0; i < s.length(); i++) {
    ptl->append_argument(charToSgType(s[i], GS));
  }
  return ptl;
}


SgVariableDeclaration *buildDecl_var(SgScopeStatement *GS, std::string name,
                                     char ty, SgType *genericType = 0)
{
  // Make hidden extern variable declaration.
  SgType *vartype = 0;
  if (!genericType) { 
    vartype = charToSgType(ty);
  } else {
    vartype = genericType;
  }
  SgScopeStatement *where = 0;
  if (isSgFunctionDefinition(GS)) {
    where = isSgFunctionDefinition(GS)->get_body();
  } else {
    where = GS;
    assert(isSgGlobal(GS));
  }
  SgVariableDeclaration *decl = 
      SageBuilder::buildVariableDeclaration(name, vartype, 0, where);
  SageInterface::setExtern(decl);
  SageInterface::prependStatement(decl, where);
  if (!debugHooks) {
      decl->unsetOutputInCodeGeneration();
  }
  return decl;
}


SgFunctionDeclaration *buildDecl_func(SgScopeStatement *GS, std::string name, 
                                     char rtype, std::string argtypes,
                                     SgType *genericRtype = 0,
                                     SgFunctionParameterList *genericPlist = 0)
{
  // Make hidden extern function declaration.
  SgFunctionParameterList *plist = 0;
  SgType *ftype = 0;
  if (!genericRtype)  {
    SgFunctionParameterTypeList *ptl = 
        stringToSgFunctionParamTypeList(argtypes, GS);
    plist = SageBuilder::buildFunctionParameterList(ptl);
    ftype = charToSgType(rtype, GS);
  } else {
    assert(genericPlist);
    plist = genericPlist;
    ftype = genericRtype;
  }
  SgFunctionDeclaration *decl = 
      SageBuilder::buildNondefiningFunctionDeclaration(name, ftype, plist, GS);

  SageInterface::setExtern(decl);
  SgScopeStatement *where = 0;
  if (isSgFunctionDefinition(GS)) {
    where = isSgFunctionDefinition(GS)->get_body();
  } else {
    where = GS;
    assert(isSgGlobal(GS));
  }
  SageInterface::prependStatement(decl, where);
  decl->unsetOutputInCodeGeneration();
  return decl;
}


SgVariableDeclaration *buildHtlVarDecl(std::string name, SgScopeStatement *GS)
{
  // Make hidden extern decl for one of the known special Htl variables. 
  if (g2d.count(GS) && g2d[GS].name2vdecl.count(name)) {
    return g2d[GS].name2vdecl[name];
  }

  SgVariableDeclaration *decl = 0;
  if (name == "PR_htValid") {
    decl = buildDecl_var(GS, name, 'i');
  } else if (name == "PR_htInst") {
    decl = buildDecl_var(GS, name, 'i');
  } else if (name == "PR_htId") {
    decl = buildDecl_var(GS, name, 'i');
  } else if (name == "SR_unitId") {
    decl = buildDecl_var(GS, name, 'i');
  } else {
    assert(0 && "not a known Htl special variable");
  }

  g2d[GS].name2vdecl[name] = decl;
  return decl;
}


SgVariableDeclaration *buildHtlVarDecl_generic(std::string name, SgType *vtype,
                                               SgScopeStatement *GS)
{
  // Make hidden extern decl for a generic Htl variable. 
  if (g2d.count(GS) && g2d[GS].name2vdecl.count(name)) {
    return g2d[GS].name2vdecl[name];
  }

  SgVariableDeclaration *decl = buildDecl_var(GS, name, '-', vtype);

  g2d[GS].name2vdecl[name] = decl;
  return decl;
}


SgFunctionDeclaration *buildHtlFuncDecl(std::string name, SgScopeStatement *GS)
{
  // Make hidden extern decl for one of the known special Htl functions. 
  if (g2d.count(GS) && g2d[GS].name2fdecl.count(name)) {
    return g2d[GS].name2fdecl[name];
  }

  static struct finfo_t {
    std::string name;
    char rtype;
    std::string atypes;
  } finfo[] = {
    { "HtContinue",      'v', "i" },
    { "HtAssert",        'v', "ii" },
    { "HtRetry",         'v', "v" },
    { "HtBarrier",       'v', "iii" },
    { "ReadMemBusy",     'b', "v" },
    { "ReadMemPause",    'v', "i" },
    { "ReadMemPoll",     'b', "v" },
    { "WriteMem",        'v', "UU" },
    { "WriteMem_uint8",  'v', "Uc" },
    { "WriteMem_uint16", 'v', "Us" },
    { "WriteMem_uint32", 'v', "Uu" },
    { "WriteMem_uint64", 'v', "UU" },
    { "WriteMemBusy",    'b', "v" },
    { "WriteMemPause",   'v', "i" },
    { "WriteMemPoll",    'b', "v" },
    { "rhomp_test_lock", 'b', "*" },
    { "rhomp_set_lock",  'v', "*" },
    { "rhomp_unset_lock",'v', "*" },
    { "rhomp_init_lock", '*', "v" },
  };

  // TODO: use bsearch instead.
  SgFunctionDeclaration *decl = 0;
  for (int i = 0; i < sizeof(finfo)/sizeof(finfo_t); i++) {
    finfo_t &fi = finfo[i];
    if (fi.name == name) {
      decl = buildDecl_func(GS, name, fi.rtype, fi.atypes);
      break;
    }
  } 

  g2d[GS].name2fdecl[name] = decl;
  return decl;
}


SgFunctionDeclaration *buildHtlFuncDecl_generic(std::string name, char rtype,
                                     std::string atypes, SgScopeStatement *GS)
{
  // Make hidden extern decl for a generic function.  This is currently used
  // for the special Htl functions that have a suffix which isn't known until
  // translation (e.g., ReadMem_foo, SendCall_bar). 
  if (g2d.count(GS) && g2d[GS].name2fdecl.count(name)) {
    return g2d[GS].name2fdecl[name];
  }

  SgFunctionDeclaration *decl = buildDecl_func(GS, name, rtype, atypes);

  g2d[GS].name2fdecl[name] = decl;
  return decl;
}


SgFunctionDeclaration *buildHtlFuncDecl_generic(std::string name, 
                                              SgType *sgtype,
                                              SgFunctionParameterList *sgplist,                                               SgScopeStatement *GS)
{
  // Make hidden extern decl for a generic function.  This is currently used
  // for the special Htl functions that have a suffix which isn't known until
  // translation (e.g., ReadMem_foo, SendCall_bar). 
  if (g2d.count(GS) && g2d[GS].name2fdecl.count(name)) {
    return g2d[GS].name2fdecl[name];
  }

  SgFunctionDeclaration *decl = buildDecl_func(GS, name, '-', "-", sgtype,
      sgplist);

  g2d[GS].name2fdecl[name] = decl;
  return decl;
}


void buildAllKnownHtlDecls(SgProject *project)
{
  // Build all the known magic decls.  It's good to do this before a
  // transformational traversal to avoid any possible iterator invalidation.
  class VisitGlobalScopes : public AstSimpleProcessing {
  private:
    void visit(SgNode *S) {
      if (SgGlobal *gs = isSgGlobal(S)) {
        // Known variables.
        buildHtlVarDecl("PR_htValid", gs);
        buildHtlVarDecl("PR_htInst", gs);
        buildHtlVarDecl("PR_htId", gs);
        buildHtlVarDecl("SR_unitId", gs);

        // Known functions.
        buildHtlFuncDecl("HtContinue", gs);
        buildHtlFuncDecl("HtAssert", gs);
        buildHtlFuncDecl("HtRetry", gs);
        buildHtlFuncDecl("HtBarrier", gs);
        buildHtlFuncDecl("ReadMemBusy", gs);
        buildHtlFuncDecl("ReadMemPause", gs);
        buildHtlFuncDecl("ReadMemPoll", gs);
        buildHtlFuncDecl("WriteMem", gs);
        buildHtlFuncDecl("WriteMem_uint8", gs);
        buildHtlFuncDecl("WriteMem_uint16", gs);
        buildHtlFuncDecl("WriteMem_uint32", gs);
        buildHtlFuncDecl("WriteMem_uint64", gs);
        buildHtlFuncDecl("WriteMemBusy", gs);
        buildHtlFuncDecl("WriteMemPause", gs);
        buildHtlFuncDecl("WriteMemPoll", gs);
        buildHtlFuncDecl("rhomp_test_lock", gs);
        buildHtlFuncDecl("rhomp_set_lock", gs);
        buildHtlFuncDecl("rhomp_unset_lock", gs);
        buildHtlFuncDecl("rhomp_init_lock", gs);

        SgVariableDeclaration *ad = SageBuilder::buildVariableDeclaration(
            "__htc00__", SageBuilder::buildIntType(), 0, gs);
        SageInterface::setExtern(ad);
        if (!debugHooks) {
          ad->unsetOutputInCodeGeneration();
        }
        SageInterface::prependStatement(ad, gs);

        SCDecls = new scDeclarations(gs);

      }
    }
  } vgs;
    vgs.traverseInputFiles(project, preorder, STRICT_INPUT_FILE_TRAVERSAL);
}


} // namespace HtDeclMgr

