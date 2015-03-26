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
#include "HtSageUtils.h"
#include "Normalize.h"
#include "HtcAttributes.h"
#include "ProcessStencils.h"

#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 


static void NormalizeSwitchCaseBodies(SgProject *);

void NormalizeSyntax(SgProject *project)
{
  //
  // Normalize.
  //
  // Perform normalizing transformations on the user's code.
  // This provides a consistent syntax that facilitates Htc
  // translation by avoiding some special case checking and codegen.
  // We can also avoid limitations in the downstream tools by rewriting 
  // troublesome constructs.
  //  - Add braces to the body of any structured statement where the user
  //    neglected to.
  //  - Add 'return' statements to void functions.
  //  - Declaration lifting / scope flattening.
  //  - TODO: Comma expression flattening.
  //  - Call flattening.
  //

  SageInterface::changeAllBodiesToBlocks(project);
  NormalizeSwitchCaseBodies(project);

  std::vector<SgFunctionDefinition *> fdefs = 
      SageInterface::querySubTree<SgFunctionDefinition>(project,
      V_SgFunctionDefinition);
  foreach (SgFunctionDefinition *fdef, fdefs) {
    SgFunctionDeclaration *fdecl = 
        SageInterface::getEnclosingFunctionDeclaration(fdef);
#if 1
      // Skip streaming stencil outlined functions, we want them empty.
      size_t pos = 0;
      if ((pos = fdecl->get_name().getString().find(StencilStreamPrefix)) 
          != std::string::npos && pos == 0) {
        continue;
      }
#endif
    if (isSgTypeVoid(fdecl->get_type()->get_return_type())) {
      SgStatementPtrList &stmts = fdef->get_body()->get_statements();
      SgStatement *sp = stmts.size() > 0 ? stmts.back() : 0; 
      if (sp && isSgReturnStmt(sp)) {
        continue;
      }
      SageInterface::appendStatement(SageBuilder::buildReturnStmt(),
        fdef->get_body());
    }
  }
}


static void NormalizeSwitchCaseBodies(SgProject *project) {
  class Visitor: public AstSimpleProcessing {
    virtual void visit(SgNode *n) {
#if 0
        std::cerr << __FUNCTION__ << " called for a node of kind " <<
            n->sage_class_name() << std::endl;
#endif
      switch (n->variantT()) {
      case V_SgCaseOptionStmt: {
        SgCaseOptionStmt *nc = isSgCaseOptionStmt(n);
        if (!isSgBasicBlock(nc->get_body())) {
          SgBasicBlock *newBody = SageBuilder::buildBasicBlock(nc->get_body());
          nc->set_body(newBody);
          newBody->set_parent(nc);
        }
        break;
        }
      case V_SgDefaultOptionStmt: {
        SgDefaultOptionStmt *nc = isSgDefaultOptionStmt(n);
        if (!isSgBasicBlock(nc->get_body())) {
          SgBasicBlock *newBody = SageBuilder::buildBasicBlock(nc->get_body());
          nc->set_body(newBody);
          newBody->set_parent(nc);
        }
        break;
        }
      default:
        break;
      }
    }
  };

    Visitor().traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}



//-------------------------------------------------------------------------
// Flatten/un-nest call expressions so that each statement has only one
// function call, and every call expression is assigned to a new temporary.
//
// For example, 
//   i = foo(8 * bar(2 + baz()));
// is transformed to:
//   t1 = baz();
//   t2 = bar(2 + t1);
//   t3 = foo(8 * t2);
//   i = t3;
//
//-------------------------------------------------------------------------
class FlattenCalls : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgBasicBlock:
    case V_SgIfStmt:
    case V_SgSwitchStatement:
      currAncestorStmt.push(isSgStatement(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in call flattening");
      break;
    default:
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
    case V_SgBasicBlock:
    case V_SgIfStmt:
    case V_SgSwitchStatement:
      currAncestorStmt.pop();
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionCallExp(SgFunctionCallExp *FCE);

  std::vector<SgFunctionCallExp *> flattenList;
  std::stack<SgStatement *> currAncestorStmt;
};


void FlattenCalls::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  flattenList.clear();
}


// Iterate over the list of call expressions, flattening each one.
// Each call expression 'foo()' is rewritten as an assignment statement
// 't = foo()', and the original expression replaced by a reference to 't'.
void FlattenCalls::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  foreach (SgFunctionCallExp *fce, flattenList) {
    // Insert point is just before the statement enclosing the original
    // call expression.  This also has the effect of putting the new
    // assignments in the proper execution order.
    SgStatement *insertPoint = SageInterface::getEnclosingStatement(fce);
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

    // Build a decl for the new temporary to hold call result.
    SgName varName = "__htc_t_fn";
    varName << ++SageInterface::gensym_counter; 
    SgType *varType = fce->get_type();
    SgVariableDeclaration *varDecl = 
        SageBuilder::buildVariableDeclaration(varName, varType, 0, 
                                              fdef->get_body());
    SageInterface::prependStatement(varDecl, fdef->get_body());

    // Build and insert 'newtemp = foo()' assignment statement.
    // Replace old call expression with a reference to the new temporary. 
    SgVarRefExp *lhs = SageBuilder::buildVarRefExp(varDecl);
    SgVarRefExp *lhsCopy = SageInterface::deepCopy(lhs);
    SgStatement *newAsg =
        SageBuilder::buildAssignStatement(lhs, SageInterface::deepCopy(fce));
    SageInterface::replaceExpression(fce, lhsCopy, false /* keepOldExp */);
    SageInterface::insertStatementBefore(insertPoint, newAsg);
  }

  flattenList.clear();
}


// Visit each call expression in post order, creating an ordered list
// of calls to be flattened. 
void FlattenCalls::postVisitSgFunctionCallExp(SgFunctionCallExp *FCE)
{
  SgFunctionDefinition *callerFuncDef =
      SageInterface::getEnclosingProcedure(FCE);

  //  Convey
    if (!callerFuncDef) {
        std::cerr << "no enclosing procedure for " << FCE->unparseToString() << std::endl;
        return;
    }
  SgFunctionDeclaration *callerFD = callerFuncDef->get_declaration();

  SgFunctionDeclaration *calleeFD = FCE->getAssociatedFunctionDeclaration();

  if (!calleeFD) {
    // Call through function pointer: this is caught by the restriction
    // checker early.
    abort();
  }

  std::string calleeName = calleeFD->get_name().getString();

  // If the call doesn't return a value, or the value is ignored, don't
  // do anything; i.e., statement "printf();".
  SgType *calleeReturnType = calleeFD->get_type()->get_return_type();
  SgStatement *ancestorStmt = currAncestorStmt.top();
  if (isSgTypeVoid(calleeReturnType) || 
      (isSgExprStatement(FCE->get_parent()) && isSgBasicBlock(ancestorStmt))) {
    return;
  } 

  // Insert the call expression on the flatten list. Order is
  // important-- we're performaing a post-order traversal and adding
  // each call to be flattened to the end of the list.  This is so we
  // have proper execution order in the resulting code.
  flattenList.push_back(FCE);
}


void FlattenFunctionCalls(SgProject *project)
{
  FlattenCalls fv;
  fv.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Expand ternary conditional expressions into explicit branching.
//
// For example, 
//   x = baz(cond ? foo() : bar());
// is transformed to:
//   int t;
//   ...
//   if (cond) {
//     t = foo();
//   } else {
//     t = bar();
//   }
//   x = baz(t);
//
//-------------------------------------------------------------------------
class ExpandConditionals : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in call flattening");
      break;
    default:
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgConditionalExp:
      postVisitSgConditionalExp(dynamic_cast<SgConditionalExp *>(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgConditionalExp(SgConditionalExp *CE);

  std::vector<SgConditionalExp *> flattenList;
};


void ExpandConditionals::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  flattenList.clear();
}


// Determine if node is in either the true or false subtree of an enclosing 
// SgConditionalExp.  A pointer to the enclosing expression is returned,
// along with true or false for the true and false sides, respectively.
static std::pair<SgConditionalExp *, bool> 
getEnclosingConditionalExpAndWhichSide(SgNode *npp)
{
  std::pair<SgConditionalExp *, bool> retval = 
      std::make_pair((SgConditionalExp *)0, false);
  do {
    SgNode *nppParent = npp->get_parent();
    if (SgConditionalExp *cond = isSgConditionalExp(nppParent)) {
      if (cond->get_true_exp() == npp) {
        retval.first = cond;
        retval.second = true;
      } else if (cond->get_false_exp() == npp) {
        retval.first = cond;
        retval.second = false;
      }
    }
    npp = nppParent;
  } while ((retval.first == 0) && !isSgStatement(npp));

  return retval;
}


// Iterate over the list of conditional expressions, expanding each one.
void ExpandConditionals::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  std::map<SgConditionalExp *, SgIfStmt *> condToIf;
  std::map<SgConditionalExp *, std::pair<SgConditionalExp *, bool> > condToParentCond;

  foreach (SgConditionalExp *ce, flattenList) {
    //
    // For most newly introduced if-stmts, the insert point is just before the
    // statement enclosing the original expression.  This has the effect of 
    // putting the new statements in the proper execution order.  However, in
    // the case that a conditional was nested in the true or false side of 
    // another conditional, then the new if-stmt must be inserted into the 
    // proper branch of the containing conditional's new if-stmt.  Otherwise,
    // we potentially evaulate expressions that would not have been evaluated
    // in the original code.  So we may therefore defer inserting the if-stmt
    // into the Ast until after all conditionals have been processed, since
    // the containing conditional's new if-stmt does not yet exist.
    //
    // Note that the subtree rootes at 'ce' will disappear, so we record
    // insertPoint and parentCond here.
    std::pair<SgConditionalExp *, bool> retval = 
        getEnclosingConditionalExpAndWhichSide(ce);
    SgStatement *insertPoint = 0;
    if (retval.first == 0) {
      insertPoint = HtSageUtils::findSafeInsertPoint(ce);
    }
    
    // Build a decl for the new temporary to hold conditional result.
    // Don't build the decl if type is void (e.g., assert.h expansions).
    SgName varName = "__htc_t_ce";
    varName << ++SageInterface::gensym_counter; 
    SgType *varType = ce->get_type();
    SgVariableDeclaration *varDecl = 0;
    if (!isSgTypeVoid(varType)) {
      varDecl = 
          SageBuilder::buildVariableDeclaration(varName, varType, 0, 
                                                fdef->get_body());
      SageInterface::prependStatement(varDecl, fdef->get_body());
    }

    // Build new if-stmt and replace the conditional expression with a
    // reference to the new temp.
    //   if (cond) {
    //     t = foo();
    //   } else {
    //     t = bar();
    //   }
    //   x = baz(t);
    //
    // If conditional return type is void, don't generate assignments.
    //
    SgExpression *ncond  = SageInterface::deepCopy(ce->get_conditional_exp());
    SgExpression *ntrue  = SageInterface::deepCopy(ce->get_true_exp());
    SgExpression *nfalse  = SageInterface::deepCopy(ce->get_false_exp());
    SgStatement *trueStmt = 0, *falseStmt = 0;
    if (isSgTypeVoid(varType)) {
      trueStmt = SageBuilder::buildExprStatement(ntrue);
      falseStmt = SageBuilder::buildExprStatement(nfalse);
      SageInterface::replaceExpression(ce,
          SageBuilder::buildNullExpression(), false /* keepOldExp */);
    } else {
      trueStmt = SageBuilder::buildAssignStatement(
          SageBuilder::buildVarRefExp(varDecl), ntrue);
      falseStmt = SageBuilder::buildAssignStatement(
          SageBuilder::buildVarRefExp(varDecl), nfalse);
      SageInterface::replaceExpression(ce,
          SageBuilder::buildVarRefExp(varDecl), false /* keepOldExp */);
    }
    SgIfStmt *ifStmt = SageBuilder::buildIfStmt(
        SageBuilder::buildExprStatement(ncond),
        SageBuilder::buildBasicBlock(trueStmt),
        SageBuilder::buildBasicBlock(falseStmt));

    condToIf[ce] = ifStmt;

    // Insert new if-stmt if it is not deferred.
    if (retval.first == 0) {
      SageInterface::insertStatementBefore(insertPoint, ifStmt);
    } else {
      condToParentCond[ce] = retval;
    }
  }

  //
  // Insert deferred if-stmts.
  // Note that we must be careful using 'ce' below, since the original Ast
  // rooted at 'ce' no longer exists.
  //
  foreach (SgConditionalExp *ce, flattenList) {
    if (condToParentCond.find(ce) != condToParentCond.end()) {
      std::pair<SgConditionalExp *, bool> parentCond = condToParentCond[ce];
      SgIfStmt *outerIf = condToIf[parentCond.first];
      SgIfStmt *innerIf = condToIf[ce];
      assert(outerIf && innerIf);
      SgBasicBlock *insertBlock = isSgBasicBlock(parentCond.second ? 
          outerIf->get_true_body() : outerIf->get_false_body());
      assert(insertBlock);
      SageInterface::prependStatement(innerIf, insertBlock);
    }
  }

  flattenList.clear();
}


// Visit each conditional expression in post order, creating an ordered list
// to be flattened. 
void ExpandConditionals::postVisitSgConditionalExp(SgConditionalExp *CE)
{
  // Insert the conditional expression on the flatten list. Order is
  // important-- we're performaing a post-order traversal and adding
  // each expression to be flattened to the end of the list.  This is
  // so we have proper execution order in the resulting code.
  flattenList.push_back(CE);
}


void ExpandConditionalExpressions(SgProject *project)
{
  ExpandConditionals ev;
  ev.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Flatten comma expressions.
//
// For example, 
//   i = (expr1, expr2, expr3);
// is transformed to:
//   expr1;
//   expr2;
//   i = expr3;
//
//-------------------------------------------------------------------------
class FlattenCommas : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in comma flattening");
      break;
    default:
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgCommaOpExp:
      postVisitSgCommaOpExp(dynamic_cast<SgCommaOpExp  *>(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgCommaOpExp(SgCommaOpExp *CE);

  std::vector<std::pair<SgExpression *, SgStatement *> > insertList;
};


void FlattenCommas::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  insertList.clear();
}


void FlattenCommas::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  std::pair<SgExpression *, SgStatement *> fl;
  foreach (fl, insertList) {
    SgExpression *ce = fl.first;
    SgStatement *insertPoint = fl.second; 
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

    SgStatement *newStmt = SageBuilder::buildExprStatement(ce);
    SageInterface::insertStatementBefore(insertPoint, newStmt);
  }

  insertList.clear();
}


// Visit each comma expression in post order, creating an ordered list
// of insertions to be made and collapsing comma expressions on the way
// up the tree.
void FlattenCommas::postVisitSgCommaOpExp(SgCommaOpExp *CE)
{
  SgExpression *lhsCopy = SageInterface::deepCopy(CE->get_lhs_operand());
  insertList.push_back(std::make_pair(lhsCopy, 
      SageInterface::getEnclosingStatement(CE)));
  SgExpression *rhsCopy = SageInterface::deepCopy(CE->get_rhs_operand());
  SageInterface::replaceExpression(CE, rhsCopy, false /* keepOldExp */);
}


void FlattenCommaExpressions(SgProject *project)
{
  FlattenCommas fv;
  fv.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Perform declaration "lifting".
//
// This class performs two related transformations:
// 1. Separate initializers from declarations.
//    Rewrite every variable declaration of the form
//      type x = initializer;
//    as
//      type x;
//      x = initializer; 
//
// 2. Relocate (lift) declarations so that they are at the beginning of
//    the function scope and not intermingled with executable statements.
//
// Since the user's entire scoping structure is eventually flattened
// and destroyed as we transform to an FSM, all variables must be placed
// into the outermost function scope. In this way, all symbols will be
// accessible across FSM states.
//-------------------------------------------------------------------------
class LiftDecls : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in comma flattening");
      break;
    default:
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgInitializedName:
      postVisitSgInitializedName(dynamic_cast<SgInitializedName *>(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgInitializedName(SgInitializedName *IN);

  std::vector<SgInitializedName *> xformList;
};


void LiftDecls::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  xformList.clear();
}


void LiftDecls::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  // Build an assignment statement for each initializer, and place the 
  // new assignment before the original declaration.  Remove the initializer
  // from the VarDecl, in preparation for its' relocation.
  foreach (SgInitializedName *xl, xformList) {
    SgAssignInitializer *ai = isSgAssignInitializer(xl->get_initializer());
    SgStatement *insertPoint = SageInterface::getEnclosingStatement(xl);
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

    if (ai) {
      SgVarRefExp *lhs = SageBuilder::buildVarRefExp(
          isSgVariableDeclaration(xl->get_parent()));
      SgExprStatement *asgStmt = SageBuilder::buildAssignStatement(lhs,
          isSgExpression(SageInterface::deepCopy(ai->get_operand())));
      SageInterface::insertStatementBefore(insertPoint, asgStmt);
      SageInterface::deleteAST(ai);
      xl->set_initializer(0);
    }
  }

  // Move the VarDecl for each SgInitializedName, possibly with renaming, 
  // to the beginning of the function.
  SgStatement *lastInsertion = 0;
  foreach (SgInitializedName *xl, xformList) {
    SgVariableDeclaration *varDecl = isSgVariableDeclaration(xl->get_parent());
    assert(varDecl && "expected a SgVariableDeclaration");

    SageInterface::removeStatement(varDecl);
    if (!lastInsertion) {
      SageInterface::prependStatement(varDecl, fdef->get_body());
    } else {
      SageInterface::insertStatementAfter(lastInsertion, varDecl, false);
    }
    lastInsertion = varDecl;

#if 1
    // Since we separate initializers from the declarations, any 'const'
    // qualifier needs to be removed.  TBD: Is this too big a hammer?
    if (isSgModifierType(xl->get_type())) {
      SgConstVolatileModifier &cvm = isSgModifierType(xl->get_type())->
          get_typeModifier().get_constVolatileModifier();
      if (cvm.isConst()) {
        //std::cerr << "Note: Htc hasremoved 'const'" << std::endl;
        cvm.unsetConst();
      }
    }
#endif

    // If old declartation was already in the same scope as fdef->get_body,
    // leave it alone.
    if (xl->get_scope() == fdef->get_body()) {
        continue;
    }

    // Check for a name collision in the target scope, renaming if needed.
    SgName nm = xl->get_name();
    SgSymbolTable *symtab = xl->get_scope()->get_symbol_table();
    SgSymbol *oldSym = symtab->find(xl);
    assert(oldSym);
    OmpUplevelAttribute *attr = NULL;
    if (isSgVariableSymbol(oldSym)) {
        attr = getOmpUplevelAttribute(isSgVariableSymbol(oldSym));
    }
    symtab->remove(oldSym);
    delete oldSym;

    SgSymbolTable *targetSymtab = fdef->get_body()->get_symbol_table();
    if (targetSymtab->exists(nm)) {
      nm << "__" << ++SageInterface::gensym_counter; 
      xl->set_name(nm);
    }
    xl->set_scope(fdef->get_body());
    SgVariableSymbol *newSym = new SgVariableSymbol(xl);
    newSym->set_parent(targetSymtab);
    targetSymtab->insert(nm, newSym);
    if (attr) {
        setOmpUplevelAttribute(newSym, attr->getUplevel(), attr->isParameter());
        delete(attr);
    }
  }

  xformList.clear();
}


void LiftDecls::postVisitSgInitializedName(SgInitializedName *IN)
{
  // TODO: VarDefinition as well (for all of lifting code).
  // TODO: Lift class/struct declarations as well.
  SgNode *parent = IN->get_parent();
  if (isSgVariableDeclaration(parent)) {
    // Make sure this is an actual declaration (e.g., not a struct field).
    if (!SageInterface::getEnclosingClassDefinition(parent)) {
      xformList.push_back(IN);
    }
  }
}


void LiftDeclarations(SgProject *project)
{
  LiftDecls dv;
  dv.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Expand the compound assignment operators (e.g., +=).
//
// Example: 
//   i ^= 0x1;
// is transformed to:
//   i = i ^ 0x1;
//
//-------------------------------------------------------------------------
class ExpandCompoundAssignOps : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgCompoundAssignOp(S)) {
      visitSgCompoundAssignOp(dynamic_cast<SgCompoundAssignOp *>(S));
    }
  }

private: 
  void visitSgCompoundAssignOp(SgCompoundAssignOp *CE);
};


// Visit each compound assign op in postorder, replacing expressions on
// the way up the tree.
void ExpandCompoundAssignOps::visitSgCompoundAssignOp(SgCompoundAssignOp *CE)
{
  // lhs *= rhs;    ==>    lhs = lhs + rhs;
  SgExpression *lhsCopy = SageInterface::deepCopy(CE->get_lhs_operand());
  SgExpression *rhsCopy = SageInterface::deepCopy(CE->get_rhs_operand());
  SgExpression *newOper = 0;
  switch (CE->variantT()) {
  case V_SgAndAssignOp:
    newOper = SageBuilder::buildBitAndOp(lhsCopy, rhsCopy); 
    break;
  case V_SgDivAssignOp:
    newOper = SageBuilder::buildDivideOp(lhsCopy, rhsCopy); 
    break;
  case V_SgIorAssignOp:
    newOper = SageBuilder::buildBitOrOp(lhsCopy, rhsCopy); 
    break;
  case V_SgLshiftAssignOp:
    newOper = SageBuilder::buildLshiftOp(lhsCopy, rhsCopy); 
    break;
  case V_SgMinusAssignOp:
    newOper = SageBuilder::buildSubtractOp(lhsCopy, rhsCopy); 
    break;
  case V_SgModAssignOp:
    newOper = SageBuilder::buildModOp(lhsCopy, rhsCopy); 
    break;
  case V_SgMultAssignOp:
    newOper = SageBuilder::buildMultiplyOp(lhsCopy, rhsCopy); 
    break;
  case V_SgPlusAssignOp:
    newOper = SageBuilder::buildAddOp(lhsCopy, rhsCopy); 
    break;
  case V_SgRshiftAssignOp:
    newOper = SageBuilder::buildRshiftOp(lhsCopy, rhsCopy); 
    break;
  case V_SgXorAssignOp:
    newOper = SageBuilder::buildBitXorOp(lhsCopy, rhsCopy); 
    break;
  default:
    assert(0 && "unexpected assign op type");
    break;
  }
  SgExpression *lhsCopy2 = SageInterface::deepCopy(CE->get_lhs_operand());
  SgExpression *newAsg = SageBuilder::buildAssignOp(lhsCopy2, newOper);
  SageInterface::replaceExpression(CE, newAsg, false /* keepOldExp */);
}


void ExpandCompoundAssignOperators(SgProject *project)
{
  ExpandCompoundAssignOps ev;
  ev.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Introduce cast of rhs of assignment to lhs type.
// HTV is very "picky".
//-------------------------------------------------------------------------
class AddCastsForAssignments : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgAssignOp(S)) {
      visitSgAssignOp(dynamic_cast<SgAssignOp *>(S));
    }
  }

private: 
  void visitSgAssignOp(SgAssignOp *CE);
};


// Visit each assign op in postorder, casting the rhs expression to the
// lhs type on the way up the tree.
void AddCastsForAssignments::visitSgAssignOp(SgAssignOp *assign)
{
    SgExpression *lhs = assign->get_lhs_operand();
    SgExpression *rhs = assign->get_rhs_operand();

    SgType *lhstype = lhs->get_type();

    SgExpression *newRhs = 
        SageBuilder::buildCastExp(SageInterface::deepCopy(rhs), lhstype);
    SageInterface::replaceExpression(rhs, newRhs, false /* keepOldExp */);
}


void AddCastsForRhsOfAssignments(SgProject *project)
{
  AddCastsForAssignments ac;
  ac.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}




//-------------------------------------------------------------------------

class NumSideEffects {
public:

    unsigned int getNumSideEffects(SgNode *S) {
        if (data.find(S) == data.end()) {
            data[S] = 0;
        }
        return data[S];
    }

    void setNumSideEffects(SgNode *S, unsigned int number) {
        data[S] = number;
    }

    void reset() {
        data.clear();
    }

private:
    std::map<SgNode*, unsigned int> data;
};


//-------------------------------------------------------------------------
// Assign Side Effect Numbering
//
// For each node, calculates the number of ++ and -- operators at and
// beneath it.
//
//-------------------------------------------------------------------------
class AssignSideEffectNumbering : public AstSimpleProcessing {
public:
    AssignSideEffectNumbering(NumSideEffects *NS) :
        ns(NS),
        firstSideEffect(0)
    {}

    void visit(SgNode *N) {
        unsigned int number;
        if (isSgUnaryOp(N)) {
            switch (N->variantT()) {
            case V_SgPlusPlusOp: 
            case V_SgMinusMinusOp:
                if (!firstSideEffect) {
                    firstSideEffect = isSgExpression(N);
                }
                number = ns->getNumSideEffects(isSgUnaryOp(N)->get_operand()) + 1;
                break;
            default:
                number = ns->getNumSideEffects(isSgUnaryOp(N)->get_operand());
                break;
            }
        } else if (isSgBinaryOp(N)) {
            SgBinaryOp *op = isSgBinaryOp(N);
            number = ns->getNumSideEffects(op->get_lhs_operand()) +
                ns->getNumSideEffects(op->get_rhs_operand());
        } else {
            number = 0;
        }

        ns->setNumSideEffects(N, number);
    }
    
    SgExpression *getFirstSideEffect() {
        return firstSideEffect;
    }

private:
    NumSideEffects *ns;
    SgExpression *firstSideEffect;
};


// Applies side effect numbering to the tree rooted at S,
// returning the first node with side effects visited in a
// postorder traversal of the tree.
SgExpression *AssignSideEffectAttributes(SgExpression *S, NumSideEffects *ns) {
    AssignSideEffectNumbering as(ns);

    as.traverse(S, postorder);

    return as.getFirstSideEffect();
}


//-------------------------------------------------------------------------
// Flatten Side Effects
//
// Pulls -- and ++ out of expressions
//
//-------------------------------------------------------------------------
class FlattenSides : public AstSimpleProcessing {
public:
    void visit(SgNode *S) {

        SgFunctionDefinition* fndef = isSgFunctionDefinition(S);
        FunctionDefs.push_back(fndef);
    }

    void doReplacements();

private: 
    void visitSgExpression(SgExpression *S, SgFunctionDefinition *fndef);
    SgExpression *findTopMostNodeWithOneSideEffect(SgExpression *start,
                                                   NumSideEffects *ns);
    Rose_STL_Container<SgFunctionDefinition *> FunctionDefs;
};


// Find root of tree with at most one side effect starting from a side
// effect node.
SgExpression *FlattenSides::findTopMostNodeWithOneSideEffect(
                               SgExpression *start, NumSideEffects *ns) {
    SgExpression *node = start;

    if (!node) return 0;
    assert(ns->getNumSideEffects(node) == 1);

    while ((ns->getNumSideEffects(node->get_parent()) == 1) &&
           (!isSgAssignOp(node->get_parent())) &&
           (!isSgPointerDerefExp(node->get_parent())) &&
           (!isSgPntrArrRefExp(node->get_parent())) ) {
        node = isSgExpression(node->get_parent());
    }

    return node;
}

// Visit each compound assign op in postorder, replacing expressions on
// the way up the tree.
void FlattenSides::visitSgExpression(SgExpression *S, 
                                     SgFunctionDefinition *fndef) {

    NumSideEffects *ns = new NumSideEffects();
    SgStatement *insertPoint = HtSageUtils::findSafeInsertPoint(S);

    assert(insertPoint);

    // node will be first node to replace
    SgExpression *node = AssignSideEffectAttributes(S, ns);

    // skip plain standalone i++; etc
    if ( (S == node) && 
         isSgExprStatement(node->get_parent()) &&
         (S->get_type()->stripTypedefsAndModifiers()->isIntegerType() || 
          isSgPointerType(S->get_type()))) {

#if DEBUG
        std::cout << "SIMPLE REPLACEMENT FOR " << S->unparseToString() <<
            std::endl;
#endif

        // expand simple ++X, --X
        SgUnaryOp *op = isSgUnaryOp(node);
        SgExpression *child = op->get_operand();
        SgUnaryOp::Sgop_mode mode = op->get_mode();

        // really technically need to do something about this?
        if (mode == SgUnaryOp::prefix) {
        }
                
        SgExpression *copy1 = SageInterface::deepCopy(child);
        SgExpression *copy2 = SageInterface::deepCopy(child);
        
        SgExpression *ival;
        if (isSgPlusPlusOp(node)) {
            ival = SageBuilder::buildIntVal(1);
        } else {
            ival = SageBuilder::buildIntVal(-1);
        }
        
        // Create assignment statement
        SgExpression *assignment = 
            SageBuilder::buildAssignOp(
                     copy1,
                     SageBuilder::buildAddOp(copy2, ival));        

        SageInterface::replaceExpression(S, assignment, false /* keep old */);

        return;
    }

    unsigned int numSideEffects = ns->getNumSideEffects(S);

#if DEBUG
    std::cout << "numSideEffects for " << S->unparseToString() << " is " <<
        numSideEffects << std::endl;
#endif

    for (unsigned int i = 0; i < numSideEffects; i++) {

        node = AssignSideEffectAttributes(S, ns);

        SgExpression *top;

#if DEBUG
        std::cout << "S is " << S->unparseToString() << std::endl;
        std::cout << "expression is " << node->unparseToString();
        if (node->get_parent()) {
            std::cout << " parent is a " << node->get_parent()->sage_class_name();
        }
        std::cout << std::endl;
#endif

        // Handle side effect node
        assert((node->variantT() == V_SgPlusPlusOp) ||
               (node->variantT() == V_SgMinusMinusOp));

        // replace side effect node with it's child
        SgUnaryOp *op = isSgUnaryOp(node);
        SgExpression *child = op->get_operand();
        SgUnaryOp::Sgop_mode mode = op->get_mode();
                
        SgExpression *copy1 = SageInterface::deepCopy(child);
        SgExpression *copy2 = SageInterface::deepCopy(child);
        SgExpression *copy3 = SageInterface::deepCopy(child);
        
        SgExpression *ival;
        if (isSgPlusPlusOp(node)) {
            ival = SageBuilder::buildIntVal(1);
        } else {
            ival = SageBuilder::buildIntVal(-1);
        }
        
        // Add increment statement before current insert point
        SgStatement *increment = 
            SageBuilder::buildAssignStatement(
                     copy1,
                     SageBuilder::buildAddOp(copy2, ival));
        SageInterface::insertStatementBefore( insertPoint, increment);
        
        SageInterface::replaceExpression(op, copy3, true /* keep old */);
        ns->setNumSideEffects(copy3, 1);

        top = findTopMostNodeWithOneSideEffect(copy3, ns);
        assert(top);

        // Create temporary and assign it to 'top'
        SgName varName = "__htc_t_pp_mm";
        varName << ++SageInterface::gensym_counter; 
        SgType *varType = top->get_type();
        SgVariableDeclaration *varDecl = 
            SageBuilder::buildVariableDeclaration(varName, varType, 0, 
                                                  fndef->get_body());
        SageInterface::prependStatement(varDecl, fndef->get_body());

        SgStatement *assignToTemp = 
            SageBuilder::buildAssignStatement(
                 SageBuilder::buildVarRefExp(varDecl),
                 SageInterface::deepCopy(top));
        // insert temp assignment before or after increment, depending on mode
        SageInterface::insertStatement(increment, 
                                       assignToTemp,
                                       mode == SgUnaryOp::postfix);

        // replace top with reference to temp
        SageInterface::replaceExpression(top, 
                                         SageBuilder::buildVarRefExp(varDecl), 
                                         true /* keep old expression */);
    }
}


void FlattenSides::doReplacements() {
    Rose_STL_Container< SgNode * > sgExpressions;
    Rose_STL_Container<SgFunctionDefinition *>::iterator fiter;

    std::vector<SgNode *> stlExpressions;

    for (fiter = FunctionDefs.begin(); fiter != FunctionDefs.end(); fiter++) {
        SgFunctionDefinition *fndef = isSgFunctionDefinition(*fiter);
        if (fndef != NULL) {   // should assert this?
            sgExpressions = NodeQuery::querySubTree (fndef, V_SgExpression);
            Rose_STL_Container<SgNode*>::iterator iter;
            for (iter = sgExpressions.begin();
                 iter != sgExpressions.end();
                 iter++) {
                SgExpression *node = isSgExpression(*iter);

                // parent pointers can become invalid if node is replaced
                // as part of another expression replacement before we 
                // reach it, so copy exactly the ones we are interested in
                // to another list, and do the visits from that list.
                if (node && (!isSgExpression(node->get_parent()) ||
                             isSgExprListExp(node->get_parent()))) {
                    stlExpressions.push_back(node);
                }
            }

            std::vector<SgNode*>::iterator iter2;
            for (iter2 = stlExpressions.begin();
                 iter2 != stlExpressions.end();
                 iter2++) {
                SgExpression *node = isSgExpression(*iter2);
                visitSgExpression(node, fndef);
            }
            stlExpressions.clear();
        }
    }
}

void FlattenSideEffectsInStatements(SgProject *project) {
  FlattenSides fs;
  fs.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
  fs.doReplacements();
}

//-------------------------------------------------------------------------
// Flatten aggregates used as array subscripts
//-------------------------------------------------------------------------
class FlattenArrayIndices : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in array index flattening");
      break;
    default:
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
      //    case V_SgDotExp:
    case V_SgPntrArrRefExp:
    case V_SgPointerDerefExp:
        postVisitAggregate(isSgExpression(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitAggregate(SgExpression *AG);

  std::vector<SgExpression *> flattenList;
};


void FlattenArrayIndices::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  flattenList.clear();
}


// Iterate over the list of aggregates used as indices, flattening each one.
// Each index is rewritten as an assignment statement 't = a[i]', and the 
// original expression replaced by a reference to 't'.
void FlattenArrayIndices::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  foreach (SgExpression *agg, flattenList) {
    // Insert point is just before the statement enclosing the original
    // expression expression.  This also has the effect of putting the new
    // assignments in the proper execution order.
    SgStatement *insertPoint = SageInterface::getEnclosingStatement(agg);
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

    // Build a decl for the new temporary
    SgName varName = "__htc_t_arrptr";
    varName << ++SageInterface::gensym_counter; 
    SgType *varType = agg->get_type(); //buildIntType();
    SgVariableDeclaration *varDecl = 
        SageBuilder::buildVariableDeclaration(varName, varType, 0, 
                                              fdef->get_body());

    SageInterface::prependStatement(varDecl, fdef->get_body());

    // Build and insert 'newtemp = foo()' assignment statement.
    // Replace old call expression with a reference to the new temporary. 
    SgVarRefExp *lhs = SageBuilder::buildVarRefExp(varDecl);
    SgVarRefExp *lhsCopy = SageInterface::deepCopy(lhs);
    SgStatement *newAsg =
        SageBuilder::buildAssignStatement(lhs, SageInterface::deepCopy(agg));
    SageInterface::replaceExpression(agg, lhsCopy, false /* keepOldExp */);
    SageInterface::insertStatementBefore(insertPoint, newAsg);

    // std::cerr << "has symbol? " << varDecl->get_variables().at(0)->search_for_symbol_from_symbol_table()->get_parent()->isChild(varDecl->get_variables().at(0)->search_for_symbol_from_symbol_table()) << std::endl;
  }

  flattenList.clear();
}

bool hasEmbeddedAggOrCall(SgExpression *exp) {
    Rose_STL_Container<SgNode *> list;

    list = NodeQuery::querySubTree (exp, V_SgPntrArrRefExp);
    if (list.size()) return true;

    list = NodeQuery::querySubTree (exp, V_SgDotExp);
    if (list.size()) return true;

    list = NodeQuery::querySubTree (exp, V_SgPointerDerefExp);
    if (list.size()) return true;

    list = NodeQuery::querySubTree (exp, V_SgFunctionCallExp);
    if (list.size()) return true;

    // uplevel variables will translate to a function call
    list = NodeQuery::querySubTree (exp, V_SgVarRefExp);
    for (int i = 0; i < list.size(); i++) {
        SgVarRefExp *vre = isSgVarRefExp(list.at(i));
        SgInitializedName *iname = vre->get_symbol()->get_declaration();
        if (iname) {
            SgVariableSymbol *sym = 
                isSgVariableSymbol(iname->get_symbol_from_symbol_table());
            
            if (sym && getOmpUplevelAttribute(sym)) {
                return true;
            }
        }
    }

    return false;
}

extern SgStatement *findEnclosingStatement( SgNode *node);

// Visit each aggregate expression in post order, creating an ordered list
// of aggregates used as array indices to be flattened. 
void FlattenArrayIndices::postVisitAggregate(SgExpression *AGG)
{

#if DEBUG
    std::cout << "postVisitAggregate called for " << 
        AGG->unparseToString() << " parent " << 
        AGG->get_parent()->sage_class_name()  << std::endl;
#endif

    SgStatement *top = findEnclosingStatement(AGG);

    SgNode *node = AGG;
    SgNode *parent = node->get_parent();

    if (isSgPntrArrRefExp(node)) {
        SgPntrArrRefExp *ar = isSgPntrArrRefExp(node);
        SgExpression *rhs = ar->get_rhs_operand();

        if (hasEmbeddedAggOrCall(rhs)) {
            flattenList.push_back(rhs);
        }

        SgExpression *lhs = ar->get_lhs_operand();

        if (SageInterface::isPointerType(lhs->get_type())) {
            flattenList.push_back(lhs);
        } 
    } else if (SgPointerDerefExp *pd = isSgPointerDerefExp(node)) {
        SgExpression *kid = pd->get_operand();
        if (hasEmbeddedAggOrCall(kid)) {
            flattenList.push_back(kid);
        }
    }

}


void FlattenArrayIndexExpressions(SgProject *project)
{
  FlattenArrayIndices fv;
  fv.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}

//-------------------------------------------------------------------------
// Flatten chains of assignOps where the lhs will generate a write call
// so that each statement only has one expression that will generate a 
// write call.
//
// For example, 
//   b[0] = b[1] = b[2] = b[3] = 4;
// is transformed to:
//   t1 = 4;
//   b[3] = t1;
//   t2 = t1;
//   b[2] = t2;
//   t3 = t2;
//   b[1] = t3;
//   b[0] = t3;
//
//-------------------------------------------------------------------------
class FlattenAssigns : public AstPrePostProcessing {
public:
  void preOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      preVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgForStatement:
    case V_SgWhileStmt:
    case V_SgDoWhileStmt:
      assert(0 && "unexpected loop in assign flattening");
      break;
    default:
      break;
    }
  }

  void postOrderVisit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      postVisitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    case V_SgAssignOp:
      postVisitSgAssignOp(dynamic_cast<SgAssignOp *>(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgAssignOp(SgAssignOp *assign);

  std::vector<SgAssignOp *> flattenList;
};


void FlattenAssigns::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  flattenList.clear();
}


// Iterate over the list of assign ops, flattening each one.
// Each assignOp rewritten as
// 't = rhs; lhs = t', and the original expression replaced by 
// a reference to 't'.
void FlattenAssigns::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
    SgFunctionDefinition *fdef = FD->get_definition();
    if (!fdef) {
        return;
    }

    foreach (SgAssignOp *assign, flattenList) {

        SgStatement *insertPoint = HtSageUtils::findSafeInsertPoint(assign);
        assert(insertPoint);

        SgExpression *rhs = assign->get_rhs_operand();
        SgExpression *lhs = assign->get_lhs_operand();

        // Build a decl for the new temporary to hold rhs value.
        SgName varName = "__htc_t_assignOp";
        varName << ++SageInterface::gensym_counter; 
        SgType *varType = rhs->get_type();
        SgVariableDeclaration *varDecl = 
            SageBuilder::buildVariableDeclaration(varName, varType, 0, 
                                                  fdef->get_body());
        SageInterface::prependStatement(varDecl, fdef->get_body());

        // Build and insert 'newtemp = rhs' assignment statement.
        SgStatement *assignTemp =
            SageBuilder::buildAssignStatement(
                SageBuilder::buildVarRefExp(varDecl),
                SageInterface::deepCopy(rhs));
        SageInterface::insertStatementBefore(insertPoint, assignTemp);

        // Build and insert 'lhs = newtemp' assignment statement.
        SgStatement *assignlhs =
            SageBuilder::buildAssignStatement(
                SageInterface::deepCopy(lhs),
                SageBuilder::buildVarRefExp(varDecl));
        SageInterface::insertStatementBefore(insertPoint, assignlhs);

        // Replace old rhs with a reference to the new temporary. 
        SgVarRefExp *refTemp = SageBuilder::buildVarRefExp(varDecl);
        SageInterface::replaceExpression(assign, refTemp, false /* keepOld */);
    }

    flattenList.clear();
}


// Visit each assignOp in post order, creating an ordered list
// of assignOps to be flattened. 
void FlattenAssigns::postVisitSgAssignOp(SgAssignOp *assign)
{
    // Skip assigns that are at the statement level already
    if (isSgExprStatement(assign->get_parent())) {
        return;
    }

    SgExpression *lhs = assign->get_lhs_operand();

    if (hasEmbeddedAggOrCall(lhs)) {
        flattenList.push_back(assign);
    }

    return;
} 

void FlattenAssignChains(SgProject *project)
{
  FlattenAssigns fa;
  fa.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Convert SgArrowExp to SgPointerDerefExp and SgDot
//
// p->i   becomes  (*p).i
//-------------------------------------------------------------------------

class ExpandSgArrowExps : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgArrowExp(S)) {
        visitSgArrowExp(isSgArrowExp(S));
    }
  }

private: 
  void visitSgArrowExp(SgArrowExp *arrow);
};


// Visit each SgArrowExp in postorder, replacing expressions on
// the way up the tree.
void ExpandSgArrowExps::visitSgArrowExp(SgArrowExp *arrow)
{
  SgExpression *lhsCopy = SageInterface::deepCopy(arrow->get_lhs_operand());
  SgExpression *rhsCopy = SageInterface::deepCopy(arrow->get_rhs_operand());

  SgExpression *pointerDeref = SageBuilder::buildPointerDerefExp(lhsCopy);
  SgExpression *newExpr = SageBuilder::buildDotExp(pointerDeref, rhsCopy);

  SageInterface::replaceExpression(arrow, newExpr, false /* keepOldExp */);
}


void ExpandSgArrowExpressions(SgProject *project)
{
  ExpandSgArrowExps ev;
  ev.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}



//-------------------------------------------------------------------------
// Remove #ifdef/#else/#endif preprocessing info.
//-------------------------------------------------------------------------

class stripIfdefs:public AstSimpleProcessing
{
  public:
    virtual void visit (SgNode * n);
};

void
stripIfdefs::visit (SgNode * n) {
  // On each node look for any comments of CPP directives
  SgLocatedNode *locatedNode = isSgLocatedNode (n);
  PreprocessingInfo::RelativePositionType relativePosition;
  bool remove;

  if (locatedNode != NULL) {

    AttachedPreprocessingInfoType *comments =
      locatedNode->getAttachedPreprocessingInfo ();

    if (comments != NULL) {
        remove = false;

      AttachedPreprocessingInfoType::iterator i;
      for (i = comments->begin (); i != comments->end (); i++) {
          switch ((*i)->getTypeOfDirective()) {
          case PreprocessingInfo::CpreprocessorIfdefDeclaration:
          case PreprocessingInfo::CpreprocessorIfndefDeclaration:
          case PreprocessingInfo::CpreprocessorIfDeclaration:
          case PreprocessingInfo::CpreprocessorDeadIfDeclaration:
          case PreprocessingInfo::CpreprocessorElseDeclaration:
          case PreprocessingInfo::CpreprocessorElifDeclaration:
          case PreprocessingInfo::CpreprocessorEndifDeclaration:
              relativePosition = (*i)->getRelativePosition();
              remove = true;
              break;
          default:
              break;
          }
      }
      if (remove) {
#if DEBUG
          int x = 0;
          for (i = comments->begin (); i != comments->end (); i++) {
              printf("   %2d %s\n", x, (*i)->getString().c_str());
              x++;
          }
#endif
          AttachedPreprocessingInfoType buffer;
          SageInterface::cutPreprocessingInfo(locatedNode,
                               relativePosition,
                               buffer);
          // std::cout << "buffer size is " << buffer.size() << std::endl;
      }
    }
  }
}

void StripPreprocessingInfo(SgProject *project) {
    stripIfdefs sid;
    sid.traverseInputFiles (project, preorder, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Remove empty 'else' clauses.  
//
// This late pass cleans up empty else branches that were inserted by the
// normalizer, which can make for more readable translator output.
//
// Example: 
//   if (x > 2) {
//     y = 3;
//   } else {
//   } 
// is transformed to:
//   if (x > 2) {
//     y = 3;
//   }
//
//-------------------------------------------------------------------------
void RemoveEmptyElseBlocks(SgProject *project)
{
  class RemoveEmptyElseBlocksVisitor : public AstSimpleProcessing {
  public:
    void visit(SgNode *S) {
      // Visit each if-stmt in postorder, deleting empty else blocks on
      // the way up the tree.
      if (SgIfStmt *IS = isSgIfStmt(S)) {
        SgStatement *ebody = IS->get_false_body();
        if (ebody && isSgBasicBlock(ebody) &&
            isSgBasicBlock(ebody)->get_statements().size() == 0) {
           SageInterface::deleteAST(ebody);
           IS->set_false_body(0);
        }  
      }
    }
  
  private: 
    void visitSgIfStmt(SgIfStmt *IS);
  } v;

    v.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Preserve preprocessor info for SgBasicBlocks.
//
// This pass moves preprocessor information from some SgBasicBlocks to new
// anchor statements before or after the original block.  In particular, we
// do this for the blocks that will later be flattened by flattenBlocks.
// The flattenBlocks code does not preserve the preprocessor information,
// which can result in missing or mismatched if/else/endif directives, etc.
//
//-------------------------------------------------------------------------
void PreservePreprocInfoForBlocks(SgProject *project)
{
  class Visitor : public AstSimpleProcessing {
  public:
    void visit(SgNode *S) {
      SgBasicBlock *BS = isSgBasicBlock(S);
      if (BS && isSgBasicBlock(BS->get_parent())) {
        // This block is nested in another-- it will later be flattened
        // by flattenBlocks.  We move the PreprocInfo later to avoid 
        // invalidating the traversal.
        processList.push_back(BS);
      }
    }
  
    std::vector<SgBasicBlock *> processList;
  } v;

    v.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
 
  foreach (SgBasicBlock *blk, v.processList) {
    AttachedPreprocessingInfoType *comments = 0;
#if 0
    comments = blk->getAttachedPreprocessingInfo();
    if (comments && comments->size() > 0) {
      std::cerr << "Moving preproc info from block:" << std::endl;
      SageInterface::dumpPreprocInfo(blk);
    }
#endif
    AttachedPreprocessingInfoType before, inside, after;
    SageInterface::cutPreprocessingInfo(blk, PreprocessingInfo::before,
        before);
    SageInterface::cutPreprocessingInfo(blk, PreprocessingInfo::after,
        after);

    if (before.size() > 0) {
      SgStatement *anchor = SageBuilder::buildNullStatement();
      SageInterface::insertStatementBefore(blk, anchor);
      SageInterface::pastePreprocessingInfo(anchor, PreprocessingInfo::before,
          before);
    } 

    if (after.size() > 0) {
      SgStatement *anchor = SageBuilder::buildNullStatement();
      SageInterface::insertStatementAfter(blk, anchor);
      SageInterface::pastePreprocessingInfo(anchor, PreprocessingInfo::after,
          after);
    } 

    comments = blk->getAttachedPreprocessingInfo();
    if (comments && comments->size() > 0) {
      // PreprocessingInfo::inside is all that remains.  We use 'move' instead
      // of 'cut' because the former allows us to change the relative position
      // type stored on the info after it has been relocated.
      SgStatement *anchor = SageBuilder::buildNullStatement();
      SageInterface::appendStatement(anchor, blk);
      SageInterface::movePreprocessingInfo(blk, anchor, 
          PreprocessingInfo::inside, PreprocessingInfo::before);
    } 
  }
}

