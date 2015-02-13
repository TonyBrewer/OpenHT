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
#include "HtdInfoAttribute.h"
#include "HtSageUtils.h"

#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 


void initAllHtdInfoAttributes(SgProject *project)
{
  class VisitFuncDecls : public AstSimpleProcessing {
  private:
    void visit(SgNode *S) {
      if (SgFunctionDeclaration *FD = isSgFunctionDeclaration(S)) {
        if (SgFunctionDefinition *fdef = FD->get_definition()) {
          fdef->addNewAttribute("htdinfo", new HtdInfoAttribute());
        }
      }
    }
  } v;
    v.traverseInputFiles(project, preorder, STRICT_INPUT_FILE_TRAVERSAL);
}


// Return a properly down-casted HtdInfoAttribute from a node.
HtdInfoAttribute *getHtdInfoAttribute(SgFunctionDefinition *fdef)
{
  AstAttribute *attr = fdef->getAttribute("htdinfo");
  assert(attr);
  return dynamic_cast<HtdInfoAttribute *>(attr);
}


void HtdInfoAttribute::appendInst(std::string name)
{
  insts.push_back(name);
}


void HtdInfoAttribute::appendPrivateVar(std::string grpStr,
                                        std::string typeStr,
                                        std::string nameStr) {
  if (HtSageUtils::isHtlReservedIdent(nameStr)) {
    assert(0 && "attempted to declare reserved Htl identifier <private>");
  }
  privates.push_back(boost::make_tuple(grpStr, typeStr, nameStr));
}


void HtdInfoAttribute::appendSharedVar(std::string typeStr,
                                       std::string nameStr) {
  if (HtSageUtils::isHtlReservedIdent(nameStr)) {
    assert(0 && "attempted to declare reserved Htl identifier <shared>");
  }
  sharedVars.push_back(boost::make_tuple(typeStr, nameStr));
}


void HtdInfoAttribute::appendModule(std::string modName,
                                    std::string grpName, 
                                    std::string grpW,
                                    std::string resetState)
{
    module.push_back(boost::make_tuple(modName, grpName, grpW, resetState));
}


void HtdInfoAttribute::appendDefine(std::string defName, std::string valueStr)
{
  defines.push_back(boost::make_tuple(defName, valueStr));
}


void HtdInfoAttribute::appendTypedef(std::string typeStr, std::string nameStr)
{
    typedefines.push_back(boost::make_tuple(typeStr, nameStr));
}


void HtdInfoAttribute::appendStructDecl(std::string sdecl)
{
  structDecls.push_back(sdecl);
}


void HtdInfoAttribute::appendEntry(std::string funcName, std::string grpName,
                                   std::string hostStr,
                                   std::vector<parm_tuple_t> parms)
{
  entries.push_back(boost::make_tuple(funcName, grpName, hostStr, parms));
}


void HtdInfoAttribute::appendReturn(std::string funcName, std::string grpName,
                                    std::vector<rparm_tuple_t> parms)
{
  returns.push_back(boost::make_tuple(funcName, grpName, parms));
}


void HtdInfoAttribute::appendCall(std::string funcStr, std::string forkStr,
                                  std::string groupStr, std::string qwidthStr)
{
  calls.push_back(boost::make_tuple(funcStr, forkStr, groupStr, qwidthStr));
}


void HtdInfoAttribute::appendBarrier(std::string width)
{
  barriers.push_back(width);
}


void HtdInfoAttribute::appendReadStream(std::string streamName,
    std::string type, std::string streamCnt, std::string elemCnt,
    std::string closeStr, std::string memSrc,
    std::string memPort, std::string streamBw)
{
  rstreams.push_back(boost::make_tuple(streamName, type, streamCnt, elemCnt,
      closeStr, memSrc, memPort, streamBw));
}


void HtdInfoAttribute::appendWriteStream(std::string streamName,
    std::string type, std::string streamCnt, std::string elemCnt,
    std::string closeStr, std::string memDst,
    std::string memPort, std::string reserve)
{
  wstreams.push_back(boost::make_tuple(streamName, type, streamCnt, elemCnt,
      closeStr, memDst, memPort, reserve));
}


void HtdInfoAttribute::appendArbitraryModuleString(std::string s) {
  modArbitrary.push_back(s);
}


void HtdInfoAttribute::appendArbitraryDsnInfoString(std::string s) {
  dsnArbitrary.push_back(s);
}

