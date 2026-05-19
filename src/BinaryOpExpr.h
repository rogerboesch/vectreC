/*  $Id: BinaryOpExpr.h,v 1.42 2019/10/19 03:26:48 sarrazip Exp $

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

#ifndef _H_BinaryOpExpr
#define _H_BinaryOpExpr

#include "Tree.h"

class VariableExpr;
class WordConstantExpr;


class BinaryOpExpr : public Tree
{
public:

    enum Op
    {
        ADD, SUB, MUL, DIV, MOD,
        EQUALITY, INEQUALITY,
        INFERIOR, INFERIOR_OR_EQUAL,
        SUPERIOR, SUPERIOR_OR_EQUAL,
        LOGICAL_AND, LOGICAL_OR,
        BITWISE_OR, BITWISE_XOR, BITWISE_AND,
        ASSIGNMENT, INC_ASSIGN, DEC_ASSIGN,
        MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN,
        XOR_ASSIGN, AND_ASSIGN, OR_ASSIGN,
        LEFT_ASSIGN, RIGHT_ASSIGN,
        LEFT_SHIFT, RIGHT_SHIFT,
        ARRAY_REF
    };

    static const char *getOperatorName(Op op);

    BinaryOpExpr(Op op, Tree *left, Tree *right);

    virtual ~BinaryOpExpr();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    Op getOperator() const;

    bool isRelationalOperator() const;

    bool isOrderComparisonOperator() const;

    Tree *getLeft() const;
    Tree *getRight() const;

    CodeStatus emitComparison(ASMText &out,
                                bool produceIntegerResult,
                                const std::string &condBranchInstr) const;

    static CodeStatus emitBoolJumps(ASMText &out,
                                    Tree *condition,
                                    const std::string &successLabel,
                                    const std::string &failureLabel);

    static const char *getOperatorToken(Op op);

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild);

    virtual bool isLValue() const { return oper == ASSIGNMENT || oper == INC_ASSIGN || oper == DEC_ASSIGN
                                        || oper == MUL_ASSIGN || oper == DIV_ASSIGN || oper == MOD_ASSIGN
                                        || oper == XOR_ASSIGN || oper == AND_ASSIGN || oper == OR_ASSIGN
                                        || oper == LEFT_ASSIGN || oper == RIGHT_ASSIGN
                                        || oper == ARRAY_REF; }

private:

    bool emitIntegralComparisonIfNoFuncAddrExprInvolved(ASMText &out) const;
    bool emitUnsignedComparisonOfByteExprWithByteConstant(ASMText &out) const;
    bool emitAssignmentIfNoFuncAddrExprInvolved(ASMText &out,
                                                bool lValue,
                                                std::string &assignedValueArg) const;
    CodeStatus emitRealOrLongComparison(ASMText &out) const;
    CodeStatus emitNullPointerComparison(ASMText &out, const Tree &ptrExpr, bool invertRelationalOperator) const;
    bool isSignedComparison() const;

    // Forbidden:
    BinaryOpExpr(const BinaryOpExpr &);
    BinaryOpExpr &operator = (const BinaryOpExpr &);

private:

    Op oper;
    Tree *subExpr0;  // owns the Tree object
    Tree *subExpr1;  // owns the Tree object
    int16_t numBytesPerElement;
    class Declaration *resultDeclaration;  // used when result is real number

private:

    CodeStatus emitAddImmediateToVariable(ASMText &out,
                                          const VariableExpr *ve0,
                                          uint16_t imm) const;
    CodeStatus emitSubExpressions(ASMText &out, bool reverseOrder = false) const;
    bool isArrayRefAndLongSubscript(const Tree *&arrayTree, const Tree *&subscriptTree) const;

    CodeStatus emitBitwiseOperation(ASMText &out, bool lValue, Op op) const;
    template <typename BinaryFunctor>
    bool emitBinOpIfConstants(ASMText &out, BinaryFunctor f) const;
    static void emitAddIntegerToPointer(ASMText &out, const Tree *subExpr0, bool doSub);
    static char emitNumericalExpr(ASMText &out, const Tree &expr, bool pushRegister = true);
    bool isRealAndLongOperation() const;
    CodeStatus emitRealOrLongOp(ASMText &out, const char *opName, bool pushAddressOfLeftOperand = false) const;
    CodeStatus emitSignedDivOrModOnLong(ASMText &out, bool isDivision) const;
    CodeStatus emitAdd(ASMText &out, bool lValue, bool doSub) const;
    CodeStatus emitMulDivMod(ASMText &out, bool lValue) const;
    bool emitMulOfTypeUnsignedBytesGivingUnsignedWord(ASMText &out) const;
    CodeStatus emitLogicalAnd(ASMText &out, bool lValue) const;
    CodeStatus emitLogicalOr(ASMText &out, bool lValue) const;
    CodeStatus emitShift(ASMText &out, bool isLeftShift, bool changeLeftSide, bool lValue) const;
    CodeStatus emitLongBitwiseOpAssign(ASMText &out) const;
    CodeStatus emitLeftSideAddressInX(ASMText &out, bool preserveD) const;
    CodeStatus emitAssignment(ASMText &out, bool lValue, Op op) const;
    static bool isArrayOrPointerVariable(const Tree *tree);
    static int16_t getNumBytesPerMultiDimArrayElement(const Tree *tree);
    CodeStatus emitArrayRef(ASMText &out, bool lValue) const;
    bool optimizeConstantAddressCase(ASMText &out, const std::string &assignedValueArg) const;
    bool optimizeVariableCase(ASMText &out, const std::string &assignedValueArg) const;

};


#endif  /* _H_BinaryOpExpr */
