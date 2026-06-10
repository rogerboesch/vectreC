/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2025 Pierre Sarrazin <http://sarrazip.com/>

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
#include "DWordConstantExpr.h"

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
    const CompoundStmt *compoundStmt = dynamic_cast<CompoundStmt *>(statement);
    if (compoundStmt)
    {
        if (! compileLabeledStatements())
            return;  // error message already given
    }
    if (expression->isReal())
    {
        expression->errormsg("switch() expression of type `%s' is not supported",
                            expression->getTypeDesc()->toString().c_str());
        return;
    }
}


// originalCaseValueLineNumber: Defined only if method returns true.
//
bool
SwitchStmt::isDuplicateCaseValue(uint32_t caseValue, string &originalCaseValueLineNumber) const
{
    originalCaseValueLineNumber.clear();

    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
        if (!it->isDefault && it->caseValue == caseValue)
        {
            originalCaseValueLineNumber = it->lineNo;
            return true;
        }
    return false;
}


std::pair<bool, uint32_t>
SwitchStmt::checkCaseExprAndGetValue(const Tree *caseExpr) const
{
    bool success = true;
    uint32_t caseValue = 0;
    uint16_t wordValue = 0;
    if (auto dwce = dynamic_cast<const DWordConstantExpr *>(caseExpr))
    {
        caseValue = dwce->getDWordValue();
        string originalCaseValueLineNumber;
        if (isDuplicateCaseValue(caseValue, originalCaseValueLineNumber))
            caseExpr->errormsg("duplicate case value (first used at %s)", originalCaseValueLineNumber.c_str());
    }
    else if (!caseExpr->evaluateConstantExpr(wordValue))
    {
        caseExpr->errormsg("case statement has a variable expression");
        success = false;
    }
    else
    {
        if (caseExpr->isSigned())
            caseValue = uint32_t(int32_t(int16_t(wordValue)));
        else
            caseValue = uint32_t(wordValue);

        string originalCaseValueLineNumber;

        if (expression->getType() == BYTE_TYPE && ! expression->isSigned() && caseValue > 0xFF)
            caseExpr->warnmsg("switch expression is unsigned char but case value is not in range 0..255");
        else if (expression->getType() == BYTE_TYPE && expression->isSigned()
                        && (int32_t(caseValue) < -128 || int32_t(caseValue) > 127))
            caseExpr->warnmsg("switch expression is signed char but case value is not in range -128..127");
        else if (isDuplicateCaseValue(caseValue, originalCaseValueLineNumber))
        {
            caseExpr->errormsg("duplicate case value (first used at %s)", originalCaseValueLineNumber.c_str());
            success = false;
        }
    }
    return make_pair(success, caseValue);
}


class SwitchStmt::SwitchCaseCollector : public Tree::Functor
{
public:
    SwitchCaseCollector(SwitchStmt *_switchStmt, SwitchCaseList &_cases)
        : switchStmt(_switchStmt), cases(_cases) {}

    bool open(Tree *tree) override
    {
        if (dynamic_cast<SwitchStmt *>(tree))
            ++innerSwitchLevel;

        if (innerSwitchLevel > 0)
            return true;  // ignore inner switch statements

        const LabeledStmt *labeledStmt = dynamic_cast<LabeledStmt *>(tree);
        if (labeledStmt && !labeledStmt->isId())
        {
            firstCaseOrDefaultSeen = true;

            // Add a case to the list.
            // The 'default' case will disregard caseValue.
            //
            const Tree *caseExpr = labeledStmt->getExpression();
            std::pair<bool, uint32_t> result(true, 0);
            if (caseExpr)
            {
                result = switchStmt->checkCaseExprAndGetValue(caseExpr);
                if (!result.first)  // if failure
                {
                    success = false;
                    return false;
                }
            }
            else
            {
                if (defaultSeen)
                {
                    labeledStmt->errormsg("more than one default statement in switch");
                    success = false;
                }
                else
                    defaultSeen = true;
            }
            uint32_t caseValue = (result.first ? result.second : 0);
            string caseLineNo = caseExpr ? caseExpr->getLineNo() : labeledStmt->getLineNo();
            cases.push_back(SwitchCase(caseExpr == NULL, caseValue, caseLineNo, *labeledStmt));
        }
        else if (tree != switchStmt->statement)  // neither case nor default nor the switch's compound stmt
        {
            if (!firstCaseOrDefaultSeen)
            {
                tree->warnmsg("statement in switch precedes first `case' or `default' statement");
                success = false;
            }
        }
        return true;
    }

    bool close(Tree *tree) override
    {
        if (innerSwitchLevel > 0 && dynamic_cast<SwitchStmt *>(tree))
            --innerSwitchLevel;
        return true;
    }

    bool success = true;

private:
    SwitchCaseCollector(const SwitchCaseCollector &) = delete;
    SwitchCaseCollector &operator = (const SwitchCaseCollector &) = delete;

    SwitchStmt *switchStmt;
    SwitchCaseList &cases;
    size_t innerSwitchLevel = 0;
    bool defaultSeen = false;
    bool firstCaseOrDefaultSeen = false;  // even if invalid one
};


// Fills cases[].
//
bool
SwitchStmt::compileLabeledStatements()
{
    SwitchCaseCollector collector(this, cases);
    statement->iterate(collector);
    return collector.success;
}


bool
SwitchStmt::signedCaseValueComparator(const CaseValueAndIndexPair &a, const CaseValueAndIndexPair &b)
{
    return int32_t(a.first) < int32_t(b.first);
}


bool
SwitchStmt::unsignedCaseValueComparator(const CaseValueAndIndexPair &a, const CaseValueAndIndexPair &b)
{
    return a.first < b.first;
}


// CaseValueType: int32_t or uint32_t.
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
            out.ins("FDB", caseLabels[caseValues[vectorIndex].second] + "-" + tableLabel,
                                        "case " + dwordToString(currentCaseValue));
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
    {
        errormsg("compiler error: cannot emit a switch() as an l-value");
        return false;
    }

    expression->writeLineNoComment(out, "switch");

    TranslationUnit &tu = TranslationUnit::instance();

    string endSwitchLabel = tu.generateLabel('L');

    const bool isLongSwitch = expression->isLong();
    if (! expression->emitCode(out, isLongSwitch))  // as l-value iff 32 bits
        return false;
    if (isLongSwitch)
        out.ins("TFR", "X,D", "use D to pass address of switch expr dword to utility routine");

    // Here, B or D is the expression value if it is a byte or word;
    // X points to the expression value if it is a dword.

    const bool exprIsByte = (expression->getType() == BYTE_TYPE);
    const char *cmpInstr = (isLongSwitch ? NULL : (exprIsByte ? "CMPB" : "CMPD"));

    // Generate a label for each case and for the default case.
    vector<string> caseLabels;
    string defaultLabel;
    for (SwitchCaseList::const_iterator it = cases.begin(); it != cases.end(); ++it)
    {
        const SwitchCase &c = *it;

        const string &caseLabel = c.labeledStmt.getAssemblyLabel();
        caseLabels.push_back(caseLabel);
        if (c.isDefault)
            defaultLabel = caseLabel;
    }

    assert(caseLabels.size() == cases.size());

    if (defaultLabel.empty())  // if no default seen:
        defaultLabel = endSwitchLabel;

    // Get an ordered list of non-default case values, each with the corresponding index in caseLabels[].
    vector<CaseValueAndIndexPair> caseValues;
    for (const SwitchCase &c : cases)
        if (!c.isDefault)
        {
            if (exprIsByte && expression->isSigned() && (int32_t(c.caseValue) < -128 || int32_t(c.caseValue) > 127))
                out.emitComment("Switch case at " + c.lineNo + " ignored.");  // no match possible
            else if (exprIsByte && ! expression->isSigned() && c.caseValue > 255)
                out.emitComment("Switch case at " + c.lineNo + " ignored.");  // no match possible
            else
                caseValues.push_back(make_pair(c.caseValue, &c - &cases[0]));
        }
    out.emitComment("Switch at " + expression->getLineNo() + " has " + dwordToString(caseValues.size()) + " non-ignored case values.");
    sort(caseValues.begin(), caseValues.end(),
         expression->isSigned() ? signedCaseValueComparator : unsignedCaseValueComparator);

    size_t ifElseCost = computeJumpModeCost(IF_ELSE, caseValues);
    size_t jumpTableCost = computeJumpModeCost(JUMP_TABLE, caseValues);

    JumpMode jumpMode = (isJumpModeForced ? forcedJumpMode : (ifElseCost <= jumpTableCost ? IF_ELSE : JUMP_TABLE));

    // Override isJumpModeForced if jump table cost is way higher.
    // Also override if long switch, b/c dword jump table routine not implemented.
    bool overrideJumpMode = (jumpTableCost > ifElseCost && jumpTableCost - ifElseCost >= 256);
    if (overrideJumpMode)
        jumpMode = IF_ELSE;

    stringstream comment;
    comment << "Switch at " << expression->getLineNo()
                            << " uses " << (jumpMode ? "jump table" : "if-else sequence")
                            << ": IF_ELSE=" << ifElseCost
                            << ", JUMP_TABLE=" << jumpTableCost;
    if (forcedJumpMode)
        comment << " (forced jump mode)";
    if (overrideJumpMode)
        comment << " (overridden)";
    out.emitComment(comment.str());

    // Emit the switching code.
    //
    if (caseValues.size() == 0)
    {
        out.ins("LBRA", defaultLabel, "switch default (no case statements)");
    }
    else
    {
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
                    out.emitComment("Switch case at " + c.lineNo);
                    const string &caseLabel = caseLabels[it - cases.begin()];
                    uint32_t caseValue = c.caseValue;
                    if (exprIsByte)
                        caseValue &= 0xFF;

                    if (isLongSwitch)
                    {
                        // X is assumed to point to the 32-bit switch expression.

                        DWordConstantExpr dwordCE(expression->isSigned() ? int32_t(caseValue) : caseValue,
                                                  expression->isSigned());
                        string dwordLabel = TranslationUnit::instance().registerDWordConstant(dwordCE);
                        dwordCE.setLabel(dwordLabel);
                        out.ins("LDD", "#" + dwordLabel + "-(*+3+3)", "address of case value, relative to return address pushed by upcoming LBSR");
                        callUtility(out, "cmpDWordSwitchExpr", "compare switch expression at X to case value designated by D");
                    }
                    else
                    {
                        out.ins(cmpInstr, "#" + wordToString(caseValue, true), "case " + wordToString(caseValue, false));
                    }
                    out.ins("LBEQ", caseLabel, c.labeledStmt.getLineNo());
                }
            }

            out.ins("LBRA", defaultLabel, "switch default");
            break;

        case JUMP_TABLE:
            assert(!(isLongSwitch && exprIsByte));  // either long or byte, not both
            if (exprIsByte)
                out.ins(expression->getConvToWordIns());  // always use a word expression
            string tableLabel = TranslationUnit::instance().generateLabel('L');
            out.ins("LEAX", tableLabel + ",PCR", "jump table for switch at " + expression->getLineNo());
            const string routine = string(expression->isSigned() ? "signed" : "unsigned") + (isLongSwitch ? "DWord" : "") + "JumpTableSwitch";
            out.emitImport(routine);
            TranslationUnit::instance().registerNeededUtility(routine);
            out.ins("LBRA", routine);

            // Pre-table data: minimum and maximum case values, default label offset.
            // Offsets are used instead of directly using the label, to preserve
            // the relocatability of the program.
            uint32_t minValue = 0, maxValue = 0;
            if (expression->isSigned())
                getSignedMinAndMaxCaseValues(minValue, maxValue);
            else
                getUnsignedMinAndMaxCaseValues(minValue, maxValue);
            if (isLongSwitch)
            {
                DWordConstantExpr::emitDWordConstantDefinition(out, minValue);
                DWordConstantExpr::emitDWordConstantDefinition(out, maxValue);
            }
            else
            {
                out.ins("FDB", expression->isSigned() ? intToString(int16_t(minValue)) : wordToString(uint16_t(minValue)), "minimum case value");
                out.ins("FDB", expression->isSigned() ? intToString(int16_t(maxValue)) : wordToString(uint16_t(maxValue)), "maximum case value");
            }
            out.ins("FDB", defaultLabel + "-" + tableLabel, "offset of default label");

            out.emitLabel(tableLabel);

            // Emit an offset for each case in the interval going from minValue to maxValue.
            if (expression->isSigned())
                emitJumpTableEntries(out, caseValues, caseLabels, int32_t(minValue), int32_t(maxValue), tableLabel, defaultLabel);
            else
                emitJumpTableEntries(out, caseValues, caseLabels, minValue, maxValue, tableLabel, defaultLabel);
            break;
        }
    }

    pushScopeIfExists();
    tu.pushBreakableLabels(endSwitchLabel, "");  // continue statement is not supported in a switch

    // Emit the code for the switch() body, which will emit the labels.
    //
    if (statement && ! statement->emitCode(out, false))
        return false;

    tu.popBreakableLabels();
    popScopeIfExists();

    out.emitLabel(endSwitchLabel, "end of switch");
    return true;
}


void
SwitchStmt::getSignedMinAndMaxCaseValues(uint32_t &minValue, uint32_t &maxValue) const
{
    int32_t m = 0x7FFFFFFFl, M = 0x80000000l;
    for (const SwitchCase &c : cases)
        if (!c.isDefault)
        {
            m = std::min(m, int32_t(c.caseValue));
            M = std::max(M, int32_t(c.caseValue));
        }
    minValue = uint32_t(m);
    maxValue = uint32_t(M);
}


void
SwitchStmt::getUnsignedMinAndMaxCaseValues(uint32_t &minValue, uint32_t &maxValue) const
{
    minValue = 0xFFFFFFFFul, maxValue = 0;
    for (const SwitchCase &c : cases)
        if (!c.isDefault)
        {
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
            uint32_t minValue = caseValues.front().first;
            uint32_t maxValue = caseValues.back().first;
            size_t numTableEntries = 1;
            if (expression->isSigned())
                numTableEntries += uint32_t(int32_t(maxValue) - int32_t(minValue));
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
