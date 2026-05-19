/*  $Id: ExpressionTypeSetter.cpp,v 1.74 2020/04/05 03:16:01 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2015 Pierre Sarrazin <http://sarrazip.com/>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ExpressionTypeSetter.h"

#include "BinaryOpExpr.h"
#include "UnaryOpExpr.h"
#include "CastExpr.h"
#include "ClassDef.h"
#include "ConditionalExpr.h"
#include "ObjectMemberExpr.h"
#include "FunctionCallExpr.h"
#include "TranslationUnit.h"
#include "VariableExpr.h"
#include "IdentifierExpr.h"
#include "CommaExpr.h"
#include "Declaration.h"

using namespace std;


ExpressionTypeSetter::ExpressionTypeSetter()
{
}


ExpressionTypeSetter::~ExpressionTypeSetter()
{
}


// Calls setTypeDesc() on 't'.
//
// This method is called for each node of a syntax tree.
// It is called on a child node before being called on the parent node.
// For example, for "return 42", this method is called on the "42" node,
// then on the JumpStmt representing the "return".
//
// N.B.: A VariableExpr is not typed here. It is typed in ScopeCreator::processIdentifierExpr().
//
bool
ExpressionTypeSetter::close(Tree *t)
{
    const TranslationUnit &tu = TranslationUnit::instance();

    BinaryOpExpr *bin = dynamic_cast<BinaryOpExpr *>(t);
    if (bin != NULL)
    {
        if (tu.isWarningOnSignCompareEnabled()
            && bin->isOrderComparisonOperator()
            && bin->getLeft()->isSigned() != bin->getRight()->isSigned())
        {
            bin->warnmsg("comparison of integers of different signs (`%s' vs `%s'); using unsigned comparison",
                         bin->getLeft()->getTypeDesc()->toString().c_str(),
                         bin->getRight()->getTypeDesc()->toString().c_str());
        }
        return processBinOp(bin);
    }

    UnaryOpExpr *un = dynamic_cast<UnaryOpExpr *>(t);
    if (un != NULL)
        return processUnaryOp(un);

    CastExpr *ce = dynamic_cast<CastExpr *>(t);
    if (ce != NULL)
    {
        if (ce->getType() == CLASS_TYPE && !ce->isNumerical())
            ce->errormsg("cannot cast to struct `%s'", ce->getTypeDesc()->toString().c_str());
        else if (ce->isReal() && ce->getSubExpr()->getTypeDesc()->isPtrOrArray())
            ce->errormsg("cannot cast `%s' to `%s'",
                         ce->getSubExpr()->getTypeDesc()->toString().c_str(),
                         ce->getTypeDesc()->toString().c_str());
        else if (ce->getTypeDesc()->isPtrOrArray() && ce->getSubExpr()->isReal())
            ce->errormsg("cannot cast `%s' to `%s'",
                         ce->getSubExpr()->getTypeDesc()->toString().c_str(),
                         ce->getTypeDesc()->toString().c_str());

        assert(ce->getType() != ARRAY_TYPE);  // no syntax for this
        return true;
    }

    FunctionCallExpr *fc = dynamic_cast<FunctionCallExpr *>(t);
    if (fc != NULL)
    {
        (void) fc->checkAndSetTypes();  // may report errors
        return true;
    }

    ConditionalExpr *cond = dynamic_cast<ConditionalExpr *>(t);
    if (cond != NULL)
    {
        // Both expressions must be of the same type, but if one of them is an 8-bit constant,
        // take it as a byte expression. This allows "char b = (cond ? 42 : 43);" without
        // a useless warning about assigning a word to a byte.
        //
        const TypeDesc *trueTD  = cond->getTrueExpression()->getTypeDesc();
        const TypeDesc *falseTD = cond->getFalseExpression()->getTypeDesc();
        if (cond->getTrueExpression()->is8BitConstant() && cond->getFalseExpression()->is8BitConstant())
            cond->setTypeDesc(TranslationUnit::getTypeManager().getIntType(BYTE_TYPE, trueTD->isSigned));
        else if (trueTD->type == BYTE_TYPE && cond->getFalseExpression()->is8BitConstant())
            cond->setTypeDesc(TranslationUnit::getTypeManager().getIntType(BYTE_TYPE, trueTD->isSigned));
        else if (cond->getTrueExpression()->is8BitConstant() && falseTD->type == BYTE_TYPE)
            cond->setTypeDesc(TranslationUnit::getTypeManager().getIntType(BYTE_TYPE, falseTD->isSigned));
        else if (trueTD->isPtrOrArray() != falseTD->isPtrOrArray())
        {
            cond->errormsg("true and false expressions of conditional are of incompatible types (%s vs %s)",
                           trueTD->toString().c_str(), falseTD->toString().c_str());
            cond->setTypeDesc(trueTD);  // fallback
        }
        else
        {
            if (trueTD->isPtrOrArray())
                cond->setTypeDesc(trueTD);  // both types must be ptr/array
            else
            {
                // The type of the result is the larger of the two types.
                // If same size, true expression's type is used.
                if (tu.getTypeSize(*trueTD) >= tu.getTypeSize(*falseTD))
                    cond->setTypeDesc(trueTD);
                else
                    cond->setTypeDesc(falseTD);
            }

            if (!TypeDesc::sameTypesModuloConst(*trueTD, *falseTD) && !trueTD->pointsToSameType(*falseTD))
                cond->warnmsg("true and false expressions of conditional are not of the same type (%s vs %s); result is of type %s",
                                trueTD->toString().c_str(), falseTD->toString().c_str(),
                                cond->getTypeDesc()->toString().c_str());
        }
        return true;
    }

    ObjectMemberExpr *om = dynamic_cast<ObjectMemberExpr *>(t);
    if (om != NULL)
    {
        Tree *subExpr = om->getSubExpr();
        if (om->isDirect() && subExpr->getType() != CLASS_TYPE)
        {
            om->errormsg("left side of dot operator must be a struct but is of type %s",
                         subExpr->getTypeDesc()->toString().c_str());
            return true;
        }
        if (!om->isDirect() && (subExpr->getType() != POINTER_TYPE
            || subExpr->getTypeDesc()->getPointedType() != CLASS_TYPE))
        {
            om->errormsg("left side of arrow operator must be a pointer to a struct but is of type %s",
                         subExpr->getTypeDesc()->toString().c_str());
            return true;
        }

        const ClassDef::ClassMember *mi = om->getClassMember();
        if (!mi)
            return true;  // error message issued

        assert(mi->getTypeDesc()->type != VOID_TYPE);
        om->setTypeDesc(mi->getTypeDesc());
        return true;
    }

    const TypeManager &tm = TranslationUnit::getTypeManager();

    // Identifier that may refer to an enumerated name or to a global variable name.
    // If an IdentifierExpr refers to something else,
    // it gets typed in ScopeCreator::processIdentifierExpr().
    //
    if (IdentifierExpr *ie = dynamic_cast<IdentifierExpr *>(t))
    {
        const VariableExpr *ve = ie->getVariableExpr();

        bool done = false;
        if (ve == NULL) // Check if this identifier refers to a global variable, give it a VariableExpr if true
        {
            const Scope &globalScope = TranslationUnit::instance().getGlobalScope();
            Declaration *decl = globalScope.getVariableDeclaration(ie->getId(), false);
            if (decl)
            {
                t->setTypeDesc(decl->getTypeDesc());

                // Give the IdentifierExpr a VariableExpr.
                VariableExpr *ve = new VariableExpr(ie->getId());
                ve->setDeclaration(decl);

                assert(decl->getType() != VOID_TYPE);
                ve->setTypeDesc(decl->getTypeDesc());
                ie->setVariableExpr(ve);  // sets the type of *ie, which was already set in this case

                done = true;
            }
        }
        if (!done)
        {
            // If the identifier is an enumerated name, we get its TypeDesc and
            // set it as the type of this IdentifierExpr.
            //
            const TypeDesc *td = tm.getEnumeratorTypeDesc(ie->getId());
            if (td && td->type != VOID_TYPE)
                t->setTypeDesc(td);

        }
    }

    // Comma expression (e.g., "x = 1, y = 2;").
    //
    if (CommaExpr *commaExpr = dynamic_cast<CommaExpr *>(t))
    {
        if (commaExpr->size() > 0)  // if at least one sub-expression
        {
            const Tree *lastSubExpr = *commaExpr->rbegin();
            const TypeDesc *subExprTD = lastSubExpr->getTypeDesc();
            if (subExprTD->type == VOID_TYPE)
                lastSubExpr->errormsg("last sub-expression of comma expression is of type void");  // not expected
            else
                t->setTypeDesc(subExprTD);
        }
    }

    return true;
}


// Size is that of larger operand.
// Signed is that of left operand.
//
static void
setBinOpTypeDescForDiffSizedOperands(BinaryOpExpr *bin)
{
    const Tree *left = bin->getLeft();
    const Tree *right = bin->getRight();
    const TypeDesc *leftTD  = left->getTypeDesc();
    const TypeDesc *rightTD = right->getTypeDesc();

    assert(leftTD->type != rightTD->type);

    size_t leftSize  =  leftTD->type == BYTE_TYPE ||  left->is8BitConstant() ? 1 : 2;
    size_t rightSize = rightTD->type == BYTE_TYPE || right->is8BitConstant() ? 1 : 2;

    BasicType resultType = (std::max(leftSize, rightSize) == 1 ? BYTE_TYPE : WORD_TYPE);
    bin->setTypeDesc(TranslationUnit::getTypeManager().getIntType(resultType, !left->isUnsignedOrPositiveConst()));
}


// If either operand is real, then the result a real type no smaller than the operands.
//
static bool
setTypeForRealOrLongOperands(BinaryOpExpr *bin, const char *opToken, const TypeDesc *leftTD, const TypeDesc *rightTD)
{
    if (leftTD->isReal() || rightTD->isReal())
    {
        if (bin->getOperator() == BinaryOpExpr::MOD || ! leftTD->isNumerical() || ! rightTD->isNumerical())
        {
            bin->errormsg("invalid use of %s with operands of types `%s' and `%s'",
                          opToken, leftTD->toString().c_str(), rightTD->toString().c_str());
            bin->setTypeDesc(leftTD);  // fallback
        }
        else
        {
            bool isResultDouble = (leftTD->isDouble() || rightTD->isDouble());
            bin->setTypeDesc(TranslationUnit::getTypeManager().getRealType(isResultDouble));
        }
        return true;
    }
    if (leftTD->isLong() || rightTD->isLong())
    {
        if (! leftTD->isNumerical() || ! rightTD->isNumerical())
        {
            bin->errormsg("invalid use of %s with operands of types `%s' and `%s'",
                          opToken, leftTD->toString().c_str(), rightTD->toString().c_str());
            bin->setTypeDesc(leftTD);  // fallback
        }
        else
        {
            bool resultIsSigned = false;
            if (leftTD->isLong() && rightTD->isLong())
                resultIsSigned = leftTD->isSigned && rightTD->isSigned;
            else if (leftTD->isLong())
                resultIsSigned = leftTD->isSigned;
            else
                resultIsSigned = rightTD->isSigned;
            bin->setTypeDesc(TranslationUnit::getTypeManager().getLongType(resultIsSigned));
        }
        return true;
    }
    return false;
}


static bool
assigningNullToPointer(const TypeDesc &leftTD, const Tree &right)
{
    return leftTD.type == POINTER_TYPE && CastExpr::isZeroCastToVoidPointer(right);
}


// This function always return true, to allow all parts of a tree to have
// their expression type set.
//
bool
ExpressionTypeSetter::processBinOp(BinaryOpExpr *bin)
{
    BinaryOpExpr::Op oper = bin->getOperator();
    const char *ot = BinaryOpExpr::getOperatorToken(oper);
    const Tree *left = bin->getLeft();
    const Tree *right = bin->getRight();
    const TypeDesc *leftTD = left->getTypeDesc();
    const TypeDesc *rightTD = right->getTypeDesc();

    assert(leftTD);
    assert(rightTD);
    if (leftTD->type == VOID_TYPE)
        left->errormsg("left side of operator %s is of type void", ot);
    if (rightTD->type == VOID_TYPE)
        right->errormsg("right side of operator %s is of type void", ot);

    switch (oper)
    {
        case BinaryOpExpr::ARRAY_REF:
            if (left->getType() != POINTER_TYPE
                && left->getType() != ARRAY_TYPE)
            {
                bin->errormsg("array reference on non array or pointer");
                return true;
            }

            bin->setTypeToPointedType(*left);
            return true;

        case BinaryOpExpr::SUB:
            if (leftTD->isPtrOrArray() && rightTD->isPtrOrArray())
            {
                if (! TypeDesc::sameTypesModuloConst(*left->getFinalArrayElementType(), *right->getFinalArrayElementType()))
                {
                    bin->errormsg("subtraction of incompatible pointers (%s vs %s)",
                                  leftTD->toString().c_str(),
                                  rightTD->toString().c_str());
                }
                bin->setTypeDesc(TranslationUnit::getTypeManager().getIntType(WORD_TYPE, false));
                return true;
            }
            if (leftTD->isPtrOrArray() && rightTD->isIntegral())
            {
                bin->setTypeDesc(leftTD);
                return true;
            }
            if (leftTD->isIntegral() && rightTD->isPtrOrArray())
            {
                bin->errormsg("subtraction of pointer or array from integral");
                bin->setTypeDesc(leftTD);
                return true;
            }
            if (   (leftTD->type == WORD_TYPE && rightTD->type == BYTE_TYPE)
                || (leftTD->type == BYTE_TYPE && rightTD->type == WORD_TYPE))
            {
                setBinOpTypeDescForDiffSizedOperands(bin);
                return true;
            }
            if (setTypeForRealOrLongOperands(bin, ot, leftTD, rightTD))
                return true;
            bin->setTypeDesc(leftTD);
            return true;

        case BinaryOpExpr::ADD:
            if (leftTD->isPtrOrArray() && rightTD->isIntegral())
            {
                bin->setTypeDesc(leftTD);
                return true;
            }
            if (leftTD->isIntegral() && rightTD->isPtrOrArray())
            {
                bin->setTypeDesc(rightTD);
                return true;
            }
            if (setTypeForRealOrLongOperands(bin, ot, leftTD, rightTD))
                return true;

            /* FALLTHROUGH */

        case BinaryOpExpr::BITWISE_OR:
        case BinaryOpExpr::BITWISE_XOR:
        case BinaryOpExpr::BITWISE_AND:
            if (leftTD->isReal() || rightTD->isReal())
            {
                bin->errormsg("invalid use of %s on a floating point type", ot);
                bin->setTypeDesc(leftTD);  // fallback
                return true;
            }
            if (leftTD->isLong() || rightTD->isLong())
            {
                bin->setTypeDesc(leftTD->isLong() ? leftTD : rightTD);
                return true;
            }
            if (leftTD->type == CLASS_TYPE || rightTD->type == CLASS_TYPE)
            {
                bin->errormsg("invalid use of %s on a struct or union", ot);
                bin->setTypeDesc(leftTD);  // fallback
                return true;
            }
            if (leftTD->isPtrOrArray() && rightTD->isIntegral())
            {
                bin->setTypeDesc(leftTD);
                return true;
            }
            if (leftTD->isIntegral() && rightTD->isPtrOrArray())
            {
                bin->setTypeDesc(rightTD);
                return true;
            }

            /* FALLTHROUGH */

        case BinaryOpExpr::MUL:
        case BinaryOpExpr::DIV:
        case BinaryOpExpr::MOD:
            if (leftTD->isPtrOrArray() || rightTD->isPtrOrArray())
            {
                bin->errormsg("operator %s cannot be applied to a pointer", ot);
                return true;
            }
            if (   (leftTD->type == WORD_TYPE && rightTD->type == BYTE_TYPE)
                || (leftTD->type == BYTE_TYPE && rightTD->type == WORD_TYPE))
            {
                setBinOpTypeDescForDiffSizedOperands(bin);
                return true;
            }
            if (setTypeForRealOrLongOperands(bin, ot, leftTD, rightTD))
                return true;
            bin->setTypeDesc(leftTD);
            return true;

        case BinaryOpExpr::EQUALITY:
        case BinaryOpExpr::INEQUALITY:
        case BinaryOpExpr::INFERIOR:
        case BinaryOpExpr::INFERIOR_OR_EQUAL:
        case BinaryOpExpr::SUPERIOR:
        case BinaryOpExpr::SUPERIOR_OR_EQUAL:
        case BinaryOpExpr::LOGICAL_AND:
        case BinaryOpExpr::LOGICAL_OR:
            bin->setTypeDesc(TranslationUnit::getTypeManager().getIntType(BYTE_TYPE, false));
            return true;

        case BinaryOpExpr::ASSIGNMENT:
        case BinaryOpExpr::INC_ASSIGN:
        case BinaryOpExpr::DEC_ASSIGN:
        case BinaryOpExpr::MUL_ASSIGN:
        case BinaryOpExpr::DIV_ASSIGN:
        case BinaryOpExpr::MOD_ASSIGN:
        case BinaryOpExpr::XOR_ASSIGN:
        case BinaryOpExpr::AND_ASSIGN:
        case BinaryOpExpr::OR_ASSIGN:
            {
                FunctionCallExpr::Diagnostic diag = FunctionCallExpr::paramAcceptsArg(*leftTD, *right);
                if (diag == FunctionCallExpr::NO_PROBLEM && leftTD->isConstant() && TranslationUnit::instance().warnOnConstIncorrect())
                    diag = FunctionCallExpr::WARN_CONST_INCORRECT;
                switch (diag)
                {
                case FunctionCallExpr::NO_PROBLEM:
                    break;
                case FunctionCallExpr::WARN_CONST_INCORRECT:
                    right->warnmsg("assigning `%s' to `%s' is not const-correct", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_NON_PTR_ARRAY_FOR_PTR:
                    if ((oper == BinaryOpExpr::INC_ASSIGN || oper == BinaryOpExpr::DEC_ASSIGN)
                        && leftTD->type == POINTER_TYPE && rightTD->isIntegral())  // accept ptr += num;
                        ;
                    else
                        right->warnmsg("assigning non-pointer/array (%s) to `%s'", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_PASSING_CONSTANT_FOR_PTR:
                    if (TranslationUnit::instance().isWarningOnPassingConstForFuncPtr())  // if -Wpass-const-for-func-pointer
                        right->warnmsg("assigning non-zero numeric constant to `%s'", leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_ARGUMENT_TOO_LARGE:
                    right->warnmsg("assigning to `%s' from larger type `%s'", leftTD->toString().c_str(), rightTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_REAL_FOR_INTEGRAL:
                    right->warnmsg("assigning real type `%s' to `%s`", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_FUNC_PTR_FOR_PTR:
                    right->warnmsg("assigning function pointer `%s' to `%s`", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARN_DIFFERENT_SIGNEDNESS:
                    right->warnmsg("assigning `%s' to `%s` changes signedness", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::WARNING_VOID_POINTER:
                    right->warnmsg("assigning `%s' to `%s' (implicit cast of void pointer)", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                case FunctionCallExpr::ERROR_MSG:
                    if (leftTD->type != VOID_TYPE && !assigningNullToPointer(*leftTD, *right))  // error message issued elsewhere
                        right->errormsg("assigning `%s' to `%s'", rightTD->toString().c_str(), leftTD->toString().c_str());
                    break;
                }
            }

            /* FALLTHROUGH */

            if (oper != BinaryOpExpr::ASSIGNMENT && (leftTD->type == CLASS_TYPE || rightTD->type == CLASS_TYPE))
            {
                bool error = false;
                switch (oper)
                {
                case BinaryOpExpr::INC_ASSIGN:
                case BinaryOpExpr::DEC_ASSIGN:
                case BinaryOpExpr::MUL_ASSIGN:
                case BinaryOpExpr::DIV_ASSIGN:
                    error = (!leftTD->isNumerical() || !rightTD->isNumerical());
                    break;
                case BinaryOpExpr::MOD_ASSIGN:
                case BinaryOpExpr::AND_ASSIGN:
                case BinaryOpExpr::OR_ASSIGN:
                case BinaryOpExpr::XOR_ASSIGN:
                    error = (!leftTD->isIntegral() || !rightTD->isIntegral());
                    break;
                default:
                    error = true;
                    break;
                }
                if (error)
                    bin->errormsg("invalid use of %s on a struct or union", ot);
            }

            /* FALLTHROUGH */

        case BinaryOpExpr::LEFT_ASSIGN:
        case BinaryOpExpr::RIGHT_ASSIGN:
        case BinaryOpExpr::LEFT_SHIFT:
        case BinaryOpExpr::RIGHT_SHIFT:
            bin->setTypeDesc(leftTD);
            return true;

        default:
            assert(false);
    }
    return true;
}


bool
ExpressionTypeSetter::checkForUnaryOnClass(const Tree &subExpr, UnaryOpExpr::Op op) const
{
    const TypeDesc *subTD = subExpr.getTypeDesc();
    if (subTD->type == CLASS_TYPE && !subTD->isLong())
    {
        subExpr.errormsg("invalid use of %s on a %s",
                          UnaryOpExpr::getOperatorName(op),
                          subTD->isReal() ? subTD->toString().c_str() : (subTD->isUnion ? "union" : "struct"));
        return false;
    }
    return true;
}


bool
ExpressionTypeSetter::processUnaryOp(UnaryOpExpr *un)
{
    Tree *subExpr = un->getSubExpr();
    const TypeDesc *subExprTD = (subExpr ? subExpr->getTypeDesc() : NULL);
    UnaryOpExpr::Op op = un->getOperator();

    const TypeManager &tm = TranslationUnit::getTypeManager();

    if (subExpr && subExpr->getType() == VOID_TYPE)
    {
        subExpr->errormsg("argument of %s operator is of type void", UnaryOpExpr::getOperatorName(op));
        un->setTypeDesc(tm.getIntType(WORD_TYPE, true));  // fall back on int
    }

    switch (op)
    {
        case UnaryOpExpr::ADDRESS_OF:
        {
            if (subExpr->getType() == ARRAY_TYPE)
            {
                un->setTypeDesc(tm.getPointerTo(subExprTD->pointedTypeDesc));  // address of T[] is T *
                return true;
            }

            const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(subExpr);
            if (ie && ie->isFuncAddrExpr())
            {
                // Operator '&' used on a function name: gives the address of that function.
                //
                un->setTypeDesc(subExprTD);
                return true;
            }

            // Note that taking the address of a pointer is supported.
            //
            un->setTypeDesc(tm.getPointerTo(subExprTD));
            return true;
        }

        case UnaryOpExpr::INDIRECTION:
            if (subExpr->getType() == VOID_TYPE)
                return true;  // error message already issued
            if (subExpr->getType() != POINTER_TYPE
                && subExpr->getType() != ARRAY_TYPE
                && subExpr->getType() != FUNCTION_TYPE)
            {
                un->setTypeDesc(tm.getPointerToVoid());
                un->errormsg("indirection using `%s' as pointer (assuming `void *')", subExprTD->toString().c_str());
                return true;
            }
            if (!checkForUnaryOnClass(*subExpr, op))  // if error message issued
                return true;
            if (subExpr->getType() == FUNCTION_TYPE)
                un->setTypeDesc(subExprTD);
            else
                un->setTypeDesc(subExprTD->pointedTypeDesc);
            return true;

        case UnaryOpExpr::SIZE_OF:
            un->setTypeDesc(tm.getIntType(WORD_TYPE, false));
            un->setSizeofArgTypeDesc();
            un->checkForSizeOfUnknownStruct();
            return true;

        case UnaryOpExpr::BOOLEAN_NEG:
            un->setTypeDesc(tm.getIntType(BYTE_TYPE, false));
            if (subExprTD->isNumerical())
                return true;
            if (!checkForUnaryOnClass(*subExpr, op))  // if error message issued
                return true;
            return true;

        case UnaryOpExpr::NEG:  // Negation always returns a signed type.
            if (subExpr->getType() == BYTE_TYPE || subExpr->getType() == WORD_TYPE)
                un->setTypeDesc(tm.getIntType(subExpr->getType(), true));
            else if (subExprTD->isReal() || subExprTD->isLong())
                un->setTypeDesc(subExprTD);  // same type
            else if (!checkForUnaryOnClass(*subExpr, op))
                un->setTypeDesc(tm.getIntType(WORD_TYPE, true));  // fall back on int, to avoid further error messages
            return true;

        case UnaryOpExpr::IDENTITY:
            if (subExprTD->isNumerical())
                un->setTypeDesc(subExprTD);  // same type
            else if (!checkForUnaryOnClass(*subExpr, op))
                un->setTypeDesc(tm.getIntType(WORD_TYPE, true));  // fall back on int, to avoid further error messages
            return true;

        case UnaryOpExpr::PREDEC:
        case UnaryOpExpr::PREINC:
        case UnaryOpExpr::POSTDEC:
        case UnaryOpExpr::POSTINC:
            un->setTypeDesc(subExprTD);  // same type
            if (! subExprTD->isNumerical())
                checkForUnaryOnClass(*subExpr, op);
            return true;

        default:
            un->setTypeDesc(subExprTD);
            checkForUnaryOnClass(*subExpr, op);
            return true;
    }
}
