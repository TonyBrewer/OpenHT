/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */

#include <cctype>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include "EmitHTDescriptionFile.h"
#include "HtdInfoAttribute.h"
#include "HtSCDecls.h"
#include "HtSageUtils.h"

extern scDeclarations *SCDecls;


#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 


static std::fstream fs;

//
//
class EmitHTDVisitor : public AstSimpleProcessing {
private:
  void visit(SgNode *S) {
    switch (S->variantT()) {
    case V_SgFunctionDeclaration:
      visitSgFunctionDeclaration(dynamic_cast<SgFunctionDeclaration *>(S));
      break;
    default:
      break;
    }
  }

  void visitSgFunctionDeclaration(SgFunctionDeclaration *FD);
};


#define tget(i, t) boost::get<(i)>((t))

 
std::string makeOptParm(std::string parm, std::string value, bool opt = true,
                        bool needComma = true)
{
  std::string s;

  std::string comma = needComma ? ", " : "";
  if (opt) { 
    s = value == "" ? "" : (comma + parm + "=" + value);
  } else {
    s = comma + parm + "=" + value;
  }
  return s;
}


std::string makeParm(std::string parm, std::string value, bool needComma = true)
{
  return makeOptParm(parm, value, false, needComma);
}


static void emitComments(std::vector<std::string> &comments)
{
  foreach (std::string &s, comments) {
    fs << "// " << s << std::endl;
  }
}


SgCaseOptionStmt* findCaseEnclosingNode (SgNode *node) {
    SgNode *n = node;
    while (n && !isSgCaseOptionStmt(n)) {
        n = n->get_parent();
    }
    return isSgCaseOptionStmt(n);
}

bool isInFunctionParameterList(std::string name,
                               SgFunctionDeclaration *fndecl) {

    SgFunctionParameterList *functionParameters =
        fndecl->get_parameterList();
    SgInitializedNamePtrList &parameterList = 
        functionParameters->get_args();
    
    SgInitializedNamePtrList::iterator j;
    for (j = parameterList.begin(); j != parameterList.end(); j++) {
        SgInitializedName *iname = isSgInitializedName(*j);
        if (iname->get_name().getString().compare("P_"+name) == 0) {
            return true;
        }
    }
    return false;
}


bool sinkPrivateToSingleState(std::string name,
                              SgFunctionDefinition *fdef) {

    std::vector<SgNode *> varRefsInFunction;
    std::vector<SgVarRefExp *> refsToName;

    // read addr index variables are written in the state machine, but 
    // referenced outside the state meachine.    Preserve them.
    if (name.compare(0, strlen("mif_rdVal_index"), 
                     "mif_rdVal_index") == 0) {
        return false;
    }
    if (name.compare(0, strlen("globalVar_index"), 
                     "globalVar_index") == 0) {
        return false;
    }
    if (name.compare(0, strlen("__htc_UplevelGV_"), 
                     "__htc_UplevelGV_") == 0) {
        return false;
    }
    if (name.compare(0, strlen("__htc_uplevel_index"), 
                     "__htc_uplevel_index") == 0) {
        return false;
    }
    // variable for return value must be preserved
    if (name.compare(0, strlen("retval_"), 
                     "retval_") == 0) {
        return false;
    }
    // parameters need a local private variable
    if (isInFunctionParameterList(name, fdef->get_declaration())) {
        return false;
    }

#if 0
    // First check that we have a prefix that looks like a temp
    if (name.compare(0, strlen("__htc_t_"), "__htc_t_") != 0) {
        return false;
    }
#endif

    varRefsInFunction = NodeQuery::querySubTree(fdef, V_SgVarRefExp);

    foreach (SgNode* node, varRefsInFunction) {
        SgVarRefExp *varref = isSgVarRefExp(node);
        std::string varname = varref->get_symbol()->get_name().getString();
        if (varname.compare("P_" + name) == 0) {
            refsToName.push_back(varref);
        }
    }

    if (refsToName.size() == 0) {
        // This must be one of the extra OpenMP args
        return false;
    }

    bool first = true;
    bool allSameCase = true;
    SgCaseOptionStmt *firstCase;
    foreach (SgVarRefExp *varref, refsToName) {
        if (first) {
            first = false;
            firstCase = findCaseEnclosingNode(varref);
        } else {
            allSameCase = allSameCase && 
                (findCaseEnclosingNode(varref) == firstCase);
        }
    }

    // Insert a local symbol declaration into the body of the case
    if (allSameCase) {
        SgBasicBlock *block = isSgBasicBlock(firstCase->get_body());
        assert(block);

        SgType *type = refsToName[0]->get_type();
        if (isPointerType(type)) {
            type = SCDecls->get_MemAddr_t_type();
        }

        SgVariableDeclaration *varDecl = 
            SageBuilder::buildVariableDeclaration(
                "P_" +name, type, 0, block);
        SageInterface::prependStatement(varDecl, block);
    }

    return allSameCase;
}

void EmitHTDVisitor::visitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  std::string modName;
  std::string modEntryState = "";

  HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
  assert(htd && "missing HtdInfoAttribute");

  //
  // Emit any defines.
  //
  std::vector<def_tuple_t> &dl = htd->getDefines();
  if (dl.size() > 0) {
    foreach (def_tuple_t ds, dl) {
        fs << "#define " + tget(0, ds) + " " + tget(1, ds) 
           << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit the Module declaration.
  //
  std::vector<module_tuple_t> &module = htd->getModule();
  assert(module.size() == 1);
  module_tuple_t mod = module.at(0);

  modName = tget(0, mod);

  fs << "//----------------------------------------------------" << std::endl;
  fs << "// Module '" << modName << "'" << std::endl;
  fs << "//----------------------------------------------------" << std::endl;
  fs << "dsnInfo.AddModule(name=" << modName 
          + makeParm("htIdW", tget(2, mod))
          + makeOptParm("group", tget(1, mod))
          + makeOptParm("reset", tget(3, mod)) 
      << ");" << std::endl << std::endl;


  //
  // Emit instructions.
  //

  emitComments(htd->getInstComments());
  std::vector<std::string> &il = htd->getInsts();
  if (il.size() > 0) {
    foreach (std::string iname, il) {
      fs << modName << ".AddInst(" + makeParm("name", iname, false) + ");" 
          << std::endl;
      if (modEntryState == "") {
        modEntryState = iname;
      }
    }
    fs << std::endl;
  }

  //
  // Emit any typedefs.
  //
  std::vector<tdef_tuple_t> &tl = htd->getTypedefs();
  if (tl.size() > 0) {
    foreach (tdef_tuple_t ts, tl) {
        std::string type = tget(0, ts);
        // Skip "::" qualification of type name
        int pos;
        if ((pos = type.find("::")) != string::npos) {
            type.erase(pos, 2);
        }
        fs << "typedef " + type + " " + tget(1, ts) + ";" 
           << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit any struct decls.
  //
  std::vector<std::string> &sdl = htd->getStructDecls();
  if (sdl.size() > 0) {
    foreach (std::string sd, sdl) {
        fs << sd << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit any private variables.
  //
  emitComments(htd->getPrivComments());
  std::vector<private_tuple_t> &privs = htd->getPrivateVars();

  if (privs.size() > 0) {
      bool first = true;
      foreach (private_tuple_t ps, privs) {
          if (!sinkPrivateToSingleState(tget(2, ps), fdef)) {
              if (first) {
                  fs << modName << ".AddPrivate()" << std::endl;
                  first = false;
              }
              std::string type = tget(1, ps);
              // Skip "::" qualification of type name
              int pos;
              if ((pos = type.find("::")) != string::npos) {
                  type.erase(pos, 2);
              }
              fs << "    .AddVar(" + makeParm("type", type, false)
                  + makeParm("name", tget(2, ps)) + ")" << std::endl;
          }
      }
      if (!first) {
          fs << "    ;" << std::endl << std::endl;
      }
  }

  //
  // Emit any shared variables.
  //
  emitComments(htd->getSharedComments());
  std::vector<shared_tuple_t> &sl = htd->getSharedVars();
  if (sl.size() > 0) {
    fs << modName << ".AddShared()" << std::endl;
    foreach (shared_tuple_t ss, sl) {
        std::string type = tget(0, ss);
        // Skip "::" qualification of type name
        int pos;
        if ((pos = type.find("::")) != string::npos) {
            type.erase(pos, 2);
        }
      fs << "    .AddVar(" + makeParm("type", type, false)
          + makeParm("name", tget(1, ss)) + ")" << std::endl;
    }
    fs << "    ;" << std::endl << std::endl;
  }

  //
  // Emit any entries.
  //
  std::vector<entry_tuple_t> &entries = htd->getEntries();
  if (entries.size() > 0) {
    foreach (entry_tuple_t es, entries) {
      fs << modName << ".AddEntry(" + makeParm("func", tget(0, es), false)
          + makeParm("inst", modEntryState)
          + makeOptParm("group",  tget(1, es))
          + makeOptParm("host", tget(2, es)) + ")" << std::endl;
      std::vector<parm_tuple_t> parms = tget(3, es);
      foreach (parm_tuple_t ps, parms) {
        std::string type = tget(0, ps);
        // Skip "::" qualification of type name
        int pos;
        if ((pos = type.find("::")) != string::npos) {
            type.erase(pos, 2);
        }
        std::string hosttype = tget(2, ps);
        // Skip "::" qualification of type name
        if (hosttype.find("::") == 0) {
            hosttype = hosttype.substr(2, string::npos);
        }
        fs << "    .AddParam(" + makeParm("type", type, false)
            + makeParm("name", tget(1, ps))
            + makeOptParm("hostType", hosttype) + ")" << std::endl;
      }
      fs << "    ;" << std::endl;
    }
    fs << std::endl;
  }

  //
  // Emit any returns.
  //
  std::vector<return_tuple_t> &returns = htd->getReturns();
  if (returns.size() > 0) {
    foreach (return_tuple_t rs, returns) {
      fs << modName << ".AddReturn(" + makeParm("func", tget(0, rs), false)
          + makeOptParm("group", tget(1, rs)) + ")" << std::endl;
      std::vector<rparm_tuple_t> parms = tget(2, rs);
      foreach (rparm_tuple_t ps, parms) {
        std::string type = tget(0, ps);
        // Skip "::" qualification of type name
        int pos;
        if ((pos = type.find("::")) != string::npos) {
            type.erase(pos, 2);
        }
        std::string hosttype = tget(2, ps);
        // Skip "::" qualification of type name
        if (hosttype.find("::") == 0) {
            hosttype = hosttype.substr(2, string::npos);
        }

        fs << "    .AddParam(" + makeParm("type", type, false)
            + makeParm("name", tget(1, ps))
            + makeOptParm("hostType", hosttype) + ")" << std::endl;
      }
      fs << "    ;" << std::endl;
    }
    fs << std::endl;
  }

  //
  // Emit any calls.
  //
  std::vector<call_tuple_t> &cl = htd->getCalls();
  if (cl.size() > 0) {
    foreach (call_tuple_t cs, cl) {
      fs << modName << ".AddCall(" + makeParm("func", tget(0, cs), false)
          + makeOptParm("fork", tget(1, cs))
          + makeOptParm("group", tget(2, cs))
          + makeOptParm("queueW", tget(3, cs)) + ");" << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit barriers.
  //

  std::vector<std::string> &bl = htd->getBarriers();
  if (bl.size() > 0) {
    foreach (std::string width, bl) {
      fs << modName << ".AddBarrier(" + makeParm("barIdW", width, false) + ");" 
          << std::endl;
    }
    fs << std::endl;
  }

  //
  // Emit any ReadStreams.
  //
  std::vector<rstream_tuple_t> &rsl = htd->getReadStreams();
  if (rsl.size() > 0) {
    foreach (rstream_tuple_t rs, rsl) {
      bool named = (tget(0, rs) != "");
      fs << modName << ".AddReadStream(" + makeOptParm("name", tget(0, rs), true, false)
          + makeParm("type", tget(1, rs), named)
          + makeOptParm("strmCnt", tget(2, rs))
          + makeOptParm("elemCntW", tget(3, rs))
          + makeOptParm("close", tget(4, rs))
          + makeOptParm("memSrc", tget(5, rs))
          + makeOptParm("memPort", tget(6, rs))
          + makeOptParm("strmBw", tget(7, rs)) + ");" << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit any WriteStreams.
  //
  std::vector<wstream_tuple_t> &wsl = htd->getWriteStreams();
  if (wsl.size() > 0) {
    foreach (wstream_tuple_t rs, wsl) {
      bool named = (tget(0, rs) != "");
      fs << modName << ".AddWriteStream(" + makeOptParm("name", tget(0, rs), true, false)
          + makeParm("type", tget(1, rs), named)
          + makeOptParm("strmCnt", tget(2, rs))
          + makeOptParm("elemCntW", tget(3, rs))
          + makeOptParm("close", tget(4, rs))
          + makeOptParm("memDst", tget(5, rs))
          + makeOptParm("memPort", tget(6, rs))
          + makeOptParm("reserve", tget(7, rs)) + ");" << std::endl;
    }
    fs << std::endl << std::endl;
  }

  //
  // Emit any module arbitrary user strings.
  //
  emitComments(htd->getModArbComments());
  foreach (std::string us, htd->getModArbitrary()) {
    fs << modName << "." << us << std::endl;
  } 
  fs << std::endl << std::endl;
  
  //
  // Emit any dsn arbitrary user strings.
  //
  emitComments(htd->getDsnArbComments());
  foreach (std::string ds, htd->getDsnArbitrary()) {
    fs << "dsnInfo." << ds << std::endl;
  } 
  fs << std::endl << std::endl;
}


void EmitHTDescriptionFile(SgProject *project)
{
  fs.open("design.htd", std::fstream::out | std::fstream::trunc);

  EmitHTDVisitor rv;
  rv.traverseInputFiles(project, preorder, STRICT_INPUT_FILE_TRAVERSAL);

  fs.close();
}





