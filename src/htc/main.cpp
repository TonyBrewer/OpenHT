/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

// Translator sandbox.

#include <rose.h>
#include "EmitHTDescriptionFile.h"
#include "HtDeclManager.h"
#include "HtSageUtils.h"
#include "HtdInfoAttribute.h"
#include "IsolateModules.h"
#include "LateOmpLowering.h"
#include "Normalize.h"
#include "ProcessCalls.h"
#include "ProcessMemRefs.h"
#include "ProcessStencils.h"
#include "RewriteAsStateMachine.h"
#include "RewriteControlFlow.h"
#include "HtSCDecls.h"


bool debugHooks = false;
bool doSystemC = false;
bool doHtc = true;
bool htc_do_split = false;
scDeclarations *SCDecls = 0;

static void HtcDriver(SgProject *);


int main (int argc, char **argv)
{
  // Process Htc-specific command line options.
  std::vector<std::string> largv = 
      CommandlineProcessing::generateArgListFromArgcArgv(argc, argv);
  int  newArgc = 0;
  char **newArgv = 0;
  if (CommandlineProcessing::isOption(largv, "-htc:", "dbg", true)) {
    debugHooks = true;
  }
  if (CommandlineProcessing::isOption(largv, "-htc:", "sysc", true)) {
    doSystemC = true;
  }
  if (CommandlineProcessing::isOption(largv, "-htc:", "off", true)) {
      doHtc = false;
  }
  if (CommandlineProcessing::isOption(largv, "-htc:", "split", true)) {
      htc_do_split = true;
      doHtc = false;
  }
  largv.insert(largv.end(), "-D_CNY_HTC_SOURCE");
  CommandlineProcessing::generateArgcArgvFromList(largv, newArgc, newArgv);

  // Parse program and build AST.
  SgProject *project = frontend(newArgc, newArgv, false);

  // Run AST sanity checks.
  AstTests::runAllTests(project);
 
  // Do Htc translation.
#if 1
  if (htc_do_split) {
    // For host side stencil generation (developer debug).
    initAllHtdInfoAttributes(project);
    HtDeclMgr::buildAllKnownHtlDecls(project);
    ProcessStencils(project);
  }
#endif
  if (doHtc) {
      HtcDriver(project);
  }
  AstTests::runAllTests(project);

  // Unparse AST.
  return backend(project);
}


void HtcDriver(SgProject *project)
{
  initAllHtdInfoAttributes(project);
  HtDeclMgr::buildAllKnownHtlDecls(project);

  //
  // Process stencil calls early.
  //

  ProcessStencils(project);

#if 0
  // Remove #ifdef/#endif, etc.
  //  StripPreprocessingInfo(project);
#endif

  //
  // Normalize syntax.
  //
  // Perform normalizing transformations on the user's code.
  //

  HtSageUtils::ScreenForHtcRestrictions(project);
  NormalizeSyntax(project);

  //
  // Perform ht-specifc (late) OMP lowering.
  //

  DoLateOmpLowering(project);

  //
  // Rewrite control flow (pass1).
  //
  // Flatten structured control flow into unstructured control flow.
  // This eliminates structured loop nests, 'break', 'continue', etc.
  //

  // Rewrite while loops, do-while loops, and any remaining 'continue'
  // statements in those loops.
  RewriteControlFlowPass1Visitor rv1;

  rv1.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);

  //
  // Perform more normalizations after loops have been rewritten.
  //

  LiftDeclarations(project);
  ExpandConditionalExpressions(project);
  ExpandCompoundAssignOperators(project);
  FlattenCommaExpressions(project); 
  FlattenFunctionCalls(project);

  //
  // Process function calls and returns.
  //

  ProcessCalls(project);

  //
  // Process memory references.
  //

  // ExpandSgArrowExpressions MUST come before FlattenArrayIndexExpressions

  ExpandSgArrowExpressions(project);
  FlattenArrayIndexExpressions(project);
  FlattenAssignChains(project);
  AddCastsForRhsOfAssignments(project);
  FlattenSideEffectsInStatements(project);

  if (!debugHooks) {
    ProcessMemRefs(project);
  }

  //
  // ScheduleAssignCycles.
  //
  // TODO: assign cycles before cf rewrite so we know whether or not
  // to translate if-then-else and switch.  Maybe rewrite loops only
  // first, assign cycles, then rewrite ifs for more simple scheduler.
  
  //
  // Rewrite control flow (pass2).
  //
  // Flatten if-then-else and switch statements into unstructured
  // control flow if necessary.
  //

  RewriteControlFlowPass2Visitor rv2(project);
  rv2.traverseInputFiles(project, postorder);
 
  // Remove empty, nested SgBasicBlocks left-over from above.
  PreservePreprocInfoForBlocks(project);
#if STRICT_INPUT_FILE_TRAVERSAL
  flattenBlocksInInputFiles(project);
#else
  flattenBlocks(project);
#endif
  removeNullStatements(project);
#if 0
  SageInterface::removeJumpsToNextStatement(project);
  SageInterface::removeUnusedLabels(project);
  SageInterface::removeConsecutiveLabels(project);
  SageInterface::removeUnusedLabels(project);
#endif

  //
  // RewriteAccordingToSchedule.
  //
  // TODO: Rewrite remaining code according to schedule (insert labels,
  // split expressions for tntra-expression cycle changes).
  //

  //
  // Rewrite code as a state machine.
  //
  // Very last thing done-- only need to use labels to determine
  // where to open states.  Insert appropriate cases and HtContinues.
  //

  RewriteAsStateMachine(project);

  // Perform some clean-ups for output readability.
  RemoveEmptyElseBlocks(project);

  //
  // Emit the hybrid threading description (.htd) file.
  //

  EmitHTDescriptionFile(project);

  //
  // Emit Pers*.cpp files.
  //
  IsolateModulesToFiles(project);

}
