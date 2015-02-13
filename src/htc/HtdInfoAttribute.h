/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_HTD_INFO_ATTRIBUTE_H
#define HTC_HTD_INFO_ATTRIBUTE_H

#include <vector>
#include <string>
#include <rose.h>
#include "boost/tuple/tuple.hpp"

typedef boost::tuple<std::string, std::string, std::string> private_tuple_t;
typedef boost::tuple<std::string, std::string> shared_tuple_t;
typedef boost::tuple<std::string, std::string> def_tuple_t;
typedef boost::tuple<std::string, std::string> tdef_tuple_t;
typedef boost::tuple<std::string, std::string, std::string, std::string> module_tuple_t;

typedef boost::tuple<std::string, std::string, std::string> parm_tuple_t;
typedef boost::tuple<std::string, std::string, std::string> rparm_tuple_t;
typedef boost::tuple<std::string, std::string, std::string, std::vector<parm_tuple_t> > entry_tuple_t;
typedef boost::tuple<std::string, std::string, std::vector<rparm_tuple_t> > return_tuple_t;
typedef boost::tuple<std::string, std::string, std::string, std::string> call_tuple_t;
typedef boost::tuple<std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string> rstream_tuple_t;
typedef boost::tuple<std::string, std::string, std::string, std::string, std::string, std::string, std::string, std::string> wstream_tuple_t;


class HtdInfoAttribute : public AstAttribute {
public:
  HtdInfoAttribute() {
    moduleWidth = -1;
  }

  // Append instruction to module.
  void appendInst(std::string name);
  void insertInstComment(std::string s) { instComments.push_back(s); }

  // Append private var to module.
  void appendPrivateVar(std::string grpName, std::string typeName, 
                        std::string varName);
  void insertPrivateVarComment(std::string s) { privComments.push_back(s); }

  // Append shared var to module.
  void appendSharedVar(std::string typeName, std::string varName);
  void insertSharedVarComment(std::string s) { sharedComments.push_back(s); }

  // Append define to design.
  void appendDefine(std::string defName, std::string valueStr);

  // Append typedef.
  void appendTypedef(std::string typeStr, std::string nameStr);

  // Append C-style struct decl.
  void appendStructDecl(std::string structStr);

  // Declare the Module.
  void appendModule(std::string modName, std::string grpName, 
                    std::string grpWidth, std::string resetState = "");

  // Append function entry to module.
  void appendEntry(std::string funcName, std::string grpName,
                   std::string hostStr, std::vector<parm_tuple_t> parms);

  // Append function return to module.
  void appendReturn(std::string funcName, std::string grpName,
                    std::vector<rparm_tuple_t> parms);

  // Append call to module.
  void appendCall(std::string funcStr, std::string forkStr = "", 
                  std::string groupStr = "", std::string qwidthStr = "");

  // Append barrier to module.
  // TODO: Named barriers.
  void appendBarrier(std::string width);

  // Append ReadStream to module.
  void appendReadStream(std::string streamName, std::string type, 
                        std::string streamCnt = "", std::string elemCnt = "",
                        std::string closeStr = "", std::string memSrc = "",
                        std::string memPort = "", std::string streamBw = "");

  // Append WriteStream to module.
  void appendWriteStream(std::string streamName, std::string type, 
                         std::string streamCnt = "", std::string elemCnt = "",
                         std::string closeStr = "", std::string memDst = "",
                         std::string memPort = "", std::string reserve = "");

  // Append an arbitrary htd module string.
  // Htd will emit 'moduleName.TheString;'.
  void appendArbitraryModuleString(std::string s);
  void insertArbitraryModuleComment(std::string s) {
    modArbComments.push_back(s); }

  // Append an arbitrary htd dsnInfo string.
  // Htd will emit 'dsnInfo.TheString;'.
  void appendArbitraryDsnInfoString(std::string s);
  void insertArbitraryDsnInfoComment(std::string s) {
    dsnArbComments.push_back(s); }

  std::vector<std::string> &getInsts() { return insts; }
  std::vector<private_tuple_t> &getPrivateVars() { return privates; }
  std::vector<shared_tuple_t> &getSharedVars() { return sharedVars; }
  std::vector<def_tuple_t> &getDefines() { return defines; }
  std::vector<tdef_tuple_t> &getTypedefs() { return typedefines; }
  std::vector<std::string> &getStructDecls() { return structDecls; }
  std::vector<module_tuple_t> &getModule() { return module; }
  std::vector<entry_tuple_t> &getEntries() { return entries; }
  std::vector<return_tuple_t> &getReturns() { return returns; }
  std::vector<call_tuple_t> &getCalls() { return calls; }
  std::vector<std::string> &getBarriers() { return barriers; }
  std::vector<rstream_tuple_t> &getReadStreams() { return rstreams; }
  std::vector<wstream_tuple_t> &getWriteStreams() { return wstreams; }
  std::vector<std::string> &getModArbitrary() { return modArbitrary; }
  std::vector<std::string> &getDsnArbitrary() { return dsnArbitrary; }

  std::vector<std::string> &getInstComments() { return instComments; }
  std::vector<std::string> &getPrivComments() { return privComments; }
  std::vector<std::string> &getSharedComments() { return sharedComments; }
  std::vector<std::string> &getModArbComments() { return modArbComments; }
  std::vector<std::string> &getDsnArbComments() { return dsnArbComments; }

  std::string debugHookStr;
  int moduleWidth;

private:
  std::vector<std::string> insts;
  std::vector<private_tuple_t> privates;
  std::vector<shared_tuple_t> sharedVars;
  std::vector<def_tuple_t> defines;
  std::vector<tdef_tuple_t> typedefines;
  std::vector<std::string > structDecls;
  std::vector<module_tuple_t> module;
  std::vector<entry_tuple_t> entries;
  std::vector<return_tuple_t> returns;
  std::vector<call_tuple_t> calls;
  std::vector<std::string> barriers;
  std::vector<rstream_tuple_t> rstreams;
  std::vector<wstream_tuple_t> wstreams;
  std::vector<std::string> modArbitrary;
  std::vector<std::string> dsnArbitrary;
  // Read/write Mems: TBD

  std::vector<std::string> instComments;
  std::vector<std::string> privComments;
  std::vector<std::string> sharedComments;
  std::vector<std::string> modArbComments;
  std::vector<std::string> dsnArbComments;
};

extern void initAllHtdInfoAttributes(SgProject *project);
extern HtdInfoAttribute *getHtdInfoAttribute(SgFunctionDefinition *fdef);

#endif //HTC_HTD_INFO_ATTRIBUTE_H


