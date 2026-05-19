/*  $Id: ConditionalExpr.cpp,v 1.9 2016/06/21 04:28:52 sarrazip Exp $

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

#include "ConditionalExpr.h"

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
ConditionalExpr::promoteIfNeeded(ASMText &out, const Tree &exprToPromote, const Tree &otherExpr)
{
    if (exprToPromote.getTypeDesc()->isPtrOrArray())
        return;  // both expressions are ptr/array, so promotion needed

    const TranslationUnit &tu = TranslationUnit::instance();
    if (tu.getTypeSize(*exprToPromote.getTypeDesc()) < tu.getTypeSize(*otherExpr.getTypeDesc()))
    {
        const char *extendIns = (exprToPromote.getTypeDesc()->isSigned ? "SEX" : "CLRA");  // as in C
        out.ins(extendIns, "", "cast from byte (conditional expression)");
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
    if (!condition->emitCode(out, false))  // condition is r-value
        return false;

    string falseLabel = TranslationUnit::genLabel('L');
    if (condition->getType() == BYTE_TYPE)
        out.ins("TSTB");
    else
        out.emitCMPDImmediate(0);
    out.ins("LBEQ", falseLabel, "if conditional expression is false");
    
    if (!trueExpr->emitCode(out, lValue))
        return false;

    promoteIfNeeded(out, *trueExpr, *falseExpr);

    string endLabel = TranslationUnit::genLabel('L');
    out.ins("LBRA", endLabel, "end of true expression of conditional");

    out.emitLabel(falseLabel);
    if (!falseExpr->emitCode(out, lValue))
        return false;

    promoteIfNeeded(out, *falseExpr, *trueExpr);

    out.emitLabel(endLabel);
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
ConditionalExpr::getTrueExpression() const
{
    return trueExpr;
}


const Tree *
ConditionalExpr::getFalseExpression() const
{
    return falseExpr;
}
