/*  $Id: WhileStmt.cpp,v 1.16 2018/02/02 02:55:59 sarrazip Exp $

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

#include "WhileStmt.h"

#include "TranslationUnit.h"
#include "BinaryOpExpr.h"

using namespace std;


WhileStmt::WhileStmt(Tree *cond, Tree *bodyStmt, bool isDoWhile)
  : Tree(),
    condition(cond),
    body(bodyStmt),
    isDo(isDoWhile)
{
}


/*virtual*/
WhileStmt::~WhileStmt()
{
    delete condition;
    delete body;
}


/*virtual*/
void
WhileStmt::checkSemantics(Functor &)
{
    if (condition->getType() == CLASS_TYPE && !condition->isRealOrLong())
        condition->errormsg("invalid use of %s as condition of while statement",
                            condition->getTypeDesc()->isUnion ? "union" : "struct");
}


// The code to evaluate condition is emitted after the loop body, instead of before,
// to save one branch instruction per iteration.
//
/*virtual*/
CodeStatus
WhileStmt::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    string stmtName = (isDo ? "do-while" : "while");

    bool alwaysFalse = condition->isExpressionAlwaysFalse();

    string bodyLabel = TranslationUnit::instance().generateLabel('L');
    string conditionLabel = TranslationUnit::instance().generateLabel('L');
    string endLabel = TranslationUnit::instance().generateLabel('L');

    // Remember that during the coming loop, a 'break' statement must jump to
    // endLabel and a 'continue' statement must jump to conditionLabel.
    //
    TranslationUnit::instance().pushBreakableLabels(endLabel, conditionLabel);

    // A while loop evaluates its condition first, then the body is emitted.
    // A do-while loop emits its body first, then evaluates its condition.
    //
    if (isDo || !alwaysFalse)
    {
        condition->writeLineNoComment(out, stmtName);

        if (!isDo)  // if while statement, jump over the body
            out.ins("LBRA", conditionLabel, "jump to " + string(isDo ? "do-" : "") + "while condition");

        out.emitLabel(bodyLabel, stmtName + " body");
        if (!body->emitCode(out, false))
            return false;
    }

    if (!alwaysFalse)
    {
        // Emit the condition (for a do-while) or a branch back up to the condition (for a while).
        //
        out.emitLabel(conditionLabel, stmtName + " condition at " + condition->getLineNo());

        if (condition->isExpressionAlwaysTrue())
            out.ins("LBRA", bodyLabel, "go to start of " + string(isDo ? "do-" : "") + "while body");
        else if (!BinaryOpExpr::emitBoolJumps(out, condition, bodyLabel, endLabel))
            return false;
    }

    out.emitLabel(endLabel, "after end of " + stmtName + " starting at " + condition->getLineNo());

    TranslationUnit::instance().popBreakableLabels();
    return true;
}


bool
WhileStmt::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!condition->iterate(f))
        return false;
    if (!body->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
