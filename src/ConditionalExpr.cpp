/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2026 Pierre Sarrazin <http://sarrazip.com/>

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

#include "ConditionalExpr.h"

#include "BinaryOpExpr.h"
#include "WordConstantExpr.h"
#include "TranslationUnit.h"

using namespace std;


ConditionalExpr::ConditionalExpr(Tree *_condition, Tree *_trueExpr, Tree *_falseExpr)
  : Tree(),
    condition(_condition),
    trueExpr(_trueExpr),
    falseExpr(_falseExpr)
{
}


//virtual
ConditionalExpr::~ConditionalExpr()
{
    delete falseExpr;
    delete trueExpr;
    delete condition;
}


void
ConditionalExpr::promoteIfNeeded(ASMText &out, const TypeDesc &typeToPromote, const TypeDesc &targetTypeDesc)
{
    if (typeToPromote.isPtrOrArray())
        return;

    const TranslationUnit &tu = TranslationUnit::instance();
    if (tu.getTypeSize(typeToPromote) < tu.getTypeSize(targetTypeDesc))
    {
        const char *extendIns = (typeToPromote.getConvToWordIns());  // as in C
        out.ins(extendIns, "", "promote from byte (conditional expression)");
    }
}


//virtual
CodeStatus
ConditionalExpr::emitCode(ASMText &out, bool lValue) const
{
    /*cout << "ConditionalExpr::emitCode:"
            << " " << getType()
            << " " << condition->getType()
            << " " << trueExpr->getType()
            << " " << falseExpr->getType()
            << endl;*/

    condition->writeLineNoComment(out, "conditional expression");

    string trueLabel = TranslationUnit::genLabel('L');
    string falseLabel = TranslationUnit::genLabel('L');
    string endLabel = TranslationUnit::genLabel('L');

    if (!BinaryOpExpr::emitBoolJumps(out, condition, trueLabel, falseLabel))
        return false;

    out.emitLabel(trueLabel);
    if (!emitSubExpr(out, lValue, *trueExpr))

        return false;

    promoteIfNeeded(out, *trueExpr->getTypeDesc(), *falseExpr->getTypeDesc());

    out.ins("LBRA", endLabel, "end of true expression of conditional");

    out.emitLabel(falseLabel);
    if (!emitSubExpr(out, lValue, *falseExpr))
        return false;

    promoteIfNeeded(out, *falseExpr->getTypeDesc(), *trueExpr->getTypeDesc());

    out.emitLabel(endLabel);
    return true;
}


// Try to emit code for a single byte, if possible, otherwise, emit normally.
//
CodeStatus
ConditionalExpr::emitSubExpr(ASMText &out, bool lValue, const Tree &subExpr) const
{
    uint16_t value = 0;
    if (lValue
            || getType() != BYTE_TYPE
            || ! subExpr.is8BitConstant(&value))
        return subExpr.emitCode(out, lValue);

    // subExpr fits a byte, so load it in B.
    const char *comment = "constant value in conditional expression";
    if (value == 0)
        out.ins("CLRB", "", comment);
    else
        out.ins("LDB", "#" + wordToString(value, true), comment);
    return true;
}


//virtual
bool
ConditionalExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!condition->iterate(f))
        return false;
    if (!trueExpr->iterate(f))
        return false;
    if (!falseExpr->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}


void
ConditionalExpr::replaceChild(Tree *existingChild, Tree *newChild)
{
    if (deleteAndAssign(condition, existingChild, newChild))
        return;
    if (deleteAndAssign(trueExpr, existingChild, newChild))
        return;
    if (deleteAndAssign(falseExpr, existingChild, newChild))
        return;
    assert(!"child not found");
}


const Tree *
ConditionalExpr::getConditionExpression() const
{
    return condition;
}


const Tree *
ConditionalExpr::getTrueExpression() const
{
    return trueExpr;
}


const Tree *
ConditionalExpr::getFalseExpression() const
{
    return falseExpr;
}
