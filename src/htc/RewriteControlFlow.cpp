/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include <algorithm>
#include <functional>
#include <numeric>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>

#include "HtSageUtils.h"
#include "RewriteControlFlow.h"

#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 


// Build: "if (cond) { goto label; }".  (or !cond).
static SgIfStmt *makeIfWithGoto(SgExpression *cond, SgLabelStatement *labStmt,
                                bool negateCond)
{
  SgExpression *newcond = SageInterface::deepCopy(cond);
  if (negateCond) {
    newcond = SageBuilder::buildNotOp(newcond);
  }
  SgIfStmt *ifStmt = SageBuilder::buildIfStmt(
      SageBuilder::buildExprStatement(newcond),
      SageBuilder::buildBasicBlock(SageBuilder::buildGotoStatement(labStmt)),
      0 /* false body empty */);
  ifStmt->addNewAttribute("dont_process", new DontProcessAttribute());
  return ifStmt;
}


// Rewrite:
//    ...
//   continue;
//    ...
// As:
//   <<incr>>                 // (for-loop only)
//   goto loop_top;           // for-loop, while-loop, do-while-loop
//
static void rewriteActiveContinueStmts(SgBasicBlock *body,
                                       SgLabelStatement *labelTopStmt,
                                       SgExpression *incrExpr)
{
  std::vector<SgContinueStmt *> cl = SageInterface::findContinueStmts(body);
  foreach (SgContinueStmt *oldContinue, cl) {
    if (incrExpr) {
      SgExpression *copyIncr = SageInterface::deepCopy(incrExpr);
      SgExprStatement *newIncr = SageBuilder::buildExprStatement(copyIncr);
      SageInterface::insertStatementBefore(oldContinue, newIncr);
    }
    SgGotoStatement *ngoto = SageBuilder::buildGotoStatement(labelTopStmt);
    SageInterface::replaceStatement(oldContinue, ngoto, true);
  }
}


//
// Rewrite 'break' to exit the current loop or switch scope.
//
static void rewriteActiveBreakStmts(SgBasicBlock *body,
                                    SgLabelStatement *labelExitStmt)
{
  std::vector<SgBreakStmt *> bl = SageInterface::findBreakStmts(body);
  foreach (SgBreakStmt *oldBreak, bl) {
    SgGotoStatement *ngoto = SageBuilder::buildGotoStatement(labelExitStmt);
    SageInterface::replaceStatement(oldBreak, ngoto, true);
  }
}


//
// Rewrite:                            OR 
//  for (init; cond; incr) {                              while (cond) {
//    body                                                  body
//  }                                                     }
//  ...                                                   ...
//
// As:
// 
//  {
//   <<init>>               // for-loop only
//   loop_top:
//   if (!<<cond>>) {
//     goto loop_exit;
//   }
//   <<body>>
//   <<incr>>               // for-loop only
//   goto loop_top;
//   loop_exit:
//     ;
//  }
//  ...
//
// The remaining outermost braces (SgBasicBlock) will be removed in
// a subsequent pass.
//
static void rewriteTopTestedLoop_Common(SgScopeStatement *S,
                           SgBasicBlock *forInitBlock, SgExpression *incrExpr)
{
  SgScopeStatement *proc = SageInterface::getEnclosingProcedure(S);
  SgBasicBlock *body = isSgBasicBlock(SageInterface::getLoopBody(S));
  assert(body && "expected SgBasicBlock-- not normalized?");

  // Construct the label statements.
  int ln = ++SageInterface::gensym_counter;
  SgLabelStatement *labelTopStmt = HtSageUtils::makeLabelStatement("loop_top", ln, proc);
  SgLabelStatement *labelExitStmt = HtSageUtils::makeLabelStatement("loop_exit", ln, proc);

  // Insert the "<<incr>>" expression (for-loops only).
  if (incrExpr) {
    SageInterface::appendStatement(SageBuilder::buildExprStatement(incrExpr),
        body);
  }

  // Build and insert "goto loop_top; loop_exit:".
  SgStatement *newGotoStmt = SageBuilder::buildGotoStatement(labelTopStmt);
  SageInterface::appendStatement(newGotoStmt, body);
  SageInterface::appendStatement(labelExitStmt, body);

  // Build "if (!<<cond>>) { goto loop_exit; }" subtree.
  SgStatement *condStmt = SageInterface::getLoopCondition(S);
  assert(condStmt && "loop cond not SgStatement?");
  SgExpression *cexpr = 0;
  if (isSgNullStatement(condStmt)) {
    cexpr = SageBuilder::buildBoolValExp(true);
  } else {
    assert(isSgExprStatement(condStmt) && "loop cond not SgExprStatement?");
    cexpr = isSgExprStatement(condStmt)->get_expression();
  }
  SgStatement *ifstmt = makeIfWithGoto(cexpr, labelExitStmt, true /* neg */);
  SageInterface::prependStatement(ifstmt, body);

  // Insert "loop_top:" statement.
  SageInterface::prependStatement(labelTopStmt, body);

  // Insert "<<init>>" statement (for-loop only).
  if (forInitBlock) {
    SageInterface::prependStatement(forInitBlock, body);
  }

  // Move preprocessor info from the original ForStmt to the first
  // statement (a new NullStmt anchor).
  // TODO: Only does PreprocessingInfo::before. 
  SgStatement *newFirstStmt = SageBuilder::buildNullStatement();
  SageInterface::prependStatement(newFirstStmt, body);
  SageInterface::movePreprocessingInfo(S, newFirstStmt,
      PreprocessingInfo::before);

  // Move preprocessor info (position PreprocessingInfo::inside) from the 
  // original body block to the last statement in the loop (i.e., the
  // new goto statement, since the loop exit label is logically outside
  // the loop).  These directives would otherwise disappear after 
  // flattenBlocks.
  SageInterface::movePreprocessingInfo(body, newGotoStmt,
      PreprocessingInfo::inside, PreprocessingInfo::after);

  // Disconnect body from old loop, and replace loop in parent with body.
  SageInterface::setLoopBody(S, SageBuilder::buildNullStatement());
  isSgStatement(S->get_parent())->replace_statement(S, body); 

  rewriteActiveContinueStmts(body, labelTopStmt, incrExpr);
  rewriteActiveBreakStmts(body, labelExitStmt);
}


void RewriteControlFlowPass1Visitor::visitSgForStatement(SgForStatement *S)
{
  // Move the init statements into a new block and disconnect from loop.
  SgBasicBlock *newInitBlock = SageBuilder::buildBasicBlock();
  SgForInitStatement *inits = S->get_for_init_stmt();
  SgStatementPtrList &initStmts = inits->get_init_stmt();
  for (size_t i = 0; i < initStmts.size(); i++) {
    SageInterface::appendStatement(initStmts[i], newInitBlock);
    assert(initStmts[i]->get_parent() == newInitBlock);
  }
  initStmts.clear(); 

  // Disconnect incr expression from loop.
  SgExpression *incr = S->get_increment();
  S->set_increment(0);
  incr->set_parent(0); 

  rewriteTopTestedLoop_Common(S, newInitBlock, incr);
}
 

void RewriteControlFlowPass1Visitor::visitSgWhileStmt(SgWhileStmt *S)
{
  rewriteTopTestedLoop_Common(S, 0, 0);
}


//
// Rewrite:
//  do {
//    body
//  } while (cond);
//  ...
//
// As:
// 
//  {
//   loop_top:
//   <<body>>
//   if (<<cond>>) {
//     goto loop_top;
//   }
//   loop_exit:                // Target only for 'break' statements.
//  }
//  ...
//
// The remaining outermost braces (SgBasicBlock) will be removed in
// a subsequent pass.
//
void RewriteControlFlowPass1Visitor::visitSgDoWhileStmt(SgDoWhileStmt *S)
{
  SgScopeStatement *proc = SageInterface::getEnclosingProcedure(S);
  SgBasicBlock *body = isSgBasicBlock(SageInterface::getLoopBody(S));
  assert(body && "expected SgBasicBlock-- not normalized?");

  // Construct and insert the label statement.
  int ln = ++SageInterface::gensym_counter;
  SgLabelStatement *labelTopStmt = HtSageUtils::makeLabelStatement("loop_top", ln, proc);
  SgLabelStatement *labelExitStmt = HtSageUtils::makeLabelStatement("loop_exit", ln, proc);
  SageInterface::prependStatement(labelTopStmt, body);

  // Build "if (<<cond>>) { goto loop_top; }" subtree.
  SgExprStatement *cond = isSgExprStatement(S->get_condition());
  assert(cond && "expected SgExprStatement for do_while condition"); 
  SgStatement *ifstmt = makeIfWithGoto(cond->get_expression(), labelTopStmt,
    false /* negate cond? */);
  SageInterface::appendStatement(ifstmt, body);
  SageInterface::appendStatement(labelExitStmt, body);

  // Move preprocessor info from the original DoWhileStmt to the first
  // statement (a new NullStmt anchor).
  // TODO: Only does PreprocessingInfo::before. 
  SgStatement *newFirstStmt = SageBuilder::buildNullStatement();
  SageInterface::prependStatement(newFirstStmt, body);
  SageInterface::movePreprocessingInfo(S, newFirstStmt,
      PreprocessingInfo::before);

  // Move preprocessor info (position PreprocessingInfo::inside) from the 
  // original body block to the last statement in the loop (i.e., a new
  // anchor just before the loop exit label).
  // These directives would otherwise disappear after flattenBlocks.
  SgStatement *anchor = SageBuilder::buildNullStatement();
  SageInterface::insertStatementBefore(labelExitStmt, anchor);
  SageInterface::movePreprocessingInfo(body, anchor,
      PreprocessingInfo::inside, PreprocessingInfo::after);

  // Disconnect body from old while, and replace while in parent with body.
  SageInterface::setLoopBody(S, SageBuilder::buildNullStatement());
  isSgStatement(S->get_parent())->replace_statement(S, body); 

  rewriteActiveContinueStmts(body, labelTopStmt, 0);
  rewriteActiveBreakStmts(body, labelExitStmt);
}


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

std::map<SgNode *, bool> nodeOpensState;

bool AssignNodeOpensState::evaluateSynthesizedAttribute(SgNode *n,
    SynthesizedAttributesList childAttributes)
{
  bool opensState = std::accumulate(childAttributes.begin(),
      childAttributes.end(), false, std::logical_or<bool>());

  VariantT t = n->variantT();
  opensState |= (t == V_SgLabelStatement || t == V_SgSwitchStatement
      || t == V_SgGotoStatement);

  nodeOpensState[n] = opensState;
  return opensState;
}


//
// This is the fully general IfStmt rewriter which introduces new
// states for each branch and the join point.  This transformation
// is only necessary if one or both branches themselves introduce
// new states.  Simple 'if' statements can be left untranslated.
//
// Rewrite:
//   if (c) {
//     <<then code>>
//   } else {
//     <<else code>>
//   }
//   ...
// As:
//       if (!( c )) {
//         goto if_else;
//       }
//       <<then code>>
//       goto if_join;
//   if_else:
//       <<else code>>
//       /* goto if_join; */
//   if_join:
//       ...
//
// If there is no 'else' clause, the IF_ELSE section is omitted,
// and the conditional branches to the IF_JOIN instead.
//
void RewriteControlFlowPass2Visitor::visitSgIfStmt(SgIfStmt *S)
{
  // Don't translate unless children opened states.
  assert(nodeOpensState.find(S) != nodeOpensState.end());
  if (nodeOpensState[S] == false
      || getDontProcessAttribute(S)) {
    return;
  }
 
  SgScopeStatement *proc = SageInterface::getEnclosingProcedure(S);
  SgBasicBlock *trueBody = isSgBasicBlock(S->get_true_body());
  SgBasicBlock *falseBody = isSgBasicBlock(S->get_false_body());
  assert(trueBody && "expected SgBasicBlock-- not normalized?");
  assert(falseBody && "expected SgBasicBlock-- not normalized?");

  SgBasicBlock *newBlock = SageBuilder::buildBasicBlock();

  bool hasElse = falseBody->get_statements().size() > 0;

  // Construct the label statements.
  int ln = ++SageInterface::gensym_counter;
  SgLabelStatement *labelElseStmt = HtSageUtils::makeLabelStatement("if_else", ln, proc);
  SgLabelStatement *labelJoinStmt = HtSageUtils::makeLabelStatement("if_join", ln, proc);

  // Build "if (!<<cond>>) { goto if_else; }" subtree.
  SgExprStatement *cond = isSgExprStatement(S->get_conditional());
  assert(cond && "expected SgExprStatement for do_while condition"); 
  SgStatement *ifstmt = makeIfWithGoto(cond->get_expression(), 
      (hasElse ? labelElseStmt : labelJoinStmt), true /* neg */);
  SageInterface::appendStatement(ifstmt, newBlock);

  SageInterface::appendStatement(trueBody, newBlock);
  if (hasElse) {
    SageInterface::appendStatement(
        SageBuilder::buildGotoStatement(labelJoinStmt), newBlock);
    SageInterface::appendStatement(labelElseStmt, newBlock);
    SageInterface::appendStatement(falseBody, newBlock);
  }
  SageInterface::appendStatement(labelJoinStmt, newBlock);

  // Move preprocessor info from the original IfStmt to the first new
  // statement.  It appears that ROSE currently only attaches preproc info
  // to an IfStmt using the relative position 'before', so that we do not 
  // need to special case the other possibilities (and all the info can be
  // attached in the same position to the new first statement).
  SageInterface::movePreprocessingInfo(S, ifstmt);
  
  // Replace old 'if' in parent with new body.
  S->set_true_body(0);
  S->set_false_body(0);
  isSgStatement(S->get_parent())->replace_statement(S, newBlock); 
}


void RewriteControlFlowPass2Visitor::visitSgSwitchStatement(SgSwitchStatement *S)
{
  // TODO: use schedule information to decide whether or not to translate.
 
  SgScopeStatement *proc = SageInterface::getEnclosingProcedure(S);
  SgBasicBlock *switchBody = isSgBasicBlock(S->get_body());
  assert(switchBody && "expected block for switch body");
  SgBasicBlock *newContainer = SageBuilder::buildBasicBlock();

  // Replace old switch in parent with a new block containing copy of
  // switch (since we need to add statements AFTER the switch).
  SgSwitchStatement *newS = SageBuilder::buildSwitchStatement(
      S->get_item_selector(), switchBody);
  S->set_item_selector(0);
  S->set_body(0);
  isSgStatement(S->get_parent())->replace_statement(S, newContainer); 
  S = newS;
  SageInterface::appendStatement(S, newContainer); 

  // Insert new join label after switch statement.
  int ln = ++SageInterface::gensym_counter;
  SgLabelStatement *ljoin = HtSageUtils::makeLabelStatement("switch_join", ln, proc);
  SageInterface::insertStatementAfter(S, ljoin);

  // Rewrite the break statements before outlining the case bodies.
  rewriteActiveBreakStmts(switchBody, ljoin);

  // Outline case bodies.
  bool hasDefault = false;
  SgBasicBlock *outlineBlock = SageBuilder::buildBasicBlock();
  foreach (SgStatement *currStmt, switchBody->get_statements()) {
    SgCaseOptionStmt *thisCase = isSgCaseOptionStmt(currStmt);
    SgDefaultOptionStmt *thisDefault = isSgDefaultOptionStmt(currStmt);
    SgBasicBlock *optionBody = 0;
    SgBasicBlock *newOptionBody = SageBuilder::buildBasicBlock();
    if (thisCase) {
      optionBody = isSgBasicBlock(thisCase->get_body());
      assert(optionBody && "expected block for case body");
      thisCase->set_body(newOptionBody);
      newOptionBody->set_parent(thisCase);
    } else if (thisDefault) {
      optionBody = isSgBasicBlock(thisDefault->get_body());
      assert(optionBody && "expected block for default body");
      thisDefault->set_body(newOptionBody);
      newOptionBody->set_parent(thisDefault);
      hasDefault = true;
    } else {
      assert(0);
    }
    int ln = ++SageInterface::gensym_counter;
    SgName lname = (thisCase ? "switch_case" : "switch_default"); 
    SgLabelStatement *lstmt = HtSageUtils::makeLabelStatement(lname, ln, proc);
    SageInterface::appendStatement(SageBuilder::buildGotoStatement(lstmt),
        newOptionBody);

    SageInterface::appendStatement(lstmt, outlineBlock);
    SageInterface::appendStatement(optionBody, outlineBlock);
  }

  if (hasDefault == false) {
    // Add a default case that jumps to the join if user didn't have one.
    SageInterface::appendStatement(SageBuilder::buildDefaultOptionStmt(
        SageBuilder::buildBasicBlock(SageBuilder::buildGotoStatement(ljoin))), 
        switchBody);
  }

  // Insert outlined case bodies just after original switch.
  SageInterface::insertStatementAfter(S, outlineBlock);
}


