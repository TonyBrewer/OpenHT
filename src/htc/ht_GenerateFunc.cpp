/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/**
 *  \file Transform/GenerateFunc.cc
 *
 *  \brief Generates an outlined (independent) C-callable function
 *  from an SgBasicBlock.
 *
 *  This outlining implementation specifically generates C-callable
 *  routines for use in an empirical tuning application. Such routines
 *  can be isolated into their own, dynamically shareable modules.
 */

// tps (01/14/2010) : Switching from rose.h to sage3.
#include "sage3basic.h"

// DQ (8/28/2012): Added this so that we can see where the macros 
// are used to control the use of new vs. old template support.
// #include <rose_config.h>

#include "sageBuilder.h"
#include <iostream>
#include <string>
#include <sstream>
#include <set>


#include "Outliner.hh"
#include "ASTtools.hh"
#include "VarSym.hh"
#include "Copy.hh"
#include "StmtRewrite.hh"
#include "Outliner.hh"
#include "ht_omp_lowering.h"
#include "HtcAttributes.h"
#include "HtSCDecls.h"

extern scDeclarations *SCDecls;

//! Stores a variable symbol remapping.
typedef std::map<const SgVariableSymbol *, SgVariableSymbol *> VarSymRemap_t;

// =====================================================================

using namespace std;
using namespace SageInterface;
using namespace SageBuilder;

/* ===========================================================
 */

//! Creates a non-member function.
static
SgFunctionDeclaration *
createFuncSkeleton (const string& name, SgType* ret_type,
                    SgFunctionParameterList* params, SgScopeStatement* scope)
{
    ROSE_ASSERT(scope != NULL);
    ROSE_ASSERT(isSgGlobal(scope)!=NULL);
    SgFunctionDeclaration* func;
    SgProcedureHeaderStatement* fortranRoutine;
    // Liao 12/13/2007, generate SgProcedureHeaderStatement for Fortran code
    if (SageInterface::is_Fortran_language()) {
#ifdef TEMPLATE_DECLARATIONS_DERIVED_FROM_NON_TEMPLATE_DECLARATIONS
        // DQ (8/28/2012): Generate a NULL pointer for now and fix that later.
        fortranRoutine = NULL;
        
        printf ("Need to use new SageBuilder API \n");
        ROSE_ASSERT(false);
#else
        fortranRoutine = 
            SageBuilder::buildProcedureHeaderStatement(
               name.c_str(),ret_type, params, 
               SgProcedureHeaderStatement::e_subroutine_subprogram_kind,scope);
#endif
        func = isSgFunctionDeclaration(fortranRoutine);  
    } else {
        func = SageBuilder::buildDefiningFunctionDeclaration(name,ret_type,
                                                             params,scope);
    }
    
    ROSE_ASSERT (func != NULL);
    
    SgFunctionSymbol* func_symbol = scope->lookup_function_symbol(func->get_name());
    ROSE_ASSERT(func_symbol != NULL);

    if (Outliner::enable_debug) {
        printf("Found function symbol in %p for function:%s\n", 
               scope, func->get_name().getString().c_str());
    }
    return func;
}

// ===========================================================

//! Creates an SgInitializedName.
static
SgInitializedName *
createInitName (const string& name, SgType* type,
                SgDeclarationStatement* decl,
                SgScopeStatement* scope,
                SgInitializer* init = 0)
{
    SgName sg_name (name.c_str ());
    
    ROSE_ASSERT(name.empty() == false);

    SgInitializedName* new_name = 
        new SgInitializedName (NULL, sg_name, type, init,decl, scope, 0);
    setOneSourcePositionForTransformation (new_name);
    ROSE_ASSERT (new_name);

    // Insert symbol
    if (scope) {
        SgVariableSymbol* new_sym = new SgVariableSymbol (new_name);
        scope->insert_symbol (sg_name, new_sym);
        ROSE_ASSERT (new_sym->get_parent() != NULL);
    }

    ROSE_ASSERT (new_name->get_endOfConstruct() != NULL);
    
    return new_name;
}


//! Returns 'true' if the base type is a primitive type.
static
bool
isBaseTypePrimitive (const SgType* type)
{
    if (!type) return false;
    const SgType* base_type = type->findBaseType ();
    if (base_type)
        switch (base_type->variantT ()) {
        case V_SgTypeBool:
        case V_SgTypeChar:
        case V_SgTypeDouble:
        case V_SgTypeFloat:
        case V_SgTypeInt:
        case V_SgTypeLong:
        case V_SgTypeLongDouble:
        case V_SgTypeLongLong:
        case V_SgTypeShort:
        case V_SgTypeSignedChar:
        case V_SgTypeSignedInt:
        case V_SgTypeSignedLong:
        case V_SgTypeSignedShort:
        case V_SgTypeUnsignedChar:
        case V_SgTypeUnsignedInt:
        case V_SgTypeUnsignedLong:
        case V_SgTypeUnsignedShort:
        case V_SgTypeVoid:
        case V_SgTypeWchar:
            return true;
        default:
            break;
        }
    return false;
}

//! Stores a new outlined-function parameter.
typedef std::pair<string, SgType *> OutlinedFuncParam_t;

/*!
 *  \brief Creates a new outlined-function parameter for a given
 *  variable. The requirement is to preserve data read/write semantics.
 *  
 *  This function is only used when wrapper parameter is not used
 *  so individual parameter needs to be created for each variable passed to 
 *  the outlined function.
 *
 *  For C/C++: we use pointer dereferencing to implement pass-by-reference
 *    So the parameter needs to be &a, which is a pointer type of a's base type
 *
 *    In a recent implementation, side effect analysis is used to find out
 *    variables which are not modified so pointer types are not used.
 *
 *  For Fortran, all parameters are passed by reference by default.
 *
 *  Given a variable (i.e., its type and name) whose references are to
 *  be outlined, create a suitable outlined-function parameter. 
 *  For C/C++, the  parameter is created as a pointer, to support parameter passing of
 *  aggregate types in C programs. 
 *  Moreover, the type is made 'void' if the base type is not a primitive type.
 *   
 *  An original type may need adjustments before we can make a pointer type from it.
 *  For example: 
 *    a)Array types from a function parameter: its first dimension is auto converted
 *  to a pointer type 
 *
 *    b) Pointer to a C++ reference type is illegal, we create a pointer to its
 *    base type in this case. It also match the semantics for addressof(refType) 
 *
 * 
 *  The implementation follows two steps:
 *     step 1: adjust a variable's base type
 *     step 2: decide on its function parameter type
 *  Liao, 8/14/2009
 */
static
OutlinedFuncParam_t
createParam (const SgInitializedName* i_name,  
             // the variable to be passed into the outlined function
             bool classic_original_type=false 
             // flag to decide if the variable's adjusted type is used directly, 
             // only applicable when -rose:outline:enable_classic  is turned on
             )
{
    ROSE_ASSERT (i_name);
    SgType* init_type = i_name->get_type();
    ROSE_ASSERT (init_type);
    
    // Store the adjusted original types into param_base_type
    // primitive types: --> original type
    // complex types: void
    // array types from function parameters:  pointer type for 1st dimension
    // C++ reference type: use base type since we want to have uniform way to 
    //   generate a pointer to the original type
    SgType* param_base_type = 0;
    if (isBaseTypePrimitive (init_type)||Outliner::enable_classic) {
        // for classic translation, there is no additional unpacking statement to 
        // convert void* type to non-primitive type of the parameter
        // So we don't convert the type to void* here

        // Duplicate the initial type.
        param_base_type = init_type; //!< \todo Is shallow copy here OK?
            
        // Adjust the original types for array or function types (TODO function types)
        // which are passed as function parameters 

        // convert the first dimension of an array type function parameter to a
        //  pointer type, 
        // This is called the auto type conversion for function or array typed 
        // variables that are passed as function parameters

        if (!SageInterface::is_Fortran_language() ) { // Only applies to C/C++
            if (isSgArrayType(param_base_type)) 
                if (isSgFunctionDefinition(i_name->get_scope()))
                    param_base_type = 
                        SageBuilder::buildPointerType(
                            isSgArrayType(param_base_type)->get_base_type());
        }
            
        // For C++ reference type, we use its base type since pointer to a reference 
        // type is not allowed
        SgReferenceType* ref = isSgReferenceType (param_base_type);
        if (ref != NULL) {
            param_base_type = ref->get_base_type();
        }
        
        ROSE_ASSERT (param_base_type);
    } else { 
        // for non-primitive types, we use void as its base type

        param_base_type = SgTypeVoid::createType ();
        ROSE_ASSERT (param_base_type);

        //Take advantage of the const modifier
        if (ASTtools::isConstObj (init_type)) {
            SgModifierType* mod = SageBuilder::buildConstType(param_base_type);
            param_base_type = mod;
        }
    }
    
    // Stores the real parameter type to be used in new_param_type
    string init_name = i_name->get_name ().str (); 

    // The parameter name reflects the type: the same name means the same type, 
    // p__ means a pointer type
    string new_param_name = init_name;
    SgType* new_param_type = NULL;
    
    // For classic behavior, read only variables are passed by values for C/C++
    // They share the same name and type
    if (Outliner::enable_classic) { 
        // read only parameter: pass-by-value, the same type and name
        if (classic_original_type ) {
            new_param_type = param_base_type;
        } else {
            new_param_name+= "p__";
            new_param_type = SgPointerType::createType (param_base_type);
        }
    } else {
        // The big assumption of this function is within the context of no wrapper 
        // parameter is used very conservative one, assume the worst side effects 
        // (all are written) 
        // TODO, why not use  classic_original_type to control this!!??
        ROSE_ASSERT (Outliner::useParameterWrapper == false && 
                     Outliner::useStructureWrapper == false);
        if (!SageInterface::is_Fortran_language()) {
            new_param_type = SgPointerType::createType (param_base_type);
            ROSE_ASSERT (new_param_type);
            new_param_name+= "p__";
        } else {
            // Fortran:
            // Liao 1/19/2010
            // We have to keep the parameter names the same as the original ones
            // Otherwise, we have to deep copy the SgArrayType used in the outlined 
            // portion and replace the name within dim info expression list.
            //
            // In an outlined function: 
            // e.g. INTEGER :: s_N
            //      DOUBLE PRECISION, DIMENSION(N) :: s_array 
            //            mismatch of N and s_N, the same SgArrayType is reused.
            new_param_name = new_param_name; //s_ means shared variables
        }
    }
    
    // Fortran parameters are passed by reference by default,
    // So use base type directly
    // C/C++ parameters will use their new param type to implement pass-by-reference
    if (SageInterface::is_Fortran_language()) {
        return OutlinedFuncParam_t (new_param_name, param_base_type);
    } else {
        return OutlinedFuncParam_t (new_param_name, new_param_type);
    }
}


/*!
 *  \brief Creates a local variable declaration to "unpack" an outlined-function's 
 *    parameter
 *  int index is optionally used as an offset inside a wrapper parameter for 
 *    multiple variables
 *
 *  The key is to set local_name, local_type, and local_val for all cases
 *
 *  There are three choices:
 *  Case 1: unpack one variable from one parameter
 *  -----------------------------------------------
 *  OUT_XXX(int *ip__)
 *  {
 *    // This is called unpacking declaration for a read-only variable, Liao, 9/11/2008
 *   int i = * (int *) ip__;
 *  }
 *
 *  Case 2: unpack one variable from an array of pointers
 *  -----------------------------------------------
 *  OUT_XXX (void * __out_argv[n]) // for written variables, we have to use pointers
 *  {
 *    int * _p_i =  (int*)__out_argv[0];
 *    int * _p_j =  (int*)__out_argv[1];
 *    ....
 *  }
 *
 *  Case 3: unpack one variable from a structure
 *  -----------------------------------------------
 *  OUT__xxx (struct OUT__xxx__data *__out_argv)
 *  {
 *    int i = __out_argv->i;
 *    int j = __out_argv->j;
 *    int (*sum)[100UL] = __out_argv->sum_p;*
 *  }
 *
 * case 1 and case 2 have two variants: 
 *   using conventional pointer dereferencing or 
 *   using cloned variables(temp_variable)
 *
 */
static
SgVariableDeclaration *
createUnpackDecl (SgInitializedName* param, // the function parameter
                  int index, // the index to the array of pointers type
                  bool isPointerDeref, // must use pointer deference or not
                  const SgInitializedName* i_name, 
                  // original variable to be passed as the function parameter
                  SgClassDeclaration* struct_decl, 
                  // the struct declaration type used to wrap parameters 
                  SgScopeStatement* scope
                  // the scope into which the statement will be inserted 
                  ) 
{
    ROSE_ASSERT(param && scope && i_name);
    
    // keep the original name 
    const string orig_var_name = i_name->get_name().str();
    
    //---------------step 1 -----------------------------------------------
    // decide on the type : local_type
    // the original data type of the variable passed via parameter
    SgType* orig_var_type = i_name ->get_type();
    
#if 0
    // Convey does not want this for HT.   Want to see the array type.
    if (! SageInterface::is_Fortran_language()) {  
        // Convert an array type parameter's first dimension to a pointer type
        // This conversion is implicit for C/C++ language. 
        // We have to make it explicit to get the right type
        // Liao, 4/24/2009  TODO we should only adjust this for the case 1
        if (isSgArrayType(orig_var_type)) {
            if (isSgFunctionDefinition(i_name->get_scope())) {
                orig_var_type = 
                    SageBuilder::buildPointerType(
                        isSgArrayType(orig_var_type)->get_base_type());
            }
        }
    }
#endif
    
    SgType* local_type = NULL;

    if (SageInterface::is_Fortran_language()) {
        local_type= orig_var_type;
    } else if (Outliner::temp_variable || Outliner::useStructureWrapper) {
        // unique processing for C/C++ if temp variables are used
        if (isPointerDeref) {// use pointer dereferencing for some
            local_type = buildPointerType(orig_var_type);
        } else { // use variable clone instead for others
                local_type = orig_var_type;
        }
    } else { // all other cases: non-fortran, not using variable clones 
        if (is_C_language()) {   
            // we use pointer types for all variables to be passed
            // the classic outlining will not use unpacking statement, but use 
            // the parameters directly.
            // So we can safely always use pointer dereferences here

            // Convey does this for HT
            local_type = buildPointerType(orig_var_type);

        } else {
            // C++ language
            // Rich's idea was to leverage C++'s reference type: two cases:
            //  a) for variables of reference type: no additional work
            //  b) for others: make a reference type to them
            //   all variable accesses in the outlined function will have
            //   access the address of the by default, not variable substitution is needed 
            local_type = isSgReferenceType(orig_var_type) ?
                orig_var_type: SgReferenceType::createType(orig_var_type);
        }
    }

    ROSE_ASSERT (local_type);
    
    SgAssignInitializer* local_val = NULL;
    
    // Declare a local variable to store the dereferenced argument.
    SgName local_name (orig_var_name.c_str ());
    if (SageInterface::is_Fortran_language())
        local_name = SgName(param->get_name());
    
    // This is the right hand of the assignment we want to build
    //
    // ----------step  2. Create the right hand expression-------------
    // No need to have right hand for fortran
    if (SageInterface::is_Fortran_language()) {
        local_val = NULL;
        return buildVariableDeclaration(local_name,local_type,local_val,scope);
    }

    // Convey added this for HT:
    // All we want is to declare the variable, just like Fortran code above
    local_val = NULL;
    if (Outliner::enable_debug) {
        std::cerr << "creating local variable " << local_name << " with type " 
                  << local_type->unparseToString() << std::endl;
    }
    return buildVariableDeclaration(local_name,local_type,local_val,scope);
    

    // for non-fortran language
    // Create an expression that "unpacks" the parameter.
    //   case 1: default is to use the parameter directly
    //   case 2:  for array of pointer type parameter,  build an array element 
    //            reference
    //   case 3: for structure type parameter, build a field reference
    //     we use type casting to have generic void * parameter and void* data 
    //     structure fields
    //     so less types are to exposed during translation
    //   class Hello **this__ptr__ = (class Hello **)(((struct OUT__1__1527___data *)__out_argv) -> this__ptr___p);
    SgExpression* param_ref = NULL;
    if (Outliner::useParameterWrapper){  
        // using index for a wrapper parameter
        if (Outliner::useStructureWrapper) { 
            // case 3: structure type parameter
            if (struct_decl != NULL) {
                SgClassDefinition* struct_def = 
                    isSgClassDeclaration(
                        struct_decl->get_definingDeclaration())->get_definition();
                ROSE_ASSERT (struct_def != NULL);
                string field_name = orig_var_name;
                if (isPointerDeref) {
                    field_name = field_name+"_p";
                }
                // __out_argv->i  or __out_argv->sum_p , depending on if pointer 
                // deference is needed

                //We use void* for all pointer types elements within the data 
                // structure. So type casting is needed here
                //e.g.   class Hello **this__ptr__ = (class Hello **)(((struct OUT__1__1527___data *)__out_argv) -> this__ptr___p);
                param_ref = 
                    buildCastExp(
                        buildArrowExp(
                            buildCastExp(
                                buildVarRefExp(param, scope), 
                                buildPointerType(struct_decl->get_type())), 
                            buildVarRefExp(field_name, struct_def)),
                        local_type);
                //local_type
            } else {
                cerr<<"Outliner::createUnpackDecl(): no need to unpack anything since struct_decl is NULL."<<endl;
                ROSE_ASSERT (false);
            }
        } else {
            // case 2: array of pointers 
            param_ref= buildPntrArrRefExp(buildVarRefExp(param, scope),buildIntVal(index));
        }
    } else {
        // default case 1:  each variable has a pointer typed parameter , 
        // this is not necessary but we have a classic model for optimizing this
        param_ref = buildVarRefExp(param, scope);
    }
    
    ROSE_ASSERT (param_ref != NULL);
    
    if (Outliner::useStructureWrapper) {
        // Or for structure type paramter
        // int (*sum)[100UL] = __out_argv->sum_p; // is PointerDeref type
        // int i = __out_argv->i;
        local_val = buildAssignInitializer(param_ref); 
    } else {
        //TODO: This is only needed for case 2 or C++ case 1, 
        // not for case 1 and case 3 since the source right hand already has the 
        // right type
        // since the array has generic void* elements. We need to cast from 
        // 'void *' to 'LOCAL_VAR_TYPE *'
        // Special handling for C++ reference type: 
        //    addressOf (refType) == addressOf(baseType) 
        // So unpacking it to baseType* 
        SgReferenceType* ref = isSgReferenceType (orig_var_type);
        SgType* local_var_type_ptr =  
            SgPointerType::createType (ref ? ref->get_base_type () : orig_var_type);
        ROSE_ASSERT (local_var_type_ptr);
        SgCastExp* cast_expr = 
            buildCastExp(param_ref,local_var_type_ptr,SgCastExp::e_C_style_cast);
            
        if (Outliner::temp_variable) {// variable cloning is enabled
            // int* ip = (int *)(__out_argv[1]); // isPointerDeref == true
            // int i = *(int *)(__out_argv[1]);
            if (isPointerDeref) {
                local_val = buildAssignInitializer(cast_expr); 
                // casting is enough for pointer types
            } else {
                // temp variable need additional dereferencing from the parameter 
                // on the right side
                local_val = buildAssignInitializer(buildPointerDerefExp(cast_expr));
            }
        } else { // conventional pointer dereferencing algorithm
            // int* ip = (int *)(__out_argv[1]);
            if  (is_C_language()) {// using pointer dereferences
                local_val = buildAssignInitializer(cast_expr);
            } else if  (is_Cxx_language()) {
                // We use reference type in the outlined function's body for C++ 
                // need the original value from a dereferenced type
                // using pointer dereferences to get the original type
                //  we use reference type instead of pointer type for C++
                /*
                  extern "C" void OUT__1__8452__(int *ip__,int *jp__,int (*sump__)[100UL])
                  {
                  int &i =  *((int *)ip__);
                  int &j =  *((int *)jp__);
                  int (&sum)[100UL] =  *((int (*)[100UL])sump__);
                  ...
                  } 
                */ 
                local_val = buildAssignInitializer(buildPointerDerefExp(cast_expr));
            } else {
                printf ("No other languages are supported by outlining currently. \n");
                ROSE_ASSERT(false);
            }
        }
    }

    // Convey added this for HT:
    local_val = 0;

    SgVariableDeclaration* decl = buildVariableDeclaration(local_name,local_type,local_val,scope);
    
    return decl;
}


//! Returns 'true' if the given type is 'const'.
static
bool
isReadOnlyType (const SgType* type)
{
    ROSE_ASSERT (type);
    
    const SgModifierType* mod = 0;
    switch (type->variantT ())
        {
        case V_SgModifierType:
            mod = isSgModifierType (type);
            break;
        case V_SgReferenceType:
            mod = isSgModifierType (isSgReferenceType (type)->get_base_type ());
            break;
        case V_SgPointerType:
            mod = isSgModifierType (isSgPointerType (type)->get_base_type ());
            break;
        default:
            mod = 0;
            break;
        }
    return mod
        && mod->get_typeModifier ().get_constVolatileModifier ().isConst ();
}

/*!
 *  \brief Creates an assignment to "pack" a local variable back into
 *  an outlined-function parameter that has been passed as a pointer
 *  value.
 *  Only applicable when variable cloning is turned on.
 *
 *  The concept of pack/unpack is associated with parameter wrapping
 *  In the outlined function, we first
 *    unpack the wrapper parameter to get individual parameters
 *  then
 *    pack individual parameter into the wrapper. 
 *  
 *  It is also write-back or transfer-back the values of clones to their original pointer variables
 *
 *  This routine takes the original "unpack" definition, of the form
 *
 *    TYPE local_unpack_var = *outlined_func_arg; // no parameter wrapping
 *    int i = *(int *)(__out_argv[1]); // parameter wrapping case
 *
 *  and creates the "re-pack" assignment expression,
 *
 *    *outlined_func_arg = local_unpack_var // no-parameter wrapping case
 *    *(int *)(__out_argv[1]) =i; // parameter wrapping case
 *
 *  C++ variables of reference types do not need this step.
 */
static
SgAssignOp *
createPackExpr (SgInitializedName* local_unpack_def)
{
    if (!Outliner::temp_variable)
        {
            if (is_C_language()) //skip for pointer dereferencing used in C language
                return NULL;
        }
    // reference types do not need copy the value back in any cases
    if (isSgReferenceType (local_unpack_def->get_type ()))  
        return NULL;
    
    // Liao 10/26/2009, Most of time, using data structure of parameters don't need copy 
    // a local variable's value back to its original parameter
    if (Outliner::useStructureWrapper)
        {
            if (is_Cxx_language())
                return NULL;
        }
    
    // We expect that the value transferring back to the original parameter is only 
    // needed for variable clone options and when the variable is being written. 
    if (local_unpack_def
        && !isReadOnlyType (local_unpack_def->get_type ()))
        //      && !isSgReferenceType (local_unpack_def->get_type ()))
        {
            SgName local_var_name (local_unpack_def->get_name ());
            
            SgAssignInitializer* local_var_init =
                isSgAssignInitializer (local_unpack_def->get_initializer ());
            ROSE_ASSERT (local_var_init);
            
            // Create the LHS, which derefs the function argument, by
            // copying the original dereference expression.
            // 
            SgPointerDerefExp* param_deref_unpack =
                isSgPointerDerefExp (local_var_init->get_operand_i ());
            if (param_deref_unpack == NULL)  
                {
                    cout<<"packing statement is:"<<local_unpack_def->get_declaration()->unparseToString()<<endl;
                    cout<<"local unpacking stmt's initializer's operand has non-pointer dereferencing type:"<<local_var_init->get_operand_i ()->class_name()<<endl;
                    ROSE_ASSERT (param_deref_unpack);
                }
            
            SgPointerDerefExp* param_deref_pack = isSgPointerDerefExp (ASTtools::deepCopy (param_deref_unpack));
            ROSE_ASSERT (param_deref_pack);
            
            // Create the RHS, which references the local variable.
            SgScopeStatement* scope = local_unpack_def->get_scope ();
            ROSE_ASSERT (scope);
            SgVariableSymbol* local_var_sym =
                scope->lookup_var_symbol (local_var_name);
            ROSE_ASSERT (local_var_sym);
            SgVarRefExp* local_var_ref = SageBuilder::buildVarRefExp (local_var_sym);
            ROSE_ASSERT (local_var_ref);
            
            // Assemble the final assignment expression.
            return SageBuilder::buildAssignOp (param_deref_pack, local_var_ref);
        }
    return 0;
}

/*!
 *  \brief Creates a pack (write-back) statement , used to support variable cloning in outlining.
 *
 *  
 *  This routine creates an SgExprStatement wrapper around the return
 *  of createPackExpr.
 *  
 *  void OUT__1__4305__(int *ip__,int *sump__)
 * {
 *   // variable clones for pointer types
 *   int i =  *((int *)ip__);
 *   int sum =  *((int *)sump__);
 *
 *  // clones participate computation
 *   for (i = 0; i < 100; i++) {
 *     sum += i;
 *   }
 *  // write back the values from clones to their original pointers 
 *  //The following are called (re)pack statements
 *    *((int *)sump__) = sum;
 *    *((int *)ip__) = i;
 }
 
*/
static
SgExprStatement *
createPackStmt (SgInitializedName* local_unpack_def)
{
    // No repacking for Fortran for now
    if (local_unpack_def==NULL || SageInterface::is_Fortran_language())
        return NULL;
    SgAssignOp* pack_expr = createPackExpr (local_unpack_def);
    if (pack_expr)
        return SageBuilder::buildExprStatement (pack_expr);
    else
        return 0;
}


/*!
 *  \brief Records a mapping between two variable symbols, and record
 *  the new symbol.
 *
 *  This routine creates the target variable symbol from the specified
 *  SgInitializedName object. If the optional scope is specified
 *  (i.e., is non-NULL), then this routine also inserts the new
 *  variable symbol into the scope's symbol table.
 */
static
void
recordSymRemap (const SgVariableSymbol* orig_sym,
                SgInitializedName* name_new,
                SgScopeStatement* scope,
                VarSymRemap_t& sym_remap)
{
    if (orig_sym && name_new)
        { //TODO use the existing symbol associated with name_new!
            // DQ (2/24/2009): Added assertion.
            ROSE_ASSERT(name_new->get_name().is_null() == false);
            
            SgVariableSymbol* sym_new = new SgVariableSymbol (name_new);
            ROSE_ASSERT (sym_new);
            sym_remap.insert (VarSymRemap_t::value_type (orig_sym, sym_new));
            
            if (scope)
                {
                    scope->insert_symbol (name_new->get_name (), sym_new);
                    name_new->set_scope (scope);
                }
        }
}

/*!
 *  \brief Records a mapping between variable symbols.
 *
 *  \pre The variable declaration must contain only 1 initialized
 *  name.
 */
static
void
recordSymRemap (const SgVariableSymbol* orig_sym,
                SgVariableDeclaration* new_decl,
                SgScopeStatement* scope,
                VarSymRemap_t& sym_remap)
{
    if (orig_sym && new_decl)
        {
            SgInitializedNamePtrList& vars = new_decl->get_variables ();
            ROSE_ASSERT (vars.size () == 1);
            for (SgInitializedNamePtrList::iterator i = vars.begin ();
                 i != vars.end (); ++i)
                recordSymRemap (orig_sym, *i, scope, sym_remap);
        }
}

// Create one parameter for an outlined function
// classic_original_type flag is used to decide the parameter type: 
//   A simplest case: readonly -> pass-by-value -> same type v.s. written -> pass-by-reference -> pointer type
// return the created parameter
SgInitializedName* createOneFunctionParameter(const SgInitializedName* i_name, 
                                              bool classic_original_type, // control if the original type should be used, instead of a pointer type, only used with enable_classic flag for now
                                              SgFunctionDeclaration* func)
{
    ROSE_ASSERT (i_name);
    
    ROSE_ASSERT (func);
    SgFunctionParameterList* params = func->get_parameterList ();
    ROSE_ASSERT (params);
    SgFunctionDefinition* def = func->get_definition ();
    ROSE_ASSERT (def);
    
    // It handles language-specific details internally, like pass-by-value, pass-by-reference
    // name and type is not enough, need the SgInitializedName also for tell 
    // if an array comes from a parameter list
    OutlinedFuncParam_t param = createParam (i_name,classic_original_type);
    SgName p_sg_name (param.first.c_str ());
    // name, type, declaration, scope, 
    // TODO function definition's declaration should not be passed to createInitName()
    SgInitializedName* p_init_name = createInitName (param.first, param.second, def->get_declaration(), def);
    ROSE_ASSERT (p_init_name);
    prependArg(params,p_init_name);
    return p_init_name;
}

// ===========================================================
//! Fixes up references in a block to point to alternative symbols.
// based on an existing symbol-to-symbol map
// Also called variable substitution or variable replacement
static void
remapVarSyms (const VarSymRemap_t& vsym_remap,  // regular shared variables
              const ASTtools::VarSymSet_t& pdSyms, // variables which must use pointer dereferencing somehow. //special shared variables using variable cloning (temp_variable)
              const VarSymRemap_t& private_remap,  // variables using private copies
              SgBasicBlock* b)
{
    // Check if variable remapping is even needed.
    if (vsym_remap.empty() && private_remap.empty())
        return;
    // Find all variable references
    typedef Rose_STL_Container<SgNode *> NodeList_t;
    NodeList_t refs = NodeQuery::querySubTree (b, V_SgVarRefExp);
    // For each of the references , 
    for (NodeList_t::iterator i = refs.begin (); i != refs.end (); ++i)
        {
            // Reference possibly in need of fix-up.
            SgVarRefExp* ref_orig = isSgVarRefExp (*i);
            ROSE_ASSERT (ref_orig);
            
            // Search for a symbol which need to be replaced.
            VarSymRemap_t::const_iterator ref_new =  vsym_remap.find (ref_orig->get_symbol ());
            VarSymRemap_t::const_iterator ref_private =  private_remap.find (ref_orig->get_symbol ());
            
            // a variable could be both a variable needing passing original value and private variable 
            // such as OpenMP firstprivate, lastprivate and reduction variable
            // For variable substitution, private remap has higher priority 
            // remapping private variables
            if (ref_private != private_remap.end()) 
                {
                    // get the replacement variable
                    SgVariableSymbol* sym_new = ref_private->second;
                    // Do the replacement
                    ref_orig->set_symbol (sym_new);
                }
            else if (ref_new != vsym_remap.end ()) // Needs replacement, regular shared variables
                {
                    SgVariableSymbol* sym_new = ref_new->second;
                    if (Outliner::temp_variable || Outliner::useStructureWrapper)
                        // uniform handling if temp variables of the same type are used
                        {// two cases: variable using temp vs. variables using pointer dereferencing!
                            
                            if (pdSyms.find(ref_orig->get_symbol())==pdSyms.end()) //using temp
                                ref_orig->set_symbol(sym_new);
                            else
                                {
                                    SgPointerDerefExp * deref_exp = SageBuilder::buildPointerDerefExp(buildVarRefExp(sym_new));
                                    deref_exp->set_need_paren(true);
                                    SageInterface::replaceExpression(isSgExpression(ref_orig),isSgExpression(deref_exp));
                                    
                                }
                        }
                    else // no variable cloning is used
                        {
                            if (is_C_language()) 
                                // old method of using pointer dereferencing indiscriminately for C input
                                // TODO compare the orig and new type, use pointer dereferencing only when necessary
                                {
                                    SgPointerDerefExp * deref_exp = SageBuilder::buildPointerDerefExp(buildVarRefExp(sym_new));
                                    deref_exp->set_need_paren(true);
                                    SageInterface::replaceExpression(isSgExpression(ref_orig),isSgExpression(deref_exp));
                                }
                            else
                                ref_orig->set_symbol (sym_new);
                        }
                } //find an entry
        } // for every refs
    
}


/*!
 *  \brief Creates new function parameters for a set of variable symbols.
 *
 *  We have several options for the organization of function parameters:
 *
 *  1. default: each variable to be passed has a function parameter
 *           To support both C and C++ programs, this routine assumes parameters passed
 *           using pointers (rather than the C++ -specific reference types).  
 *  2, useParameterWrapper: use an array as the function parameter, each
 *              pointer stores the address of the variable to be passed
 *  3. useStructureWrapper: use a structure, each field stores a variable's
 *              value or address according to use-by-address or not semantics
 *
 *  It inserts "unpacking/unwrapping" and "repacking" statements at the 
 *  beginning and end of the function body, respectively, when necessary.
 *
 *  This routine records the mapping between the given variable symbols and the new
 *  symbols corresponding to the new parameters. 
 *
 *  Finally, it performs variable replacement in the end.
 *
 */
static
void
variableHandling(const ASTtools::VarSymSet_t& syms, // all variables passed to the outlined function: //regular (shared) parameters?
              const ASTtools::VarSymSet_t& pdSyms, // those must use pointer dereference: use pass-by-reference
//              const std::set<SgInitializedName*> & readOnlyVars, // optional analysis: those which can use pass-by-value, used for classic outlining without parameter wrapping, and also for variable clone to decide on if write-back is needed
//              const std::set<SgInitializedName*> & liveOutVars, // optional analysis: used to control if a write-back is needed when variable cloning is used.
              const std::set<SgInitializedName*> & restoreVars, // variables to be restored after variable cloning
              SgClassDeclaration* struct_decl, // an optional struct wrapper for all variables
                 SgFunctionDeclaration* func, // the outlined function
                 bool add_htc_parameters)
{
    VarSymRemap_t sym_remap; 
    // variable remapping for regular(shared) variables: 
    // all passed by reference using pointer types?
    VarSymRemap_t private_remap; 
    // variable remapping for private/firstprivate/reduction variables
    ROSE_ASSERT (func);
    SgFunctionParameterList* params = func->get_parameterList ();
    ROSE_ASSERT (params);
    SgFunctionDefinition* def = func->get_definition ();
    ROSE_ASSERT (def);
    SgBasicBlock* body = def->get_body ();
    ROSE_ASSERT (body);
    
    // Place in which to put new outlined variable symbols.
    SgScopeStatement* args_scope = isSgScopeStatement (body);
    ROSE_ASSERT (args_scope);
    
    // For each variable symbol, create an equivalent function parameter. 
    // Also create unpacking and repacking statements.
    int counter=0;
    SgInitializedName* parameter1=NULL; // the wrapper parameter
    SgVariableDeclaration*  local_var_decl  =  NULL;
    
    // --------------------------------------------------
    // for each parameter passed to the outlined function
    // They include parameters for 
    // *  regular shared variables and also 
    // *  shared copies for firstprivate and reduction variables
    for (ASTtools::VarSymSet_t::const_reverse_iterator i = syms.rbegin ();
         i != syms.rend (); ++i) {
        // Basic information about the variable to be passed into the outlined function
        // Variable symbol name
        const SgInitializedName *i_name = (*i)->get_declaration ();
        ROSE_ASSERT (i_name);
        string name_str = i_name->get_name ().str ();
        SgName p_sg_name (name_str);
        const SgVariableSymbol *sym = isSgVariableSymbol(*i);
        
        OmpUplevelAttribute *attr = getOmpUplevelAttribute(sym);
        int level = -1;
        if (attr) {
            level = attr->getUplevel();
        }
        
#if 0
        std::cerr << "In variableHandling with symbol " <<
            sym->get_name().getString() << " " << sym <<
            " at level " <<
            level << std::endl;
#endif
        
        bool use_orig_type = false;
        
        if (pdSyms.find(sym) == pdSyms.end()) {
            // not a variable to use AddressOf, then it should be a variable using its 
            // original type
            use_orig_type = true;
        }
        
        // step 1. Create parameters and insert into the parameter list of the outlined
        //         function.
        // ----------------------------------------
        SgInitializedName* p_init_name = NULL;

        // Case 1: using a wrapper for all variables 
        //   two choices: array of pointers (default)  vs. structure 
        if (!Outliner::enable_classic && Outliner::useParameterWrapper) {
            
            // add wrapper parameter
            if (i == syms.rbegin()) {
                SgName var1_name = "__out_argv";
                // This is needed to support the pass-by-value semantics across 
                // different thread local stacks
                // In this situation, pointer dereferencing cannot be used to get the 
                // value  of an inactive parent thread's local variables
                SgType* ptype= NULL; 
                
                if (Outliner::useStructureWrapper) {
                    // To have strict type matching in C++ model
                    // between the outlined function and the function pointer passed 
                    // to the gomp runtime lib we use void* for the parameter type
                    ptype = buildPointerType (buildVoidType());
                }
                else {
                    // use array of pointers, regardless of the pass-by-value vs. 
                    // pass-by-reference difference
                    ptype= buildPointerType(buildPointerType(buildVoidType()));
                }
                
#if 1
            // We do not want to pass the parameter wrapper for HT
            // Convey did this.   We need parent_htid and my_omp_thread_num
            // and my_omp_team_size

                if (SCDecls == 0) {
                    SCDecls = new scDeclarations(SageInterface::getGlobalScope(def));
                }
                ptype= SCDecls->get_htc_tid_t_type();
                
                if ( add_htc_parameters) {
                    var1_name = "__htc_parent_htid";
                    parameter1 = buildInitializedName(var1_name,ptype);
                    appendArg(params,parameter1);
                
                    var1_name = "__htc_my_omp_thread_num";
                    parameter1 = buildInitializedName(var1_name,ptype);
                    appendArg(params,parameter1);
                    
                    var1_name = "__htc_my_omp_team_size";
                    parameter1 = buildInitializedName(var1_name,ptype);
                    appendArg(params,parameter1);

                    ptype = SCDecls->get_htc_teams_t_type();
                    var1_name = "__htc_my_omp_league_size";
                    parameter1 = buildInitializedName(var1_name,ptype);
                    appendArg(params,parameter1);
                }
#else
                parameter1 = buildInitializedName(var1_name,ptype);
                
                appendArg(params,parameter1);
#endif
            }
            p_init_name = parameter1; // set the source parameter to the wrapper
            
        } else {
            // case 3: use a parameter for each variable, the default case and the 
            // classic case
            p_init_name = createOneFunctionParameter(i_name, use_orig_type , func); 
        }
        
        // step 2. Create unpacking/unwrapping statements, also record variables to 
        //         be replaced
        // ----------------------------------------
        bool isPointerDeref = false; 
        if (Outliner::temp_variable || Outliner::useStructureWrapper) {
            // Check if the current variable belongs to the symbol set 
            //suitable for using pointer dereferencing
            const SgVariableSymbol* i_sym = 
                isSgVariableSymbol(i_name->get_symbol_from_symbol_table ());
            ROSE_ASSERT(i_sym!=NULL);
            isPointerDeref = (pdSyms.find(i_sym) != pdSyms.end());
        }  
        
        if (Outliner::enable_classic) {
            // classic methods use parameters directly, no unpacking is needed
            if (!use_orig_type) {
                //read only variable should not have local variable declaration,
                // using parameter directly taking advantage of the same parameter 
                // names for readOnly variables.
                // Let postprocessing patch up symbols for them.
                
                // non-readonly variables need to be mapped to their parameters with 
                // different names (p__)
                // remapVarSyms() will use pointer dereferencing for all of them by 
                // default in C, // this is enough to mimic the classic outlining work 
                recordSymRemap(*i,p_init_name, args_scope, sym_remap); 
            }
            
        } else {
            // create unwrapping statements from parameters/ or the array parameter 
            // for pointers
            
            local_var_decl  = 
                createUnpackDecl (p_init_name, counter, isPointerDeref, i_name , 
                                  struct_decl, body);
            ROSE_ASSERT (local_var_decl);
            prependStatement (local_var_decl,body);
            
            
            // regular and shared variables used the first local declaration
            recordSymRemap (*i, local_var_decl, args_scope, sym_remap);
            
            const SgVariableSymbol* i_sym = 
                SageInterface::getFirstVarSym(local_var_decl);

            OmpUplevelAttribute *attr = getOmpUplevelAttribute(sym);
            int level = -1;
            bool is_parm = false;
            if (attr) {
                level = attr->getUplevel();
                is_parm = attr->isParameter();
            }
        
            setOmpUplevelAttribute((SgVariableSymbol *)i_sym, level, is_parm);
            setOmpUplevelAttribute((SgVariableSymbol *)sym, 0, is_parm);
#if 0
            std::cerr << "Set uplevel for symbol " <<
                i_sym->get_name().getString() << " " << i_sym <<
                " to " <<
                level << " sym is " << sym << std::endl;
#endif

            // transfer the value for firstprivate variables. 
            // TODO
        }
        
        // step 3. Create and insert companion re-pack statement in the end of the 
        //         function body
        // ----------------------------------------
        SgInitializedName* local_var_init = NULL;
        if (local_var_decl != NULL )
            local_var_init = local_var_decl->get_decl_item (SgName (name_str.c_str ()));
        
        if (!SageInterface::is_Fortran_language() && !Outliner::enable_classic)  
            ROSE_ASSERT(local_var_init!=NULL);  
        
        // Only generate restoring statement for non-pointer dereferencing cases
        // if temp variable mode is enabled
        if (Outliner::temp_variable) {
            if (!isPointerDeref) {
                if (restoreVars.find(const_cast<SgInitializedName*> (i_name)) !=
                    restoreVars.end()) {
                    
                    if (Outliner::enable_debug && local_var_init != NULL) {
                        cout<< "Generating restoring statement for non-read-only variable:"
                            << local_var_init->unparseToString() 
                            << endl;
                    }
                    
                    SgExprStatement* pack_stmt = createPackStmt (local_var_init);
                    if (pack_stmt) {
                        appendStatement (pack_stmt,body);
                    }
                } else {
                    if (Outliner::enable_debug && local_var_init != NULL)
                        cout<< "skipping a read-only variable for restoring its value:"
                            << local_var_init->unparseToString()
                            << endl;
                }
            } else {
                if (Outliner::enable_debug && local_var_init != NULL)
                    cout << "skipping restore of a variable using pointer-dereferencing:"
                         << local_var_init->unparseToString()
                         << endl;
            }
            
        }
        counter ++;
    } //end for
    
    SgBasicBlock* func_body = func->get_definition()->get_body();
    
#if 1
    // TODO: move this outside of outliner since it is OpenMP-specific.
    // omp_lowering.cpp generateOutlinedTask()
    // A caveat is the moving this also means we have to patch up prototype later
    // For OpenMP lowering, we have to have a void * parameter even if there is no 
    // need to pass any parameters in order to match the gomp runtime lib 's 
    // function prototype for function pointers

    SgFile* cur_file = getEnclosingFileNode(func);
    ROSE_ASSERT (cur_file != NULL);

    if (cur_file->get_openmp_lowering ()) {
        if (syms.size() ==0) {
#if 1
            // For HT, we need to pass parent_htid and my_omp_thread_num
            // Convey did this
            SgName var_name;
            if (SCDecls == 0) {
                SCDecls = new scDeclarations(SageInterface::getGlobalScope(def));
            }
            SgType* ptype = SCDecls->get_htc_tid_t_type();

            if ( add_htc_parameters) {
                var_name = "__htc_parent_htid";
                parameter1 = buildInitializedName(var_name,ptype);
                appendArg(params,parameter1);
                
                var_name = "__htc_my_omp_thread_num";
                parameter1 = buildInitializedName(var_name,ptype);
                appendArg(params,parameter1);
                
                var_name = "__htc_my_omp_team_size";
                parameter1 = buildInitializedName(var_name,ptype);
                appendArg(params,parameter1);

                ptype = SCDecls->get_htc_teams_t_type();
                var_name = "__htc_my_omp_league_size";
                parameter1 = buildInitializedName(var_name,ptype);
                appendArg(params,parameter1);
            }
#else
            SgName var1_name = "__out_argv";
            SgType* ptype= NULL; 
            // A dummy integer parameter for Fortran outlined function
            ptype = buildPointerType (buildVoidType());
            ROSE_ASSERT (Outliner::useStructureWrapper); 
            // TODO: this assertion may no longer true for "omp target" + 
            // "omp parallel for", in which map() may not show up at all (0 variables)
            
            parameter1 = buildInitializedName(var1_name,ptype);
            appendArg(params,parameter1);
#endif
        }
    }
#endif

    // variable substitution 
    remapVarSyms (sym_remap, pdSyms, private_remap , func_body);

}

// =====================================================================

namespace HtOmpSupport {
    
    //! Create a function named 'func_name_str', with a parameter list from 'syms'
    SgFunctionDeclaration *
    generateFunction ( SgBasicBlock* s,  // block to be outlined
                       const string& func_name_str, // function name provided
                       const ASTtools::VarSymSet_t& syms, 
                           // variables to be passed in/out the outlined function
                       const ASTtools::VarSymSet_t& pdSyms, 
                           // variables to be passed using its address. using pointer 
                           // dereferencing (AddressOf() for pass-by-reference), mostly
                           // used for struct wrapper.
                       const std::set< SgInitializedName *>& restoreVars, 
                           // optional information about variables to be restored 
                           // after variable clones finish computation
                       SgClassDeclaration* struct_decl,  
                           // an optional wrapper structure for parameters
                       SgScopeStatement* scope,
                       bool add_htc_parameters)
{
    ROSE_ASSERT (s&&scope);
    ROSE_ASSERT(isSgGlobal(scope));
    
#if 0  
    // step 1: perform necessary liveness and side effect analysis, if requested.
    // This is moved out to the callers, who has freedom to decide if additional 
    // analysis is needed in the first place
    // For OpenMP implementation, directives with clauses have sufficient information 
    // to guide variable handling, no other analysis is needed at all. 
#endif
    
    //step 2. Create function skeleton, 'func'.
    // -----------------------------------------
    SgName func_name (func_name_str);
    SgFunctionParameterList *parameterList = buildFunctionParameterList();
    
    SgFunctionDeclaration* func = 
        createFuncSkeleton (func_name,SgTypeVoid::createType (),parameterList, scope);
    ROSE_ASSERT (func);
    
    // Liao, 4/15/2009 , enforce C-bindings  for C++ outlined code
    // enable C code to call this outlined function
    // Only apply to C++ , pure C has trouble in recognizing extern "C"
    // Another way is to attach the function with preprocessing info:
    // #if __cplusplus 
    // extern "C"
    // #endif
    // We don't choose it since the language linkage information is not explicit in AST
    // if (!SageInterface::is_Fortran_language())
    if ( SageInterface::is_Cxx_language() || 
         is_mixed_C_and_Cxx_language() || 
         is_mixed_Fortran_and_Cxx_language() || 
         is_mixed_Fortran_and_C_and_Cxx_language() ) {
        // Make function 'extern "C"'
        func->get_declarationModifier().get_storageModifier().setExtern();
        func->set_linkage ("C");
    }
    
    //step 3. Create the function body
    // -----------------------------------------
    // Generate the function body by deep-copying 's'.
    SgBasicBlock* func_body = func->get_definition()->get_body();
    ROSE_ASSERT (func_body != NULL);
    
    // This does a copy of the statements in "s" to the function body of the outlined 
    // function.
    ROSE_ASSERT(func_body->get_statements().empty() == true);
    SageInterface::moveStatementsBetweenBlocks (s, func_body);
    
    if (Outliner::useNewFile) {
        ASTtools::setSourcePositionAtRootAndAllChildrenAsTransformation(func_body);
    }

    //step 4: variable handling, including: 
    // -----------------------------------------
    //   create parameters of the outlined functions
    //   add statements to unwrap the parameters if necessary
    //   add repacking statements if necessary
    //   replace variables to access to parameters, directly or indirectly
    
    variableHandling(syms, pdSyms, restoreVars, struct_decl, func, 
                     add_htc_parameters);
    ROSE_ASSERT (func != NULL);

    // Liao 12/6/2012. It is essential to rebuild function type after the parameter 
    // list is finalized.
    // The original function type was build using empty parameter list.
    SgType* stale_func_type = func->get_type();
    func->set_type(buildFunctionType(
                     func->get_type()->get_return_type(), 
                     buildFunctionParameterTypeList(func->get_parameterList())));

    // We now have mandatory non-defining declaration for every defining decl. 
    // We have to update its type also.
    SgFunctionDeclaration* non_def_func = 
        isSgFunctionDeclaration(func->get_firstNondefiningDeclaration ()) ;
    ROSE_ASSERT (non_def_func != NULL);
    ROSE_ASSERT (stale_func_type == non_def_func->get_type());
    non_def_func->set_type(func->get_type());
    
    // Retest this...
    ROSE_ASSERT(func->get_definition()->get_body()->get_parent() ==     
                func->get_definition());

    ROSE_ASSERT(scope->lookup_function_symbol(func->get_name()));
    return func;
}

}  // namespace HtOmpSupport

// eof
