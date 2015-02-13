/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
// This file implements constant folding, but leverages the work done
// by the frontend and adds support for constant folding on any transformations
// introduced after the frontend processing (the transformations are typically
// not seen by the frontend, so we have to handle constant folding directly
// for transformations.  Since many transformations are introduced automatically
// there is a substantial requirement for constant folding to clean the generated 
// code up a bit.  However, since the backend compiler (vendor's compiler) 

#ifndef MY_CONSTANT_FOLDING_H
#define MY_CONSTANT_FOLDING_H

namespace MyConstantFolding {

// Build an inherited attribute for the tree traversal to skip constant folded expressions
class ConstantFoldingInheritedAttribute
   {
     public:
          bool isConstantFoldedValue;

      //! Specific constructors are required
          ConstantFoldingInheritedAttribute()
             : isConstantFoldedValue(false)
             {};

       // Need to implement the copy constructor
          ConstantFoldingInheritedAttribute ( const ConstantFoldingInheritedAttribute & X )
             : isConstantFoldedValue(X.isConstantFoldedValue) 
             {};
   };

class ConstantFoldingSynthesizedAttribute
   {
     public:
          SgValueExp* newValueExp;

          ConstantFoldingSynthesizedAttribute() : newValueExp(NULL) {};
          ConstantFoldingSynthesizedAttribute ( const ConstantFoldingSynthesizedAttribute & X )
             : newValueExp(X.newValueExp) {};
   };

class ConstantFoldingTraversal
   : public SgTopDownBottomUpProcessing<ConstantFoldingInheritedAttribute,ConstantFoldingSynthesizedAttribute>
   {
     public:
       // Functions required by the rewrite mechanism
          ConstantFoldingInheritedAttribute evaluateInheritedAttribute (
             SgNode* n, 
             ConstantFoldingInheritedAttribute inheritedAttribute );
          ConstantFoldingSynthesizedAttribute evaluateSynthesizedAttribute (
             SgNode* n,
             ConstantFoldingInheritedAttribute inheritedAttribute,
             SubTreeSynthesizedAttributes synthesizedAttributeList );
   };

//! This is the external interface of constant folding:
 void constantFoldingOptimization(SgNode* n);

}

#endif
