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
#include "ProcessMemRefs.h"
#include "Normalize.h"
#include "HtSageUtils.h"

#define foreach BOOST_FOREACH  // Replace with range-based for when C++'11 


#define BREAKSTATES 1

#include "rose.h"
#include "sageInterface.h"
#include "MyConstantFolding.h"
#include "EmitHTDescriptionFile.h"
#include "HtdInfoAttribute.h"
#include "HtcAttributes.h"
#include "HtSageUtils.h"
#include "HtSCDecls.h"
#include "ProcessStencils.h"

extern bool debugHooks;

#include <iostream>

using namespace std;
using namespace SageInterface;
using namespace SageBuilder;

SgGlobal *global_scope;

extern scDeclarations *SCDecls;
class SharedScalarManager *SSM;

std::map<std::string, int> uplevelWriteCounts;


// Stolen from safeInterface.C (set_name) and modified to handle
// parameters.  

// Changes the name of the initializednameNode in the symbol table to new_name.
int my_change_name ( SgInitializedName *initializedNameNode, SgName new_name) {
    // find the appropriate symbol table, delete the symbol 
    // with the old name and add a symbol with the new name.
    ROSE_ASSERT(initializedNameNode != NULL);
    
    SgScopeStatement *scope_stmt = initializedNameNode->get_scope();
    
    ROSE_ASSERT(scope_stmt != NULL);
    ROSE_ASSERT(scope_stmt->get_symbol_table() != NULL);
    ROSE_ASSERT(scope_stmt->get_symbol_table()->get_table() != NULL);
    
    SgDeclarationStatement * parent_declaration = 
        initializedNameNode->get_declaration();
    
    ROSE_ASSERT(parent_declaration != NULL);
    
    // Find the symbols associated with p_name
    std::pair<SgSymbolTable::hash_iterator,SgSymbolTable::hash_iterator> pair_it = scope_stmt->get_symbol_table()->get_table()->equal_range(initializedNameNode->get_name());
    
    SgSymbolTable::hash_iterator found_it = scope_stmt->get_symbol_table()->get_table()->end();
    
    for (SgSymbolTable::hash_iterator it = pair_it.first; it != pair_it.second; ++it) {
        switch(parent_declaration->variantT()) {
        case V_SgVariableDeclaration:
        case V_SgFunctionParameterList:
            {
                if (isSgVariableSymbol((*it).second)!=NULL)
                    found_it = it;
                break;
            }
        case V_SgClassDeclaration:
            {
                if (isSgClassSymbol((*it).second)!=NULL)
                    found_it = it;
                break;
            }
        case V_SgFunctionDeclaration:
            {
                if (isSgFunctionSymbol((*it).second)!=NULL)
                    found_it = it;
                break;
            }
        default:
            {
            }
        };
    }
    
    // there is no Variable,Class or Function symbol associated with p_name
    if (found_it == scope_stmt->get_symbol_table()->get_table()->end()) {
        printf ("Warning: There is no Variable, Parameter, Class or Function symbol associated with p_name \n");
        return 0;
    }
    
    SgSymbol * associated_symbol = (*found_it).second;
    
    // erase the name from there
    scope_stmt->get_symbol_table()->get_table()->erase(found_it);
    
    // insert the new_name in the symbol table
    found_it = scope_stmt->get_symbol_table()->get_table()->insert(pair<SgName,SgSymbol*> ( new_name,associated_symbol));
    
    // if insertion failed
    if (found_it == scope_stmt->get_symbol_table()->get_table()->end()) {
        printf ("Warning: insertion of new symbol failed \n");
        return 0;
    }

    // Set the p_name to the new_name
    //     printf ("Reset initializedNameNode->get_name() = %s to new_name = %s \n",initializedNameNode->get_name().str(),new_name.str());
    initializedNameNode->set_name(new_name);

    return 1;
}

void prefixSymInSymbolTable(SgInitializedName *n2, std::string prefix) {

    // code borrowed from inlinerSupport.C
    SgName name(prefix + n2->get_name().getString());
    // std::cerr << "prefixing " << n2->get_name().getString() << std::endl;
    my_change_name(n2, name);
}

bool isGlobalVariable(SgInitializedName *iname) {
    SgScopeStatement *scope = iname->get_scope();
    return (isSgGlobal(scope) != 0);
}


// This function stolen from rose/projects/RTC/ArithCheck.C
void printNode(SgNode* node) {
    ROSE_ASSERT(node);
    if (node->get_parent()) {
        printf("node: %s = %s parent %s\n", node->sage_class_name(), node->unparseToString().c_str(), node->get_parent()->sage_class_name());
        printf("%s\n",  node->unparseToString().c_str());
    }    
}

// Search up the tree for first SgStatement that is not
// an SgExprStatement.
SgStatement *findEnclosingStatement( SgNode *node) {
    SgNode *n = node;
    
    while (n && !(isSgStatement(n) && isSgBasicBlock(n->get_parent()))) {
        n = n->get_parent();
    }
    return isSgStatement(n);
}


//---------------------------------------------------------------------- 


class MyNonpackedTypeLayoutGenerator: public ChainableTypeLayoutGenerator {
public:
    MyNonpackedTypeLayoutGenerator(ChainableTypeLayoutGenerator* next)
        : ChainableTypeLayoutGenerator(next)
    {}
    virtual StructLayoutInfo layoutType(SgType* t) const;
    
private:
    void layoutOneField(SgType* fieldType, SgNode* decl, bool isUnion /* Is type being laid out a union? */, size_t& currentOffset, StructLayoutInfo& layout) const;
};

void MyNonpackedTypeLayoutGenerator::layoutOneField(SgType* fieldType, SgNode* decl, bool isUnion, size_t& currentOffset, StructLayoutInfo& layout) const {
    StructLayoutInfo fieldInfo = this->beginning->layoutType(fieldType);
    if (fieldInfo.alignment > layout.alignment) {
        layout.alignment = fieldInfo.alignment;
    }
    if (fieldInfo.alignment && (currentOffset % fieldInfo.alignment != 0)) {
        size_t paddingNeeded = fieldInfo.alignment - (currentOffset % fieldInfo.alignment);
        if (!isUnion) {
            layout.fields.push_back(StructLayoutEntry(NULL, currentOffset, paddingNeeded));
        }
        currentOffset += paddingNeeded;
    }
    layout.fields.push_back(StructLayoutEntry(decl, currentOffset, fieldInfo.size));
    currentOffset += fieldInfo.size; // We need to do this even for unions to set the object size
    if (currentOffset > layout.size) {
        layout.size = currentOffset;
    }
    if (isUnion) {
        ROSE_ASSERT (currentOffset == fieldInfo.size);
        currentOffset = 0;
    }
}

StructLayoutInfo MyNonpackedTypeLayoutGenerator::layoutType(SgType* t) const {
    switch (t->variantT()) {
    case V_SgClassType: { // Also covers structs and unions
        SgClassDeclaration* decl = isSgClassDeclaration(isSgClassType(t)->get_declaration());
        ROSE_ASSERT (decl);
        decl = isSgClassDeclaration(decl->get_definingDeclaration());
        ROSE_ASSERT (decl);
        SgClassDefinition* def = decl->get_definition();
        ROSE_ASSERT (def);
        StructLayoutInfo layout;
        size_t currentOffset = 0;
        const SgBaseClassPtrList& bases = def->get_inheritances();
        for (SgBaseClassPtrList::const_iterator i = bases.begin();
             i != bases.end(); ++i) {
            SgBaseClass* base = *i;
            SgClassDeclaration* basecls = base->get_base_class();
            layoutOneField(basecls->get_type(), base, false, currentOffset, layout);
        }
        const SgDeclarationStatementPtrList& body = def->get_members();
        bool isUnion = (decl->get_class_type() == SgClassDeclaration::e_union);
        for (SgDeclarationStatementPtrList::const_iterator i = body.begin();
             i != body.end(); ++i) {
            SgDeclarationStatement* mem = *i;
            SgVariableDeclaration* vardecl = isSgVariableDeclaration(mem);
            SgClassDeclaration* classdecl = isSgClassDeclaration(mem);
            bool isUnnamedUnion = classdecl ? classdecl->get_isUnNamed() : false;
            if (vardecl) {
                // if (!vardecl->get_declarationModifier().isDefault()) continue; // Static fields and friends
                ROSE_ASSERT (!vardecl->get_bitfield());
                const SgInitializedNamePtrList& vars = isSgVariableDeclaration(mem)->get_variables();
                for (SgInitializedNamePtrList::const_iterator j = vars.begin();
                     j != vars.end(); ++j) {
                    SgInitializedName* var = *j;
                    layoutOneField(var->get_type(), var, isUnion, currentOffset, layout);
                }
            } else if (isUnnamedUnion) {
                layoutOneField(classdecl->get_type(), classdecl, isUnion, currentOffset, layout);
            } // else continue;
        }
        if (layout.alignment != 0 && layout.size % layout.alignment != 0) {
            size_t paddingNeeded = layout.alignment - (layout.size % layout.alignment);
            if (!isUnion) {
                layout.fields.push_back(StructLayoutEntry(NULL, layout.size, paddingNeeded));
            }
            layout.size += paddingNeeded;
        }
        return layout;
    }
    case V_SgArrayType: {
        StructLayoutInfo layout = this->beginning->layoutType(isSgArrayType(t)->get_base_type());
        layout.fields.clear();
        SgArrayType *tp = isSgArrayType(t);
        MyConstantFolding::constantFoldingOptimization(tp);
        
        SgExpression* numElements = tp->get_index();
        
        //Adjustment for UPC array like a[100*THREADS],treat it as a[100]
        // Liao, 8/7/2008
        if (isUpcArrayWithThreads(isSgArrayType(t)))
            {
                SgMultiplyOp* multiply = isSgMultiplyOp(isSgArrayType(t)->get_index());
                ROSE_ASSERT(multiply);
                
                // DQ (9/26/2011): Do constant folding if required.
                // SageInterface::constantFolding(multiply);
                
                numElements = multiply->get_lhs_operand();
            }  
        if (!isSgValueExp(numElements)) {
            cerr << "Error: trying to compute static size of an array with non-constant size" << endl;
            layout.size = 4;
            return layout;
            abort();
        }
        SgValueExp *value = isSgValueExp(numElements);
        if (!value) {
            layout.size = 4;
            return layout;
        }
        layout.size *= SageInterface::getIntegerConstantValue(value);
        return layout;
    }
    case V_SgTypeComplex: {
        //"Each complex type has the same representation and alignment requirements as 
        //an array type containing exactly two elements of the corresponding real type"
        StructLayoutInfo layout = this->beginning->layoutType(isSgTypeComplex(t)->get_base_type());
        layout.size *= 2;
        return layout;
    }
    case V_SgTypeImaginary: {
        StructLayoutInfo layout = this->beginning->layoutType(isSgTypeImaginary(t)->get_base_type());
        return layout;
    }
        
    default: return ChainableTypeLayoutGenerator::layoutType(t);
    }
}

#if DEBUG
void typeLayout(SgFunctionDefinition *fdef) {

    static X86_64PrimitiveTypeLayoutGenerator gen1_x86_64(NULL);
    static MyNonpackedTypeLayoutGenerator gen_x86_64(&gen1_x86_64);

    // Process every type used in a variable or parameter declaration
    vector<SgNode*> initNames = NodeQuery::querySubTree(fdef, V_SgInitializedName);
    for (size_t i = 0; i < initNames.size(); ++i) {
        SgInitializedName* in = isSgInitializedName(initNames[i]);
        SgType* t = in->get_type();
        if (isSgTypeEllipse(t)) continue;
        cout << in->get_name().getString() << " has type " << t->unparseToString() << " " << t->sage_class_name() << ":\n";
        //    cout << "On x86-64: ";
        //    cout << gen_x86_64.layoutType(t) << "\n";
        StructLayoutInfo const &info = gen_x86_64.layoutType(t);
        cout << info << std::endl;
        if (isSgArrayType(t)) 
            cout<<"Array element count is:"<<SageInterface::getArrayElementCount(isSgArrayType(t))<<endl;
    }
}
#endif


size_t getSizeOf(SgType *ty) {
    static X86_64PrimitiveTypeLayoutGenerator gen1_x86_64(NULL);
    static MyNonpackedTypeLayoutGenerator gen_x86_64(&gen1_x86_64);

    StructLayoutInfo const &info = gen_x86_64.layoutType(ty);

    return info.size;
}


int getFieldOffset(SgInitializedName *field, SgClassType *S) {

    static X86_64PrimitiveTypeLayoutGenerator gen1_x86_64(NULL);
    static MyNonpackedTypeLayoutGenerator gen_x86_64(&gen1_x86_64);

    StructLayoutInfo const &info = gen_x86_64.layoutType(S);

    vector<StructLayoutEntry>::const_iterator si;

    for (si = info.fields.begin(); si != info.fields.end(); si++) {
        if (si->decl == field) {
#if DEBUG
            std::cout << "found field for " << field->get_name().getString() <<
                std::endl;
#endif
            return si->byteOffset;
        }
    }
    std::cerr << "Did not find  field for " << field->get_name().getString() <<
        std::endl;
    assert(0);

}

//-----------------------------------------------------------------------
// Add the design info level declarations, attaching them to
// the first function encountered.
//-----------------------------------------------------------------------
void emitDesignInfoDeclarations(SgProject *project) {

    class VisitFunctionDefs : public AstSimpleProcessing {
    private:
        void visit(SgNode *S) {
            static bool first = true;
            if (!first) {
                return;
            }
            switch (S->variantT()) {
            case V_SgFunctionDefinition: {
                SgFunctionDefinition *fdef = isSgFunctionDefinition(S);
                HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
                assert(htd && "missing HtdInfoAttribute");
                htd->appendTypedef("ht_uint48", "MemAddr_t");
                htd->appendTypedef("ht_uint10", "htc_tid_t");
                htd->appendTypedef("ht_uint4", "htc_teams_t");
                first = false;
                return;
            }
            default:
                break;
            }
        }
    } edi;
    edi.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}


// Exact calculation of integer log2, except returns 0 for 0 
inline int ActualLg2 (unsigned int val) {
    int ret = -1;

    if (val <= 1) {
        return 0;
    }

    while (val != 0) {
        val >>= 1;
        ret++;
    }
    return ret;
}

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

// Matches calculations done in CnyHt for determining widths
inline int FindLg2(uint64_t value, bool bLg2ZeroEqOne=true) {
    int lg2;
    for (lg2 = 0; value != 0 && lg2 < 64; lg2 += 1)
        value >>= 1;
    return (lg2==0 && bLg2ZeroEqOne) ? 1 : lg2;
}


// Capitalize a string
// String mut have at least one character --- not checked
inline void capitalize(std::string &str) {
    std::transform(str.begin(), str.begin()+1, str.begin(), 
                   std::ptr_fun<int, int>(std::toupper));
}

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

std::string tempNameFor(std::string S) {
    static unsigned tempCount = 0;
    
    std::string result;
    result = S + "_x_hwt_" + to_string<int>(tempCount);
    tempCount++;
    
    return result;
}

// Search up the tree for the top of this array reference chain
SgPntrArrRefExp *findTopSgPntrArrRefExp( SgPntrArrRefExp *PAR) {
    SgPntrArrRefExp *top = PAR;
    
    while (isSgPntrArrRefExp(top->get_parent())) {
        top = isSgPntrArrRefExp(top->get_parent());
    }
    return top;
}

// Search up the tree for the top of the aggregate reference chain
SgExpression *findTopAggregateExp( SgExpression *AR) {
    SgExpression *top = AR;

    SgExpression *parent = isSgExpression(top->get_parent());
    
    while (isSgPntrArrRefExp(parent) || isSgDotExp(parent)) {
        top = parent;
        parent = isSgExpression(parent->get_parent());
    }
    return top;
}
    
SgType *createVoidPointerType() {
    return buildPointerType(buildVoidType());
}

SgSymbol *nameIsDeclaredInScope(std::string &name, SgScopeStatement *scope) {
    SgSymbolTable *symtab = scope->get_symbol_table();
    SgName sname(name);
    return symtab->find_any(sname, NULL, NULL);
}



void declareGRReadFunction( std::string &func_name_string,
                            SgType *type) {

    HtSageUtils::DeclareFunctionInScope(func_name_string, type, global_scope);
}


// Declares a variable of given name and type and adds it into the 
// function scope if it does not already exist.
void declareVariableInFunction(SgFunctionDefinition *f_def,
                               std::string &varName,
                               SgType *type,
                               bool addprivate=true) {
    
    std::string prefixedName;

    if (addprivate) {
        prefixedName = "P_"+varName;
    } else {
        prefixedName = varName;
    }
    
    if (!nameIsDeclaredInScope(prefixedName, f_def->get_body())) {
        SgVariableDeclaration *varDecl =
            buildVariableDeclaration(prefixedName, type, 0, f_def->get_body());
        SageInterface::prependStatement(varDecl, f_def->get_body());
        if (!debugHooks) {
            varDecl->unsetOutputInCodeGeneration();
        }
        
        if (addprivate) {
            HtdInfoAttribute *htd = getHtdInfoAttribute(f_def);
            assert(htd && "missing HtdInfoAttribute");
            htd->appendPrivateVar("", type->stripType(SgType::STRIP_MODIFIER_TYPE)->unparseToString(), varName);
        }
    }
}


//----------------------------------------------------------------------

void addMemberFunctionPointer( std::string function_name,
                               SgType *return_type,
                               SgFunctionParameterTypeList *parameters,
                               SgClassDefinition *structDef) {
    SgFunctionType *fn_t = buildFunctionType(return_type, parameters);
    SgPointerType *fn_ptr_t = buildPointerType(fn_t);

    SgVariableDeclaration *vd = buildVariableDeclaration(function_name,
                                                         fn_ptr_t,
                                                         NULL,
                                                         structDef);
    structDef->append_member(vd);
}


// NOT USED until we implement block rams
SgNode *declareSharedMemTypeForBlockRams(SgVarRefExp *var) {
    static std::set<SgInitializedName *> sharedDecls;

    SgFunctionParameterTypeList *parms;    
    SgVariableDeclaration *member;

    SgType *type = var->get_type();
    SgName name = var->get_symbol()->get_name();
    SgInitializedName *iname = var->get_symbol()->get_declaration();

    // Only declare once
    if (sharedDecls.find(iname) != sharedDecls.end()) {
        return 0;
    }
    sharedDecls.insert(iname);

    SgClassDeclaration *structDecl = 
        buildStructDeclaration(name, global_scope);
    SgClassDefinition *structDef = buildClassDefinition(structDecl);
    
    // readmem
    parms = buildFunctionParameterTypeList();
    addMemberFunctionPointer("read_mem", type, parms, structDef);

    // read_addr
    parms = buildFunctionParameterTypeList();
    parms->append_argument(SCDecls->get_MemAddr_t_type());
    addMemberFunctionPointer("read_addr", buildVoidType(), parms, structDef);

    // write_mem
    parms = buildFunctionParameterTypeList();
    parms->append_argument(type);
    addMemberFunctionPointer("write_mem", buildVoidType(), parms, structDef);

    // write_addr
    parms = buildFunctionParameterTypeList();
    parms->append_argument(SCDecls->get_MemAddr_t_type());
    addMemberFunctionPointer("write_addr", buildVoidType(), parms, structDef);

    // add to AST
    //    SageInterface::prependStatement(structDecl, global_scope);
    insertStatement(SCDecls->get_MemAddr_t(), structDecl, false /* after */);
    if (!debugHooks) {
        structDecl->unsetOutputInCodeGeneration();
    }    
    return structDecl;

}

class UplevelRef {
public:
    UplevelRef(SgExpression *ref,
               SgVariableSymbol *sym,
               SgExpression *addr2,
               bool assignment) :
        REF(ref),
        SYM(sym),
        ADDR2(addr2),
        _assignment(assignment)
    { }

    int getLevel()  { 
        return getOmpUplevelAttribute(SYM)->getUplevel();
    }
    SgExpression *getRefExp()   { return REF; }
    SgVariableSymbol *getSym()  { return SYM; }
    bool isAssignment()         { return _assignment; }
    SgExpression *getAddr2()    { return ADDR2; }

private :
    SgExpression *REF;
    SgVariableSymbol *SYM;
    SgExpression *ADDR2;
    bool _assignment;
};

class MifRef {
public :
    MifRef(SgExpression *addr,
           SgExpression *ref, 
           bool assignment) :
        address(addr),
        REF(ref),
        _assignment(assignment),
        fieldId(-1)
    { }
    
    SgExpression *getAddress()  { return address; }
    SgExpression *getRefExp()       { return REF; }
    bool isAssignment()         { return _assignment; }
    SgType* getBaseType()       { return REF->get_type(); }
    unsigned int getFieldId()   { return fieldId; }
    
    void setFieldId(int id) { fieldId = id; }
    
private :
    SgExpression *address;
    SgExpression *REF;
    bool _assignment;
    int fieldId;
};



class MifField {
public:
    MifField(SgType* bt) :
        baseType(bt),
        allocated(false)
    {}
    
    SgType* getBaseType() { return baseType; }
    bool isAllocated()    { return allocated; }
    bool isUnallocated()  { return !allocated; }
    
    void setAllocated()   { allocated = true; }
    void clearAllocated() { allocated = false; }
    
private:
    bool _assignment;
    SgType* baseType;
    bool allocated;
};


class GlobalVarRef {
public:
    GlobalVarRef (SgExpression *expr,
            SgInitializedName *iname, 
            bool assignment, 
            SgExpression *index,
            unsigned int lgSize, 
            SgInitializedName *fld, 
            SgType* qt ) :
        EXPR(expr),
        IN(iname),
        _assignment(assignment),
        indexExpr(index),
        _lgSize(lgSize),
        field(fld),
        qualType(qt)
    {}
    
    SgExpression *getSgExpression() { return EXPR; }
    SgInitializedName *getSgInitializedName() { return IN; }
    bool isAssignment() { return _assignment; }
    SgExpression *getIndexExpr() { return indexExpr; }
    unsigned int getLgSize() { return _lgSize; }
    SgInitializedName* getField() { return field; }
    SgType* getSgType() { return qualType; }
    
private:
    SgExpression *EXPR;
    SgInitializedName *IN;
    bool _assignment;
    SgExpression *indexExpr;
    unsigned int _lgSize;
    SgInitializedName* field;
    SgType*  qualType;
};

class FieldManager {
public:
    FieldManager() {
        fieldList.clear();
    }
    
    void unallocateAllFields() {
        for (unsigned int i = 0; i < fieldList.size(); i++) {
            fieldList.at(i)->clearAllocated();
        }
    }
    
    unsigned int allocateFieldId(SgType* bt, SgFunctionDefinition *f_def) {
        unsigned int id;
        
        id = findMatchingUnallocatedField(bt, f_def);
        fieldList.at(id)->setAllocated();

        return id;
    }
    
    void clear() { fieldList.clear(); }
    
    SgFunctionDeclaration *declareReadMem(int field, 
                                          SgType *type,
                                          SgFunctionDefinition *f_def) {
        SgFunctionParameterTypeList *parms = buildFunctionParameterTypeList();

        parms->append_argument(SCDecls->get_MemAddr_t_type());
        parms->append_argument(buildIntType());
        
        std::string func_name_string = "ReadMem_f";
        func_name_string += to_string<unsigned int>(field);
        SgFunctionDeclaration *func = 
            buildNondefiningFunctionDeclaration(
                  func_name_string,
                  buildVoidType(),
                  buildFunctionParameterList(parms),
                  global_scope);
        SageInterface::setExtern(func);

        SageInterface::prependStatement(func, f_def->get_body());
        if (!debugHooks) {
            func->unsetOutputInCodeGeneration();
        }

        return func;
    }

    void declareMifReadInterface(std::string &functionName,
                                 SgFunctionDefinition *fndef) {
        
        HtdInfoAttribute *htd = getHtdInfoAttribute(fndef);
        assert(htd && "missing HtdInfoAttribute");

        // P_ref_rdVal_index is constant, so assign at beginning of function
        SgExprStatement *AssignRdValIndex =
            buildAssignStatement(buildVarRefExp("P_mif_rdVal_index", fndef),
                                 buildVarRefExp("PR_htId", global_scope));
        insertStatementAfterLastDeclaration (AssignRdValIndex, fndef);
        
        std::string uniqueId = tempNameFor("mif_rdVal_index");
        
        htd->appendTypedef( "sc_uint<" + Upper(functionName) + "_HTID_W>",
                            uniqueId + "_t");

        htd->appendPrivateVar("", uniqueId + "_t", "mif_rdVal_index");
        
        std::string ReadMemString("AddReadMem()\n");

        ReadMemString += "// ReadMem interface to global off-chip memory\n";        
        for (unsigned int i = 0; i < fieldList.size(); i++) {

            MifField *field = fieldList.at(i);
            SgType* baseType = field->getBaseType();
            baseType = baseType->stripType(SgType::STRIP_TYPEDEF_TYPE);
            size_t basesize = getSizeOf(baseType) * 8;

            if (basesize > 64) {
                std::cerr << "Reads larger than 64 bits are not yet supported in HTC" << std::endl;
                //                assert(0);
            }

            ReadMemString += 
                "    .AddDst(name=f" +
                to_string<int>(i) + 
                ", var=" + functionName + "_fld" + 
                to_string<int>(i) +
                ", memSrc=host)\n";
        }
        ReadMemString +=  "    ;\n" ;
        htd->appendArbitraryModuleString( ReadMemString) ;
        
        std::string RamString;
        RamString += "AddGlobal()\n";
        
        RamString += "// Ram for reading global off-chip memory\n";

        // FIXME: need to insert correct width for fields
        for (unsigned int i = 0; i < fieldList.size(); i++) {
            MifField *field = fieldList.at(i);
            SgType* baseType = field->getBaseType();

            if (isPointerType(baseType)) {
                baseType = SCDecls->get_MemAddr_t_type();
            }
            
            std::string typeString = baseType->unparseToString();
            
            RamString += "    .AddVar(type=" + 
                typeString +
                ", name=" + functionName + "_fld" + 
                to_string<int>(i) +
                ", addr1=mif_rdVal_index, addr1W=" +
                Upper(functionName) +
                "_HTID_W" +
                ", read=true, write=false)\n" ;
        }
        RamString += "    ;\n\n" ;

        htd->appendArbitraryModuleString( RamString) ;
    }
    
    //    void setTextFile (ostream *f) { textFile = f; };
    
private:

    unsigned int addField(SgType* bt, SgFunctionDefinition *f_def) {
        MifField *newField = new MifField(bt);
        fieldList.push_back(newField);
        declareReadMem(fieldList.size()-1, bt, f_def);
        return (fieldList.size()-1);
    }
    
    unsigned int findMatchingUnallocatedField(SgType* bt, SgFunctionDefinition *f_def) {
        for (unsigned int i = 0; i < fieldList.size(); i++) {
            MifField *f = fieldList.at(i);
            
            if ( f->isUnallocated()  && 
                 (f->getBaseType() == bt))  {
                return i;
            }
        }
        return addField(bt, f_def);
    }
    
    std::vector<MifField *> fieldList;
    //    ostream *textFile;
    
};

class StateNamer {
public:
    StateNamer( const char *base ) :
        baseName(base),
        counter(0)
    { }
    
    std::string nextState() {
        std::string result;
        
        counter++;
        result = baseName + to_string<int>(counter);
        
        return result;
    }

    int currentStateNum() {
        return counter-1;
    }
    
    int numStates() { return counter; }
    void clear() { counter = 0; }
private:
    const char *baseName;
    int counter;
};


class TransformMemoryReferences : public AstSimpleProcessing {
    
    std::vector<SgInitializedName *> local_scalars;
    std::set<SgInitializedName *> local_arrays;
    std::vector<SgInitializedName *> parameters;
    std::set<SgInitializedName *> global_scalars;
    std::set<SgVariableSymbol *> array_parameter_address_assigns;
    std::vector<GlobalVarRef *> globalVarRefs;
    std::vector<UplevelRef *> uplevelRefs;
    std::vector<SgInitializedName *> fields;
    std::vector<SgExprStatement *> writeMemStatements;
    std::set<int> writeMemSizes;
    std::string functionName;
    std::map<SgStatement *, std::vector<MifRef *>* >  mifrefs;
    FieldManager FM;
    bool haveMifReads;
    bool haveMifWrites;
    StateNamer mifStates;
    StateNamer globalVarStates;
    
public:
    TransformMemoryReferences( ) :
        mifStates("MIF_STATE_"),
        globalVarStates("GLOBALMEM_STATE_")
    {
        local_scalars.clear();
        global_scalars.clear();
        array_parameter_address_assigns.clear();
        local_arrays.clear();
        parameters.clear();
        globalVarRefs.clear();
        uplevelRefs.clear();
        fields.clear();
        functionName.clear();
        mifrefs.clear();
        FM.clear();
        haveMifReads = false;
        haveMifWrites = false;
        mifStates.clear();
        globalVarStates.clear();
        writeMemStatements.clear();
        writeMemSizes.clear();
    }

    std::string getFunctionName() { return functionName; }
    std::map<SgStatement *, std::vector<MifRef *>* > getMifRefs() {
        return mifrefs; 
    }
    
    void addMifRef(SgStatement *S, MifRef *MR) {
        assert(S);
        std::vector<MifRef *> *miflist = mifrefs[S];
        if (miflist == 0) {
            miflist = new std::vector<MifRef *>;
            mifrefs[S] = miflist;
        }
        miflist->push_back(MR);
    }
    
    void ProcessMifRead(SgStatement *stmt, 
                        MifRef *mr,
                        bool doPause) {
        
        // fprintf(stderr,"%s was called\n", __FUNCTION__);

        SgScopeStatement *scope = stmt->get_scope();
        SgFunctionDefinition * f_def = getEnclosingProcedure (scope);

        SgType* baseType = mr->getBaseType();
        
        SgType *MemAddr_t = SCDecls->get_MemAddr_t_type();

        SgExpression *top = mr->getRefExp();

        mr->setFieldId(FM.allocateFieldId(baseType, f_def));
        
        std::string nm = "ReadMem_f";
        nm += to_string<unsigned int>(mr->getFieldId());
        SgName readMemName(nm);
        SgExprStatement* readMemCallStmt = 
            buildFunctionCallStmt( readMemName,
                                   buildVoidType(), 
                                   buildExprListExp(
                                      buildCastExp(
                                         mr->getAddress(),
                                         SCDecls->get_MemAddr_t_type()),
                                      buildVarRefExp("PR_htId", global_scope),
                                      buildIntVal(1)), /* length */
                                   scope);
        insertStatement (stmt, readMemCallStmt, true /* before */);

#if BREAKSTATES
        if (doPause) {
            // This state stuff needs to be revisited!!!!
            std::string nextStateName = mifStates.nextState();

            SgName readMemPauseName("ReadMemPause");
            SgExpression *nextState = buildIntVal(-999);
            SgExprStatement* readMemPauseCallStmt = 
                buildFunctionCallStmt( readMemPauseName,
                                       buildVoidType(), 
                                       buildExprListExp(nextState),
                                       scope);
            insertStatement (stmt, readMemPauseCallStmt, true /* before */);

            SgLabelStatement *nextLabel = HtSageUtils::makeLabelStatement("READMEM", f_def);
            SageInterface::insertStatementAfter(readMemPauseCallStmt, nextLabel);
            // Mark the placeholder expression with an FsmPlaceHolderAttribute to 
            // indicate a deferred FSM name is needed.
            FsmPlaceHolderAttribute *attr = new FsmPlaceHolderAttribute(nextLabel);
            nextState->addNewAttribute("fsm_placeholder", attr);
        } else {
            SgLabelStatement *nextLabel = 
                HtSageUtils::makeLabelStatement("READMEM", f_def);
            SgGotoStatement *gts = buildGotoStatement(nextLabel);
            insertStatement(stmt, gts, true /* before */);
            insertStatement(gts, nextLabel, false /* after */);
        }
#endif
        
        // Replace original reference with reference to GR_fld[n]
        std::string GRName("GR_");
        GRName += functionName + "_fld";

        GRName += to_string<unsigned int>(mr->getFieldId());

        declareVariableInFunction(f_def,
                                  GRName,
                                  baseType,
                                  false);
        SgVarRefExp *GR_exp = buildVarRefExp(GRName, scope);
        replaceExpression(top, GR_exp, true);

    }
    
    
    void declareMifWriteInterface(std::string &functionName,
                                  SgFunctionDefinition *fndef) {
        
        HtdInfoAttribute *htd = getHtdInfoAttribute(fndef);
        assert(htd && "missing HtdInfoAttribute");

        std::string addWriteMem = "AddWriteMem()\n";

        std::set<int>::iterator iter;
        for (iter = writeMemSizes.begin(); iter != writeMemSizes.end(); 
             iter++) {
            int size = *iter;
            std::string addSrc = "    .AddSrc(type=uint";
            addSrc += to_string<int>(size);
            addSrc += "_t, memDst=host)\n";

            addWriteMem += addSrc;
        }
        addWriteMem += "    ;\n\n";
        
        htd->appendArbitraryModuleString( addWriteMem );
    }

    void ProcessMifWrite(SgStatement *stmt, 
                         MifRef *mr) {
        // fprintf(stderr,"%s was called\n", __FUNCTION__);

        SgScopeStatement *scope = stmt->get_scope();
        SgFunctionDefinition * f_def = getEnclosingProcedure (scope);
        
        SgType *MemAddr_t = SCDecls->get_MemAddr_t_type();
        SgExpression *top = mr->getRefExp();

        SgAssignOp *assign = isSgAssignOp(top->get_parent());
        assert(assign);
        SgExpression *rhs = deepCopy(assign->get_rhs_operand());

        SgType* baseType = mr->getBaseType();
        baseType = baseType->stripType(SgType::STRIP_TYPEDEF_TYPE);
        size_t basesize = getSizeOf(baseType) * 8;
        writeMemSizes.insert(basesize);
        std::string wrName = "WriteMem_uint";
        wrName += to_string<int>(basesize);
        wrName += "_t";

        SgName writeMemName(wrName);
        SgExprStatement* writeMemCallStmt = 
            buildFunctionCallStmt( 
                writeMemName,
                buildVoidType(), 
                buildExprListExp( 
                    buildCastExp(
                        mr->getAddress(),
                        SCDecls->get_MemAddr_t_type()),
                    rhs), 
                scope);
        insertStatement (stmt, writeMemCallStmt, false /* after */);

#if BREAKSTATES
        writeMemStatements.push_back(writeMemCallStmt);
#endif
    }  

    void InsertWriteMemPauses() {
#if BREAKSTATES
        int i;

        for (i = 0; i < writeMemStatements.size(); i++) {
            SgStatement *S = isSgStatement(writeMemStatements.at(i));

            while (SageInterface::isAssignmentStatement(getNextStatement(S))) {
                S = getNextStatement(S);
            }
            SgGotoStatement *gt = isSgGotoStatement(getNextStatement(S));
            SgLabelStatement *nl = isSgLabelStatement(getNextStatement(S));

            SgScopeStatement *scope = S->get_scope();
            SgFunctionDefinition * f_def = getEnclosingProcedure (scope);

            SgName writeMemPauseName("WriteMemPause");

                
            SgExpression *nextState = buildIntVal(-999);
            SgExprStatement* writeMemPauseCallStmt = 
                buildFunctionCallStmt( writeMemPauseName,
                                       buildVoidType(), 
                                       buildExprListExp(nextState),
                                       scope);
            insertStatement (S, writeMemPauseCallStmt, false /* after */);
            
            // Mark the placeholder expression with an 
            // FsmPlaceHolderAttribute to indicate a deferred FSM name 
            // is needed.
            FsmPlaceHolderAttribute *attr;
            // If the next statement is a goto, the WriteMemPause will return
            // to the target of the goto and we delete the original goto 
            // statement.
            
            if (gt) {
                attr = new FsmPlaceHolderAttribute(gt->get_label());
                SageInterface::removeStatement(gt);
            } else if (nl) {
                attr = new FsmPlaceHolderAttribute(nl);
            } else {
                SgLabelStatement *nextLabel = 
                    HtSageUtils::makeLabelStatement("WRITEMEM", f_def);
                SageInterface::insertStatementAfter(writeMemPauseCallStmt, 
                                                nextLabel);
                attr = new FsmPlaceHolderAttribute(nextLabel);
            }
            nextState->addNewAttribute("fsm_placeholder", attr);
        }
#endif
    }
    
    
    bool ProcessMifRefsForSgStatement(SgStatement *stmt, 
                                      std::vector<MifRef *> *miflist) {

        bool hasWrite = false;

        // Find last read -- will force ReadMemPause
        unsigned int lastReadIndex = miflist->size();
        for (unsigned int i = 0; i < miflist->size(); i++) {
            MifRef *mr = miflist->at(i);
            
            if (!mr->isAssignment()) {
                lastReadIndex = i;
            }
        }
        
        // Do all reads before writes because writes need to see
        // transformed r.h.s
        for (unsigned int i = 0; i < miflist->size(); i++) {
            MifRef *mr = miflist->at(i);
            
            if (!mr->isAssignment()) {
                haveMifReads = true;
                ProcessMifRead(stmt, mr, lastReadIndex == i);
            }
        }
        
        for (unsigned int i = 0; i < miflist->size(); i++) {
            MifRef *mr = miflist->at(i);
            
            if (mr->isAssignment()) {
                haveMifWrites = true;
                hasWrite = true;
                ProcessMifWrite(stmt, mr);
            }
        }

        FM.unallocateAllFields();

        return hasWrite;
    }
    
    
    void ProcessMifRefs() {
        std::map<SgStatement *, std::vector<MifRef *> *>::iterator iter;
        std::vector<SgStatement *> statementsToRemove;
        
        for (iter = mifrefs.begin(); iter != mifrefs.end(); iter++) {
            SgStatement *S = iter->first;
            std::vector<MifRef *> *miflist = iter->second;

            if (miflist->size()) {

                SageInterface::attachComment(S,
                                             S->unparseToString(),
                                             PreprocessingInfo::before,
                                             PreprocessingInfo::CpreprocessorUnknownDeclaration);

                if (ProcessMifRefsForSgStatement(S, miflist)) {
                    statementsToRemove.push_back(S);
                }
            }
        }

        for (int i = 0; i < statementsToRemove.size(); i++) {
            statementsToRemove[i]->setCompilerGenerated();
            statementsToRemove[i]->unsetOutputInCodeGeneration();
        }

    }
    
    void DeclareMifInterfaces(SgFunctionDefinition *fndef) {
        if (haveMifReads) {
            FM.declareMifReadInterface(functionName, fndef);
        }
        if (haveMifWrites) {
            declareMifWriteInterface(functionName, fndef);
        }
    }
    
    unsigned int FindLgSizeForSgVariableDeclaration(SgInitializedName *iname) {
        for (unsigned int i = 0; i < globalVarRefs.size(); i++) {
            GlobalVarRef *ref = globalVarRefs.at(i);
            if (iname == ref->getSgInitializedName()) {
                return ref->getLgSize();
            }
        }
        assert(0);
    }
    
    std::string  DeclareBlockGlobalMemVars(SgInitializedName *iname,
                                           std::string &addrstring,
                                           std::string &functionName) {
        std::string returnString;
        std::set<SgInitializedName *> fieldSet;
        
        for (unsigned int i = 0; i < globalVarRefs.size(); i++) {
            GlobalVarRef *ref = globalVarRefs.at(i);
            if (iname == ref->getSgInitializedName()) {
                SgInitializedName* field = ref->getField();
                if (field) {
                    fieldSet.insert(field);
                }
            }
        }
        
        std::set<SgInitializedName *>::iterator iter;
        for (iter = fieldSet.begin(); iter != fieldSet.end(); iter++) {
            SgInitializedName* fld = isSgInitializedName(*iter);
            
            returnString += "   .AddVar(type="
                + fld->get_type()->unparseToString()
                + ", name="
                + functionName
                + "_"
                + fld->get_name().getString()
                + addrstring
                + ", read=true, write=true)\n";
        }

        returnString += "   ;\n";

        return returnString;

    }
    
    void DeclareBlockGlobalMems(SgFunctionDefinition *fndef) {
        std::set<SgInitializedName *> globalVarSet;
        
        for (unsigned int i = 0; i < globalVarRefs.size(); i++) {
            GlobalVarRef *ref = globalVarRefs.at(i);
            globalVarSet.insert(ref->getSgInitializedName());
        }
        
        HtdInfoAttribute *htd = getHtdInfoAttribute(fndef);
        assert(htd && "missing HtdInfoAttribute");

        std::set<SgInitializedName *>::iterator iter;
        for (iter = globalVarSet.begin(); iter != globalVarSet.end(); iter++) {
            SgInitializedName *vdecl = *iter;
            std::string name = vdecl->get_name().getString();
            
            std::string uniqueId = "globalVar_index_" + name;
            uniqueId = tempNameFor(uniqueId);
            
            std::string width =to_string<int>(FindLgSizeForSgVariableDeclaration(vdecl)) +
                       "+" +
                      Upper(functionName) +
                      "_HTID_W" ;
            
            htd->appendTypedef("sc_uint<" + width + ">",
                               uniqueId + "_t");

            htd->appendPrivateVar("", 
                                  uniqueId + "_t", 
                                  "globalVar_index_" + name);
            
            std::string declareGlobal;
            declareGlobal =  "AddGlobal()\n";
            std::string addrstring = 
                ", addr1=globalVar_index_" + name;
            declareGlobal += "// Global Variable (on-chip) for " + name + "\n";
            
            // output fields (Variables) here
            declareGlobal += DeclareBlockGlobalMemVars(vdecl, 
                                                       addrstring,
                                                       functionName);
            
            htd->appendArbitraryModuleString( declareGlobal) ;
        }
    }
    
    void ProcessGlobalVarRefs( SgFunctionDefinition *fndef) {
        
        DeclareBlockGlobalMems(fndef);
        
        for (unsigned int i = 0; i < globalVarRefs.size(); i++) {
            GlobalVarRef *ref = globalVarRefs.at(i);
            
            SgInitializedName *ME = ref->getField();
            std::string fieldString("");
            
            if (ME) {
#if 0
                if (!ref->isAssignment()) {
                    fieldString += ".m";
                }
#endif
                fieldString += "_" + ME->get_name().getString();
            }
            
            SgPntrArrRefExp *AR = isSgPntrArrRefExp(ref->getSgExpression());
            assert(AR);
            SgExpression *top = findTopAggregateExp(AR);
            SgStatement *stmt = getEnclosingStatement(top);
        
            SgScopeStatement *scope = fndef->get_body();
            SgType* baseType = ref->getSgType();
            SgType *MemAddr_t = SCDecls->get_MemAddr_t_type();
            SgExpression *name;

            ROSE_ASSERT(isArrayReference(AR, &name, 0 /* subscripts */));
        
            std::string arrayName = name->unparseToString();
            std::string tempVarName = tempNameFor(arrayName);

            // Use a register temp for the value because we may have
            // multiple refs in one expression
            
            // Declare temporary variable
            declareVariableInFunction(fndef, tempVarName, baseType);
            tempVarName = "P_" + tempVarName;


            if (ref->isAssignment()) {

                // handle assignment -- writes

                // replace array reference with tempvar in assignment
                SgVarRefExp *refTempVar = buildVarRefExp(tempVarName, scope);
                replaceExpression(top, refTempVar, true);

                std::string GWFuncName = "GW_" + arrayName + fieldString;

                SgExprStatement* GWCallStmt = 
                    buildFunctionCallStmt( 
                        GWFuncName,
                        buildVoidType(), 
                        buildExprListExp( 
                           // ref->getIndexExpr(),
                           buildVarRefExp(tempVarName, scope)),
                        scope);

                SgExprStatement* GWWriteAddr = 
                    buildFunctionCallStmt("__htc_GW_write_addr",
                                          buildVoidType(),
                                          buildExprListExp(
                                             ref->getIndexExpr()),
                                          scope);

                // These are inserted in reverse order so write_addr is first
                // in the resulting sequential output.
                insertStatement (stmt, GWCallStmt, false /* after */);
                insertStatement (stmt, GWWriteAddr, false /* after */);

#if BREAKSTATES
                SgLabelStatement *nextLabel = 
                    HtSageUtils::makeLabelStatement("GW", fndef);
                SgGotoStatement *gts = buildGotoStatement(nextLabel);
                insertStatement(GWCallStmt, gts, false /* after */);
                insertStatement(gts, nextLabel, false /* after */);
#endif
                
            } else {

                // READS
                
                // replace array reference with tempvar in expression
                SgVarRefExp *refTempVar = buildVarRefExp(tempVarName, scope);

                replaceExpression(top, refTempVar, true);

                SgStatement *esParent = 
                    stmt ? isSgStatement(stmt->get_parent()) : 0;
                if (stmt && (isSgSwitchStatement(esParent) 
                             || isSgIfStmt(esParent))) {
                    // Make sure insertion point is outside of the condition of an if-stmt
                    // or the selector of a switch-stmt.
                    stmt = esParent; 
                } 

                // Declare index variable
                std::string indexVarName("P_globalVar_index_" + arrayName);
                declareVariableInFunction(fndef, indexVarName, buildIntType(),
                                          false);

                SgExprStatement *AssignIndexVar = 
                    buildAssignStatement(buildVarRefExp(indexVarName, scope),
                                         ref->getIndexExpr());

                insertStatement (stmt, AssignIndexVar, true /* before */);

                
#if BREAKSTATES
                SgLabelStatement *nextLabel = HtSageUtils::makeLabelStatement("GR", fndef);
                SgGotoStatement *gts = buildGotoStatement(nextLabel);
                insertStatement(stmt, gts, true /* before */);
                insertStatement(gts, nextLabel, false /* after */);
#endif
                // handle use
                std::string GRName("GR_" + arrayName + fieldString);

                declareVariableInFunction(fndef, GRName, baseType, false);

                SgExprStatement* assignToTemp = 
                    buildAssignStatement(buildVarRefExp(tempVarName, scope),
                                         buildVarRefExp(GRName, scope));

                insertStatement (stmt, assignToTemp, true /* before */);

            }
        }
        
    } // ProcessGlobalVarRefs

    //------------------------  UPLEVEL -----------------------------

    // This function has one or more uplevel reads of sym
    bool hasUplevelRead(SgVariableSymbol *sym) {
        for (int i = 0; i < uplevelRefs.size(); i++) {
            UplevelRef *ref = uplevelRefs.at(i);
            if ((ref->getSym() == sym) && (!ref->isAssignment())) {
                return true;
            }
        }
        return false;
    }

    // This function has one or more uplevel writes to sym
    bool hasUplevelWrite(SgVariableSymbol *sym) {
        for (int i = 0; i < uplevelRefs.size(); i++) {
            UplevelRef *ref = uplevelRefs.at(i);
            if ((ref->getSym() == sym) && (ref->isAssignment())) {
                return true;
            }
        }
        return false;
    }

    // Return function at level "level"
    SgFunctionDeclaration *getUplevelFunction (SgFunctionDefinition *fdef,
                                               int level) {
        
        SgFunctionDeclaration *enclosing_function = fdef->get_declaration();
        
        for (int i = 0; i < level; i++) {
            OmpEnclosingFunctionAttribute *enclosing_attr =
                getOmpEnclosingFunctionAttribute(enclosing_function);
            assert(enclosing_attr);
            enclosing_function = enclosing_attr->getEnclosingFunction();
        }
        
        return enclosing_function;
    }
    
    // Return the name of the HT Global Variable for function and symbol
    std::string getUplevelGVName(SgFunctionDeclaration *fdecl,
                                 std::string symname) {
        std::string result("__htc_UplevelGV_");
        result += fdecl->get_name().getString();
        result += "_";
        result += symname;
        return result;
    }

    std::string getUplevelIndexName(int level) {
        std::string result = "__htc";

        result += "_uplevel_index_" + to_string<int>(level);

        return result;
    }

    // Returns the maximum uplevel value for all symbol references in
    // the function
    int maxUplevel() {
        int result = -1;

        for (int i = 0; i < uplevelRefs.size(); i++) {
            UplevelRef *ref = uplevelRefs.at(i);
            int level = ref->getLevel();
            if (level > result) {
                result = level;
            }
        }
        return result;
    }

    void emitAddGlobal(SgFunctionDefinition *fdef,
                       std::string varname,
                       std::string indexname,
                       std::string width,
                       bool Xtern,
                       int addr2W,
                       std::string addr2VarName,
                       std::string comment,
                       std::string fieldname,
                       std::string fieldtype,
                       bool read,
                       bool write) {

        HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
        assert(htd && "missing HtdInfoAttribute");

        std::string declareGlobal;
        declareGlobal =  "AddGlobal()\n";

        declareGlobal += comment;

        declareGlobal += 
            "   .AddVar(type=" + fieldtype + ", name=" + fieldname
            + ", addr1=" + indexname;
#if 0
        // addr1W is now superfulous
            + ", addr1W=" + width;
#endif
        if (addr2W > -1) {
            declareGlobal += ", addr2=" + addr2VarName + ", addr2W=" +
                to_string<int>(addr2W);
        }
#if 0
        // extern is deprecated
        if (Xtern) {
            declareGlobal += ", extern=true";
        }
#endif
        std::string readstr = read ? "true" : "false";
        std::string writestr = write ? "true" : "false";
        declareGlobal += 
            ", read=" + readstr
            + ", write=" + writestr
            + ");\n";

        htd->appendArbitraryModuleString( declareGlobal) ;
    }


    void assignUplevelValue(SgFunctionDefinition *fdef, 
                            std::string uplevelGVName) {
        // Insert store of __htc_parent_htid into uplevel slot 
        
        SgScopeStatement *scope = fdef->get_body();
        
        std::string pid("P___htc_parent_htid");

        std::string htdTypeName = uplevelGVName + "_t";
        
        std::string GWFuncName = 
            "GW_" + uplevelGVName + "_uplevel_index";

        SgExprStatement* GWCallStmt = 
            buildFunctionCallStmt(
                GWFuncName,
                buildVoidType(),
                buildExprListExp(
                    buildVarRefExp("PR_htId", global_scope),
                    buildVarRefExp(pid, scope)),
                scope);
        SageInterface::insertStatementAfterLastDeclaration(GWCallStmt, scope);

        // NOTE: We do NOT need a state break here.    This value will only
        // be read by other Modules (never this one).    By the time a call
        // to start another module can be processed, this value will already
        // be settled in memory.
    }


    void declareUplevelIndexVariables(SgFunctionDefinition *fdef, 
                                      int maxlevel) {

        SgScopeStatement *scope = fdef->get_body();

        for (int i = 0; i <= maxlevel; i++) {

            std::string varName = getUplevelIndexName(i);
            SgFunctionDeclaration *fdecl = getUplevelFunction(fdef, i);
            std::string modulename = fdecl->get_name().getString();
            std::string htdTypeName = modulename + "_uplevel_index_t";

            std::string prefixedName = "P_" + varName;

            SgTypedefDeclaration *htdType = 
                buildTypedefDeclaration(htdTypeName,
                                        buildIntType(),
                                        fdef);
            SageInterface::prependStatement(htdType, fdef->get_body());

            SgVariableDeclaration *varDecl =
                buildVariableDeclaration(prefixedName, 
                                         htdType->get_type(), 0, fdef->get_body());
            insertStatement(htdType, varDecl, false /* after */);
            //            SageInterface::prependStatement(varDecl, fdef->get_body());
            if (!debugHooks) {
                htdType->unsetOutputInCodeGeneration();
                varDecl->unsetOutputInCodeGeneration();
            }
            
            HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
            assert(htd && "missing HtdInfoAttribute");
            htd->appendPrivateVar("", htdTypeName, varName);
        }
    }


    // Assign values to the uplevel index variables
    void assignUplevelIndexVariables(SgFunctionDefinition *fdef, 
                                     int maxlevel) {

        SgScopeStatement *scope = fdef->get_body();

        for (int i = maxlevel; i >=0; i--) {
            std::string pid = getUplevelIndexName(i);
            pid = "P_" + pid;

            SgVarRefExp *lhs = buildVarRefExp(pid, scope);
            SgType* baseType = lhs->get_type();

            // SgType* baseType = buildIntType();
            SgExprStatement *assign;

            switch(i) {
            case 0:
                assign = 
                    buildAssignStatement(
                       lhs,
                       buildCastExp(
                           buildVarRefExp("PR_htId", scope),
                           baseType));
                break;
            case 1:
                assign = 
                    buildAssignStatement(
                       lhs,
                       buildCastExp(
                           buildVarRefExp("P___htc_parent_htid", scope),
                           baseType));
                break;
            default:
                SgFunctionDeclaration *fdecl = getUplevelFunction(fdef, i-1);
                std::string uplevelGVName = 
                    getUplevelGVName(fdecl, "uplevel_index");

                std::string GRName("GR_" + uplevelGVName + "_uplevel_index");

                declareVariableInFunction(fdef, GRName, baseType, false);

                assign = buildAssignStatement(
                            lhs,
                            buildCastExp(
                                buildVarRefExp(GRName, scope),
                                baseType));
                break;
            }

            SageInterface::insertStatementAfterLastDeclaration(assign, scope);

#if BREAKSTATES
            if (i > 0) {
                SgLabelStatement *nextLabel = 
                    HtSageUtils::makeLabelStatement("GR", fdef);
                SgGotoStatement *gts = buildGotoStatement(nextLabel);
                insertStatement(assign, gts, false /* after */);
                insertStatement(gts, nextLabel, false /* after */);
            }
#endif

        }
    }


    // Emits the declarations of the HT Global Variables (GVs) needed
    // for the function 'fdef'.   Returns the maximum uplevel index
    // of all uplevel variables in this function.
    int DeclareUplevelGlobalVariables(SgFunctionDefinition *fdef) {
        std::set<SgVariableSymbol *> uplevel_syms;

        int maxlevel = -1;

        bool contains_parallel_region = 
            fdef->attributeExists("contains_omp_parallel");
        
        SgScopeStatement *scope = fdef->get_body();

        // Collect all uplevel symbol declarations
        Rose_STL_Container< SgNode *> sgVariableDecls;
        sgVariableDecls = 
            NodeQuery::querySubTree (fdef, V_SgVariableDeclaration);
        for (Rose_STL_Container<SgNode *>::iterator 
                 iter = sgVariableDecls.begin();
             iter != sgVariableDecls.end();
             iter++) {
            SgVariableSymbol *sym = 
                SageInterface::getFirstVarSym(isSgVariableDeclaration(*iter));
            if (!sym) {
                continue;
            }            
            OmpUplevelAttribute *attr = getOmpUplevelAttribute(sym);
            if (attr) {
                uplevel_syms.insert(sym);
            }
        }

        // Have to check for parameters separately.   They are not 
        // sgVariableDecls.
        for (int i = 0; i < parameters.size(); i++) {
            SgInitializedName *iname = parameters.at(i);
            SgVariableSymbol * sym = isSgVariableSymbol(iname->get_symbol_from_symbol_table());
            if (sym) {
                OmpUplevelAttribute *attr = getOmpUplevelAttribute(sym);
                if (attr) {
                    uplevel_syms.insert(sym);
                }
            }
        }

        std::set<SgVariableSymbol *>::iterator iter;
        for (iter = uplevel_syms.begin();
             iter != uplevel_syms.end(); iter++) {
            SgVariableSymbol *sym = *iter;

            int level = getOmpUplevelAttribute(sym)->getUplevel();
            bool isParameter = getOmpUplevelAttribute(sym)->isParameter();
            SgFunctionDeclaration *fdecl = getUplevelFunction(fdef, level);
            std::string modulename = fdecl->get_name().getString();
            std::string width = Upper(modulename) + "_HTID_W";
            std::string symname = sym->get_name().getString();
            std::string uplevelGVName = getUplevelGVName(fdecl, symname);
            SgType *symtype = sym->get_type();
            int addr2W = -1;
            std::string addr2VarName;

            // Assign parameter value to the Global Variable
            // Emit calls directly.   They do not need Ht_Continue.
            if (isParameter && (level == 0)) {
                SgVarRefExp *rhs = 
                    buildVarRefExp(sym->get_name().getString(), fdef);

                std::string symname = sym->get_name().getString();
                std::string uplevelGVName = 
                    getUplevelGVName(fdef->get_declaration(), symname);

                std::string GWName("GW_" + uplevelGVName + "_" + symname);

                declareVariableInFunction(fdef,
                                          GWName,
                                          rhs->get_type(),
                                          false);

                SgExprStatement* GWWriteAddr = 
                    buildFunctionCallStmt(
                       "__htc_GW_write_addr",
                       buildVoidType(),
                       buildExprListExp(
                          buildVarRefExp(GWName, scope),
                          buildVarRefExp("PR_htId", global_scope)),
                       fdef);

                SgExprStatement *GWAssignStmt =
                    buildAssignStatement(
                       buildVarRefExp(GWName, scope),
                       rhs);
                
                // These are inserted in reverse order so write_addr is first
                // in the resulting sequential output.
                insertStatementAfterLastDeclaration(GWAssignStmt, fdef);
                insertStatementAfterLastDeclaration(GWWriteAddr, fdef);
            }


            if (isSgArrayType(symtype)) {
                if (isParameter) {
                    symtype = SCDecls->get_MemAddr_t_type();
                } else {
                    addr2W = calculateGVArrayWidth(isSgArrayType(symtype));
                    addr2VarName = uplevelGVName + "_addr2";
                    
                    declareVariableInFunction(fdef, 
                                              addr2VarName, 
                                              SCDecls->get_sc_type(addr2W), 
                                              true);
                    // symtype = getArrayElementType(symtype);
                    while (isSgArrayType(symtype)) {
                        symtype = getElementType(symtype);
                    }
                }
            }

            if (isPointerType(symtype)) {
                symtype = SCDecls->get_MemAddr_t_type();
            }

            std::string tyname = symtype->unparseToString();

            bool writesSym = hasUplevelWrite(sym) ||
                (isParameter && (level == 0));
            bool readsSym = hasUplevelRead(sym);

            std::string indexName = getUplevelIndexName(level);

            emitAddGlobal(fdef,
                          uplevelGVName,
                          indexName,
                          width,
                          level > 0,
                          addr2W,
                          addr2VarName,
                          "// Global Variable OMP shared variable \n",
                          uplevelGVName + "_" + symname,
                          tyname,
                          readsSym,
                          writesSym);

            if (writesSym) {
                uplevelWriteCounts[uplevelGVName]++;

                //                std::cerr << "symbol " << sym->get_name().getString() << " " << sym << " has " << uplevelWriteCounts[uplevelGVName] << " writes " << " level is " << level << std::endl;
            }

            if (level > maxlevel) {
                maxlevel = level;
            }
        }

        // Declare the htd typedef for index variables accessing this GV
        SgFunctionDeclaration *fdecl = fdef->get_declaration();
        std::string modulename = fdecl->get_name().getString();
        std::string htdType = modulename + "_uplevel_index_t";
        
        HtdInfoAttribute *htd = getHtdInfoAttribute(fdef);
        assert(htd && "missing HtdInfoAttribute");
       
#if 1
        size_t pos = 0;
        size_t np = std::string::npos;
        if ((pos = modulename.find(StencilStreamPrefix)) != np && pos == 0) {
          // The streaming version of a stencil has width 0, which isn't
          // acceptable (sc_uint<0>).  But this typedef isn't needed for
          // a streaming stencil kernel in the first place.
        } else
#endif 
        htd->appendTypedef("sc_uint<" + Upper(modulename) + "_HTID_W>",
                           htdType);

        if (getUplevelFunction(fdef)) {
            std::string pid("P___htc_parent_htid");
        
            declareVariableInFunction(fdef,
                                      pid,
                                      buildIntType(),
                                      false);
        }
        
        if (contains_parallel_region) {
            maxlevel = maxlevel < 0 ? 0 : maxlevel;
        }

        // Declare GV's for the uplevel chain
        for (int level = 0; level < maxlevel; level++) {

            // if this function does not contain any parallel regions,
            // we do not need to allocate the uplevel_index GV for it
            if ((level == 0) && 
                !(contains_parallel_region && getUplevelFunction(fdef))) {

#if 0
                std::cerr << "skipping level 0 GV for function " <<
                    fdef->get_declaration()->get_name().getString() <<
                    " contains par reg is " <<
                    (contains_parallel_region ? "true" : "false") <<
                    std::endl;
#endif
                continue;
            }
#if 0
            std::cerr << "level is " << level << " for function " <<
                    fdef->get_declaration()->get_name().getString() <<
                    " contains par reg is " <<
                    (contains_parallel_region ? "true" : "false") <<
                    std::endl;
#endif

            SgFunctionDeclaration *fdecl = getUplevelFunction(fdef, level);
            std::string modulename = fdecl->get_name().getString();
            std::string width = Upper(modulename) + "_HTID_W";
            std::string uplevelGVName = 
                getUplevelGVName(fdecl, "uplevel_index");

            std::string indexName = getUplevelIndexName(level);

            emitAddGlobal(fdef,
                          uplevelGVName,
                          indexName,
                          width,
                          level > 0,
                          -1, /* addr2W, */
                          "", /* addr2VarName */
                          "// Global Variable for OMP uplevel_index \n",
                          uplevelGVName + "_" + "uplevel_index",
                          "int",
                          level > 0,   // read
                          level == 0); // write

            if (level == 0) {
                assignUplevelValue(fdef, uplevelGVName);
            }
        }

        return maxlevel;

    }
    

    void replaceUplevelRefs( SgFunctionDefinition *fdef, bool doReads) {
        
        SgScopeStatement *scope = fdef->get_body();

        for (unsigned int i = 0; i < uplevelRefs.size(); i++) {

            UplevelRef *ref = uplevelRefs.at(i);
            SgVariableSymbol *sym = ref->getSym();
            int level = ref->getLevel();
            SgExpression *expr = ref->getRefExp();
            SgExpression *addr2 = ref->getAddr2();


            SgFunctionDeclaration *fdecl = 
                getUplevelFunction(fdef, level);
            std::string symname = sym->get_name().getString();
            std::string uplevelGVName = 
                getUplevelGVName(fdecl, symname);

            SgStatement *stmt = getEnclosingStatement(expr);
        
            SgVarRefExp *base = findBaseVarRefForExpression(expr);
            if (!base) {
                // SgThisExp
                continue;
            }
            SgType* baseType = base->get_type();

            SgNode *baseParent = base->get_parent();
            SgPntrArrRefExp *top = NULL;

            if (isSgPntrArrRefExp(baseParent)) {
                SgPntrArrRefExp *PAR = isSgPntrArrRefExp(baseParent);
                top = findTopSgPntrArrRefExp(PAR);
                baseType = top->get_type();
            }

            if (ref->isAssignment() && !doReads) {

                SgExpression *lhs = 0;
                SgExpression *rhs = 0;
                bool readlhs = false;
                bool isAsg = false;

                isAsg = SageInterface::isAssignmentStatement( 
                                    stmt, &lhs, &rhs, &readlhs);
                assert(isAsg);

                std::string GWName("GW_" + uplevelGVName + "_" + symname);
                std::string index_name = getUplevelIndexName(level);

                declareVariableInFunction(fdef,
                                          GWName,
                                          baseType,
                                          false);

                index_name = "P_" + index_name;

                SgExprStatement* GWWriteAddr;
                if (addr2) {
                    GWWriteAddr= 
                        buildFunctionCallStmt(
                           "__htc_GW_write_addr2",
                           buildVoidType(),
                           buildExprListExp(
                              buildVarRefExp(GWName, scope),
                              buildVarRefExp(index_name, scope),
                              addr2),
                           fdef);
                } else {
                    GWWriteAddr= 
                        buildFunctionCallStmt(
                           "__htc_GW_write_addr",
                           buildVoidType(),
                           buildExprListExp(
                              buildVarRefExp(GWName, scope),
                              buildVarRefExp(index_name, scope)),
                           fdef);
                }
                // Check if baseType is a struct/class.   Those need 
                // read/modify/write.   Others can just do a write.
                if (isSgClassType(baseType->stripTypedefsAndModifiers())) {

                    std::string tempName("__htc_tempforGVWrite");
                    
                    SgVariableDeclaration *tempVar = 
                        buildVariableDeclaration(tempName,
                                                 baseType,
                                                 0,
                                                 scope);
                    insertStatementBefore(stmt, tempVar);
                    
                    // Read old value
                    std::string GRName("GR_" + uplevelGVName + "_uplevel_index");
                    declareVariableInFunction(fdef,
                                              GRName,
                                              baseType,
                                              false);

                    SgExprStatement *assignTemp =
                        buildAssignStatement(
                            buildVarRefExp(tempName, scope),
                            buildVarRefExp(GRName, scope));
                    insertStatementBefore(stmt, assignTemp);
                    
                    if (addr2) {
                        SageInterface::replaceExpression(
                                          top, 
                                          buildVarRefExp(tempName,
                                                         scope));

                    } else {
                        SageInterface::replaceExpression(
                                          base, 
                                          buildVarRefExp(tempName,
                                                         scope));
                    }

                    SgExprStatement* GWAssignStmt = 
                        buildAssignStatement(
                           buildVarRefExp(GWName, scope),
                           buildVarRefExp(tempName, scope));

                    replaceStatement(stmt, GWWriteAddr, true);
                    SageInterface::insertStatementAfter(GWWriteAddr,
                                                        GWAssignStmt);
                    
#if BREAKSTATES
                    SgLabelStatement *nextLabel = 
                        HtSageUtils::makeLabelStatement("GW", fdef);
                    SgGotoStatement *gts = buildGotoStatement(nextLabel);
                    insertStatement(GWAssignStmt, gts, false /* after */);
                    insertStatement(gts, nextLabel, false /* after */);
#endif
                    
                } else {  // simple write

                    SgExprStatement* GWAssignStmt = 
                        buildAssignStatement(
                           buildVarRefExp(GWName, scope),
                           rhs);
                    replaceStatement(stmt, GWWriteAddr, true);
                    SageInterface::insertStatementAfter(GWWriteAddr,
                                                        GWAssignStmt);

#if BREAKSTATES
                    SgLabelStatement *nextLabel = 
                        HtSageUtils::makeLabelStatement("GW", fdef);
                    SgGotoStatement *gts = buildGotoStatement(nextLabel);
                    insertStatement(GWAssignStmt, gts, false /* after */);
                    insertStatement(gts, nextLabel, false /* after */);
#endif
                    
                }
            }

            if (!ref->isAssignment() && doReads) {
                // READS
                
                // index variables for uplevels set already set
                // only need to replace reference with read of the GV
                // unless the variable is an array.   Then we need
                // to set the addr2 index.

                std::string GRName("GR_" + uplevelGVName + "_" + symname);

                declareVariableInFunction(fdef,
                                          GRName,
                                          baseType,
                                          false);
                SgVarRefExp*GRRef = buildVarRefExp(GRName, scope);

                if (addr2 == NULL) {
                    replaceExpression(base, GRRef, true);
                } else {
                    stmt = HtSageUtils::findSafeInsertPoint(stmt);

                    // If addr2, need to set it and use a temporary for the
                    // read result
                    std::string arrayName = sym->unparseToString();
                    std::string tempVarName = tempNameFor(arrayName);

                    // Use a register temp for the value because we may have
                    // multiple refs in one expression
            
                    // Declare temporary variable
                    declareVariableInFunction(fdef, tempVarName, baseType);
                    tempVarName = "P_" + tempVarName;

                    // replace array reference with tempvar in expression
                    SgVarRefExp *refTempVar = 
                        buildVarRefExp(tempVarName, scope);

                    replaceExpression(top,
                                      refTempVar, true);

                    // Declare index variable
                    int width = 
                        calculateGVArrayWidth(isSgArrayType(sym->get_type()));
                    std::string indexVarName = uplevelGVName + "_addr2";
                    std::string prefixedIndexVarName = "P_" + indexVarName;

                    SgExprStatement *AssignIndexVar = 
                        buildAssignStatement(buildVarRefExp(
                                                prefixedIndexVarName, scope),
                                             addr2);

                    insertStatement (stmt, AssignIndexVar, true /* before */);

                
#if BREAKSTATES
                    SgLabelStatement *nextLabel = HtSageUtils::makeLabelStatement("GR", fdef);
                    SgGotoStatement *gts = buildGotoStatement(nextLabel);
                    insertStatement(stmt, gts, true /* before */);
                    insertStatement(gts, nextLabel, false /* after */);
#endif
                    SgExprStatement* assignToTemp = 
                        buildAssignStatement(
                           buildVarRefExp(tempVarName, scope),
                                         GRRef);

                    insertStatement (stmt, assignToTemp, true /* before */);
                }
            }
        }
    } // replaceUplevelRefs


    // "Driver" function to transform all of the OpenMP Uplevel Refs
    void ProcessUplevelRefs( SgFunctionDefinition *fdef) {
        int maxlevel;

        maxlevel = DeclareUplevelGlobalVariables(fdef);

        // return if no uplevel variables in this function
        if (maxlevel == -1) return;

        // Declare the uplevel index variables
        declareUplevelIndexVariables(fdef, maxlevel);

        // Assign values to the uplevel index variables
        assignUplevelIndexVariables(fdef, maxlevel);

    }
    
    //------------------- END OF UPLEVEL -----------------------------

    bool SgVariableDeclarationIsInList(SgVariableDeclaration *VD,
                                       std::vector<SgVariableDeclaration *> *list) {
        unsigned int i;
        for (i = 0; i < list->size(); i++) {
            if (list->at(i) == VD) {
                return true;
            }
        }
        return false;
    }

    bool SgInitializedNameIsInList(SgInitializedName *iname,
                                   std::vector<SgInitializedName *> *list) {
        unsigned int i;
        for (i = 0; i < list->size(); i++) {
            if (list->at(i) == iname) {
                return true;
            }
        }
        return false;
    }
    
    
    void ProcessLocalDecls(SgFunctionDefinition *fndef) {
        unsigned int i;

        HtdInfoAttribute *htd = getHtdInfoAttribute(fndef);

        assert(htd && "missing HtdInfoAttribute");

        SgInitializedName *scalar;
        std::set<SgInitializedName *> localDefSet;
        for (i = 0; i < local_scalars.size(); i++) {
            scalar = local_scalars.at(i);
            if (localDefSet.find(scalar) == localDefSet.end()) {
                localDefSet.insert(scalar);
                
                SgType* tp = scalar->get_type();
                if (isPointerType(tp) || isSgArrayType(tp)) {
                    tp = SCDecls->get_MemAddr_t_type();
                }
                std::string type = tp->stripType(SgType::STRIP_MODIFIER_TYPE)->unparseToString();
                std::string name = scalar->get_name().getString();
                
                htd->appendPrivateVar("", type, name);
                prefixSymInSymbolTable(scalar, "P_");
                
            }
        }


        SgInitializedName *parm;
        std::set<SgInitializedName *> parmDefSet;
        for (i = 0; i < parameters.size(); i++) {
            parm = parameters.at(i);
            if (parmDefSet.find(parm) == parmDefSet.end()) {
                parmDefSet.insert(parm);

                SgType* tp = parm->get_type()->stripType(SgType::STRIP_MODIFIER_TYPE);
                
                std::string type;
                if (isSgArrayType(tp) || 
                    isSgPointerType(tp) ||
                    isSgReferenceType(tp)) {
                    type = "MemAddr_t";
                } else {
                    type = tp->unparseToString();
                }
                std::string name = 
                    parm->get_name().getString();
                
                htd->appendPrivateVar("", type, name);
                prefixSymInSymbolTable(parm, "P_");
            }
        }
            
        std::set<SgInitializedName *>::iterator iter;
        for (iter = global_scalars.begin(); 
             iter != global_scalars.end(); iter++) {
            SgInitializedName *vdecl = *iter;
            std::string name = vdecl->get_name().getString();

            if (HtSageUtils::isHtlReservedIdent(name)) {
                // Do not attempt to redeclare Htl identifiers.  We should
                // probably filter these as global_scalars is being created.
                // For now, we do it here.
                continue;
            }
            
            SgType* tp = vdecl->get_type();
            std::string type;
            
            if (isSgArrayType(tp) || 
                isSgPointerType(tp) ||
                isSgReferenceType(tp)) {
                type = "MemAddr_t";
            } else {
                type = tp->unparseToString();
            }
                
            htd->appendSharedVar(type, name);
        }
        
        return;
    }

    SgExpression *calculateGVArrayIndex(SgExpression *top,
                                        SgArrayType *at) {
    
        std::vector<SgExpression *> *subscripts = 
            new std::vector<SgExpression *>;
        SgExpression *name;
        bool isArray = isArrayReference(top, &name, &subscripts);

        assert(isArray);
                
        const std::vector<SgExpression *> &dimsizes = 
            get_C_array_dimensions(at); 
        
        int size = atoi(dimsizes.at(1)->unparseToString().c_str()) - 1;
        unsigned int globalVarSize = FindLg2((uint64_t)size);
        SgExpression *globalVarIndex = buildIntVal(0);
        
        for (unsigned int sub = 0; sub < subscripts->size()-1;  sub++) {
            SgExpression *dimVal = deepCopy(dimsizes.at(sub+2));
            size = atoi(dimVal->unparseToString().c_str()) - 1;
            unsigned int lgSize = FindLg2((uint64_t)size);
            
            globalVarSize += lgSize;
            
            SgExpression *subVal = deepCopy(subscripts->at(sub));
            
            globalVarIndex = buildBitOrOp(globalVarIndex, subVal);
            globalVarIndex = buildLshiftOp(globalVarIndex, buildIntVal(lgSize));
            
        }
        globalVarIndex = buildBitOrOp(globalVarIndex, deepCopy(subscripts->at(subscripts->size()-1)));

        return globalVarIndex;
    }


    int calculateGVArrayWidth(SgArrayType *at) {
    
        const std::vector<SgExpression *> &dimsizes = 
            get_C_array_dimensions(at); 
        
        unsigned int globalVarSize = 0;
        
        for (unsigned int i = 1; i < dimsizes.size();  i++) {
            SgExpression *dimVal = dimsizes.at(i);

            int size = atoi(dimVal->unparseToString().c_str()) - 1;
            unsigned int lgSize = FindLg2((uint64_t)size);
            
            globalVarSize += lgSize;
        }

        return globalVarSize;
    }



    SgExpression *calculateArrayIndex(SgType *tp,
                                      SgPntrArrRefExp *top) {

#if DEBUG
        std::cout << "Array type is " << tp->unparseToString() << std::endl;
        std::cout << "top is " << top->unparseToString() << std::endl;
#endif

        std::vector<SgExpression *> *subscripts = 
            new std::vector<SgExpression *>;
        SgExpression *name;


        bool isArray = isArrayReference(top, &name, &subscripts);

#if DEBUG
        if (isArray) {
            fprintf(stderr,"subscripts size is %d\n", 
                    subscripts->size());
            std::cout << "name is " << name->unparseToString() << "\n";
        } else {
            fprintf(stderr,"isArrayReference returned false\n");
        }
#endif


        SgType *baseType;

        if (isSgPointerType(tp)) {
            baseType = isSgPointerType(tp)->get_base_type();
        } else {
            baseType = getArrayElementType(tp);
        }

        const std::vector<SgExpression *> &dimsizes = 
            get_C_array_dimensions(tp);      

        SgExpression *offset = buildIntVal(0);

#if DEBUG
        fprintf(stderr,"dimsizes.size() is %d\n", dimsizes.size());
        for (int x = 0; x < dimsizes.size(); x++) {
            std::cout << "    ds[ " << x << "] is " << dimsizes.at(x)->unparseToString() << std::endl;
        }
#endif


        int sizediff = dimsizes.size() - subscripts->size();

        for (unsigned int sub = 0; sub < subscripts->size()-1;  sub++) {
            
            SgExpression *dimVal = deepCopy(dimsizes.at(sub+1+sizediff));
            SgExpression *subVal = deepCopy(subscripts->at(sub));
            
            offset = buildAddOp(offset, subVal);
            offset = buildMultiplyOp(offset, dimVal);
        }

        offset = buildAddOp(offset, 
                            deepCopy(subscripts->at(subscripts->size()-1)));

        size_t basesize = getSizeOf(baseType);

        if (ActualLg2(basesize) != ActualLg2(basesize-1)) {
            // even power of 2, use shift
            offset = buildLshiftOp(offset,
                                   buildUnsignedIntVal(ActualLg2(basesize)));
        } else {
            offset = buildMultiplyOp(offset, 
                                     buildUnsignedIntVal(basesize));
        }

        SgExpression *base = findBaseForArray(top);
        if (isSgDotExp(base)) {
            SgDotExp *dotExpr = isSgDotExp(base);
            offset = buildAddOp(flattenSgDotExp(dotExpr), offset);
        } else {
            assert(isSgVarRefExp(base));
            base = buildCastExp(base, SCDecls->get_MemAddr_t_type());
            offset = buildAddOp(base, offset);
        }

        return offset;
    }


    SgExpression *flattenSgDotExp(SgDotExp *exp) {

        SgExpression *lhs = exp->get_lhs_operand();
        SgExpression *rhs = exp->get_rhs_operand();

        SgType *rhstype = rhs->get_type();
        SgType *lhstype = lhs->get_type();

#if DEBUG
        std::string rhsscalar = isScalarType(rhstype) ? " scalar " : " aggregate ";
        std::string lhsscalar = isScalarType(lhstype) ? " scalar " : " aggregate ";


        std::cout << "lhs node is " << lhs->sage_class_name() << " " 
                  << lhs->unparseToString() << " type "         
                  << lhstype->unparseToString() << lhsscalar << std::endl;
        std::cout << "rhs node is " << rhs->sage_class_name() << " " 
                  << rhs->unparseToString() << " type " 
                  << rhstype->unparseToString() << rhsscalar << std::endl;
#endif

        SgVarRefExp *rhsVarRef = isSgVarRefExp(rhs);

        if (!rhsVarRef) {
            return buildIntVal(0);
        }

        assert(rhsVarRef);

        SgReferenceType *ref;
        ref = isSgReferenceType(lhstype->stripTypedefsAndModifiers());
        if (ref) {
            lhstype = ref->get_base_type();
        }

        int off = getFieldOffset(rhsVarRef->get_symbol()->get_declaration(),
                                 isSgClassType(lhstype->stripTypedefsAndModifiers()));


        SgExpression *offset = buildIntVal(off);
        SgExpression *base   = getBaseAddress(lhs);

        if (base) {
            return buildAddOp(base, offset);
        } else {
            return offset;
        }

    }


    SgExpression *calculatePointerIndex(SgPointerDerefExp *ptr) {

        SgExpression *index = deepCopy(ptr->get_operand());
        index = buildCastExp(index, SCDecls->get_MemAddr_t_type());

        return index;
    }



    // Base address for an aggregate or scalar
    SgExpression *getBaseAddress(SgExpression *expr) {
#if DEBUG
        std::cout << "getBaseAddress node " << expr->unparseToString() <<
            " is a " << expr->sage_class_name() << std::endl;
#endif

        if (isSgVarRefExp(expr)) {
            return buildCastExp(
                       buildAddressOfOp(deepCopy(expr)),
                       SCDecls->get_MemAddr_t_type());
        }
        if (isSgPntrArrRefExp(expr)) {
            SgPntrArrRefExp *AR = isSgPntrArrRefExp(expr);
            SgType *at = findBaseForArray(AR)->get_type();
            return calculateArrayIndex(at, AR);
        }
        if (isSgPointerDerefExp(expr)) {
            SgPointerDerefExp *PTR = isSgPointerDerefExp(expr);
            return calculatePointerIndex(PTR);
        }
        if (isSgDotExp(expr)) {
            SgDotExp *DR = isSgDotExp(expr);
            return flattenSgDotExp(DR);
        }
	assert(0 && "unexpected expr type in getBaseAddress");
    }



    void VisitPointerDerefExp(SgExpression *deref) {

        SgExpression *lhs = 0;
        SgExpression *rhs = 0;
        bool readlhs = false;
        bool isAsg = false;

        SgStatement *currentStatement = findEnclosingStatement(deref);
        isAsg = SageInterface::isAssignmentStatement( currentStatement, 
                                              &lhs, &rhs, &readlhs);

        bool write = isAsg && (deref == lhs);
        bool read = (isAsg && readlhs) || !write;

        SgExpression *addrVal;
        // global array transformed into mifRef
        addrVal = getBaseAddress(deref);

        if (SgAddressOfOp *addr_op = 
            isSgAddressOfOp(deref->get_parent())) {
            SageInterface::replaceExpression(addr_op, addrVal, false);
        } else {
            MifRef *MR = new MifRef(addrVal, 
                                    deref,
                                    write); /* assignment */
            addMifRef(currentStatement, MR);
        }
    }


    void  VisitAggregateExp( SgExpression *AE,
                             SgInitializedName *iname,
                             bool isGlobal,
                             SgFunctionDefinition *fndef) {

        std::vector<SgExpression *> *subscripts = 
            new std::vector<SgExpression *>;
        SgExpression *name;

        SgExpression *lhs = 0;
        SgExpression *rhs = 0;
        bool readlhs = false;
        bool isAsg = false;

        SgVarRefExp *baseofchain = findBaseVarRefForExpression(AE);
        if (!baseofchain) {
            // SgThisExp
            return;
        }
        SgType *tp = baseofchain->get_type();

#if DEBUG
        std::cout << "base of chain is " << baseofchain->unparseToString() 
                  << std::endl;
        std::cout << "base of chain type is " << tp->sage_class_name() 
                  << " " << tp->unparseToString() << std::endl;
#endif

        SgStatement *currentStatement = findEnclosingStatement(AE);
        isAsg = SageInterface::isAssignmentStatement( currentStatement, 
                                              &lhs, &rhs, &readlhs);

        bool write = isAsg && (AE == lhs);
        bool read = (isAsg && readlhs) || !write;
                
        if (read && write) {    
            // Should not happen.    These have been flattened.
            std::cerr << "Warning: cannot handle this expression: " <<
                AE->unparseToString() << endl;
            return;
        }
        
        if (isSgArrayType(tp) || isSgPointerType(tp)) {
            // base is an array
            SgExpression *top;

            SgNode *baseParent = baseofchain->get_parent();
            if (isSgPntrArrRefExp(baseParent)) {
                SgPntrArrRefExp *PAR = isSgPntrArrRefExp(baseParent);
                top = findTopSgPntrArrRefExp(PAR);
#if DEBUG
                std::cout << "PAR is " << PAR->unparseToString() << std::endl;
                std::cout << "lhs is " << PAR->get_lhs_operand()->unparseToString() << std::endl;
                
                std::cout << "top is " << top->unparseToString() << std::endl;
#endif
            } 

            SgType *baseType = AE->get_type();

            if (isGlobal || isSgPointerType(tp)) {  /* global array */
                SgExpression *addrVal;
                // global array transformed into mifRef
                addrVal = getBaseAddress(AE);

                if (SgAddressOfOp *addr_op = 
                    isSgAddressOfOp(AE->get_parent())) {
                    SageInterface::replaceExpression(addr_op, addrVal, false);
#if 1
                } else if (isSgArrayType(AE->get_type())) {
                           // ||   isSgPointerType(AE->get_type())) {
                    /* pointer or address of an array */
                    SageInterface::replaceExpression(AE, addrVal, false);
#endif
                } else {
                    MifRef *MR = new MifRef(addrVal, 
                                            AE,
                                            write); /* assignment */
                    addMifRef(currentStatement, MR);
                }

            } else {  /* local array */
                // local array transformed to globalVar
                
                // ((c_ts_htId) << log2(M) | (i))
                SgArrayType *at = isSgArrayType(tp);
                
                SgExpression *globalVarIndex = buildIntVal(0);
                
                bool isArray = isArrayReference(top, &name, &subscripts);
#if DEBUG
                if (isArray) {
                    fprintf(stderr,"subscripts size is %d\n", 
                            subscripts->size());
                    std::cout << "name is " << name->unparseToString() << "\n";
                } else {
                    fprintf(stderr,"isArrayReference returned false\n");
                }
#endif
                
                const std::vector<SgExpression *> &dimsizes = 
                    get_C_array_dimensions(at); 
                
                int size = atoi(dimsizes.at(1)->unparseToString().c_str()) - 1;
                unsigned int globalVarSize = FindLg2((uint64_t)size);
                
                for (unsigned int sub = 0; sub < subscripts->size()-1;  sub++) {
                    SgExpression *dimVal = deepCopy(dimsizes.at(sub+2));
                    size = atoi(dimVal->unparseToString().c_str()) - 1;
                    unsigned int lgSize = FindLg2((uint64_t)size);
                    
                    globalVarSize += lgSize;
                    
                    SgExpression *subVal = deepCopy(subscripts->at(sub));
                    
                    globalVarIndex = buildBitOrOp(globalVarIndex, subVal);
                    globalVarIndex = buildLshiftOp(globalVarIndex, buildIntVal(lgSize));
                    
                }
                globalVarIndex = buildBitOrOp(globalVarIndex, deepCopy(subscripts->at(subscripts->size()-1)));
                
                globalVarIndex = buildBitOrOp(
                                    buildLshiftOp(
                                       buildVarRefExp("PR_htId", global_scope),
                                       buildIntVal(globalVarSize)),
                                    globalVarIndex);

                
                if (SgAddressOfOp *addr_op = 
                    isSgAddressOfOp(top->get_parent())) {
                    // FIXME:   This does not work!    We need to add in
                    // the base address of the memory, but we do not have it!
                    SageInterface::replaceExpression(addr_op, 
                                                     globalVarIndex,
                                                     false);
                    fields.clear();
                    return;
                } 
#if 1
                if (isSgArrayType(AE->get_type())) {
                    // || isSgPointerType(AE->get_type())) {
                    /* pointer or address of an array */

                    // FIXME:   This does not work!    We need to add in
                    // the base address of the memory, but we do not have it!
                    SageInterface::replaceExpression(AE, globalVarIndex, false);
                    fields.clear();
                    return;
                }
#endif

#if DEBUG
                cerr << "\nGLOBALMEMINDEX " << globalVarIndex->unparseToString() << "\n" ;
                cerr << "\nglobalVarSize " << globalVarSize << "\n" ;
                cerr << "baseType of globalVar array is " <<
                    baseType->unparseToString() << "\n" ;
#endif
                    
                SgInitializedName *field = 0;

                // AE is the lowest level of the reference.   For array of
                // scalars, it is the array element.   For array of structs,
                // it is the field.   (We cannot access an entire struct.)
                if (isSgDotExp(AE)) {
                    SgDotExp *DE = isSgDotExp(AE);
                    if (!isScalarType(baseType)) {
                        // FINISH THIS!                    
                        std::cerr << "This expression is too complicated\n";
                    } else {
                        // reference to a struct field
                        SgVarRefExp *vr = isSgVarRefExp(DE->get_rhs_operand());
                        assert(vr);
                        field = vr->get_symbol()->get_declaration();
                    }
                } else {
                    // if no field to read into, create default data field
                    std::string dataName = iname->get_name().getString() +
                        "__hwt_data";
                    declareVariableInFunction(fndef, dataName, baseType, false);
                    SgVariableSymbol *var = 
                        isSgVariableSymbol(nameIsDeclaredInScope(dataName, fndef->get_body()));
                    assert(var);
                    field = var->get_declaration();

                }

                GlobalVarRef *ref = new GlobalVarRef(isSgPntrArrRefExp(top),
                                                     iname,
                                                     isLhsOfAssign(findTopAggregateExp(top)), /* assignment */
                                                     globalVarIndex,
                                                     globalVarSize,
                                                     field,
                                                     baseType);
                globalVarRefs.push_back(ref);
            }
        } else {  // start of expression is a struct
            
            bool doit = false;
#if 1
            // TURBO HACK: Force ProcessMemRefs to treat certain struct
            // references as a local variable.  For now, we will only do
            // this for the special CCoef structs created to support the
            // calls to streaming stencil modules.  The idea is to get
            // struct references such as 'foo.k[2][3]' to be left alone
            // and passed through as is to SystemC.
            std::vector<SgNode *> vnl =
                NodeQuery::querySubTree(AE, V_SgVarRefExp);
            foreach(SgNode *vn, vnl) {
              SgVarRefExp *vre = isSgVarRefExp(vn);
              std::string vname = vre->get_symbol()->get_name();
              size_t pos = 0;
              if ((pos = vname.find("__htc_coef")) != std::string::npos
                  && pos == 0) {
                doit = false;
              }
            }
#endif 
            if (isGlobal || doit) {
                SgExpression *addrVal = getBaseAddress(AE);

                if (SgAddressOfOp *addr_op = 
                    isSgAddressOfOp(AE->get_parent())) {
                    SageInterface::replaceExpression(addr_op, addrVal, false);
#if 1
                } else if (isSgArrayType(AE->get_type())) {
                    // || isSgPointerType(DE->get_type())) {
                    /* pointer or address of an array */
                    SageInterface::replaceExpression(AE, addrVal, false);
#endif
                } else {
                    MifRef *MR = new MifRef(addrVal, 
                                            AE, 
                                            write); /* assignment */
                    addMifRef(currentStatement, MR);
                }

            }
        }
        fields.clear();
    }


    SgExpression *findBaseForArray(SgPntrArrRefExp *expr) {
        SgExpression *node = expr;

#if DEBUG
        std::cerr << "findBaseForArray called with a " << 
            expr->sage_class_name() << " " << expr->unparseToString() << 
            " " << expr->get_type()->unparseToString() << std::endl;
#endif

        while (isSgPntrArrRefExp(node)) {
            node = isSgPntrArrRefExp(node)->get_lhs_operand();
        }

        return deepCopy(node);   /* FIXME need deepCopy??? */
    }

    SgVarRefExp *findBaseVarRefForExpression(SgExpression *expr) {
        SgExpression *node = expr;
        while (!isSgVarRefExp(node)) {
            if (SgPntrArrRefExp *ae = isSgPntrArrRefExp(node)) {
                node = ae->get_lhs_operand();
            } else if (SgDotExp *de = isSgDotExp(node)) {
                node = de->get_lhs_operand();
            } else if (SgPointerDerefExp *pd = isSgPointerDerefExp(node)) {
                node = pd->get_operand();
#if 1
                // ?????? SgCast ????
            } else if (SgCastExp *cast = isSgCastExp(node)) {
                node = cast->get_operand();
#endif
            } else if (SgArrowExp *arrow = isSgArrowExp(node)) {
                node = arrow->get_lhs_operand();
            } else if (isSgThisExp(node) || 
                       isSgFunctionCallExp(node) ||
                       isSgConstructorInitializer(node)) {
                return 0;
            } else {
                assert(0);
            }
        }
        return isSgVarRefExp(node);
    }

    void VisitSgExpression(SgExpression *expr, SgFunctionDefinition* fndef) {


#if  DEBUG
        std::cout << "expression " << expr ->unparseToString() << " is a " <<
            expr->sage_class_name() << " parent op " <<
            expr->get_parent()->sage_class_name() << " VISIT " <<std::endl;
        if (isSgPointerDerefExp(expr)) {
            std::cout << "kind of operand to PointerDerefExp is " <<
                isSgPointerDerefExp(expr)->get_operand()->sage_class_name() <<
                std::endl;
        }
#endif

        // ??? Move inside valDecl check ????
        SgPointerDerefExp *deref = isSgPointerDerefExp(expr);
        if (deref) {
            VisitPointerDerefExp(expr);
            return;
        }

        SgVarRefExp *base = findBaseVarRefForExpression(expr);

        if (!base) {
            // SgThisExp
            return;
        }
        SgType *tp = base->get_type();

        SgInitializedName *valDecl = base->get_symbol()->get_declaration();

        if (valDecl) {
            // Collect uplevel references
            SgVariableSymbol *sym = 
                isSgVariableSymbol(valDecl->get_symbol_from_symbol_table());
            if (sym) {
                OmpUplevelAttribute *attr = 
                    getOmpUplevelAttribute(sym);
                if (attr) {
                    SgExpression *lhs = 0;
                    SgExpression *rhs = 0;
                    bool readlhs = false;
                    bool isAsg = false;

                    bool isParameter = attr->isParameter();

                    SgStatement *currentStatement = 
                        findEnclosingStatement(expr);
                    isAsg =
                        SageInterface::isAssignmentStatement( currentStatement, 
                                                          &lhs, &rhs, &readlhs);

                    bool write = isAsg && (expr == lhs);
                    bool read = (isAsg && readlhs) || !write;

                    SgExpression *addr2 = NULL;
                    int addr2W = -1;
#if 0
                    std::cerr << "Expression " << expr->unparseToString() <<
                        " is uplevel " << attr->getUplevel() << " " << expr->sage_class_name() << " base type " << tp->sage_class_name() << " base is " << base->unparseToString() << std::endl;
#endif

                    if (isSgArrayType(tp)) {
                        if (!isParameter) {
                            SgExpression *top = 
                                findTopSgPntrArrRefExp(
                                    isSgPntrArrRefExp(base->get_parent()));

                            addr2 = calculateGVArrayIndex(top,
                                                          isSgArrayType(tp));
                            addr2W = calculateGVArrayWidth(isSgArrayType(tp));
                            addr2 = buildCastExp(addr2, 
                                                 SCDecls->get_sc_type(addr2W));
                        }
                    }

                    if (isSgArrayType(tp) && isParameter) {
                        // Array parameters are treated as globals
                        if ((attr->getUplevel() > 0) &&
                            // Only once per symbol per function
                            (array_parameter_address_assigns.find(sym) ==
                             array_parameter_address_assigns.end())) {

                            array_parameter_address_assigns.insert(sym);

                            SgVarRefExp *rhs = 
                                buildVarRefExp(sym->get_name().getString(),
                                               fndef);
                            std::string lhs_name = "P_";
                            lhs_name += sym->get_name().getString();

                            SgVarRefExp *lhs = 
                                buildVarRefExp(lhs_name,
                                               fndef);
                            
                            local_scalars.push_back(valDecl);

                            UplevelRef *ref = 
                                new UplevelRef(rhs, sym, 0, false);
                            uplevelRefs.push_back(ref);
                            
                            SgExprStatement *AssignArrayAddr =
                                buildAssignStatement(lhs, rhs);
                            insertStatementAfterLastDeclaration(
                                AssignArrayAddr, fndef);
                        }

                        VisitAggregateExp(expr, valDecl, true, fndef);
                    } else {
                        if (!isParameter || (attr->getUplevel() > 0)) {
                            UplevelRef *ref = new UplevelRef(expr,
                                                             sym,
                                                             addr2,
                                                             write);
                            
                            uplevelRefs.push_back(ref);
                        }
                    }
                    return;
                }
            }

            // Not uplevel, continue...

            bool global = isGlobalVariable(valDecl);
            bool parm = SgInitializedNameIsInList(valDecl, &parameters);
            bool scalar = isScalarType(valDecl->get_type());
            
            switch (expr->variantT()) {
            case V_SgPntrArrRefExp:
            case V_SgDotExp:
                VisitAggregateExp(expr, valDecl, global || parm, fndef);
                break;
            default:  /* scalar -- not aggregate */
                if (global) {
                    SgVarRefExp *VR = isSgVarRefExp(expr);
                    // declareScalarSharedMemVariable(VR, fndef);
        
                    global_scalars.insert(valDecl);
                } else {
                    if (!parm) {
                        local_scalars.push_back(valDecl);
                    }
                }
                break;
            }
        } else {
            std::cerr << "NO ValDecl\n";
        }
        
        fields.clear();

        return;
        
    }  /* VisitSgExpression */
    

//---------------------------

    bool isLhsOfAssign(SgNode *astNode) {
        SgNode *parent = astNode->get_parent();
        SgExpression * lhs;
        SgExpression * rhs;
        
        if (isAssignmentStatement(parent, &lhs, &rhs)) {
            return (astNode == lhs);
        } else {
            return false;
        }
    }
    

    void GatherParameterList(SgFunctionDeclaration *fndecl) {

        SgFunctionParameterList *functionParameters =
            fndecl->get_parameterList();
        SgInitializedNamePtrList &parameterList = 
            functionParameters->get_args();

        SgInitializedNamePtrList::iterator j;
        for (j = parameterList.begin(); j != parameterList.end(); j++) {
            SgInitializedName *iname = isSgInitializedName(*j);
            parameters.push_back(iname);
        }
    }

static SgFunctionDeclaration *getUplevelFunction (SgFunctionDefinition *fdef) {
    SgFunctionDeclaration *enclosing_function = 0;

    SgFunctionDeclaration *fdecl = fdef->get_declaration();

    OmpEnclosingFunctionAttribute *enclosing_attr =
        getOmpEnclosingFunctionAttribute(fdecl);

    if (enclosing_attr) {
        enclosing_function = enclosing_attr->getEnclosingFunction();
    }

    return enclosing_function;
}


static void dumpUplevel(SgFunctionDefinition *fdef)
{
    Rose_STL_Container< SgNode *> sgVariableDecls;

    sgVariableDecls = NodeQuery::querySubTree (fdef, V_SgVariableDeclaration);

    SgFunctionDeclaration *enclosing = getUplevelFunction(fdef);

    std::cerr << "dumpUplevel was called" << std::endl;

    std::cerr << "Function " <<
        fdef->get_declaration()->get_name().getString();

    if (enclosing) {
        std::cerr << " enclosing function " <<
            enclosing->get_name().getString();

    }

    std::cerr << std::endl;

    for (Rose_STL_Container<SgNode *>::iterator iter = sgVariableDecls.begin();
                 iter != sgVariableDecls.end();
                 iter++) {
        SgVariableSymbol *sym = 
            SageInterface::getFirstVarSym(isSgVariableDeclaration(*iter));

        std::cerr << "In function " <<
            fdef->get_declaration()->get_name().getString() << 
            " symbol " << 
            sym->get_name().getString() << " " << sym;

        OmpUplevelAttribute *attr = getOmpUplevelAttribute(sym);
        if (attr) {
            int level = attr->getUplevel();
            std::cerr << " at level " << level;
        }
        std::cerr << std::endl;
    }
}

    virtual void visit (SgNode* astNode) {

        Rose_STL_Container< SgNode * > sgVarDecls;
        Rose_STL_Container< SgNode * > sgExpressions;
        
        SgFunctionDefinition* fndef = isSgFunctionDefinition(astNode);


        if (fndef != NULL) {
            // dumpUplevel(fndef);

            functionName = fndef->get_declaration()->get_name().getString();

            sgVarDecls = NodeQuery::querySubTree (fndef, V_SgVariableDeclaration);            
            sgExpressions = NodeQuery::querySubTree (fndef, V_SgExpression);

            // Mark local variables to eliminate them from unparsed output
            for (Rose_STL_Container<SgNode*>::iterator iter = sgVarDecls.begin();
                 iter != sgVarDecls.end();
                 iter++) {
                SgVariableDeclaration *node = isSgVariableDeclaration(*iter);
                if (!debugHooks) {
                    node->setCompilerGenerated();
                    node->unsetOutputInCodeGeneration();
                }
            }
            
            for (Rose_STL_Container<SgNode*>::iterator iter = sgExpressions.begin();
                 iter != sgExpressions.end();
                 iter++) {
                SgExpression *node = isSgExpression(*iter);
                
                // It is not logical that  
                //    NodeQuery::querySubTree (fndef, V_SgExpression);
                // returns nodes that are not SgExpressions, but it seems
                // that it does when nodes are deleted from the AST.
                // Se we need to test for NULL nodes and skip them.
                if (!node) {
                    continue;
                }

                // Cannot handle member functions                
                if (isSgMemberFunctionType(node->get_type())) {
                    continue;
                }

                if (isSgDotExp(node) ||
                    isSgPntrArrRefExp(node) ||
                    isSgPointerDerefExp(node) || 
                    isSgVarRefExp(node)) {
                    SgNode *parent = node->get_parent();
                    if (! (isSgDotExp(parent) ||
                           (isSgPntrArrRefExp(parent)))) {
                        // top level tree node of expression
                        VisitSgExpression(node, fndef);
                    }
                }
            }

            FM.unallocateAllFields();

            // ProcessGlobalVarRefs must occur before ProcessMifRefs because
            // ProcessMifRefs does a DeepCopy of the rhs of assignments for 
            // MifWrites. The GlobalVarRefs must already be applied before 
            // the DeepCopy is done, or they will not affect the copied tree,
            // only the original.   Likewise for ProcessUplevelRefs.
            ProcessGlobalVarRefs(fndef);
            ProcessUplevelRefs(fndef);
            // All reads need to be handled first before any writes are done.
            replaceUplevelRefs(fndef, true);  // reads
            ProcessMifRefs();
            replaceUplevelRefs(fndef, false); // Writes
            ProcessLocalDecls(fndef);
            DeclareMifInterfaces(fndef);
            InsertWriteMemPauses();

            mifrefs.clear();
        }
    }

};  /* class TransformMemoryReferences */


void RenameAndHideGlobals(SgProject *project) {
    Rose_STL_Container< SgNode * > sgVarDecls;
    sgVarDecls = NodeQuery::querySubTree (project, V_SgVariableDeclaration);

    std::set<SgInitializedName *> globalVarSet;

    for (Rose_STL_Container<SgNode*>::iterator iter = sgVarDecls.begin();
         iter != sgVarDecls.end();
         iter++) {
        SgVariableDeclaration *varDecl = isSgVariableDeclaration(*iter);
        SgInitializedName *initName = varDecl->get_definition()->get_vardefn();
        if (initName->getFilenameString() == std::string("transformation" )) {
            continue;
        }
        if (isGlobalVariable(initName) && isScalarType(initName->get_type())) {
            globalVarSet.insert(initName);

            // Supress for final code generation
            if (!debugHooks) {
                varDecl->setCompilerGenerated();
                varDecl->unsetOutputInCodeGeneration();
            }
        }
    }

    std::set<SgInitializedName *>::iterator siter;
    for (siter = globalVarSet.begin(); siter != globalVarSet.end(); siter++) {
        SgInitializedName *initName = isSgInitializedName(*siter);
        prefixSymInSymbolTable(initName, "S_");
    }
}

//-----------------------------------------------------------------------

void setGlobalScope(SgProject *project)
{
  // find the global scope
  class VisitFunctionDefs : public AstSimpleProcessing {
  private:
      void visit(SgNode *S) {
          switch (S->variantT()) {
          case V_SgFunctionDefinition:
              global_scope = SageInterface::getGlobalScope(S);
              return;
          default:
              break;
          }
      }
  } vgs;
    vgs.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}


//-------------------------------------------------------------------------
// Convert SgSizeof to SgUnsignedIntVal
//-------------------------------------------------------------------------

class CollapseSizeOfOps : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgSizeOfOp(S)) {
        visitSgSizeOfOp(isSgSizeOfOp(S));
    }
  }

private: 
  void visitSgSizeOfOp(SgSizeOfOp *S);
};


// Visit each SgSizeOfOp in postorder, replacing it with the
// calculated SgUnsignedIntVal.
void CollapseSizeOfOps::visitSgSizeOfOp(SgSizeOfOp *S)
{
    SgType *ty = S->get_operand_type();

    if (!ty) {  // sizeof applied to expression
        ty = S->get_operand_expr()->get_type();
    }

    SgExpression *value = buildIntVal(getSizeOf(ty));

#if DEBUG
    std::cout << "collapse size of " << S->unparseToString() << 
        " operand type " << ty->unparseToString() << " size is " << 
        value->unparseToString() << std::endl;
#endif

    SageInterface::replaceExpression(S, value, false /* keepOldExp */);
}


void CollapseSgSizeOfOperators(SgProject *project)
{
  CollapseSizeOfOps cs;
  cs.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}



//-------------------------------------------------------------------------
// Convert casts to pointer type to cast to MemAddr_t
//-------------------------------------------------------------------------

class ConvertPointerCastsToMemAddr_t : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgCastExp(S)) {
        visitSgCastExp(isSgCastExp(S));
    }
  }

private: 
  void visitSgCastExp(SgCastExp *S);
};


// Visit each SgCastExp in postorder.    If the type is a pointer type,
// change it to MemAddr_t.
void ConvertPointerCastsToMemAddr_t::visitSgCastExp(SgCastExp *S)
{
    SgType *ty = S->get_type();

    if (isPointerType(ty)) {
        S->set_type(SCDecls->get_MemAddr_t_type());
    }
}



void ConvertPointerCasts(SgProject *project)
{
  ConvertPointerCastsToMemAddr_t cs;
  cs.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
}



//-------------------------------------------------------------------------
// Convert pointer difference (p - q)
//-------------------------------------------------------------------------
class ConvertPointerDiff : public AstPrePostProcessing {
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
    case V_SgSubtractOp:
        postVisitExpression(isSgExpression(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitExpression(SgExpression *AG);

  std::vector<SgExpression *> expressionList;
};


void ConvertPointerDiff::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  expressionList.clear();
}


// Iterate over the list of expressions, converting pointer arithmetic.
void ConvertPointerDiff::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  SgType *MemAddr_t = SCDecls->get_MemAddr_t_type();

  foreach (SgExpression *expr, expressionList) {

      SgBinaryOp *sub = isSgSubtractOp(expr);
      assert(sub);

      SgPointerType *pt = isSgPointerType(expr->get_type()->stripTypedefsAndModifiers());
      assert(pt);

      SgType *baseType = pt->get_base_type();

      SgExpression *lhs = sub->get_lhs_operand();
      SgExpression *rhs = sub->get_rhs_operand();

      SgExpression *lhsCopy = deepCopy(lhs);
      SgExpression *rhsCopy = deepCopy(rhs);

      SgExpression *newRhs, *newLhs;

      bool lhsIsPtr = isSgPointerType(lhs->get_type()->
                                      stripTypedefsAndModifiers());
      bool rhsIsPtr = isSgPointerType(rhs->get_type()->
                                      stripTypedefsAndModifiers());

      assert (lhsIsPtr && rhsIsPtr);

      newLhs = buildCastExp(lhsCopy, MemAddr_t);
      newRhs = buildCastExp(rhsCopy, MemAddr_t);

      SgExpression *newExpr = buildSubtractOp(newLhs, newRhs);
      newExpr = buildDivideOp(newExpr, 
                              buildUnsignedIntVal(getSizeOf(baseType)));

      SageInterface::replaceExpression(expr, newExpr, false /* keepOldExp */);
  }

  expressionList.clear();
}


// Visit each expression in post order, creating an ordered list
// of pointer difference expressions (p - q)
void ConvertPointerDiff::postVisitExpression(SgExpression *expr)
{
    SgType *type = expr->get_type();
    if (isPointerType(type)) {
        SgSubtractOp *sub = isSgSubtractOp(expr);
        if (sub && 
            isPointerType(sub->get_lhs_operand()->get_type()) &&
            isPointerType(sub->get_rhs_operand()->get_type())) {

            expressionList.push_back(expr);
        }
    }
}


//-------------------------------------------------------------------------
// Convert pointer math
//-------------------------------------------------------------------------
class ConvertPointerMath : public AstPrePostProcessing {
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
    case V_SgAddOp:
    case V_SgSubtractOp:
        postVisitExpression(isSgExpression(S));
      break;
    default:
      break;
    }
  }

private: 
  void preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD);
  void postVisitExpression(SgExpression *AG);

  std::vector<SgExpression *> expressionList;
};


void ConvertPointerMath::preVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }
  expressionList.clear();
}


// Iterate over the list of expressions, converting pointer arithmetic.
void ConvertPointerMath::postVisitSgFunctionDeclaration(SgFunctionDeclaration *FD)
{
  SgFunctionDefinition *fdef = FD->get_definition();
  if (!fdef) {
    return;
  }

  SgType *MemAddr_t = SCDecls->get_MemAddr_t_type();

  foreach (SgExpression *expr, expressionList) {

      SgExpression *newExpr = deepCopy(expr);
      SgBinaryOp *bin = isSgBinaryOp(newExpr);
      assert(bin);

      SgPointerType *pt = isSgPointerType(expr->get_type());
      if (!pt) {
          // This can happen when there is pointer math somewhere lower
          // in the tree.
          return;
      }

      SgType *baseType = pt->get_base_type();

      SgExpression *lhs = bin->get_lhs_operand();
      SgExpression *rhs = bin->get_rhs_operand();

      SgExpression *lhsCopy = deepCopy(lhs);
      SgExpression *rhsCopy = deepCopy(rhs);

      SgExpression *newRhs, *newLhs;

      bool lhsIsPtr = isSgPointerType(lhs->get_type());
      bool rhsIsPtr = isSgPointerType(rhs->get_type());

      size_t basesize = getSizeOf(baseType);

      bool useshift = ActualLg2(basesize) != ActualLg2(basesize-1);

      if (lhsIsPtr) {
          newLhs = buildCastExp(lhsCopy, MemAddr_t);
      } else {
          if (useshift) {
              newLhs = buildLshiftOp(lhsCopy,
                                     buildUnsignedIntVal(ActualLg2(basesize)));
          } else {
              newLhs = buildMultiplyOp(lhsCopy,
                                       buildUnsignedIntVal(basesize));
          }
      }

      if (rhsIsPtr) {
          newRhs = buildCastExp(rhsCopy, MemAddr_t);
      } else {
          if (useshift) {
              newRhs = buildLshiftOp(rhsCopy, 
                                     buildUnsignedIntVal(ActualLg2(basesize)));
          } else {
              newRhs = buildMultiplyOp(rhsCopy, 
                                       buildUnsignedIntVal(basesize));
          }
      }

      SageInterface::replaceExpression(lhs, newLhs, false /* keepOldExp */);
      SageInterface::replaceExpression(rhs, newRhs, false /* keepOldExp */);

      // newExpr = buildCastExp(newExpr, pt);

      SageInterface::replaceExpression(expr, newExpr, false /* keepOldExp */);

  }

  expressionList.clear();
}


// Visit each expression in post order, creating an ordered list
// of +/- ops with pointer type
void ConvertPointerMath::postVisitExpression(SgExpression *expr)
{
    SgType *type = expr->get_type();
    if (isPointerType(type)) {
        expressionList.push_back(expr);
    }
}


void ConvertPointerArithmetic(SgProject *project)
{
    ConvertPointerDiff pd;
    ConvertPointerMath pm;

    pd.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
    pm.traverseInputFiles(project, STRICT_INPUT_FILE_TRAVERSAL);
}

//-------------------------------------------------------------------------
// Declare structs and unions in HTD file 
// Returns name, which is original source name, or temp name if original 
// did not have a name.
//-------------------------------------------------------------------------
std::string declareStructOrUnionInHtd(SgType* t) {
    std::string name;

    switch (t->variantT()) {
    default:
        break;
    case V_SgClassType: { // Also covers structs and unions
        SgClassDeclaration* decl = 
            isSgClassDeclaration(isSgClassType(t)->get_declaration());
        ROSE_ASSERT (decl);
        decl = isSgClassDeclaration(decl->get_definingDeclaration());
        ROSE_ASSERT (decl);
        SgClassDefinition* def = decl->get_definition();
        ROSE_ASSERT (def);

        SgFunctionDefinition *fndef = getEnclosingFunctionDefinition(t);
        
#if 0
        const SgBaseClassPtrList& bases = def->get_inheritances();
        for (SgBaseClassPtrList::const_iterator i = bases.begin();
             i != bases.end(); ++i) {
            SgBaseClass* base = *i;
            SgClassDeclaration* basecls = base->get_base_class();
            layoutOneField(basecls->get_type(), base, false, currentOffset, layout);
        }
#endif
        const SgDeclarationStatementPtrList& body = def->get_members();
        bool isUnion = (decl->get_class_type() == SgClassDeclaration::e_union);
        
        std::string S;

        if (isUnion) {
            S = "AddUnion(";
        } else {
            S = "AddStruct(";
        }
        
        if (!decl->get_isUnNamed()) {
            name = decl->get_name().getString();
        } else {
            name = tempNameFor("__htc_unnamed_struct_union");
            decl->set_name(name);
        }
        S += "name=" + name + ", ";
        
        S += "scope=host)\n";

        if (isUnion) {
            name = "union " + name;
        } else {
            name = "struct " + name;
        }
        
        for (SgDeclarationStatementPtrList::const_iterator i = body.begin();
             i != body.end(); ++i) {
            SgDeclarationStatement* mem = *i;
            SgVariableDeclaration* vardecl = isSgVariableDeclaration(mem);
            SgClassDeclaration* classdecl = isSgClassDeclaration(mem);
            bool isUnnamedUnion = 
                classdecl ? classdecl->get_isUnNamed() : false;
            if (vardecl) {
                ROSE_ASSERT (!vardecl->get_bitfield());
                const SgInitializedNamePtrList& vars = 
                    isSgVariableDeclaration(mem)->get_variables();
                for (SgInitializedNamePtrList::const_iterator j = vars.begin();
                     j != vars.end(); ++j) {
                    SgInitializedName* var = *j;
                    // SgType *field_type = 
                    //     var->get_type()->stripTypedefsAndModifiers();
                    SgType *field_type = 
                        var->get_type();
                    
                    S += "   .AddField(name=";
                    if (!isUnnamedUnion) {
                        S += var->get_name().getString();
                    }
                    S += ", ";

                    S += "type=";
                    if (isSgArrayType(field_type)) {
                        S+= getArrayElementType(field_type)->unparseToString();
                        S+= ", dimen1=";
                        S+= to_string<int>(getArrayElementCount(isSgArrayType(field_type)));
                    } else if (isSgPointerType(field_type) || 
                               isSgReferenceType(field_type)) {
                        S+= "MemAddr_t";
                    } else {
                        S+= field_type->unparseToString() + "";
                    }
                    
                    S += ")\n";
                    
                    // layoutOneField(var->get_type(), var, isUnion, currentOffset, layout);
                }
            } 
        }

        S += "   ;\n";

        if (fndef) {
            std::cout << "MODULE ";
        } else {
            std::cout << "GLOBAL ";
        }
        
        std::cout << "Add this to HTD file\n" << S;
        
    }
    }
    return name;
}

//-------------------------------------------------------------------------
// Declare structs and unions in HTD file
//
//-------------------------------------------------------------------------

class DeclareStructsAndUnions : public AstSimpleProcessing {
public:
    void visit(SgNode *S) {

        if (isSgClassType(S)) {
            std::cerr << "have a SgClassType" << std::endl;
            addStructsForClass(isSgClassType(S));
        }

        if (isSgTypedefDeclaration(S)) {
            SgTypedefDeclaration *TD = isSgTypedefDeclaration(S);

            if (!TD->get_file_info()->isTransformation()) {
                typedef_list.insert(TD);
            }

            SgType *base = TD->get_base_type();
            if (isSgClassType(base)) {
                addStructsForClass(isSgClassType(base));
            }
        }
    }

    void addStructsForClass(SgClassType *T) {
        SgClassDeclaration* decl = isSgClassDeclaration(T->get_declaration());
        decl = isSgClassDeclaration(decl->get_definingDeclaration());
        SgClassDefinition* def = decl->get_definition();
        ROSE_ASSERT (def);
        
        const SgDeclarationStatementPtrList& body = def->get_members();
        
        for (SgDeclarationStatementPtrList::const_iterator i = body.begin();
             i != body.end(); ++i) {
            SgDeclarationStatement* mem = *i;
            SgVariableDeclaration* vardecl = isSgVariableDeclaration(mem);
            SgClassDeclaration* classdecl = isSgClassDeclaration(mem);

            if (vardecl) {
                const SgInitializedNamePtrList& vars = 
                    isSgVariableDeclaration(mem)->get_variables();
                for (SgInitializedNamePtrList::const_iterator j = vars.begin();
                     j != vars.end(); ++j) {
                    SgInitializedName* var = *j;
                    SgType *field_type = var->get_type();
                    SgClassType *classtype = isSgClassType(field_type);
                    if (classtype) {
                        addStructsForClass(classtype);
                    }
                }
            }
        }

        // name unnamed struct or union
        if (decl->get_isUnNamed()) {
            decl->set_name(tempNameFor("__htc_unnamed_struct_union"));
            decl->set_isUnNamed(false); // setting the name does not do this
        }

        // add it to the list (set) to emit
        class_list.insert(T);
    }

    void emit_structs_and_unions() {
        std::set<SgClassType *>::iterator iter;

        std::cerr << "emit_structs_and_unions emiiting " <<
            class_list.size() << " declarations " << std::endl;

        for (iter = class_list.begin(); iter != class_list.end(); iter++) {
            std::string name = declareStructOrUnionInHtd(isSgClassType(*iter));
        }
    }

    void emit_typedefs() {
        std::set<SgTypedefDeclaration *>::iterator iter;
        SgFunctionDefinition *fndef;
        std::string S;

        for (iter = typedef_list.begin(); iter != typedef_list.end(); iter++) {
            fndef = getEnclosingFunctionDefinition(*iter);

            std::string name = (*iter)->get_name().getString();
            SgType *type = (*iter)->get_base_type();
            std::string type_name;      

            if ( isSgPointerType(type) || isSgReferenceType(type)) {
                type_name = "MemAddr_t";
            } else {
                type_name = (*iter)->get_base_type()->unparseToString();
            }
            if (fndef) {
                std::cout << "MODULE ";
            } else {
                std::cout << "GLOBAL ";
            }

            S = "AddTypedef(name=" + name + ", type=" + 
                type_name + ");\n";
            std::cout << "need to declare typedef " << S << std::endl;
        }
    }

private:
    std::set<SgClassType *> class_list;
    std::set<SgTypedefDeclaration *> typedef_list;
};


void DeclareStructsAndUnionsInHtd(SgProject *project)
{
  DeclareStructsAndUnions esu;

  esu.traverseInputFiles(project, postorder, STRICT_INPUT_FILE_TRAVERSAL);
  esu.emit_structs_and_unions();
  esu.emit_typedefs();
}


//-------------------------------------------------------------------------
// Vist functions one by one and perform memory ref translation
//-------------------------------------------------------------------------

class TransformFunctions : public AstSimpleProcessing {
public:
  void visit(SgNode *S) {
    if (isSgFunctionDefinition(S)) {
        visitFunctionDef(isSgFunctionDefinition(S));
    }
  }

private: 
    void visitFunctionDef(SgFunctionDefinition *fdef);
};




void TransformFunctions::visitFunctionDef(SgFunctionDefinition *fdef) {
    TransformMemoryReferences transformMemRefs;

#if DEBUG
    typeLayout(fdef);
#endif

    transformMemRefs.GatherParameterList(fdef->get_declaration());
    transformMemRefs.traverse(fdef, postorder);

}


void TransformFunctionDefinitions(SgProject *project)
{
    TransformFunctions tf;
    tf.traverseInputFiles(project, preorder, STRICT_INPUT_FILE_TRAVERSAL);
}


void ProcessMemRefs(SgProject *project) {

    uplevelWriteCounts.clear();

    setGlobalScope(project);
    if (!global_scope) {
        // fprintf(stderr,"No global_scope\n");
        return;
    }
    
    emitDesignInfoDeclarations(project);

    CollapseSgSizeOfOperators(project);

    TransformFunctionDefinitions(project);
    ConvertPointerArithmetic(project);
    ConvertPointerCasts(project);

#if 0
    DeclareStructsAndUnionsInHtd(project);
#endif

    RenameAndHideGlobals(project);
}
