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
#include "IsolateModules.h"

extern bool debugHooks;
std::set<std::string> LockFunctionsInUse;


inline std::string Capitalize(std::string str) {
    std::string cap = str;
    if (str.size()) {
        cap[0] = toupper(cap[0]);
    }
    return cap;
}

// Copied these from CnyHt code
inline std::string Upper(std::string str) {
    std::string upper = str;
    for (size_t i = 0; i < upper.size(); i += 1)
        upper[i] = toupper(upper[i]);
    return upper;
}

// Insert preprocessing macros to remove original function declarations
// and wrap each function so it can be isolated from the others by enabling
// the specific function macro.
//
// This is a simple-minded way to do this.   Ideally, we'll eventually break
// each module out into it's own file.

class IsolateModules : public AstSimpleProcessing {

public:

    virtual void visit (SgNode* astNode) {

        SgFunctionDeclaration *fdecl = isSgFunctionDeclaration(astNode);

        if (fdecl) {
            fdeclList.push_back(fdecl);
        }

        SgFunctionDefinition *fdef = isSgFunctionDefinition(astNode);
        if (fdef) {
            fdefList.push_back(fdef);
        }
    }

    void process() {
        int i;

        for (i = 0; i < fdefList.size(); i++) {
            SgFunctionDefinition* fndef = fdefList.at(i);       

            if (fndef == NULL) {
                return;
            }

            std::string functionName = 
                fndef->get_declaration()->get_name().getString();

            SgFunctionDeclaration *f = fndef->get_declaration();

            if (!debugHooks) {
                SgNode *body = fndef->get_body();

                // Move any preprocessing info before the function to a new
                // null statement preceding the function.
                AttachedPreprocessingInfoType save_buf;
                SageInterface::cutPreprocessingInfo(f, 
                                                    PreprocessingInfo::before, 
                                                    save_buf) ;
                if (save_buf.size()) {
                    SgVariableDeclaration *dummy = 
                        SageBuilder::buildVariableDeclaration(
                            "___htc_dummy_decl_for_preproc_info", 
                            SageBuilder::buildIntType(), 0, f->get_scope());
                    SageInterface::setExtern(dummy);

                    SageInterface::insertStatementBefore(f, dummy);
                    SageInterface::pastePreprocessingInfo(
                        dummy, 
                        PreprocessingInfo::before, 
                        save_buf); 
                }

                SageInterface::addTextForUnparser(
                    f, 
                    "\n#if 0",     
                    AstUnparseAttribute::e_before);

                std::string CapFnName = Capitalize(functionName);
                std::string before_body = "\n#endif\n";
                std::string macro_name = "HTC_KEEP_MODULE_" + Upper(functionName);
                before_body += 
                    "#ifdef " +
                    macro_name +
                    "\n";
                before_body += "#include \"Ht.h\"\n";
                before_body += "#include \"Pers" +
                    CapFnName +
                    ".h\"\n";
                before_body += "#ifndef __htc_GW_write_addr\n";
                before_body += "#define __htc_GW_write_addr(v,a) v.write_addr(a)\n";
                before_body += "#endif\n";
                before_body += "void CPers" +
                    CapFnName +
                    "::Pers" +
                    CapFnName +
                    "()\n";
                SageInterface::addTextForUnparser(body, before_body,
                                                  AstUnparseAttribute::e_before);
                std::string after_body =
                    "\n#endif /* " +
                    macro_name +
                    " */\n";
                SageInterface::addTextForUnparser(body, after_body,
                                                  AstUnparseAttribute::e_after);
                
                // Write the _src.cpp file
                generate_src_cpp_file(fndef, CapFnName, macro_name);            
            }
        }
        for (i = 0; i < fdeclList.size(); i++) {

            SgFunctionDeclaration *fdecl = fdeclList.at(i);

            if (!debugHooks && 
                fdecl && 
                fdecl->get_definingDeclaration() != fdecl) {

                // Move any preprocessing info before the function to a new
                // null statement preceding the function.
                AttachedPreprocessingInfoType save_buf;
                SageInterface::cutPreprocessingInfo(fdecl, 
                                                    PreprocessingInfo::before, 
                                                    save_buf) ;
                if (save_buf.size()) {
                    SgVariableDeclaration *dummy2 = 
                        SageBuilder::buildVariableDeclaration(
                            "___htc_dummy_decl2_for_preproc_info", 
                            SageBuilder::buildIntType(), 
                            0, 
                            fdecl->get_scope());
                    SageInterface::setExtern(dummy2);

                    SageInterface::insertStatementBefore(fdecl, dummy2);
                    SageInterface::pastePreprocessingInfo(
                        dummy2, 
                        PreprocessingInfo::before, 
                        save_buf); 

                }

                SageInterface::addTextForUnparser(fdecl, "\n/* ",
                                                  AstUnparseAttribute::e_before);
                SageInterface::addTextForUnparser(fdecl, " */",
                                                  AstUnparseAttribute::e_after);
                // comment out function prototypes
                //            fdecl->setCompilerGenerated();
                //            fdecl->unsetOutputInCodeGeneration();
            }
        }

    }


    void generate_lock_src_cpp_files(std::string &path_name) {

        // Generate Pers_*_src.cpp file for Lock functions
        std::set<std::string>::iterator it;

        for (it = LockFunctionsInUse.begin(); it != LockFunctionsInUse.end(); it++) {
            std::string module_name = *it;
            std::string macro_name = "HTC_KEEP_MODULE_" + Upper(module_name);
            std::string cpp_name = path_name + "Pers" + 
                Capitalize(module_name) + "_src.cpp";
            std::fstream fs;
            
            fs.open(cpp_name.c_str(), std::fstream::out | std::fstream::trunc);

            fs << "#define " << macro_name << std::endl;
            fs << "#include \"rose_lock.c\"" << std::endl;

            fs.close();
        }

        LockFunctionsInUse.clear();

    }

    void generate_src_cpp_file(SgFunctionDefinition *fndef,
                               std::string &module_name,
                               std::string &macro_name) {

        std::pair<bool, std::string> fret = HtSageUtils::getFileName(fndef);
        std::string src_name = fret.second;
        std::string base_name = StringUtility::stripPathFromFileName(src_name);
        std::string path_name = StringUtility::getPathFromFileName(src_name) + "/";
        std::string cpp_name= path_name + "Pers" + module_name + "_src.cpp";
        std::fstream fs;

        fs.open(cpp_name.c_str(), std::fstream::out | std::fstream::trunc);

        fs << "#define " << macro_name << std::endl;
        fs << "#define CNY_HTC_COPROC" << std::endl;
        fs << "#include \"rose_" << base_name << "\"" << std::endl;

        fs.close();

        generate_lock_src_cpp_files(path_name);

    }

private:
    std::vector<SgFunctionDeclaration *> fdeclList;
    std::vector<SgFunctionDefinition *> fdefList;
};

void IsolateModulesToFiles(SgProject *project) {
    IsolateModules im;
    im.traverseInputFiles(project, preorder, STRICT_INPUT_FILE_TRAVERSAL);
    im.process();
}
