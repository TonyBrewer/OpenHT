/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef HTC_HTC_ATTRIBUTES_H
#define HTC_HTC_ATTRIBUTES_H

#include <rose.h>

//----------------------------------------------------------------------------
// FsmPlaceHolderAttribute
//----------------------------------------------------------------------------

//
// Mark place holder expressions that must be replaced by an FSM state
// after the FSM has been constructed.  Currently this is simply a known
// target label, which will be used to replace the expression with the
// corresponding state name.
//
class FsmPlaceHolderAttribute : public AstAttribute {
public:
  FsmPlaceHolderAttribute(SgLabelStatement *); 
  SgLabelStatement *targetLabel;

private:
  FsmPlaceHolderAttribute();
};

extern FsmPlaceHolderAttribute *getFsmPlaceHolderAttribute(SgExpression *);

//----------------------------------------------------------------------------
// OmpUplevelAttribute
//----------------------------------------------------------------------------

//
// Indicates shared OMP variables by tagging them with the "uplevel"
// count to reach the actual data.    An uplevel count of 0 indicates
// a variable that is originally private but must be allocated in an HT
// Global Variable because it is shared in an enclosed region.

class OmpUplevelAttribute : public AstAttribute {
public:
    OmpUplevelAttribute(int level, bool is_parm) {
        uplevel = level;
        is_parameter = is_parm;
    }
    void setUplevel(int level) {
        uplevel = level;
    }
    void setIsParameter(bool parm) {
        is_parameter = parm;
    }
    int getUplevel() {
        return uplevel;
    }
    bool isParameter() {
        return is_parameter;
    }

private:
    int uplevel;
    bool is_parameter;

    // Hide default constructor
    OmpUplevelAttribute();
};

extern OmpUplevelAttribute *getOmpUplevelAttribute(const SgVariableSymbol *);
extern void setOmpUplevelAttribute(SgVariableSymbol *sym, int level, bool is_parm);


//----------------------------------------------------------------------------
// OmpEnclosingFunctionAttribute
//----------------------------------------------------------------------------

//
// Indicates shared OMP variables by tagging them with the "uplevel"
// count to reach the actual data.    An uplevel count of 0 indicates
// a variable that is originally private but must be allocated in an HT
// Global Variable because it is shared in an enclosed region.

class OmpEnclosingFunctionAttribute : public AstAttribute {
public:
    OmpEnclosingFunctionAttribute(SgFunctionDeclaration *outer) {
        enclosing_function = outer;
    }
    void setEnclosingFunction(SgFunctionDeclaration *outer) {
        enclosing_function = outer;
    }
    SgFunctionDeclaration *getEnclosingFunction() {
        return enclosing_function;
    }

private:
    SgFunctionDeclaration *enclosing_function;

    // Hide default constructor
    OmpEnclosingFunctionAttribute();
};

extern OmpEnclosingFunctionAttribute*
       getOmpEnclosingFunctionAttribute(const SgFunctionDeclaration *inner);
extern void setOmpEnclosingFunctionAttribute(SgFunctionDeclaration *inner_region,
                                             SgFunctionDeclaration *outer_region);


//----------------------------------------------------------------------------
// ParallelCallAttribute
//----------------------------------------------------------------------------
//
// Mark parallel call expressions which must be handled specially by
// ProcessCalls.  Carry the special Ht join cycle label with the call.
//
class ParallelCallAttribute : public AstAttribute {
public:
  ParallelCallAttribute(SgLabelStatement *); 
  SgLabelStatement *joinCycleLabel;

private:
  ParallelCallAttribute();
};

extern ParallelCallAttribute *getParallelCallAttribute(SgExpression *);


//----------------------------------------------------------------------------
// DontProcessAttribute
//----------------------------------------------------------------------------
//
// Mark nodes that have been translated in such a way that later phases
// should not re-process them.  Currently, this is only used for if-stmts
// that are introduced during loop rewriting, where those should not be
// subsequently translated during the if-stmt rewriting.
//
class DontProcessAttribute : public AstAttribute {
public:
  DontProcessAttribute() { }
};

extern DontProcessAttribute *getDontProcessAttribute(SgNode *);


//----------------------------------------------------------------------------
// DefaultModuleWidthAttribute
//----------------------------------------------------------------------------
//
// FIXME
//
class DefaultModuleWidthAttribute : public AstAttribute {
public:
  DefaultModuleWidthAttribute(int); 
  int width;

private:
  DefaultModuleWidthAttribute();
};

extern DefaultModuleWidthAttribute *getDefaultModuleWidthAttribute(SgGlobal *);


#endif //HTC_HTC_ATTRIBUTES_H


