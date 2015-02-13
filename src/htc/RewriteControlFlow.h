/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#ifndef HTC_REWRITE_CONTROL_FLOW_H
#define HTC_REWRITE_CONTROL_FLOW_H

#include <rose.h>
#include <map>

#include "HtcAttributes.h"


extern void RewriteControlFlowPreparation(SgProject *);


//
// RewriteControlFlowPass1Visitor.
//
// Flatten structured control flow into unstructured control flow.
// This eliminates structured loop nests, 'break', and 'continue'.
//
class RewriteControlFlowPass1Visitor : public AstSimpleProcessing {
private:
  void visit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgForStatement:
      visitSgForStatement(dynamic_cast<SgForStatement *>(S));
      break;
    case V_SgWhileStmt:
      visitSgWhileStmt(dynamic_cast<SgWhileStmt *>(S));
      break;
    case V_SgDoWhileStmt:
      visitSgDoWhileStmt(dynamic_cast<SgDoWhileStmt *>(S));
      break;
    }
  }

  void visitSgForStatement(SgForStatement *S);
  void visitSgWhileStmt(SgWhileStmt *S);
  void visitSgDoWhileStmt(SgDoWhileStmt *S);
};


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


class AssignNodeOpensState : public AstBottomUpProcessing<bool>
{
public:
  virtual bool evaluateSynthesizedAttribute(SgNode *n,
      SynthesizedAttributesList childAttributes);
};


//
// RewriteControlFlowPass2Visitor.
//
// Flatten conditional structured control flow into unstructured control
// flow.  This eliminates if-then-else and switch statements.
//
class RewriteControlFlowPass2Visitor : public AstSimpleProcessing {
public:
  RewriteControlFlowPass2Visitor(SgProject *project) {
    AssignNodeOpensState v;
    v.traverseInputFiles(project);
  }

private:
  RewriteControlFlowPass2Visitor();

  void visit(SgNode *S) {

    switch (S->variantT()) {
    case V_SgIfStmt:
      visitSgIfStmt(dynamic_cast<SgIfStmt *>(S));
      break;
    case V_SgSwitchStatement:
      visitSgSwitchStatement(dynamic_cast<SgSwitchStatement *>(S));
      break;
    }

  }

  void visitSgIfStmt(SgIfStmt *S);
  void visitSgSwitchStatement(SgSwitchStatement *S);
};



#endif // HTC_REWRITE_CONTROL_FLOW_H

