/*  $Id: IfStmt.cpp,v 1.11 2017/08/26 21:11:32 sarrazip Exp $

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

#include "IfStmt.h"

#include "TranslationUnit.h"
#include "BinaryOpExpr.h"

using namespace std;


IfStmt::IfStmt(Tree *cond, Tree *conseq, Tree *alt /*= NULL*/)
  : Tree(),
    condition(cond),
    consequence(conseq),
    alternative(alt)
{
}


/*virtual*/
IfStmt::~IfStmt()
{
    delete condition;
    delete consequence;
    delete alternative;
}


/*virtual*/
void
IfStmt::checkSemantics(Functor &)
{
    if (condition->getType() == CLASS_TYPE && !condition->isRealOrLong())
        condition->errormsg("invalid use of %s as condition of if statement",
                            condition->getTypeDesc()->isUnion ? "union" : "struct");
}


/*virtual*/
CodeStatus
IfStmt::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    uint16_t value = 0;
    bool isCondConst = condition->evaluateConstantExpr(value);

    if (isCondConst && value != 0)  // if condition always true, only emit "then" clause
        return consequence->emitCode(out, lValue);

    if (isCondConst && value == 0)  // if condition always false, only emit "else" clause, if any
    {
        if (alternative != NULL && !alternative->emitCode(out, lValue))
            return false;
        return true;
    }
    
    string thenLabel = TranslationUnit::instance().generateLabel('L');
    string elseLabel = TranslationUnit::instance().generateLabel('L');

    condition->writeLineNoComment(out, "if");

    if (! BinaryOpExpr::emitBoolJumps(out, condition, thenLabel, elseLabel))
        return false;

    out.emitLabel(thenLabel, "then");

    if (!consequence->emitCode(out, false))
        return false;

    string endifLabel = TranslationUnit::instance().generateLabel('L');

    if (alternative != NULL)
        out.ins("LBRA", endifLabel, "jump over else clause");

    out.emitLabel(elseLabel, "else");
    if (alternative != NULL && !alternative->emitCode(out, false))
        return false;
    out.emitLabel(endifLabel, "end if");
    return true;
}


bool
IfStmt::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!condition->iterate(f))
        return false;
    if (!consequence->iterate(f))
        return false;
    if (alternative != NULL && !alternative->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
