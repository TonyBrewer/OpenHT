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
#include "HtcAttributes.h"
#include "HtDeclManager.h"
#include "HtSageUtils.h"
#include "HtdInfoAttribute.h"
#include "ProcessCalls.h"
#include "HtSCDecls.h"

extern scDeclarations *SCDecls;


#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 

extern bool debugHooks;
extern bool doSystemC;


//
//
class RewriteCallsRets : public AstPrePostProcessing {
private:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgFunctionCallExp:
      postVisitSgFunctionCallExp(dynamic_cast<SgFunctionCallExp *>(S));
      break;
    case V_SgReturnStmt:
      rewriteReturnList.push_back(dynamic_cast<SgReturnStmt *>(S));
      break;
    }
  }

  
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionCallExp(SgFunctionCallExp *FCE);

public:
  std::vector<SgFunctionCallExp *> rewriteCallList;
  std::vector<SgReturnStmt *> rewriteReturnList;
};


// Emit htd entry points for each function.
void RewriteCallsRets::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  rewriteCallList.clear();
  rewriteReturnList.clear();
  HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);

  // Note that when we get to C++ translation support, we will probably
  // need to use get_mangled_name() insteaad. 
  std::string modName = FD->get_name().getString();

  // Don't process Ht calls inserted during earlier phases.
  size_t pos = 0;
  size_t np = std::string::npos;
  if (((pos = modName.find("RecvReturnPause_")) != np && pos == 0)
        || ((pos = modName.find("RecvReturnJoin_")) != np && pos == 0)) { 
    return;
  }

  SgFunctionParameterList *parmList = FD->get_parameterList();
  SgInitializedNamePtrList &initNameList = parmList->get_args();
  std::vector<parm_tuple_t> htdArgs;
  bool isHostEntry = ((pos = modName.find("__HTC_HOST_ENTRY_")) != np &&
                       pos == 0);
  foreach (SgInitializedName *initName, initNameList) {
    // TBD: Is it sufficient to simply use the argument type obtained
    // from the function decl as the type that we supply to .htd? 
    std::string aname = initName->get_name().getString(); 
    std::string atype;
    SgType *tp = initName->get_type()->stripType(SgType::STRIP_MODIFIER_TYPE);
    if (!debugHooks &&
        (isSgArrayType(tp) || 
         isSgPointerType(tp) ||
         isSgReferenceType(tp))) {
        atype = "MemAddr_t";
    } else {
        atype = tp->unparseToString();
    }

    std::string hosttype = "";
    if (isHostEntry) {
        if ((tp == SCDecls->get_htc_tid_t_type()) ||
            (tp == SCDecls->get_htc_teams_t_type())) {
            hosttype = "uint16_t";
        } else {
            hosttype = tp->unparseToString();
        }
    }
    htdArgs.push_back(boost::make_tuple(atype, aname, hosttype));


#if 0
    // Emit a private var to mirror each function parameter.
    htd->appendPrivateVar("" /* grp */, atype, aname);
#endif
  }
  if (isHostEntry) {
      htd->appendEntry(modName, "" /* grp */, "true" /* host */, htdArgs);
  } else {
      htd->appendEntry(modName, "" /* grp */, "" /* host */, htdArgs);
  }
}


static void makeDebugStringForSendCalls(HtdInfoAttribute *htd,
                                        SgFunctionDeclaration *calleeFD, 
                                        std::string calleeName,
                                        std::string retvarName,
                                        bool isVoid)
{
  // Add debugging hook string to define fake macros for 'SendCall_foo'
  // and 'SendCallBusy_foo()'.
  htd->debugHookStr += "#define SendCallBusy_" + calleeName + "() 0\n";
  SgFunctionType *calleeFtype = calleeFD->get_type();
  std::string s = "#define SendCall_" + calleeName + "(__ns"; 
  // Formals (need to account for varargs). 
  bool isVarargs = false;
  SgTypePtrList &ctpl = calleeFtype->get_arguments();
  for (int ai = 0; ai < ctpl.size(); ai++) {
    if (isSgTypeEllipse(ctpl[ai])) {
      s += ", ...";
      isVarargs = true;
    } else {
      s += ", __a" + boost::lexical_cast<std::string>(ai);
    }
  } 
  
  s += ") ";
  if (!isVoid) {
    s += retvarName + " = ";
  } 
  s += calleeName + "(";
  // Actuals.
  for (int ai = 0; ai < ctpl.size(); ai++) {
    if (isSgTypeEllipse(ctpl[ai])) {
      s += " ##__VA_ARGS__" ;
      break;
    } else {
      s += "(__a" + boost::lexical_cast<std::string>(ai) + ")";
    }
    if (ai < ctpl.size() - 1) {
      s += ", ";
    }
  } 
  s += "); htc_statenum = (__ns);\n";
  htd->debugHookStr += s;
}


void RewriteCallsRets::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
  std::set<std::string> calledFuncs;
  std::map<std::string, bool> calledFuncIsParallel;

  //
  // Rewrite each call 't=foo()' as a 'SendCall_foo(); .. ; t=P_retval_foo;'.
  //

  foreach (SgFunctionCallExp *fce, rewriteCallList) {
    SgFunctionDeclaration *calleeFD = fce->getAssociatedFunctionDeclaration();
    std::string calleeName = calleeFD->get_name().getString();
    bool calledBefore = calledFuncs.find(calleeName) != calledFuncs.end();
    calledFuncs.insert(calleeName);

    ParallelCallAttribute *pcAttr = getParallelCallAttribute(fce);
    if (!calledBefore) {
      calledFuncIsParallel[calleeName] = pcAttr ? true : false;
    }

    // Build the 'SendCall_foo' decl (but only once, handled by HtDeclMgr).
    // Obtain the argument types from the original decl, and prepend a 
    // new 'int' formal for the 'resume state'.
    std::string sendCallName = 
        (pcAttr ? "SendCallFork_" : "SendCall_") + calleeName;
    SgFunctionParameterList *calleePlist = 
        SageInterface::deepCopy(calleeFD->get_parameterList());
    calleePlist->prepend_arg(
        SageBuilder::buildInitializedName("", SageBuilder::buildIntType()));

    SgFunctionDeclaration *sendCalleeDecl = 
        HtDeclMgr::buildHtlFuncDecl_generic(sendCallName,
        SageBuilder::buildVoidType(), calleePlist, fdef);
#if 0
    sendCalleeDecl->setOutputInCodeGeneration();
#endif
    if (calleePlist->get_parent() == 0) {
      // Need to delete the copied plist if HtDecl didn't need to build
      // the func decl.
      SageInterface::deleteAST(calleePlist);
    }

    // Build the new 'SendCall_foo' call statement and insert it just
    // before the statement enclosing the original call expression.
    // Prepend a 'resume state' placeholder argument to the original args.
    SgExprListExp *parmList = SageInterface::deepCopy(fce->get_args());

    // Add in the extra args for the HT OpenMP arguments (in reverse).
    // Only do this for non-Outlined callees.

    if (SageInterface::is_OpenMP_language() && 
        (strncmp(calleeName.c_str(), "rhomp_", strlen("rhomp_"))) &&
        (strncmp(calleeName.c_str(), "OUT__", strlen("OUT__")))) {

        if (!debugHooks) {
        parmList->prepend_expression(
            SageBuilder::buildVarRefExp( "__htc_my_omp_league_size", fdef));
        parmList->prepend_expression(
            SageBuilder::buildVarRefExp( "__htc_my_omp_team_size", fdef));
        parmList->prepend_expression(
            SageBuilder::buildVarRefExp( "__htc_my_omp_thread_num", fdef));

        // TODO:: ___htc_parent_htid should not be used in the called
        /// function.   Do we want to pass -1 or PR_htId instead?
        parmList->prepend_expression(
            SageBuilder::buildVarRefExp( "__htc_parent_htid", fdef));
        }
    }

    SgExpression *placeholder = SageBuilder::buildIntVal(-999);
    parmList->prepend_expression(placeholder); 
    SgExprStatement *callStmt = 
        SageBuilder::buildFunctionCallStmt(sendCallName, 
        SageBuilder::buildVoidType(), parmList, fdef->get_body());
    SgStatement *insertPoint = SageInterface::getEnclosingStatement(fce);
    SageInterface::insertStatementBefore(insertPoint, callStmt);

    // Insert a state-breaking label just after the call.
    // NOTE: This is a simple approach until we do something smarter later,
    // such as implementing a scheduling phase, etc.
    //
    // Also mark the placeholder expression with an FsmPlaceHolderAttribute
    // to indicate a deferred FSM name is needed.
    FsmPlaceHolderAttribute *attr = 0;
    if (!pcAttr) {
      SgLabelStatement *schedBrk = HtSageUtils::makeLabelStatement("call_rtn", fdef);
      SageInterface::insertStatementAfter(callStmt, schedBrk);
      attr = new FsmPlaceHolderAttribute(schedBrk);
    } else {
      attr = new FsmPlaceHolderAttribute(pcAttr->joinCycleLabel);
    }
    placeholder->addNewAttribute("fsm_placeholder", attr);

    // Build a decl for the special call result variable 'retval_foo',
    // if the type is not void.  Only build it once (HtDeclMgr handles this).
    SgType *retvarType = fce->get_type();
    SgVariableDeclaration *varDecl = 0;
    SgName retvarName = "retval_" + calleeName;
    if (debugHooks) {
      retvarName = "P_" + retvarName;
    }
    if (!isSgTypeVoid(retvarType)) {
      varDecl = HtDeclMgr::buildHtlVarDecl_generic(retvarName, retvarType,
          fdef);
      if (debugHooks) {
        // Make the declaration visible to unparsing.  Also, clear the
        // 'extern' modifier, so we actually declare the space.
        // ROSE provides 'SageInterface::setExtern' but not 'unsetExtern'.
        varDecl->get_declarationModifier().get_storageModifier().setUnknown();
        varDecl->setOutputInCodeGeneration();
      }

      // Emit the private var 'retval_foo' corresponding to each callee's 
      // return value (only do it once) in the caller's private state.
#if 0
      if (!calledBefore) {
        htd->appendPrivateVar("", retvarType->unparseToString(), 
            ("retval_" + calleeName));
      }
#endif
    }

    if (!isSgTypeVoid(retvarType)) {
      // Replace old call expression with a reference to 'P_retval_foo'. 
      // Note that for call expressions like 'printf()' where the return
      // value is unused, replace expression seems to eliminate the
      // expression altogether-- which is exactly what we desire for that
      // case anyway.
      SgVarRefExp *retval = SageBuilder::buildVarRefExp(varDecl);
      SageInterface::replaceExpression(fce, retval, false /* keepOldExp */);
    } else {
      // Replace old call expression with a null expression.
      SageInterface::replaceExpression(fce, 
          SageBuilder::buildNullExpression(), false /* keepOldExp */);
    }

    if (debugHooks && !calledBefore) {
      makeDebugStringForSendCalls(htd, calleeFD, calleeName, retvarName, 
          isSgTypeVoid(retvarType) ? true : false);
    }
  }

  //
  // Emit an AddCall for each called module.
  //
  foreach (std::string fname, calledFuncs) {
    std::string forkArg = calledFuncIsParallel[fname] ? "true" : "";
    htd->appendCall(fname, forkArg, "" /* grp */, "" /* queueW */);
  }

  //
  // Rewrite each return statement as 'SendReturn_foo()' and insert a single
  // declaration for 'SendReturn_foo()'.
  //

  std::string myname = FD->get_name().getString();


  size_t pos = 0;
  size_t np = std::string::npos;
  bool isHostEntry = ((pos = myname.find("__HTC_HOST_ENTRY_")) != np &&
                      pos == 0);

  std::string sendReturnName = "SendReturn_" + myname;
  SgFunctionParameterList *myplist = 0;
  SgType *returnType = FD->get_type()->get_return_type();
  if (isSgTypeVoid(returnType)) {
    myplist = SageBuilder::buildFunctionParameterList();
  } else {
    myplist = SageBuilder::buildFunctionParameterList(
        SageBuilder::buildInitializedName("", returnType));
  }
  SgFunctionDeclaration *sendReturnDecl = 
      HtDeclMgr::buildHtlFuncDecl_generic(sendReturnName,
      SageBuilder::buildVoidType(), myplist, fdef);
#if 0
  sendReturnDecl->setOutputInCodeGeneration();
#endif

  // Emit an Htd AddReturn (for the caller-side 'retval_myname').
  std::vector<rparm_tuple_t> htdParms;
  if (!isSgTypeVoid(returnType)) {
    std::string rn = "retval_" + myname;
    std::string rt;
    if (!debugHooks &&
        (isSgArrayType(returnType) || 
         isSgPointerType(returnType) ||
         isSgReferenceType(returnType))) {
        rt = "MemAddr_t";
    } else {
        rt = returnType->unparseToString();
    }

    std::string hosttype = "";
    if (isHostEntry) {
        hosttype = returnType->unparseToString();       
    }
    htdParms.push_back(boost::make_tuple(rt, rn, hosttype));

  }
  htd->appendReturn(myname, "" /* grp */, htdParms);

  // Add debugging hook string to define fake macros for 'SendReturn_foo'
  // and 'SendReturnBusy_foo()'.
  if (debugHooks) {
    std::string s = "#define SendReturn_" + myname;
    if (!isSgTypeVoid(returnType)) {
      s += "(__rval) return (__rval);\n";
    } else {
      s += "() return;\n";
    }
    htd->debugHookStr += s;
    htd->debugHookStr += "#define SendReturnBusy_" + myname + "() 0\n";
  }
  
  foreach (SgReturnStmt *rtn, rewriteReturnList) {
    // Build the new 'SendReturn_foo' call stmt and replace original return.
    SgExprListExp *parmList = 0;
    if (isSgTypeVoid(returnType)) {
      parmList = SageBuilder::buildExprListExp();
    } else {
      SgExpression *rvalCopy = SageInterface::deepCopy(rtn->get_expression());
      parmList = SageBuilder::buildExprListExp(rvalCopy);
    }
    SgExprStatement *callStmt = 
        SageBuilder::buildFunctionCallStmt(sendReturnName, 
        SageBuilder::buildVoidType(), parmList, fdef->get_body());
    SageInterface::replaceStatement(rtn, callStmt, true);

#if 0
    // Insert a state-breaking label just after the SendReturn.
    // NOTE: This is a simple approach until we do something smarter later,
    // such as implementing a scheduling phase, etc.
    SgLabelStatement *schedBrk = HtSageUtils::makeLabelStatement("sched_brk", fdef);
    SageInterface::insertStatementAfter(callStmt, schedBrk);
#endif
  }

  rewriteCallList.clear();
  rewriteReturnList.clear();
}


void RewriteCallsRets::postVisitSgFunctionCallExp(SgFunctionCallExp *FCE)
{
  // Don't process Ht calls inserted during earlier phases.
  SgFunctionDeclaration *calleeFD = FCE->getAssociatedFunctionDeclaration();
  std::string calleeName = calleeFD->get_name().getString();
  size_t pos = 0;
  size_t np = std::string::npos;
  if (((pos = calleeName.find("RecvReturnPause_")) != np && pos == 0)
        || ((pos = calleeName.find("RecvReturnJoin_")) != np && pos == 0)
        || calleeName == "HtBarrier") { 
    return;
  }

  // If we're targeting SystemC, let some stdio flow through untranslated
  // so that the user can insert debugging prints.
  if (doSystemC) {
    if (calleeName == "printf" || calleeName == "fprintf") {
      return;
    }
  }

  rewriteCallList.push_back(FCE);
}


void ProcessCalls(SgProject *project)
{
  RewriteCallsRets rv;
  rv.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}

