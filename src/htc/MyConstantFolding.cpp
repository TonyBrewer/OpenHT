/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "sage3basic.h"
#include "sageBuilder.h"
#include "HtSageUtils.h"
#include "MyConstantFolding.h"
#include <string>

using namespace MyConstantFolding;
using namespace std;


void
MyConstantFolding::constantFoldingOptimization(SgNode* n) {
  // This is the main function interface for constant folding
     ConstantFoldingTraversal t;
     ConstantFoldingInheritedAttribute ih;

     t.traverse(n,ih);
}


ConstantFoldingInheritedAttribute
ConstantFoldingTraversal::evaluateInheritedAttribute ( 
   SgNode* astNode, 
   ConstantFoldingInheritedAttribute inheritedAttribute ) {

    inheritedAttribute.isConstantFoldedValue = false;

    return inheritedAttribute;
}

// A helper function: get value if it fits into int type
// T1: the internal value's type
// T2: the concrete value exp type
template <typename T1, typename T2> 
static T1 cf_get_value_t(T2* sg_value_exp)
{
  return sg_value_exp->get_value();
}

static int cf_get_int_value(SgValueExp * sg_value_exp)
{
  int rtval;
  if (isSgBoolValExp(sg_value_exp))
    rtval = isSgBoolValExp(sg_value_exp)->get_value();
  else if (isSgCharVal(sg_value_exp))
    rtval = isSgCharVal(sg_value_exp)->get_value();
  else if (isSgEnumVal(sg_value_exp))
    rtval = isSgEnumVal(sg_value_exp)->get_value();
  else if (isSgIntVal(sg_value_exp))
    rtval = isSgIntVal(sg_value_exp)->get_value();
  else if (isSgShortVal(sg_value_exp))
    rtval = isSgShortVal(sg_value_exp)->get_value();
  else if (isSgUpcMythread(sg_value_exp))
    rtval = isSgUpcMythread(sg_value_exp)->get_value();
  else if (isSgUpcThreads(sg_value_exp))
    rtval = isSgUpcThreads(sg_value_exp)->get_value();
  else
  {
    cerr<<"error: wrong value exp type for cf_get_int_value():"<<sg_value_exp->class_name()<<endl;
    ROSE_ASSERT(false);
  }
  return rtval;
}



//!  Data type --> allowed operations

//! Calculate the  result of a binary operation on two constant values
// Only contain binary operations allows integer operands
// Those binary operations are: %, <<, >>, &, ^, |
//
// We ignore the compound assignment operations for constant folding, since the folding
// will lose the side effect on the assigned variables.
template<typename T>
T calculate_integer_t (SgBinaryOp* binaryOperator, T lhsValue, T rhsValue)
{
  T foldedValue; // to be converted to result type   
  switch (binaryOperator->variantT())
  {
    case V_SgModOp:
      {
        foldedValue = lhsValue % rhsValue;
        break;
      }
    case V_SgLshiftOp:
      {
        foldedValue = (lhsValue << rhsValue);
        break;
      }
    case V_SgRshiftOp:
      {
        foldedValue = (lhsValue >> rhsValue);
        break;
      }
      // bitwise operations
    case V_SgBitAndOp:
      {
        foldedValue = lhsValue & rhsValue;
        break;
      }
    case V_SgBitOrOp:
      {
        foldedValue = lhsValue | rhsValue;
        break;
      }
    case V_SgBitXorOp:
      {
        foldedValue = lhsValue ^ rhsValue;
        break;
      }
    default:
      cerr<<"warning: calculuate_integer_t - illegal operator type:"<<binaryOperator->class_name()<<endl;
  } 
  return foldedValue;
}  

//! Calculate the  result of a binary operation on two constant float-kind values, 
//   Integer-type-only binary operations are excluded : %, <<, >>, &, ^, |
// We ignore pointer and complex types for constant folding
//   none_integer types: floating point types
// The operators compatible with floating-point types are
//  + - * / <, > <=, >=, ==, !=  &&, ||
//
template<typename T>
T calculate_float_t (SgBinaryOp* binaryOperator, T lhsValue, T rhsValue)
{
  T foldedValue; // to be converted to result type   

  switch (binaryOperator->variantT())
  {
    // Basic arithmetic 
    case V_SgAddOp:
      {
        foldedValue = lhsValue + rhsValue;
        break;
      }
    case V_SgSubtractOp:
      {
        foldedValue = lhsValue - rhsValue;
        break;
      }
    case V_SgMultiplyOp:
      {
        foldedValue = lhsValue * rhsValue;
        break;
      }
    case V_SgDivideOp:
      {
        foldedValue = lhsValue / rhsValue;
        break;
      }
     // Fortran only? a**b
      //    case V_SgExponentiationOp: 
      //      {
      //        foldedValue = lhsValue ** rhsValue;
      //        break;
      //      }

    case V_SgIntegerDivideOp://TODO what is this ??
      {
        foldedValue = lhsValue / rhsValue;
        break;
      }
    // logic operations
    case V_SgAndOp:
      {
        foldedValue = lhsValue && rhsValue;
        break;
      }

    case V_SgOrOp:
      {
        foldedValue = lhsValue || rhsValue;
        break;
      }

      // relational operations
    case V_SgEqualityOp:
      {
        foldedValue = (lhsValue == rhsValue);
        break;
      }
    case V_SgNotEqualOp:
      {
        foldedValue = (lhsValue != rhsValue);
        break;
      }
    case V_SgGreaterOrEqualOp:
      {
        foldedValue = (lhsValue >= rhsValue);
        break;
      }
    case V_SgGreaterThanOp:
      {
        foldedValue = (lhsValue > rhsValue);
        break;
      }
    case V_SgLessOrEqualOp:
      {
        foldedValue = (lhsValue <= rhsValue);
        break;
      }
    case V_SgLessThanOp:
      {
        foldedValue = (lhsValue < rhsValue);
        break;
      }
   default:
      {
        cerr<<"warning: calculuate - unhandled operator type:"<<binaryOperator->class_name()<<endl;
        //ROSE_ASSERT(false); // not every binary operation type can be evaluated
      }

  } // end switch

  return foldedValue;
}


//! string type and binary operator: the allowed operations on string values
// + , <, >, <=, >=, ==, !=  
template<typename T>
T calculate_string_t (SgBinaryOp* binaryOperator, T lhsValue, T rhsValue)
{
  T foldedValue; // to be converted to result type   

  switch (binaryOperator->variantT())
  {
    // Basic arithmetic 
    case V_SgAddOp:
      {
        foldedValue = lhsValue + rhsValue;
        break;
      }
      // relational operations
    case V_SgEqualityOp:
      {
        foldedValue = (lhsValue == rhsValue);
        break;
      }
    case V_SgNotEqualOp:
      {
        foldedValue = (lhsValue != rhsValue);
        break;
      }
    case V_SgGreaterOrEqualOp:
      {
        foldedValue = (lhsValue >= rhsValue);
        break;
      }
    case V_SgGreaterThanOp:
      {
        foldedValue = (lhsValue > rhsValue);
        break;
      }
    case V_SgLessOrEqualOp:
      {
        foldedValue = (lhsValue <= rhsValue);
        break;
      }
    case V_SgLessThanOp:
      {
        foldedValue = (lhsValue < rhsValue);
        break;
      }
    default:
      {
        cerr<<"warning: calculate_string_t - unacceptable operator type:"<<binaryOperator->class_name()<<endl;
        ROSE_ASSERT(false); 
      }
  } // end switch
  return foldedValue;
}


// For T type which is compatible for all binary operators we are interested in.
template<typename T>
T calculate_t (SgBinaryOp* binaryOperator, T lhsValue, T rhsValue)
{
  T foldedValue; // to be converted to result type   
  switch (binaryOperator->variantT())
  {
    // integer-exclusive oprations
    case V_SgModOp:
      {
        foldedValue = lhsValue % rhsValue;
        break;
      }
    case V_SgLshiftOp:
      {
        foldedValue = (lhsValue << rhsValue);
        break;
      }
    case V_SgRshiftOp:
      {
        foldedValue = (lhsValue >> rhsValue);
        break;
      }
      // bitwise operations
    case V_SgBitAndOp:
      {
        foldedValue = lhsValue & rhsValue;
        break;
      }
    case V_SgBitOrOp:
      {
        foldedValue = lhsValue | rhsValue;
        break;
      }
    case V_SgBitXorOp:
      {
        foldedValue = lhsValue ^ rhsValue;
        break;
      }
   // non-integer-exclusive operations   
     case V_SgAddOp:
      {
        foldedValue = lhsValue + rhsValue;
        break;
      }
    case V_SgSubtractOp:
      {
        foldedValue = lhsValue - rhsValue;
        break;
      }
    case V_SgMultiplyOp:
      {
        foldedValue = lhsValue * rhsValue;
        break;
      }
    case V_SgDivideOp:
      {
        foldedValue = lhsValue / rhsValue;
        break;
      }
     // Fortran only? a**b
      //    case V_SgExponentiationOp: 
      //      {
      //        foldedValue = lhsValue ** rhsValue;
      //        break;
      //      }

    case V_SgIntegerDivideOp://TODO what is this ??
      {
        foldedValue = lhsValue / rhsValue;
        break;
      }
    // logic operations
    case V_SgAndOp:
      {
        foldedValue = lhsValue && rhsValue;
        break;
      }

    case V_SgOrOp:
      {
        foldedValue = lhsValue || rhsValue;
        break;
      }

      // relational operations
    case V_SgEqualityOp:
      {
        foldedValue = (lhsValue == rhsValue);
        break;
      }
    case V_SgNotEqualOp:
      {
        foldedValue = (lhsValue != rhsValue);
        break;
      }
    case V_SgGreaterOrEqualOp:
      {
        foldedValue = (lhsValue >= rhsValue);
        break;
      }
    case V_SgGreaterThanOp:
      {
        foldedValue = (lhsValue > rhsValue);
        break;
      }
    case V_SgLessOrEqualOp:
      {
        foldedValue = (lhsValue <= rhsValue);
        break;
      }
    case V_SgLessThanOp:
      {
        foldedValue = (lhsValue < rhsValue);
        break;
      }
   default:
      {
        cerr<<"warning: calculuate - unhandled operator type:"<<binaryOperator->class_name()<<endl;
        //ROSE_ASSERT(false); // not every binary operation type can be evaluated
      }
  }
  return foldedValue;
}


// T1: a SgValExp's child SAGE class type, 
// T2: its internal storage C type for its value
template <typename T1, typename T2> 
static SgValueExp* buildResultValueExp_t (SgBinaryOp* binaryOperator, SgValueExp* lhsValue, SgValueExp* rhsValue)
{
  SgValueExp* result = NULL;
  T1* lhs_v = dynamic_cast<T1*>(lhsValue);
  T1* rhs_v = dynamic_cast<T1*>(rhsValue);
  ROSE_ASSERT(lhs_v);
  ROSE_ASSERT(rhs_v);

  T2 lhs = cf_get_value_t<T2>(lhs_v);
  T2 rhs = cf_get_value_t<T2>(rhs_v);
  T2 foldedValue ;
  foldedValue = calculate_t(binaryOperator,lhs,rhs);
  result = new T1(foldedValue,"");
  ROSE_ASSERT(result != NULL);
  SageInterface::setOneSourcePositionForTransformation(result);
  return result;
}

// T1: a SgValExp's child SAGE class type, 
// T2: its internal storage C type for its value
// for T2 types (floating point types) which are incompatible with integer-only operations: e.g. if T2 is double, then % is now allowed
template <typename T1, typename T2> 
static SgValueExp* buildResultValueExp_float_t (SgBinaryOp* binaryOperator, SgValueExp* lhsValue, SgValueExp* rhsValue)
{
  SgValueExp* result = NULL;
  T1* lhs_v = dynamic_cast<T1*>(lhsValue);
  T1* rhs_v = dynamic_cast<T1*>(rhsValue);
  ROSE_ASSERT(lhs_v);
  ROSE_ASSERT(rhs_v);

  T2 lhs = cf_get_value_t<T2>(lhs_v);
  T2 rhs = cf_get_value_t<T2>(rhs_v);
  T2 foldedValue ;
  foldedValue = calculate_float_t(binaryOperator,lhs,rhs);
  result = new T1(foldedValue,"");
  ROSE_ASSERT(result != NULL);
  SageInterface::setOneSourcePositionForTransformation(result);
  return result;
}



//!Evaluate a binary expression  a binOp b, return a value exp if both operands are constants
static SgValueExp* evaluateBinaryOp(SgBinaryOp* binaryOperator)
{
  SgValueExp* result = NULL;
  ROSE_ASSERT(binaryOperator != NULL);

  SgExpression* lhsOperand = binaryOperator->get_lhs_operand();
  SgExpression* rhsOperand = binaryOperator->get_rhs_operand();

  if (lhsOperand == NULL || rhsOperand == NULL) 
    return NULL;
  // Check if these are value expressions (constants appropriate for constant folding)
  SgValueExp* lhsValue = isSgValueExp(HtSageUtils::SkipCasting(lhsOperand));
  SgValueExp* rhsValue = isSgValueExp(HtSageUtils::SkipCasting(rhsOperand));

  if (lhsValue != NULL && rhsValue != NULL)
  {
    // we ignore the cases of different operand types for now
      if (lhsValue->variantT() != rhsValue->variantT()) {
          std::cout << "returning due to different types " << std::endl;
          return NULL;
      }

    // TODO what if lhs and rhs have different types?
    switch (lhsValue->variantT())
    {
      case V_SgBoolValExp:
        { // special handling for bool type, since intVal is built as a result
          int lhsInteger = cf_get_value_t<int>(isSgBoolValExp(lhsValue));
          int rhsInteger = cf_get_value_t<int>(isSgBoolValExp(rhsValue));
          int foldedValue = calculate_t(binaryOperator, lhsInteger, rhsInteger);
          result = SageBuilder::buildIntVal(foldedValue);
          break;
        }
      case V_SgCharVal: // mostly provide a pair of SgValueExp class type and its internal value type
                        // for SgCharVal, the internal value type is also char
        {
          result = buildResultValueExp_t<SgCharVal,char>(binaryOperator,lhsValue,rhsValue);
          break;
        }
        // each calls a subset (bitvec again?) of binary calculation according to data types
      case V_SgDoubleVal: 
        {
          result = buildResultValueExp_float_t<SgDoubleVal,double>(binaryOperator,lhsValue,rhsValue);
          break;
        }
// enumerate value is special, we build an integer equivalent here for simplicity
      case V_SgEnumVal: 
        {
          result = buildResultValueExp_t<SgIntVal,int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgFloatVal: 
        {
          // Liao, 9/22/2009, folding float to float may decrease the accuracy. promoting it to double
          // TODO, need an option to skip folding float point operations
          result = buildResultValueExp_float_t<SgFloatVal,float>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgIntVal:
        {
          result = buildResultValueExp_t<SgIntVal,int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgLongDoubleVal: 
        {
          result = buildResultValueExp_float_t<SgLongDoubleVal,long double>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgLongIntVal: 
        {
          result = buildResultValueExp_t<SgLongIntVal,long int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgLongLongIntVal: 
        {
          result = buildResultValueExp_t<SgLongLongIntVal,long long int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgShortVal: 
        {
          result = buildResultValueExp_t<SgShortVal,short>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      //special handling for string val due to its unique constructor  
      case V_SgStringVal: 
        {
          std::string lhsInteger = cf_get_value_t<std::string>(isSgStringVal(lhsValue));
          std::string rhsInteger = cf_get_value_t<std::string>(isSgStringVal(rhsValue));
          std::string foldedValue = calculate_string_t(binaryOperator, lhsInteger, rhsInteger);
          result = SageBuilder::buildStringVal(foldedValue);
          break;
        }
      case V_SgUnsignedCharVal: 
        {
          result = buildResultValueExp_t<SgUnsignedCharVal,unsigned char>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgUnsignedIntVal: 
        {
          result = buildResultValueExp_t<SgUnsignedIntVal,unsigned int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgUnsignedLongLongIntVal: 
        {
          result = buildResultValueExp_t<SgUnsignedLongLongIntVal,unsigned long long int>(binaryOperator,lhsValue,rhsValue);
          break;
        }
        
      case V_SgUnsignedLongVal: 
        {
          result = buildResultValueExp_t<SgUnsignedLongVal,unsigned long>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      case V_SgUnsignedShortVal: 
        {
          result = buildResultValueExp_t<SgUnsignedShortVal,unsigned short>(binaryOperator,lhsValue,rhsValue);
          break;
        }
      default:
        {
          cout<<"evaluateBinaryOp(): "<<"Unhandled SgValueExp case for:"<<lhsValue->class_name()<<endl;
          // Not a handled type
        }
    }

  } // both operands are constant

  // std::cerr << " returning " << result->unparseToString() << std::endl;

  return result;
}

//! Evaluate a conditional expression i.e. a?b:c
static SgValueExp* evaluateConditionalExp(SgConditionalExp * cond_exp)
{
  SgValueExp* result = NULL;

  // Calculate the current node's synthesized attribute value
  SgExpression* c_exp_value = cond_exp->get_conditional_exp();
  SgValueExp* value_exp = isSgValueExp(c_exp_value);

  SgExpression* t_exp_value = cond_exp->get_true_exp();
  SgValueExp* value_exp_t = isSgValueExp(t_exp_value);

  SgExpression* f_exp_value = cond_exp->get_false_exp();
  SgValueExp* value_exp_f = isSgValueExp(f_exp_value);

  if (value_exp)
  {
    switch (value_exp->variantT())
    {
      //TODO first operand can be scalar type: arithmetic (integer + floating point) + pointer
      //  case V_SgCharVal:
      //  {
      //    std::string v = cf_get_value_t<std::string>(value_exp);
      //    break;
      //  }
      //  case V_SgDoubleVal:
      //  {
      //    double v = cf_get_value_t<double>(value_exp);
      //    break;
      //  }
      //   case V_SgFloatVal:
      //  {
      //    float v = cf_get_value_t<float>(value_exp);
      //    break;
      //  }
      // integer value
      case V_SgCharVal:
      case V_SgShortVal:
      case V_SgBoolValExp:
      case V_SgEnumVal:
      case V_SgIntVal:
      case V_SgUpcMythread:
      case V_SgUpcThreads:
        {
          int v= cf_get_int_value(value_exp);
          if (v)
          {
            if (value_exp_t) // set the value only if the true exp is a constant
              result = value_exp_t;
          }
          else
          {
            if (value_exp_f)
              result = value_exp_f;
          }
          break;
        }
      default:
        {
          cout<<"evaluateConditionalExp(): unhandled value expression type of a conditional exp:"<<value_exp->class_name()<<endl;
        }
    } // end switch
  }// end if (value_exp) the condition is a constant
  return result ;
}

ConstantFoldingSynthesizedAttribute
ConstantFoldingTraversal::evaluateSynthesizedAttribute (
     SgNode* astNode,
     ConstantFoldingInheritedAttribute inheritedAttribute,
     SubTreeSynthesizedAttributes synthesizedAttributeList ) {

     ConstantFoldingSynthesizedAttribute returnAttribute;

#if DEBUG
     if (astNode) {
         std::cout << "visiting node kind " 
             " " << astNode->sage_class_name() << std::endl;
         std::cout << "synthesizedAttributeList size is " <<
             synthesizedAttributeList.size() << std::endl;

         for (int i = 0; i < synthesizedAttributeList.size(); i++) {
             SgExpression *attr = synthesizedAttributeList[i].newValueExp;
             if (attr) {
                 std::cout << "attr " << i << " is " << attr->unparseToString() << std::endl;
             } else {
                 std::cout << "attr " << i << " is null\n";
             }
         }
     }
#endif


     if (inheritedAttribute.isConstantFoldedValue == false) {

          SgExpression* expr = isSgExpression(astNode);
          if ( expr != NULL ) {
              if (isSgBinaryOp(expr)) {
                 SgBinaryOp* binaryOperator = isSgBinaryOp(expr);
                 ROSE_ASSERT(binaryOperator != NULL);

                 SgExpression* lhsSynthesizedValue = 
                     synthesizedAttributeList[SgBinaryOp_lhs_operand_i].newValueExp;
                 SgExpression* rhsSynthesizedValue = 
                     synthesizedAttributeList[SgBinaryOp_rhs_operand_i].newValueExp;

                 if (lhsSynthesizedValue != NULL)
                   binaryOperator->set_lhs_operand(lhsSynthesizedValue);
                 if (rhsSynthesizedValue != NULL)
                   binaryOperator->set_rhs_operand(rhsSynthesizedValue);

                  returnAttribute.newValueExp = evaluateBinaryOp(binaryOperator);

                  // end if binary op
               } else if (isSgUnaryOp(expr)) {
                  SgUnaryOp* unaryOperator = isSgUnaryOp(expr);
                  ROSE_ASSERT(unaryOperator != NULL);

                 SgExpression* synthesizedValue = 
                     synthesizedAttributeList[SgUnaryOp_operand_i].newValueExp;

                 if (synthesizedValue != NULL)
                     unaryOperator->set_operand(synthesizedValue);

                 SgExpression* operand = unaryOperator->get_operand();
                 SgValueExp* value = isSgValueExp(operand);

                 if (value != NULL) {
                     switch (unaryOperator->variantT()) {
                     case V_SgUnaryAddOp:
                     case V_SgMinusOp:
                     case V_SgCastExp:
                         //TODO
                         break;
                     default:
                         // There are a lot of expressions where constant 
                         // folding does not apply
                         break;
                     }
                 }
                 // end of unary op
              } else if (isSgAssignInitializer(expr)) {

                 SgAssignInitializer* assignInit = isSgAssignInitializer(expr);
                 ROSE_ASSERT (assignInit != NULL);

                 SgExpression* synthesizedValue = 
                     synthesizedAttributeList[SgAssignInitializer_operand_i].newValueExp;

                 if (synthesizedValue != NULL)
                     assignInit->set_operand(synthesizedValue);
                 // end of assign initializer
              } else if (isSgExprListExp(expr)) {
                  SgExprListExp* exprList = isSgExprListExp(expr);
                  ROSE_ASSERT(exprList != NULL);

                  for (size_t i = 0; 
                       i < exprList->get_expressions().size(); 
                       ++i) {
                      SgExpression* synthesizedValue = synthesizedAttributeList[i].newValueExp;
                      if (synthesizedValue != NULL)
                          exprList->get_expressions()[i] = synthesizedValue;
                  }
                  // end of ExprList
              } else if (isSgConditionalExp(expr)) {
                  // a? b: c

                  SgConditionalExp* cond_exp = isSgConditionalExp(expr);
                  ROSE_ASSERT(cond_exp);

                  SgExpression* c_exp_value = 
                      synthesizedAttributeList[SgConditionalExp_conditional_exp].newValueExp; 
                  SgExpression* t_exp_value = 
                      synthesizedAttributeList[SgConditionalExp_true_exp].newValueExp; 
                  SgExpression* f_exp_value = 
                      synthesizedAttributeList[SgConditionalExp_false_exp].newValueExp; 

                  if (c_exp_value!= NULL)
                      cond_exp->set_conditional_exp(c_exp_value);
                  if (t_exp_value!= NULL)
                      cond_exp->set_true_exp(t_exp_value);
                  if (f_exp_value!= NULL)
                      cond_exp->set_false_exp(f_exp_value);

                  // step 2. calculate the current node's synthesized attribute value
                  returnAttribute.newValueExp = evaluateConditionalExp(cond_exp);

              }
              
              //TODO SgSizeOfOp () 
              //return constant 1 for operand of type char, unsigned char, signed char
              //
              // ignore expressions without any constant valued children expressions, they are not supposed to be handled here
              else if (!isSgValueExp(expr) && 
                       !isSgVarRefExp(expr) &&
                       !isSgFunctionRefExp(expr) &&
                       !isSgFunctionCallExp(expr)) {
                  cout<<"constant folding: unhandled expression type:"<<expr->class_name()<<endl;
              }
              // end if (expr)
          } else if (isSgArrayType(astNode)) {

              SgArrayType* arrayType = isSgArrayType(astNode);
              ROSE_ASSERT (arrayType != NULL);

              int pos = SgArrayType_index;

              SgExpression* synthesizedValue = 
                  synthesizedAttributeList[pos].newValueExp;
              
              if (synthesizedValue != NULL) {
                  arrayType->set_index(synthesizedValue);
              }
              // end of arrayType
          }
     }

     return returnAttribute;
}

