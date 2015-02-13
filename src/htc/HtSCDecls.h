/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "rose.h"
#include "sageInterface.h"

using namespace std;
using namespace SageInterface;
using namespace SageBuilder;

#define MAX_SCUINT 64

extern bool debugHooks;

class scDeclarations {
 public:
    scDeclarations(SgGlobal *global_scope) {

        // Declare P_mif_rdVal_index
        P_mif_rdVal_index = 
            buildVariableDeclaration("P_mif_rdVal_index", buildIntType());
        addDeclarationToAST(P_mif_rdVal_index, global_scope);

        // Create ht_uint typedefs
        SgTypeUnsignedLongLong *baseType = buildUnsignedLongLongType();
        for (int i = MAX_SCUINT-1; i >= 0; i--) {
            std::string name = "ht_uint" + to_string<unsigned int>(i+1);
            sc_uint_types[i] = 
                buildTypedefDeclaration(name, baseType, global_scope);
            addDeclarationToAST(sc_uint_types[i], global_scope);
        }

        // Create MemAddr_t typedef
        // Uses ht_uint48, so must come after it
        // are prepended, so add it first
        memaddrt = buildTypedefDeclaration("MemAddr_t", 
                                           get_sc_type(48),
                                           global_scope);
        insertStatement(sc_uint_types[MAX_SCUINT-1], memaddrt,  false /* after */);
        if (!debugHooks) {
            memaddrt->unsetOutputInCodeGeneration();
        }

        // Create htc_tid_t typedef
        // Uses ht_uint9, so must come after it
        // are prepended, so add it first
        htctidt = buildTypedefDeclaration("htc_tid_t", 
                                           get_sc_type(9),
                                           global_scope);
        insertStatement(memaddrt, htctidt,  false /* after */);
        if (!debugHooks) {
            htctidt->unsetOutputInCodeGeneration();
        }

        // Create htc_teams_t typedef
        // Uses ht_uint4, so must come after it
        // are prepended, so add it first
        htcteamst = buildTypedefDeclaration("htc_teams_t", 
                                           get_sc_type(4),
                                           global_scope);
        insertStatement(memaddrt, htcteamst,  false /* after */);
        if (!debugHooks) {
            htcteamst->unsetOutputInCodeGeneration();
        }
    }

    SgTypedefType *get_sc_type(int width) {
        ROSE_ASSERT(width > 0 && width <= MAX_SCUINT);
        return sc_uint_types[width-1]->get_type();
    }

    SgTypedefType *get_MemAddr_t_type() {
        return memaddrt->get_type();
    }

    SgTypedefDeclaration *get_MemAddr_t() {
        return memaddrt;
    }

    SgTypedefType *get_htc_tid_t_type() {
        return htctidt->get_type();
    }

    SgTypedefDeclaration *get_htc_tid_t() {
        return htctidt;
    }

    SgTypedefType *get_htc_teams_t_type() {
        return htcteamst->get_type();
    }

    SgTypedefDeclaration *get_htc_teams_t() {
        return htcteamst;
    }

    SgVariableDeclaration *get_P_mif_rdVal_index() {
        return P_mif_rdVal_index;
    }

 private :

    void addDeclarationToAST(SgStatement *S, SgGlobal *global_scope) {
        SageInterface::prependStatement(S, global_scope);
        if (!debugHooks) {
            S->unsetOutputInCodeGeneration();
        }
    }

    template <class T>
    std::string to_string (const T& t) {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }

    SgTypedefDeclaration *sc_uint_types[MAX_SCUINT];
    SgTypedefDeclaration *memaddrt;
    SgTypedefDeclaration *htctidt;
    SgTypedefDeclaration *htcteamst;

    SgVariableDeclaration *P_mif_rdVal_index;
    
};
