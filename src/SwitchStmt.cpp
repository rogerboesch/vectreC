/*  $Id: SwitchStmt.cpp,v 1.16 2020/05/07 01:04:08 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2017 Pierre Sarrazin <http://sarrazip.com/>

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

#include "SwitchStmt.h"

#include "TranslationUnit.h"
#include "BinaryOpExpr.h"
#include "CompoundStmt.h"
#include "LabeledStmt.h"

using namespace std;


bool SwitchStmt::isJumpModeForced = false;
SwitchStmt::JumpMode SwitchStmt::forcedJumpMode = IF_ELSE;


// static
void
SwitchStmt::forceJumpMode(JumpMode _forcedJumpMode)
{
    isJumpModeForced = true;
    forcedJumpMode = _forcedJumpMode;
}


SwitchStmt::SwitchStmt(Tree *_expression, Tree *_statement)
  : Tree(),
    expression(_expression),
    statement(_statement),
    cases()
{
}


/*virtual*/
SwitchStmt::~SwitchStmt()
{
    delete expression;
    delete statement;
}


// Fills cases[].
//
/*virtual*/
void
SwitchStmt::checkSemantics(Functor & /*f*/)
{
    CompoundStmt *compoundStmt = dynamic_cast<CompoundStmt *>(statement);
    if (compoundStmt)
    {
        if (! compileLabeledStatements(*compoundStmt))
            return;  // error message already given
    }
    if (expression->isRealOrLong())
    {
        statement->errormsg("switch() expression of type `%s' is not supported",
                            expression->getTypeDesc()->toString().c_str());
        return;
    }
}


// originalCaseValueLineNumber: Defined only if method returns true.
//
bool
SwitchStmt::isDuplicateCaseValue(uint16_t caseValue, string &originalCaseValueLineNumber) const
{
    originalCaseValueLineNumber = -1;

    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
        if (!it->isDefault && it->caseValue == caseValue)
        {
            originalCaseValueLineNumber = it->lineNo;
            return true;
        }
    return false;
}


// Fills cases[].
//
bool
SwitchStmt::compileLabeledStatements(TreeSequence &statements)
{
    bool success = true, defaultSeen = false;
    for (std::vector<Tree *>::iterator it = statements.begin(); it != statements.end(); ++it)
    {
        const Tree *tree = *it;
        if (tree == NULL)
            continue;

        const LabeledStmt *labeledStmt = dynamic_cast<const LabeledStmt *>(tree);
        if (labeledStmt && !labeledStmt->isId())
        {
            while (labeledStmt && !labeledStmt->isId())
            {
                uint16_t caseValue = 0;
                const Tree *caseExpr = labeledStmt->getExpression();
                if (labeledStmt->isCase())  // if 'case':
                {
                    assert(caseExpr);
                    if (!caseExpr->evaluateConstantExpr(caseValue))
                    {
                        labeledStmt->errormsg("case statement has a variable expression");
                        success = false;
                    }
                    else
                    {
                        string originalCaseValueLineNumber;

                        if (expression->getType() == BYTE_TYPE && ! expression->isSigned() && caseValue > 0xFF)
                            caseExpr->warnmsg("switch expression is unsigned char but case value is not in range 0..255");
                        else if (expression->getType() == BYTE_TYPE && expression->isSigned() && caseValue >= 0x80 && caseValue < 0xFF80)
                            caseExpr->warnmsg("switch expression is signed char but case value is not in range -128..127");
                        else if (isDuplicateCaseValue(caseValue, originalCaseValueLineNumber))
                            caseExpr->errormsg("duplicate case value (first used at %s)", originalCaseValueLineNumber.c_str());
                    }
                }
                else
                {
                    assert(labeledStmt->isDefault());
                    assert(!caseExpr);
                    if (defaultSeen)
                    {
                        labeledStmt->errormsg("more than one default statement in switch");
                        success = false;
                    }
                    else
                        defaultSeen = true;
                }

                // Add a case to the list.
                // The 'default' case will disregard caseValue.
                //
                string caseLineNo = caseExpr ? caseExpr->getLineNo() : labeledStmt->getLineNo();
                cases.push_back(SwitchCase(caseExpr == NULL, caseValue, caseLineNo));

                const Tree *subStmt = labeledStmt->getStatement();
                const LabeledStmt *subLabeledStmt = dynamic_cast<const LabeledStmt *>(subStmt);

                // Support case A: case B: foobar;
                // This is a LabeledStmt containing a LabeledStmt containing statement foobar.
                // Push the sub-statement in the list of statements for the current case
                // EXCEPT if the sub-statement is itself a labeled statement (case B: foobar;
                // in this example). In this case, we want case A to have no statements and
                // case B to have foobar as its first statement.
                //
                if (!subLabeledStmt || subLabeledStmt->isId())
                    cases.back().statements.push_back(subStmt);

                // If the sub-statement is a LabeledStmt, loop to process it.
                //
                labeledStmt = subLabeledStmt;
            }
        }
        else  // neither case nor default:
        {
            if (cases.size() == 0)
            {
                tree->errormsg("statement in switch precedes first `case' or `default' statement");
                success = false;
            }
            else
                cases.back().statements.push_back(tree);
        }
    }
    return success;
}


bool
SwitchStmt::signedCaseValueComparator(const CaseValueAndIndexPair &a, const CaseValueAndIndexPair &b)
{
    return int16_t(a.first) < int16_t(b.first);
}


bool
SwitchStmt::unsignedCaseValueComparator(const CaseValueAndIndexPair &a, const CaseValueAndIndexPair &b)
{
    return a.first < b.first;
}


// CaseValueType: int16_t or uint16_t.
// caseValues: Must not be empty.
//
template <typename CaseValueType>
static void
emitJumpTableEntries(ASMText &out,
                     const vector<SwitchStmt::CaseValueAndIndexPair> &caseValues,
                     const vector<string> &caseLabels,
                     CaseValueType minValue,
                     CaseValueType maxValue,
                     const string &tableLabel,
                     const string &defaultLabel)
{
    assert(caseValues.size() != 0);
    assert(minValue <= maxValue);  // at least one table entry to emit

    size_t vectorIndex = 0;
    for (CaseValueType value = minValue; ; ++value)
    {
        assert(vectorIndex < caseValues.size());
        CaseValueType currentCaseValue = (CaseValueType) caseValues[vectorIndex].first;
        if (value < currentCaseValue)
            out.ins("FDB", defaultLabel + "-" + tableLabel);
        else
        {
            out.ins("FDB", caseLabels[caseValues[vectorIndex].second] + "-" + tableLabel);
            ++vectorIndex;
        }

        // 'value' might be highest valid value for CaseValueType, so test before incrementing
        if (value == maxValue)
            break;
    }
}


/*virtual*/
CodeStatus
SwitchStmt::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    expression->writeLineNoComment(out, "switch");

    TranslationUnit &tu = TranslationUnit::instance();

    string endSwitchLabel = tu.generateLabel('L');

    if (! expression->emitCode(out, lValue))
        return false;

    bool exprIsByte = (expression->getType() == BYTE_TYPE);
    const char *cmpInstr = (exprIsByte ? "CMPB" : "CMPD");

    // Generate a label for each case and for the default case.
    vector<string> caseLabels;
    string defaultLabel;
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
    {
        const SwitchCase &c = *it;

        string caseLabel = TranslationUnit::instance().generateLabel('L');
        caseLabels.push_back(caseLabel);
        if (c.isDefault)
            defaultLabel = caseLabel;
    }

    assert(caseLabels.size() == cases.size());

    if (defaultLabel.empty())  // if no default seen:
        defaultLabel = endSwitchLabel;

    // Get an ordered list of non-default case values, each with the corresponding index in caseLabels[].
    vector<CaseValueAndIndexPair> caseValues;
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
        if (!it->isDefault)
            caseValues.push_back(make_pair(it->caseValue, it - cases.begin()));
    sort(caseValues.begin(), caseValues.end(),
         expression->isSigned() ? signedCaseValueComparator : unsignedCaseValueComparator);

    size_t ifElseCost = computeJumpModeCost(IF_ELSE, caseValues);
    size_t jumpTableCost = computeJumpModeCost(JUMP_TABLE, caseValues);

    JumpMode jumpMode = (isJumpModeForced ? forcedJumpMode : (ifElseCost <= jumpTableCost ? IF_ELSE : JUMP_TABLE));

    // Override isJumpModeForced if jump table cost is way higher.
    if (jumpTableCost > ifElseCost && jumpTableCost - ifElseCost >= 256)
        jumpMode = IF_ELSE;

    out.emitComment("Switch at " + expression->getLineNo() + ": IF_ELSE=" + dwordToString(uint32_t(ifElseCost)) + ", JUMP_TABLE=" + dwordToString(uint32_t(jumpTableCost)));

    // Emit the switching code.
    //
    switch (jumpMode)
    {
    case IF_ELSE:
        // Emit a series of comparisons and conditional branches:
        //      CMPr #caseValue1
        //      LBEQ label1
        //      etc.
        //
        for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
        {
            const SwitchCase &c = *it;

            if (!c.isDefault)
            {
                if (exprIsByte && int16_t(c.caseValue) > 0xFF)
                    ;  // no match possible: don't generate CMP+LBEQ
                else
                {
                    uint16_t caseValue = c.caseValue;
                    if (exprIsByte)
                        caseValue &= 0xFF;
                    out.ins(cmpInstr, "#" + wordToString(caseValue, true), "case " + wordToString(caseValue, false));
                    out.ins("LBEQ", caseLabels[it - cases.begin()]);
                }
            }
        }

        out.ins("LBRA", defaultLabel, "switch default");
        break;
    case JUMP_TABLE:
        if (caseValues.size() == 0)
        {
            out.ins("LBRA", defaultLabel, "switch default (no case statements)");
        }
        else
        {
            if (exprIsByte)
                out.ins(expression->getConvToWordIns());  // always use a word expression
            string tableLabel = TranslationUnit::instance().generateLabel('L');
            out.ins("LEAX", tableLabel + ",PCR", "jump table for switch at " + expression->getLineNo());
            const char *routine = expression->isSigned() ? "signedJumpTableSwitch" : "unsignedJumpTableSwitch";
            out.emitImport(routine);
            TranslationUnit::instance().registerNeededUtility(routine);
            out.ins("LBRA", routine);

            // Pre-table data: minimum and maximum case value, default label offset.
            // Offsets are used instead of directly using the label, to preserve
            // the relocatability of the program.
            uint16_t minValue = 0, maxValue = 0;
            if (expression->isSigned())
                getSignedMinAndMaxCaseValues(minValue, maxValue);
            else
                getUnsignedMinAndMaxCaseValues(minValue, maxValue);
            out.ins("FDB", expression->isSigned() ? intToString(minValue) : wordToString(minValue), "minimum case value");
            out.ins("FDB", expression->isSigned() ? intToString(maxValue) : wordToString(maxValue), "maximum case value");
            out.ins("FDB", defaultLabel + "-" + tableLabel, "default label");

            out.emitLabel(tableLabel);

            // Emit an offset for each case in the interval going from minValue to maxValue.
            if (expression->isSigned())
                emitJumpTableEntries(out, caseValues, caseLabels, int16_t(minValue), int16_t(maxValue), tableLabel, defaultLabel);
            else
                emitJumpTableEntries(out, caseValues, caseLabels, minValue, maxValue, tableLabel, defaultLabel);
        }
        break;
    }

    pushScopeIfExists();
    tu.pushBreakableLabels(endSwitchLabel, "");  // continue statement is not supported in a switch

    // Emit the code for the switch() body.
    //
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
    {
        const SwitchCase &c = *it;

        const string &caseLabel = caseLabels[it - cases.begin()];
        string comment;
        if (c.isDefault)
            comment = "default";
        else
            comment = "case " + wordToString(c.caseValue, false);

        out.emitLabel(caseLabel, comment);

        for (std::vector<const Tree *>::const_iterator jt = c.statements.begin(); jt != c.statements.end(); ++jt)
            if (! (*jt)->emitCode(out, lValue))
                return false;
    }

    tu.popBreakableLabels();
    popScopeIfExists();

    out.emitLabel(endSwitchLabel, "end of switch");
    return true;
}


void
SwitchStmt::getSignedMinAndMaxCaseValues(uint16_t &minValue, uint16_t &maxValue) const
{
    int16_t m = 32767, M = -32768;
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
    {
        const SwitchCase &c = *it;
        m = std::min(m, int16_t(c.caseValue));
        M = std::max(M, int16_t(c.caseValue));
    }
    minValue = uint16_t(m);
    maxValue = uint16_t(M);
}


void
SwitchStmt::getUnsignedMinAndMaxCaseValues(uint16_t &minValue, uint16_t &maxValue) const
{
    minValue = 0xFFFF, maxValue = 0;
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
    {
        const SwitchCase &c = *it;
        minValue = std::min(minValue, c.caseValue);
        maxValue = std::max(maxValue, c.caseValue);
    }
}


// caseValues: Must be sorted by case value.
//
size_t
SwitchStmt::computeJumpModeCost(JumpMode jumpMode,
                                const vector<CaseValueAndIndexPair> &caseValues) const
{
    if (caseValues.size() == 0)
        return 0;

    switch (jumpMode)
    {
    case IF_ELSE:
        {
            // Cost of CMPB/CMPD with immediate argument (assumes either byte or word):
            size_t cmpCost = (expression->getType() == BYTE_TYPE ? 2 : 4);

            // LBEQ takes 4 bytes. LBRA (for default case) takes 3.
            return caseValues.size() * (cmpCost + 4) + 3;
        }
    case JUMP_TABLE:
        {
            uint16_t minValue = caseValues.front().first;
            uint16_t maxValue = caseValues.back().first;
            uint16_t numTableEntries = 1;
            if (expression->isSigned())
                numTableEntries += uint16_t(int16_t(maxValue) - int16_t(minValue));
            else
                numTableEntries += maxValue - minValue;

            // LEAX takes 4 bytes. LBRA takes 3. Each table entry is 2 bytes.
            // 3 entries are added for the minimum value, maximum value, and default case offset.
            // The cost of the signedJumpTableSwitch/unsignedJumpTableSwitch routine is difficult
            // to factor in because that cost is spread among all the switches, across the whole
            // program, that use a jump table. Since we may be compiling only one module of a
            // program, we cannot know the total number of switches.
            // Here, we blindly guess that there will be 5 switches and that the routine is 30 bytes,
            // so we add 6 as a fudge factor.
            //
            size_t promotionCost = (expression->getType() == BYTE_TYPE ? 1 : 0);  // CLRA/SEX if needed
            return promotionCost + 4 + 3 + 2 * (3 + numTableEntries) + 6;
        }
    }
    return 0;
}


bool
SwitchStmt::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!expression->iterate(f))
        return false;
    if (!statement->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
