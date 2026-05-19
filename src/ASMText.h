/*  $Id: ASMText.h,v 1.68 2020/05/13 23:23:50 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2018 Pierre Sarrazin <http://sarrazip.com/>
    Copyright (C) 2016 Jamie Cho <https://github.com/jamieleecho>

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

#ifndef _H_ASMText
#define _H_ASMText

#include "util.h"
#include <stack>
#include <utility>
#include <algorithm>
#include <map>


// Internal representation of the assembly language program.
// The ins() and "emit" methods accumulate elements in memory,
// then the writeFile() method writes the assembly to a text file.
// Before calling writeFile(), optimizations can be made.
//
class ASMText
{
public:
    ASMText();
    void ins(const std::string &instr, const std::string &arg = "", const std::string &comment = "");
    void emitCMPDImmediate(uint16_t immediateValue, const std::string &comment = "");  // calls ins()
    void emitFunctionStart(const std::string &functionId, const std::string &lineNo);
    void emitFunctionEnd(const std::string &functionId, const std::string &lineNo);
    void emitInlineAssembly(const std::string &text);
    void emitLabel(const std::string &label, const std::string &comment = "");
    void emitComment(const std::string &text);
    void emitSeparatorComment();
    void emitInclude(const std::string &filename);
    void startSection(const char *sectionName);
    void endSection();
    void emitExport(const char *label);
    void emitExport(const std::string &label) { emitExport(label.c_str()); }
    void emitImport(const char *label);
    void emitImport(const std::string &label) { emitImport(label.c_str()); }
    void emitEnd();

    void optimizeWholeFunctions();
    void peepholeOptimize(bool useStage2Optims);

    // Writes assembly text into 'out'.
    // Does not close 'out'.
    // Returns out.good().
    //
    bool writeFile(std::ostream &out);

    // ins: Comparison is case-insensitive. Long branches are also recognized.
    //      BRA and BRN are not considered to be conditional branches.
    //
    static bool isConditionalBranch(const char *ins);

    enum Type { INSTR, LABEL, INLINE_ASM, COMMENT, SEPARATOR, INCLUDE,
                FUNCTION_START, FUNCTION_END, SECTION_START, SECTION_END, EXPORT, IMPORT, END };

    // An 'Element' is an instruction, a label line, a comment line, etc.
    //
    struct Element
    {
        Type type;
        std::string fields[3];
        uint8_t liveRegs;       // registers that are live BEFORE this element (bit field based on register enum)

        Element() : type(COMMENT), fields(), liveRegs(0) {}
        bool isCommentLike() const { return type != INSTR && type != LABEL && type != INLINE_ASM && type != INCLUDE; }
    };

    // Effects of an instruction on some registers.
    class InsEffects
    {
    public:
        // These two fields do not register changes to PC and DP.
        uint8_t read;
        uint8_t written;

        InsEffects(const Element &e);
        std::string toString() const;
    private:
        static uint8_t parsePushPullArg(const std::string &arg);
        static bool onlyDecimalDigits(const std::string &s);
        static bool onlyHexDigits(const std::string &s, size_t offset);
    };

private:

    static std::string listRegisters(uint8_t registers);
    static uint8_t parseRegName(const char *name);
    static void getRegPairNames(const std::string &arg, uint8_t &firstReg, uint8_t &secondReg);

    enum { INSTR_NAME_BUFSIZ = 8 };  // length in bytes of an array that contains an instruction name

    void addElement(Type type, const std::string &field0 = "", const std::string &field1 = "", const std::string &field2 = "");

    // Actual assembly writing methods:
    void writeElement(std::ostream &out, const Element &e);
    void writeIns(std::ostream &out, const Element &e);
    void writeLabel(std::ostream &out, const Element &e);
    void writeInlineAssembly(std::ostream &out, const Element &e);
    void writeComment(std::ostream &out, const Element &e);
    void writeSeparatorComment(std::ostream &out, const Element &e);
    void writeInclude(std::ostream &out, const Element &e);

    // Optimization names:
    bool branchToNextLocation(size_t index);
    bool instrFollowingUncondBranch(size_t index);
    bool lddToLDB(size_t index);
    bool pushLoadDiscardAdd(size_t index) const;
    bool pushBLoadAdd(size_t index) const;
    bool pushDLoadAdd(size_t index) const;
    bool pushLoadDLoadX(size_t index) const;
    bool pushDLoadXLoadD(size_t index) const;
    bool loadCmpZeroBeqOrBne(size_t index);
    bool pushWordForByteComparison(size_t index);
    bool stripConsecOppositeTFRs(size_t index);
    bool stripOpToDeadReg(size_t index);
    bool stripUselessPushPull(size_t index);
    bool stripConsecutiveLoadsToSameReg(size_t &index);
    bool storeLoad(size_t &index);
    bool condBranchOverUncondBranch(size_t &index);
    bool shortenBranch(size_t index);
    bool fasterPointerIndexing(size_t index);
    bool fasterPointerPushing(size_t index);
    bool stripExtraClrA_B(size_t index);
    bool stripExtraPulsX(size_t index);
    bool stripExtraPushPullB(size_t index);
    bool andA_B0(size_t index);
    bool changeLoadDToLoadB(size_t index);
    bool changeAddDToAddB(size_t index);
    bool stripPushLeas1(size_t index);
    bool orAndA_B(size_t index);
    bool loadDToClrALoadB(size_t index);
    bool optimizeStackOperations1(size_t index);
    bool optimizeStackOperations2(size_t index);
    bool optimizeStackOperations3(size_t index);
    bool optimizeStackOperations4(size_t index);
    bool optimizeStackOperations5(size_t index);
    bool removeClr(size_t index);
    bool removeAndOrMulAddSub(size_t index);
    bool optimizeLoadDX(size_t index);
    bool optimizeTfrPush(size_t index);
    bool optimizeTfrOp(size_t index);
    bool removePushB(size_t index);
    bool optimizeLdbTfrClrb(size_t index);
    bool remove16BitStackOperation(size_t index);
    bool optimizePostIncrement(size_t index);
    bool removeUselessOps(size_t index);
    bool optimize16BitStackOps1(size_t index);
    bool optimize16BitStackOps2(size_t index);
    bool optimize8BitStackOps(size_t index);
    bool removeTfrDX(size_t index);
    bool removeUselessLeax(size_t index);
    bool removeUselessLdx(size_t index);
    bool removeUnusedLoad(size_t index);
    bool optimizeAndbTstb(size_t index);
    bool optimizeIndexedX(size_t index);
    bool optimizeIndexedX2(size_t index);
    bool removeUselessLdb(size_t index);
    bool removeUselessLdd(size_t index);
    bool transformPshsDPshsD(size_t index);
    bool transformPshsXPshsX(size_t index);
    bool optimizePshsOps(size_t index);
    bool optimize16BitCompares(size_t index);
    bool combineConsecutiveOps(size_t index);
    bool removeConsecutivePshsPul(size_t index);
    bool coalesceConsecutiveLeax(size_t index);
    bool optimizeLeaxLdx(size_t index);
    bool optimizeLeaxLdd(size_t index);
    bool optimizeLdx(size_t index);
    bool optimizeLeax(size_t index);
    bool removeUselessTfr1(size_t index);
    bool removeUselessTfr2(size_t index);
    bool removeUselessClrb(size_t index);
    bool optimizeDXAliases(size_t index);
    bool removeLoadInComparisonWithTwoValues(size_t index);

    // Whole-function optimizer:
    bool isBasicBlockEndingInstruction(const Element &e) const;
    void createBasicBlock(size_t startIndex, size_t endIndex);
    void processBasicBlocks(const std::string &functionId);
    size_t findBlockIndex(size_t elementIndex) const;

    // Utilities:
    void removeUselessLabels();
    bool isInstr(size_t index, const char *ins, const char *arg) const;
    bool isInstrAnyArg(size_t index, const char *ins) const;
    bool isInstrWithImmedArg(size_t index, const char *ins) const;
    bool isInstrWithVarArg(size_t index, const char *ins) const;
    const char *getInstr(size_t index) const;
    const char *getInstrArg(size_t index) const;
    bool isConditionalBranch(size_t index, char inverseBranchInstr[8]) const;
    bool isRelativeSizeConditionalBranch(size_t index, char invertedOperandsBranchInstr[8]) const;
    uint16_t extractImmedArg(size_t index) const;
    void replaceWithInstr(size_t index, const char *ins, const char *arg, const char *comment);
    void replaceWithInstr(size_t index, const char *ins, const std::string &arg, const char *comment);
    void replaceWithInstr(size_t index, const char *ins, const std::string &arg = "", const std::string &comment = "");
    void insertInstr(size_t index, const char *ins, const std::string &arg = "", const std::string &comment = "");
    void commentOut(size_t index, const std::string &comment = "");
    static bool isDataDirective(const std::string &instruction);
    size_t findNextInstrBeforeLabel(size_t index) const;
    size_t findNextInstr(size_t index) const;
    size_t findLabelIndex(const std::string &label) const;
    bool isLabel(size_t index, const std::string &label) const;
    bool isInstrWithPreDecrOrPostIncr(size_t index) const;
    bool parseRelativeOffset(const std::string &s, int &offset);
    bool parseConstantLiteral(const std::string &s, int &literal);
    static bool isAbsoluteAddress(const std::string &arg);
    static bool isConstantOffsetFromX(const std::string &arg);

    struct BasicBlock
    {
        size_t startIndex;                  // in elements[]
        size_t endIndex;                    // in elements[]
        std::string firstSuccessorLabel;    // key in labelTable: if not empty, used to determine firstSuccessorIndex
        size_t firstSuccessorIndex;         // in elements[]: must be valid if firstSuccessorLabel empty
        size_t secondSuccessorIndex;        // in elements[]

        BasicBlock(size_t _startIndex, size_t _endIndex)
        :   startIndex(_startIndex),
            endIndex(_endIndex),
            firstSuccessorLabel(),
            firstSuccessorIndex(size_t(-1)),
            secondSuccessorIndex(size_t(-1))
        {
        }
    };

    typedef std::map<std::string, size_t> LabelTable;
        // Key: Assembly label from a LABEL-type Element.
        // Value: Index in elements[].

    struct Task
    {
        size_t blockIndex;  // index in basicBlocks[]
        uint8_t liveRegsAtEnd;  // registers that are live at the end of basicBlocks[blockIndex]

        Task(size_t bi, uint8_t lr) : blockIndex(bi), liveRegsAtEnd(lr) {}
    };


    std::vector<Element> elements;

    std::string currentSection;  // contains non empty name when an assembly SECTION is currently open

    // Used by whole-function optimizer.

    LabelTable labelTable;  // key: label; value: index in elements[]
    std::vector<BasicBlock> basicBlocks;
};


#endif  /* _H_ASMText */
