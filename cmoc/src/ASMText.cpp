/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2025 Pierre Sarrazin <http://sarrazip.com/>
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

#include "ASMText.h"

#include "Pseudo6809.h"
#include "TranslationUnit.h"
#include "FunctionDef.h"

#include <iomanip>
#include <climits>

using namespace std;


const ASMText::OptimizingMethod ASMText::level1OptimFuncs[] =
{
    &ASMText::branchToNextLocation,
    &ASMText::lddToLDB,
    &ASMText::pushLoadDiscardAdd,
    &ASMText::pushBLoadAdd,
    &ASMText::pushDLoadAdd,
    &ASMText::pushLoadDLoadX,
    &ASMText::replaceLDDZero,
    &ASMText::pushDLoadXLoadD,
    &ASMText::pushDLoadX,
    &ASMText::stripConsecutiveLoadsToSameReg,
    &ASMText::storeLoad,
    &ASMText::condBranchOverUncondBranch,
    &ASMText::shortenBranch,
    &ASMText::loadCmpZeroBeqOrBne,
    &ASMText::pushWordForByteComparison,
    &ASMText::stripConsecOppositeTFRs,
    &ASMText::stripOpToDeadReg,
    &ASMText::stripUselessPushPull,
    &ASMText::stripUselessBitwiseOp,
};


const ASMText::OptimizingMethod ASMText::level2OptimFuncs[] =
{
    &ASMText::fasterPointerIndexing,
    &ASMText::fasterPointerPushing,
    &ASMText::stripExtraClrA_B,
    &ASMText::stripExtraPulsX,
    &ASMText::stripExtraPushPullB,
    &ASMText::andA_B0,
    &ASMText::transformPshsDPshsD,
    &ASMText::changeLoadDToLoadB,
    &ASMText::changeAddDToAddB,
    &ASMText::stripPushLeas1,
    &ASMText::orAndA_B,
    &ASMText::loadDToClrALoadB,
    &ASMText::optimizeStackOperations1,
    &ASMText::optimizeStackOperations2,
    &ASMText::optimizeStackOperations3,
    &ASMText::optimizeStackOperations4,
    &ASMText::optimizeStackOperations5,
    &ASMText::mergePushXPushD,
    &ASMText::removeClr,
    &ASMText::removeAndOrMulAddSub,
    &ASMText::replaceCMPWithTST,
    &ASMText::optimizeLoadDX,
    &ASMText::optimizeLoadXX,
    &ASMText::optimizeTfrPush,
    &ASMText::removePushB,
    &ASMText::optimizeLdbTfrClrb,
    &ASMText::optimizeLoadTest,
    &ASMText::remove16BitStackOperation,
    &ASMText::optimizePostIncrement,
    &ASMText::removeUselessOps,
    &ASMText::optimize16BitStackOps1,
    &ASMText::optimize16BitStackOps2,
    &ASMText::optimize8BitStackOps,
    &ASMText::removeTfrDX,
    &ASMText::removeUselessLeax,
    &ASMText::removeUselessLdx,
    &ASMText::removeUnusedLoad,
    &ASMText::optimizeAndbTstb,
    &ASMText::optimizeIndexedX,
    &ASMText::optimizeIndexedX2,
    &ASMText::transformPshsXPshsX,
    &ASMText::optimizePshsOps,
    &ASMText::optimize16BitCompares,
    &ASMText::combineConsecutiveOps,
    &ASMText::removeConsecutivePshsPul,
    &ASMText::coalesceConsecutiveLeax,
    &ASMText::optimizeLeaxLdx,
    &ASMText::optimizeLeaxLdd,
    &ASMText::optimizeLdx,
    &ASMText::optimizeLeax,
    &ASMText::removeUselessTfr1,
    &ASMText::removeUselessTfr2,
    &ASMText::removeUselessClrb,
    &ASMText::optimizeDXAliases,
    &ASMText::removeLoadInComparisonWithTwoValues,
    &ASMText::removePCRIfRelocatabilityNotSupported,
    &ASMText::removeCMPZeroAfterLDIfBHI,
    &ASMText::replaceBranchToUncondBranch,
    &ASMText::ldbBranchLdbCompare,
    &ASMText::removePushBFromLdbPushLdbCmp,
    &ASMText::removeLDDAfterPushingD,
    &ASMText::arrayIndexMul,
    &ASMText::removeClrAFromArrayIndexMul,
    &ASMText::removeRepeatedLDX,
    &ASMText::avoidPushingDoubledD,
    &ASMText::removePushBPullABeforeMul,
    &ASMText::removePushBBeforeSubB,
    &ASMText::avoidSavingConstantRegisterPassedParameter,
    &ASMText::removeUselessGCCCallLoads,
    &ASMText::functionPointerGCCCall1,
    &ASMText::functionPointerGCCCall2,
    &ASMText::removeConsecutiveLEASInstructions,
    &ASMText::tightenArrayElementAddressComputation,
    &ASMText::mul16ByConstant,
    &ASMText::negbToSubb,
    &ASMText::removeUnneededLEAS,
};


// For debugging.
//
ostream &operator << (ostream &out, const ASMText::Element &el)
{
    return out << el.fields[0] << '\t' << el.fields[1] << '\t' << el.fields[2] << '\t' << "(" << el.type << ")";
}


ostream &operator << (ostream &out, const ASMText::InsEffects &ie)
{
    return out << "[read=0x" << hex << (unsigned) ie.read << ", written=0x" << (unsigned) ie.written << dec << "]";
}


ASMText::ASMText(bool _yRegAllowed)
:   elements(),
    currentSection(),
    currentCallConvention(DEFAULT_CMOC_CALL_CONV),
    yRegAllowed(_yRegAllowed),
    labelTable(),
    basicBlocks()
{
    elements.reserve(16384);  // as of 2015-07-27, Color Verbiste 0.1.3 takes about 7800 elements
}


void
ASMText::addElement(Type type, const string &field0, const string &field1, const string &field2)
{
    elements.push_back(Element());
    Element &e = elements.back();
    e.type = type;
    e.fields[0] = field0;
    e.fields[1] = field1;
    e.fields[2] = field2;
}


void
ASMText::ins(const string &instr, const string &arg, const string &comment)
{
    assert(! (instr == "LDA" && arg.empty()));
    addElement(INSTR, instr, arg, comment);
    //cout << "# ASMText::ins: [" << elements.size() - 1 << "]: " << elements.back() << endl;
}


// Emits a CMPD #xxxx, or an equivalent instruction.
// The emitted code affects flags N, Z, V, C.
//
void
ASMText::emitCMPDImmediate(uint16_t immediateValue, const string &comment)
{
    if (immediateValue == 0)
        ins("ADDD", "#0", comment);  // 1 fewer byte, 1 fewer cycle than CMPD
    else
        ins("CMPD", "#" + wordToString(immediateValue, true), comment);
}


void
ASMText::writeIns(ostream &out, const Element &e) const
{
    string instr = e.fields[0], arg = e.fields[1], comment = e.fields[2];

    assert(!instr.empty());
    assert(!(instr == "LDB" && arg == ",S++"));
    assert(!(instr == "LDA" && arg == ",S++"));
    assert(!(instr == "LDD" && arg == ",S+"));

    out << "\t" << instr;
    if (!arg.empty() || !comment.empty())
        out << "\t" << arg;
    if (!comment.empty())
        out << (arg.length() < 8 ? "\t\t" : "\t") << comment;
    out << "\n";
}


void
ASMText::emitFunctionStart(const string &functionId, const string &lineNo)
{
    addElement(FUNCTION_START, functionId, lineNo);
}


void
ASMText::emitFunctionEnd(const string &functionId, const string &lineNo)
{
    addElement(FUNCTION_END, functionId, lineNo);
}


void
ASMText::emitInlineAssembly(const string &text)
{
    addElement(INLINE_ASM, text);
}


void
ASMText::writeInlineAssembly(ostream &out, const Element &e) const
{
    out << "* Inline assembly:\n\n";
    out << e.fields[0] << "\n";
    out << "\n* End of inline assembly.\n";
}


void
ASMText::emitLabel(const string &label, const string &comment)
{
    assert(!label.empty());
    addElement(LABEL, label, comment);
}


void
ASMText::writeLabel(ostream &out, const Element &e) const
{
    const string &label = e.fields[0], &comment = e.fields[1];

    // Always EQU *, in case a comment follows, so the 1st word of the comment
    // is not taken for an opcode.
    //
    assert(!label.empty());
    out << label << "\tEQU\t*";
    if (!comment.empty())
        out << "\t\t" << comment;
    out << "\n";
}


void
ASMText::emitComment(const string &text)
{
    addElement(COMMENT, text);
}


const char ASMText::cFunctionArgumentPassingCommentPrefix[] = "C function argument ";


std::string
ASMText::getArgumentPassingComment(uint16_t argIndex, const std::string &functionId, const TypeDesc &argTypeDesc)
{
    return cFunctionArgumentPassingCommentPrefix + wordToString(argIndex)
                + (functionId.empty() ? "" : " of " + functionId + "()")
                + ": " + argTypeDesc.toString();
}


// Must be consistent with getArgumentPassingComment().
//
bool
ASMText::isArgumentPassingComment(const std::string &comment)
{
    return startsWith(comment, cFunctionArgumentPassingCommentPrefix);
}


void
ASMText::writeComment(ostream &out, const Element &e) const
{
    out << "* " << e.fields[0] << "\n";
}


void
ASMText::emitSeparatorComment()
{
    addElement(SEPARATOR);
}


void
ASMText::writeSeparatorComment(ostream &out, const Element &) const
{
    out << "\n";
    out << "\n";
    out << "*******************************************************************************\n";
    out << "\n";
}


void
ASMText::emitInclude(const string &filename)
{
    addElement(INCLUDE, filename);
}




void
ASMText::writeInclude(ostream &out, const Element &e) const
{
      out << "\t" "INCLUDE " << e.fields[0] << "\n";
}


void
ASMText::startSection(const char *sectionName)
{
    assert(sectionName && sectionName[0]);
    if (!currentSection.empty())
        errormsg("starting section %s, but section %s already started", sectionName, currentSection.c_str());
    addElement(SECTION_START, sectionName);
    currentSection = sectionName;
}


void
ASMText::endSection()
{
    if (currentSection.empty())
        errormsg("ending section, but no section started");
    addElement(SECTION_END, currentSection);
    currentSection.clear();
}


void
ASMText::emitExport(const char *label)
{
    assert(label && label[0]);
    addElement(EXPORT, label);
}


void
ASMText::emitImport(const char *label)
{
    assert(label && label[0]);
    addElement(IMPORT, label);
}


void
ASMText::emitEnd()
{
    addElement(END);
}


// Creates basic blocks from elements[] and calls processBasicBlocks() at
// the end of each function.
//
void
ASMText::optimizeWholeFunctions()
{
    string curFuncId;  // empty means not currently in a function, as per FUNCTION_START/FUNCTION_END
    size_t blockStartIndex = size_t(-1);  // not inside a block initially

    basicBlocks.reserve(1024);

    for (size_t i = 0; i < elements.size(); ++i)
    {
        const Element &e = elements[i];
        cout << "### " << setw(5) << i << ". ";
        writeElement(cout, e);
        switch (e.type)
        {
        case FUNCTION_START:
            curFuncId = e.fields[0];  // remember function's id
            assert(!curFuncId.empty());
            labelTable.clear();
            basicBlocks.clear();
            break;
        case COMMENT:
            break;  // ignore comments
        case INSTR:
        case INLINE_ASM:
            if (!curFuncId.empty())  // if inside a function
            {
                if (blockStartIndex == size_t(-1))
                    blockStartIndex = i;  // start block if no block already started

                if (e.type == INSTR && isBasicBlockEndingInstruction(e))  // does the element at i end the current block?
                {
                    createBasicBlock(blockStartIndex, i + 1);  // include current element in block
                    blockStartIndex = size_t(-1);  // no inside a block anymore
                }
            }
            break;
        case LABEL:
            labelTable[e.fields[0]] = i;  // remember elements[] index where label is
            /* FALLTHROUGH */
        default:
            if (blockStartIndex != size_t(-1))  // if inside function
            {
                createBasicBlock(blockStartIndex, i);  // exclude the label from the block
                blockStartIndex = size_t(-1);  // no inside a block anymore
            }
            break;
        case FUNCTION_END:
            processBasicBlocks(curFuncId);
            blockStartIndex = size_t(-1);  // not inside a block anymore
            curFuncId.clear();  // remember that not inside function anymore
            break;
        }
    }
}


// Must be in alphabetical order.
//
static const char *basicBlockEndingInstructions[] =
{
    "BCC", "BCS", "BEQ", "BGE", "BGT", "BHI", "BHS", "BLE", "BLO", "BLS",
    "BLT", "BMI", "BNE", "BPL", "BRA", "BVC", "BVS", "JMP", "LBCC",
    "LBCS", "LBEQ", "LBGE", "LBGT", "LBHI", "LBHS", "LBLE", "LBLO", "LBLS",
    "LBLT", "LBMI", "LBNE", "LBPL", "LBRA", "LBVC", "LBVS", "RTI", "RTS",
};


inline bool firstStringComesBeforeSecond(const char *a, const char *b)
{
    return strcmp(a, b) < 0;
}


// Determines if the given instruction marks the end of the basic block
// it is part of. Typically, 'e' must be a branch or a return.
//
bool
ASMText::isBasicBlockEndingInstruction(const Element &e) const
{
    assert(e.type == INSTR);
    if (binary_search(basicBlockEndingInstructions,
                         basicBlockEndingInstructions + sizeof(basicBlockEndingInstructions) / sizeof(basicBlockEndingInstructions[0]),
                         e.fields[0].c_str(),
                         firstStringComesBeforeSecond))
        return true;
    return e.fields[0] == "PULS" && e.fields[1] == "U,PC";
}


bool
ASMText::isFunctionEndingInstruction(const Element &e) const
{
    assert(e.type == INSTR);
    if (e.fields[0] == "LEAS" && e.fields[1] == ",U")
        return true;
    if (e.fields[0] == "RTS")
        return true;
    if (e.fields[0] == "RTI")
        return true;
    return false;
}


// Creates a basic block containing the instructions from startIndex
// (inclusively) to endIndex (exclusively).
// Does not create a block if these indices designate an empty interval,
// of if nothing but comments appear in the interval,
// or if startIndex is invalid.
//
void
ASMText::createBasicBlock(size_t startIndex, size_t endIndex)
{
    //cout << "# createBasicBlock(" << startIndex << ", " << endIndex << ")\n";

    if (startIndex >= elements.size())
        return;  // ignore: we are not inside a function
    assert(endIndex >= 1 && endIndex <= elements.size());

    // Decrement endIndex as long as the last block element is a comment.
    while (elements[endIndex - 1].isCommentLike())
        --endIndex;

    if (startIndex >= endIndex)
        return;  // ignore: empty interval

    basicBlocks.push_back(BasicBlock(startIndex, endIndex));
    BasicBlock &newBlock = basicBlocks.back();

    const Element &lastElem = elements[endIndex - 1];
    if (lastElem.type == INSTR)
    {
        const string &lastIns = lastElem.fields[0];
        const string &lastArg = lastElem.fields[1];

        if (lastIns == "RTS" || lastIns == "RTI" || (lastIns == "PULS" && lastArg == "U,PC"))
        {
            //cout << "#   no successor block\n";
        }
        else if (lastIns == "BRA" || lastIns == "LBRA" || lastIns == "JMP")
        {
            //cout << "#   single successor at label '" << lastArg << "'\n";
            assert(!lastArg.empty());
            newBlock.firstSuccessorLabel = lastArg;
        }
        else if ((lastIns[0] == 'B' && lastIns != "BSR")
                 || (lastIns[0] == 'L' && lastIns[1] == 'B' && lastIns != "LBSR"))  // short or long branch
        {
            //cout << "#   2 successors: next block and block at label '" << lastArg << "'\n";
            assert(!lastArg.empty());
            newBlock.firstSuccessorLabel = lastArg;
            newBlock.secondSuccessorIndex = endIndex;
        }
        else
        {
            //cout << "#   fall through at index " << endIndex << "\n";
            newBlock.firstSuccessorIndex = endIndex;  // firstSuccessorLabel left empty, b/c firstSuccessorIndex already set
        }
        /*cout << "# BasicBlock at " << basicBlocks.size() - 1
                << ": newBlock.firstSuccessorLabel='" << newBlock.firstSuccessorLabel << "'"
                << ", newBlock.firstSuccessorIndex=" << newBlock.firstSuccessorIndex
                << endl;*/
    }
    else if (lastElem.type == INLINE_ASM)
    {
        //cout << "#   inline asm falls through at index " << endIndex << "\n";
        newBlock.firstSuccessorIndex = endIndex;  // firstSuccessorLabel left empty, b/c firstSuccessorIndex already set
    }
    else
        assert(!"failed to determine successors: last element is not instruction");
}


void
ASMText::processBasicBlocks(const string & /*functionId*/)
{
}


size_t
ASMText::findBlockIndex(size_t elementIndex) const
{
    for (vector<BasicBlock>::const_iterator it = basicBlocks.begin(); it != basicBlocks.end(); ++it)
        if (it->startIndex >= elementIndex)
            return it - basicBlocks.begin();
    return size_t(-1);
}


// "Stage 2" optimizations are the ones implemented by Jamie Cho
// in early 2016.
//
void
ASMText::peepholeOptimize(size_t optimizationLevel, FramePointerOption framePointerOption)
{
    if (optimizationLevel >= 1)
    {
        const size_t maxIters = 300;  // limit iterations in case of a bug in an optim method

        size_t numIters;
        for (numIters = 0; numIters < maxIters; ++numIters)
        {
            bool modified = false;

            if (removeUselessLabels())
                modified = true;

            for (size_t i = 0; i < elements.size(); ++i)
            {
                // Advance to the next instuction.
                size_t instrIndex = findNextInstrBeforeLabel(i, &currentCallConvention);
                if (instrIndex != size_t(-1))
                    i = instrIndex;

                for (auto method : level1OptimFuncs)
                    if ((this->*method)(i))
                    {
                        modified = true;
                        break;
                    }

                if (optimizationLevel >= 2)
                    for (auto method : level2OptimFuncs)
                        if ((this->*method)(i))
                        {
                            modified = true;
                            break;
                        }
            }

            if (!modified)
                break;
        }

        if (numIters >= maxIters)  // if too many iterations
        {
            cerr << "cmoc: ERROR: Too many iterations (" << numIters << ") in the peephole optimizer\n";
            exit(1);
        }
    }

    removeUselessParameterSaves();

    // This phase must be done last because earlier optimization phases do not expect
    // local variables and function arguments to be referred to through the S register.
    // Also, this phase does not need to be run more than once.
    //
    if (framePointerOption == FramePointerOption::OMIT_A_LOT)
        for (size_t i = 0; i < elements.size(); ++i)
            if (elements[i].type == FUNCTION_START)
                removeFramePointer(i);  // advances 'i'
}


static const char callConventionPrefix[] = "Calling convention: ";


// Uses callConventionPrefix. Must be compatible with findNextInstrBeforeLabel().
//
void
ASMText::emitCallConventionMarker(CallConvention callConv)
{
    const char *callConvName = NULL;
    switch (callConv)
    {
        case FIRST_PARAM_IN_REG_CALL_CONV: callConvName = "First parameter received in register"; break;
        case GCC6809_CALL_CONV: callConvName = "GCC6809"; break;
        case DEFAULT_CMOC_CALL_CONV: callConvName = "CMOC Default"; break;
        default: callConvName = "???";
    }
    emitComment(callConventionPrefix + intToString(callConv) + " (" + string(callConvName) + ")");
}


const char ASMText::parameterSaveCommentPrefix[] = "save register for call at ";


// Useful for the GCC6809 calling convention.
//
void
ASMText::emitParameterSaveInstruction(const char *regToSpill,
                                      const string &frameDisplacementArg,
                                      const string &functionLineNo)
{
    ins(string("ST") + regToSpill,
        frameDisplacementArg,
        parameterSaveCommentPrefix + functionLineNo);
}


static bool
isGeneratedLabel(const char *s)
{
    if (s[0] != 'L')
        return false;
    for (size_t i = 1; i < 6; ++i)
        if (!isdigit(s[i]))
            return false;
    return true;
}


// Adds to 'dest', keeping it sorted.
//
// Expected cases:
// - L#####
// - L#####,PCR
// - L#####-L#####
// where # is a digit.
//
static void
extractGeneratedLabels(vector<string> &dest, const string &str)
{
    const char *s = str.c_str();
    if (!isGeneratedLabel(s))
        return;

    if (str.length() == 6)
    {
        addUnique(dest, str);
        return;
    }

    addUnique(dest, string(str, 0, 6));

    if (!strcmp(s + 6, ",PCR"))
        return;

    if (s[6] != '-')
        return;

    if (!isGeneratedLabel(s + 7))
        return;

    addUnique(dest, string(str, 7, 6));
}


// Returns true if at least one label was removed.
//
bool
ASMText::removeUselessLabels()
{
    // Fill 'usedLabels' with every generated label that is used in the argument
    // of an instruction, then comment out generated labels that do not appear in this list.
    //
    vector<string> usedLabels;  // must remain sorted
    usedLabels.reserve(2048);

    for (size_t i = 0; i < elements.size(); ++i)
    {
        const Element &e = elements[i];
        if (e.type == INSTR)
            extractGeneratedLabels(usedLabels, e.fields[1]);
    }
    bool modified = false;
    for (size_t i = 0; i < elements.size(); ++i)
    {
        Element &e = elements[i];
        if (e.type != LABEL)
            continue;
        string &label = e.fields[0];
        if (!isGeneratedLabel(label.c_str()))
            continue;
        if (isPresent(usedLabels, label))
            continue;
        e.type = COMMENT;
        label = "Useless label " + label + " removed";
        modified = true;
    }
    return modified;
}


// Useful with the __gcccall convention.
// Must be done before omitting the stack frame pointers.
//
void
ASMText::removeUselessParameterSaves()
{
    map<string, size_t> storeInstructionTable;  // key: argumente of STB/STX; value: index in elements[]

    for (size_t index = 0; index < elements.size(); ++index)
    {
        const Element &e = elements[index];
        if (e.type == INSTR)
        {
            if (startsWith(e.fields[2], parameterSaveCommentPrefix))  // if __gcccall param store
            {
                // This is a store instruction that may or may not be useful.
                // If it remains in storeInstructionTable by the end of the
                // current function, it will be deemed useless and removed.
                // Here, we remember the argument (e.g., -42,U) of the store
                // and its position in elements[].
                //
                storeInstructionTable.insert(make_pair(e.fields[1], index));
            }
            else if ((e.fields[0] == "LDB" || e.fields[0] == "LDX") && e.fields[1][0] == '-')
            {
                // 'e' is an LDB/LDX that MAY load from a location where a value was saved by an STB/STX
                // stored in storeInstructionTable.
                // If that is NOT the case, the following storeInstructionTable.erase() will do nothing.
                // If that is the case, then the LDB/LDX argument, i.e., e.fields[1],
                // will be found and removed by the erase(), which means that the store instruction
                // was useful and must be kept.
                //
                storeInstructionTable.erase(e.fields[1]);
            }
        }
        else if (e.type == FUNCTION_START)
        {
            storeInstructionTable.clear();
        }
        else if (e.type == FUNCTION_END)
        {
            // The instructions still in storeInstructionTable by the end
            // of a function are useless stores.
            //
            for (const auto &pair : storeInstructionTable)
            {
                commentOut(pair.second, "optim: ASMText::removeUselessParameterSaves: " + pair.first);  // mention store arg
            }
            storeInstructionTable.clear();
        }
    }
}


// Tries to remove the use of a register for the frame pointer from the function
// that starts at 'index'.
// Returns true if it succeeds, false if it could not be done.
//
bool
ASMText::removeFramePointer(size_t &index)
{
    if (index + 1 >= elements.size() || elements[index].type != FUNCTION_START)
        return false;
    ++index;
    if (elements[index].type != LABEL)
        return false;
    ++index;
    while (index < elements.size() && elements[index].type == COMMENT)
        ++index;
    if (!isInstr(index, "PSHS", "U"))
        return false;
    ++index;
    if (!isInstr(index, "LEAU", ",S"))
        return false;

    vector<size_t> toBeCommentedOut;
    toBeCommentedOut.push_back(index - 1);  // remove PSHS U
    toBeCommentedOut.push_back(index);  // remove LEAU ,S

    ++index;

    int16_t localVarSpace = 0;  // in bytes
    if (isInstrAnyArg(index, "LEAS"))  // extract the number subtracted from S, if any
    {
        const string &arg = elements[index].fields[1];  // expect "-nnn,S", where nnn is a positive integer
        if (!startsWith(arg, "-") || !endsWith(arg, ",S"))
            return false;   // unexpected
        localVarSpace = atoi(arg.c_str() + 1);

        ++index;
    }

    // Iterate over the instructions of the current function.
    // List the changes to be made to remove the frame pointer (U),
    // but only apply them if no contraindication is seen.
    //
    // References to U are replaced with references to S.
    // Example: Assume we have this initially:
    //      PSHS U; LEAU ,S; LEAS -10,S; LDA -3,U
    //   This creates this stack frame:
    //      LLLLLLLLLLFFRR
    //      ^         ^
    //      S      a  U
    //   where LLLLLLLLLL is the local variable space (10 bytes),
    //   FF is the value of U that got pushed,
    //   RR is the return address,
    //   'a' is the address of the byte that LDA loads.
    //   The stack frame will become this, because U will not be pushed anymore:
    //      LLLLLLLLLLRR
    //      ^
    //      S      a
    //   The sequence will become this:
    //      LEAS -10,S; LDA -3+10,S
    //
    //   Now consider the case where we access a function argument, i.e., LDD 4,U:
    //      LLLLLLLLLLFFRRPP
    //      ^         ^   ^
    //      S         U   U+4
    //   With tne new stack frame, we have this:
    //      LLLLLLLLLLRRPP
    //      ^         ^ ^
    //      S           S+10+2
    //   Thus, we now have LDD 4-2+10,S.
    //
    //   Also, each instruction that changes S causes an additional adjustment to the displacement on S.
    //
    struct DisplacementChange
    {
        size_t index;
        int16_t displacementFromS;
        DisplacementChange(size_t idx, int16_t displ) : index(idx), displacementFromS(displ) {}
    };
    vector<DisplacementChange> displacementChanges;
    int16_t adjustment = 0;  // in bytes; tracks changes to S
    size_t rtsIndex = size_t(-1);  // index of the PSHS U,PC to be replaced with RTS
    size_t destroyLocalVarSpaceIndex = size_t(-1);  // index of the LEAS ,U instruction

    for ( ; index < elements.size() && elements[index].type != FUNCTION_END; ++index)
    {
        const Element &el = elements[index];
        //cout << "# removeFramePointer: " << adjustment << " " << index << ". " << el << endl;
        if (el.type == INLINE_ASM)
            return false;
        if (el.type != INSTR)
            continue;
        const string &ins = el.fields[0];
        const string &arg = el.fields[1];
        if (ins == "LEAS")
        {
            if (arg == ",U")  // undo stack frame
                destroyLocalVarSpaceIndex = index;
            else if (endsWith(arg, ",S"))
                adjustment -= (int16_t) atoi(arg.c_str());
        }
        else if (endsWith(arg, ",S+"))
            adjustment -= 1;
        else if (endsWith(arg, ",S++") || arg == "[,S++]")
            adjustment -= 2;
        else if (endsWith(arg, ",-S"))
            adjustment += 1;
        else if (endsWith(arg, ",--S"))
            adjustment += 2;
        else if (ins == "PULS" && arg == "U,PC")
            rtsIndex = index;
        else if (ins == "PSHS" || ins == "PULS")
        {
            uint8_t argRegs = parsePushPullArg(arg);
            static const uint8_t byteRegs[] = { A, B, CC };
            static const uint8_t wordRegs[] = { X, Y, U };
            int16_t bytesMoved = 0;
            for (uint8_t r : byteRegs)
                if (argRegs & r)
                    bytesMoved += 1;
            for (uint8_t r : wordRegs)
                if (argRegs & r)
                    bytesMoved += 2;
            if (ins == "PSHS")
                adjustment += bytesMoved;
            else
                adjustment -= bytesMoved;
        }
        else if (ins == "LBSR" || (ins == "JSR" && arg.find(',') == string::npos))  // LBSR, or JSR to a label
        {
            // Check for CMOC routines that pop the 2 bytes that follow their return address before returning.
            if (arg == "shiftByteLeft" || arg == "shiftByteRightSigned" || arg == "shiftByteRightUnsigned")
                adjustment -= 1;
            else if (arg == "shiftLeft" || arg == "shiftRightSigned" || arg == "shiftRightUnsigned")
                adjustment -= 2;
        }
        else if (endsWith(arg, ",U") || endsWith(arg, ",U]"))  // if reference to local var or func arg (ins could be JSR)
        {
            int16_t displacementFromU = (int16_t) atoi(arg.c_str() + int(arg[0] == '['));  // negative means local var, positive means func arg
            int16_t displacementFromS = displacementFromU + localVarSpace + (displacementFromU > 0 ? -2 : 0) + adjustment;
            displacementChanges.push_back(DisplacementChange(index, displacementFromS));
        }
    }

    if (destroyLocalVarSpaceIndex == size_t(-1))
        return false;  // abort: expected LEAS ,U not found; can happen in function that contains infinite loop, e.g., for (;;);

    if (rtsIndex == size_t(-1))
    {
        assert(!"expected PULS U,PC not found");
        return false;  // abort: expected PULS U,PC not found
    }

    // Remove LEAS ,U.
    //
    if (adjustment + localVarSpace != 0)
    {
        char newArg[16];
        snprintf(newArg, sizeof(newArg), "%d+%d,S", adjustment, localVarSpace);
        elements[destroyLocalVarSpaceIndex].fields[1] = newArg;
        elements[destroyLocalVarSpaceIndex].fields[2] = "optim: removeFramePointer";
    }
    else
        commentOut(destroyLocalVarSpaceIndex, "optim: removeFramePointer");

    // Remove PULS U,PC.
    //
    elements[rtsIndex].fields[0] = "RTS";
    elements[rtsIndex].fields[1].clear();
    elements[rtsIndex].fields[2] = "optim: removeFramePointer";

    // Remove or change some other instructions.
    //
    for (size_t i : toBeCommentedOut)
        commentOut(i, "optim: removeFramePointer");
    for (const DisplacementChange &dc : displacementChanges)
    {
        string &arg = elements[dc.index].fields[1];
        char newArg[16];
        if (dc.displacementFromS == 0)
            strcpy(newArg, ",S");
        else
            snprintf(newArg, sizeof(newArg), "%d,S", dc.displacementFromS);
        if (arg[0] == '[')
        {
            arg.reserve(1 + strlen(newArg) + 1);  // make sure only 1 allocation
            arg = '[';
            arg.append(newArg);
            arg.append("]");
        }
        else
            arg = newArg;
        elements[dc.index].fields[2] += " (optim: removeFramePointer)";
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////


bool
ASMText::isInstr(size_t index, const char *ins, const char *arg) const
{
    return isInstrAnyArg(index, ins) && elements[index].fields[1] == arg;
}


// index: Allowed to be size_t(-1): method returns false.
//
bool
ASMText::isInstrAnyArg(size_t index, const char *ins) const
{
    if (index >= elements.size())
        return false;
    const Element &e = elements[index];
    return e.type == INSTR && e.fields[0] == ins;
}


bool
ASMText::isInstrWithImmedArg(size_t index, const char *ins) const
{
    return isInstrAnyArg(index, ins) && elements[index].fields[1][0] == '#';
}


// Requires the argument to contain ",U", ",Y" or ",P" (to match ",PCR").
//
bool
ASMText::isInstrWithVarArg(size_t index, const char *ins) const
{
    if (!isInstrAnyArg(index, ins))
        return false;
    const char *comma = strchr(elements[index].fields[1].c_str(), ',');
    if (!comma)
        return false;
    return strchr("UPY", comma[1]) != NULL;  // accept ",U", ",PCR" and ",Y"
}


const char *
ASMText::getInstr(size_t index) const
{
    if (index >= elements.size())
        return "";
    const Element &e = elements[index];
    if (e.type != INSTR)
        return "";
    return e.fields[0].c_str();
}


const char *
ASMText::getInstrArg(size_t index) const
{
    if (index >= elements.size())
        return "";
    const Element &e = elements[index];
    if (e.type != INSTR)
        return "";
    return e.fields[1].c_str();
}


// None of the instruction names must exceed 6 characters in length.
// See isConditionalBranch().
//
const static struct
{
    const char *branchInstr;
    const char *inverseInstr;
} condBranchInstrTable[] =
{
    { "BCC", "BCS" },
    { "BEQ", "BNE" },
    { "BGE", "BLT" },
    { "BGT", "BLE" },
    { "BHI", "BLS" },
    { "BHS", "BLO" },
    { "BMI", "BPL" },
    { "BVC", "BVS" },
};


// None of the instruction names must exceed 6 characters in length.
// See isRelativeSizeConditionalBranch().
//
const static struct
{
    const char *branchInstr;
    const char *invertedOperandsInstr;
} relativeSizeBranchInstrTable[] =
{
    { "BEQ", "BEQ" },
    { "BNE", "BNE" },

    { "BGE", "BLE" },
    { "BGT", "BLT" },

    { "BHI", "BLO" },
    { "BHS", "BLS" },
};


// Determines if elements[index] is a conditional branch (short or long).
// If it is, true is returned and 'inverseBranchInstr' receives the branch
// instruction that uses the opposite condition (e.g., BEQ becomes BNE).
// Uses condBranchInstrTable.
//
bool
ASMText::isConditionalBranch(size_t index, char inverseBranchInstr[INSTR_NAME_BUFSIZ], bool forceLongBranch) const
{
    if (index >= elements.size())
        return false;
    const Element &e = elements[index];
    if (e.type != INSTR)
        return false;
    const char *ins = e.fields[0].c_str();
    bool isLongBranchInstr = false;
    if (ins[0] == 'L')
    {
        if (ins[1] != 'B')
            return false;
        isLongBranchInstr = true;
    }
    else if (ins[0] != 'B')
        return false;  // not a branch instruction

    char *writer = inverseBranchInstr;
    size_t room = INSTR_NAME_BUFSIZ;
    if (isLongBranchInstr || forceLongBranch)
    {
        *writer++ = 'L';
        --room;
        if (isLongBranchInstr)
            ++ins;
    }

    size_t n = sizeof(condBranchInstrTable) / sizeof(condBranchInstrTable[0]);
    for (size_t i = 0; i < n; ++i)
    {
        if (!strcmp(ins, condBranchInstrTable[i].branchInstr))
        {
            strncpy(writer, condBranchInstrTable[i].inverseInstr, room);
            inverseBranchInstr[INSTR_NAME_BUFSIZ - 1] = '\0';
            return true;
        }
        if (!strcmp(ins, condBranchInstrTable[i].inverseInstr))
        {
            strncpy(writer, condBranchInstrTable[i].branchInstr, room);
            inverseBranchInstr[INSTR_NAME_BUFSIZ - 1] = '\0';
            return true;
        }
    }

    return false;
}


bool
ASMText::isConditionalBranch(const char *ins)
{
    if (!ins)
        return false;

    if (toupper(ins[0]) == 'L')
        ++ins;
    if (toupper(ins[0]) != 'B')
        return false;

    size_t n = sizeof(condBranchInstrTable) / sizeof(condBranchInstrTable[0]);
    for (size_t i = 0; i < n; ++i)
    {
        if (!strcasecmp(ins, condBranchInstrTable[i].branchInstr))
            return true;
        if (!strcasecmp(ins, condBranchInstrTable[i].inverseInstr))
            return true;
    }

    return false;
}


bool
ASMText::isConditionalBranchAtIndex(size_t index) const
{
    if (index >= elements.size())
        return false;
    return isConditionalBranch(elements[index].fields[0].c_str());
}


bool
ASMText::isBranchAtIndex(size_t index) const
{
    return isInstrAnyArg(index, "BRA") || isInstrAnyArg(index, "LBRA") || isInstrAnyArg(index, "JMP") || isConditionalBranchAtIndex(index);
}


bool
ASMText::isBranchOrCallAtIndex(size_t index) const
{
    if (index >= elements.size())
        return false;
    const Element &e = elements[index];
    if (e.type != INSTR)
        return false;
    if (endsWith(e.fields[0], "BSR") || e.fields[0] == "JSR")
        return true;
    return isBranchAtIndex(index);
}


// Checks if elements[index] appears in relativeSizeBranchInstrTable.
// If it does, true is returned and 'invertedOperandsBranchInstr' returns
// the branch instruction that is equivalent when the comparison operands
// are reversed.
// For example, if k <= n is to be replaced with n >= k, then
// { LDD k; CMPD n; BLS z } must be replaced with { LDD n; CMPD k; BHS z }.
//
bool
ASMText::isRelativeSizeConditionalBranch(size_t index, char invertedOperandsBranchInstr[INSTR_NAME_BUFSIZ]) const
{
    const Element &e = elements[index];
    if (e.type != INSTR)
        return false;
    const char *ins = e.fields[0].c_str();
    bool isLongBranchInstr = false;
    if (ins[0] == 'L')
    {
        if (ins[1] != 'B')
            return false;
        isLongBranchInstr = true;
    }
    else if (ins[0] != 'B')
        return false;  // not a branch instruction

    char *writer = invertedOperandsBranchInstr;
    size_t room = INSTR_NAME_BUFSIZ;
    if (isLongBranchInstr)
    {
        *writer++ = 'L';
        --room;
        ++ins;
    }

    size_t n = sizeof(relativeSizeBranchInstrTable) / sizeof(relativeSizeBranchInstrTable[0]);
    for (size_t i = 0; i < n; ++i)
    {
        if (!strcmp(ins, relativeSizeBranchInstrTable[i].branchInstr))
        {
            strncpy(writer, relativeSizeBranchInstrTable[i].invertedOperandsInstr, room);
            invertedOperandsBranchInstr[INSTR_NAME_BUFSIZ - 1] = '\0';
            return true;
        }
        if (!strcmp(ins, relativeSizeBranchInstrTable[i].invertedOperandsInstr))
        {
            strncpy(writer, relativeSizeBranchInstrTable[i].branchInstr, room);
            invertedOperandsBranchInstr[INSTR_NAME_BUFSIZ - 1] = '\0';
            return true;
        }
    }

    return false;
}


uint16_t
ASMText::extractImmedArg(size_t index) const
{
    const Element &e = elements[index];
    const char *arg = e.fields[1].c_str();
    unsigned long n;
    char *endptr;
    if (arg[1] == '$')
        n = strtoul(arg + 2, &endptr, 16);
    else
        n = strtoul(arg + 1, &endptr, 10);
    return uint16_t(n) & 0xFFFF;
}

// Supports +decimal, -decimal, +hex and -$hex.
// Tolerates a suffix after the numerical value (e.g., ",S").
// Assumes that the numerical value fits an int16_t.
// Returns 0 if no numerical value is found (e.g., s is just ",U").
// N.B.: This function partly duplicates ASMText::extractConstantLiteral().
//
int16_t
ASMText::extractNumericalPrefix(const string &s)
{
    bool isNegative = false;
    const char *p = s.c_str();
    if (*p == '-')
    {
        isNegative = true;
        ++p;
    }
    else if (*p == '+')
        ++p;
    bool isHex = false;
    if (*p == '$')
    {
        isHex = true;
        ++p;
    }

    char *endptr;
    long n = strtol(p, &endptr, isHex ? 16 : 10);
    if (endptr == p)  // if no numerical value
        return 0;  // assume something like ",S", which means "0,S"
    if (isNegative)
        n = -n;
    return int16_t(n);
}


void
ASMText::replaceWithInstr(size_t index, const char *ins, const char *arg, const char *comment)
{
    Element &e = elements[index];
    e.type = INSTR;
    e.fields[0] = ins;
    e.fields[1] = arg;
    e.fields[2] = comment;
}


void
ASMText::replaceWithInstr(size_t index, const char *ins, const string &arg, const char *comment)
{
    replaceWithInstr(index, ins, arg.c_str(), comment);
}


void
ASMText::replaceWithInstr(size_t index, const char *ins, const string &arg, const string &comment)
{
    replaceWithInstr(index, ins, arg.c_str(), comment.c_str());
}


// Inserts the given instruction at position 'index' in elements[]
// and pushes all elements at and after that index one position forward.
//
void
ASMText::insertInstr(size_t index, const char *ins, const string &arg, const string &comment)
{
    elements.insert(elements.begin() + index, Element());
    replaceWithInstr(index, ins, arg, comment);
}


void
ASMText::commentOut(size_t index, const string &comment)
{
    Element &e = elements[index];
    e.type = COMMENT;
    e.fields[0] = comment;
}


bool
ASMText::isDataDirective(const std::string &instruction)
{
    const char *ins = instruction.c_str();
    return !strncmp(ins, "FD", 2) || !strcmp(ins, "FCC") || !strcmp(ins, "RMB") || !strcmp(ins, "ZMB");
}


// Returns size_t(-1) if no instruction is found before a non-instruction is found.
// The search starts at elements[index], inclusively.
// Passes comments. Looks for callConventionPrefix if callConvention is not null.
//
size_t
ASMText::findNextInstrBeforeLabel(size_t index, CallConvention *callConvention) const
{
    for ( ; index < elements.size(); ++index)
    {
        const Element &e = elements[index];
        if (e.type == INSTR)
        {
            if (isDataDirective(e.fields[0]))
                return size_t(-1);  // failure
            return index;
        }
        if (e.type != COMMENT)
            return size_t(-1);  // failure because found LABEL, etc. before INSTR
        
        // If the caller wants to know if a calling convention marker has been seen,
        // check if this comment is such a marker, and store the calling convention type
        // in the output parameter.
        //
        if (callConvention != NULL && startsWith(e.fields[0], callConventionPrefix))
            *callConvention = (CallConvention) atoi(e.fields[0].c_str() + sizeof(callConventionPrefix) - 1);
    }
    return size_t(-1);  // reached end of elements
}


// Returns size_t(-1) if no instruction found before an inline assembly element
// or an #include element.
// Tolerates comments.
//
size_t
ASMText::findNextInstr(size_t index) const
{
    for ( ; index < elements.size(); ++index)
    {
        const Element &e = elements[index];
        if (e.type == INSTR)
            return index;
        if (e.type == INLINE_ASM || e.type == INCLUDE || e.type == SEPARATOR)
            return size_t(-1);
    }
    return size_t(-1);  // reached end of elements
}


// Searches elements[] for a LABEL element with the given 'label'.
// The search starts at elements[startIndex] and ends before elements[startIndex + numElementsToSearch].
// The search extends to the end if numElementsToSearch is zero.
// The two parameters get clamped as needed.
// Returns the index in elements[] if found, or size_t(-1) if not.
//
size_t
ASMText::findLabelIndex(const string &label, size_t startIndex, size_t numElementsToSearch) const
{
    if (startIndex >= elements.size())
        return size_t(-1);
    if (numElementsToSearch == 0 || numElementsToSearch > elements.size())
        numElementsToSearch = elements.size();
    if (startIndex + numElementsToSearch > elements.size())
        numElementsToSearch = elements.size() - startIndex;

    for (vector<Element>::const_iterator it = elements.begin() + startIndex,
                                        end = it + numElementsToSearch; it != end; ++it)
        if (it->type == LABEL && it->fields[0] == label)
            return size_t(it - elements.begin());
    return size_t(-1);
}


size_t
ASMText::findLabelAroundIndex(const string &label, size_t index, size_t span) const
{
    return findLabelIndex(label, index < span ? 0 : index - span, 2 * span + 1);
}


bool
ASMText::isLabel(size_t index) const
{
    if (index >= elements.size())
        return false;
    return elements[index].type == LABEL;
}


bool
ASMText::isLabel(size_t index, const string &label) const
{
    return isLabel(index) && elements[index].fields[0] == label;
}
    

bool
ASMText::extractConstantLiteral(const string &s, int &val)
{
    size_t len = s.length();
    if (len == 0 || s[0] != '#')
        return false;
    const bool isHex = (len >= 2 && s[1] == '$');
    const char *arg = s.c_str() + (isHex ? 2 : 1);
    char *endptr;
    val = (int) strtol(arg, &endptr, isHex ? 16 : 10);
    return endptr != arg && *endptr == '\0';  // constant means non empty and entire argument used by strtol()
}


bool
ASMText::isInstrWithPreDecrOrPostIncr(size_t index) const {
  const Element &e = elements[index];
  const string &op = e.fields[1];
  return (startsWith(op, ",-") || startsWith(op, "[,-") ||
          endsWith(op, "+") || endsWith(op, "+]"));
}


bool
ASMText::parseRelativeOffset(const string &s, int &offset)
{
  size_t commaIndex = s.find(",");
  if ((commaIndex == string::npos) || (commaIndex == 0) ||
      (s.find("[") != string::npos)) {
    return false;
  }

  offset = (int) (s[0] == '$' ? strtol(s.c_str() + 1, NULL, 16)
                              : strtol(s.c_str(), NULL, 10));
  return true;
}


bool
ASMText::parseConstantLiteral(const string &s, int &literal)
{
  if (!startsWith(s, "#"))
    return false;

  literal = (int) (s[1] == '$' ? strtol(s.c_str() + 2, NULL, 16)
                               : strtol(s.c_str() + 1, NULL, 10));
  return true;
}


// Returns true iff 'arg' is an decimal or hex integer constant.
//
bool
ASMText::isAbsoluteAddress(const string &arg)
{
    if (arg.length() == 0)
        return false;
    if (arg[0] == '$')
    {
        if (arg.length() == 1)
            return false;
        for (size_t i = 1; i < arg.length(); ++i)
            if (!isxdigit(arg[i]))
                return false;
    }
    else
    {
        for (size_t i = 0; i < arg.length(); ++i)
            if (!isdigit(arg[i]))
                return false;
    }
    return true;
}


void
ASMText::writeElement(ostream &out, const Element &e) const
{
    switch (e.type)
    {
    case INSTR:         writeIns(out, e);                 break;
    case INLINE_ASM:    writeInlineAssembly(out, e);      break;
    case LABEL:         writeLabel(out, e);               break;
    case COMMENT:       writeComment(out, e);             break;
    case SEPARATOR:     writeSeparatorComment(out, e);    break;
    case INCLUDE:       writeInclude(out, e);             break;
    case FUNCTION_START:
        out << "* FUNCTION " << e.fields[0] << "(): defined at " << e.fields[1] << "\n";
        break;
    case FUNCTION_END  :
        out << "* END FUNCTION " << e.fields[0] << "(): defined at " << e.fields[1] << "\n";
        // Emit labels that will give the function's size in the assembly listing file.
        out << "funcend_" << e.fields[0] << "\tEQU *\n";
        out << "funcsize_" << e.fields[0] << "\tEQU\tfuncend_" << e.fields[0] << "-_" << e.fields[0] << "\n";
        break;
    case SECTION_START:
        out << "\n\n\t""SECTION\t" << e.fields[0] << "\n\n\n";
        break;
    case SECTION_END:
        out << "\n\n\t""ENDSECTION\n\n\n";
        break;
    case EXPORT:
        out << e.fields[0] << "\t""EXPORT\n";
        break;
    case IMPORT:
        out << e.fields[0] << "\t""IMPORT\n";
        break;
    case END:
        out << "\t""END\n";
        break;
    default: assert(!"unsupported element");
    }
}


bool
ASMText::writeFile(ostream &out) const
{
    for (vector<Element>::const_iterator it = elements.begin(); it != elements.end(); ++it)
        writeElement(out, *it);

    return out.good();
}


// Returns a comma-separated list of the registers whose bit is set
// in the given bit field, based on the private enum.
//
string
ASMText::listRegisters(uint8_t registers)
{
    stringstream ss;
    static const char *names[] =
    {
        "PC", "U", "Y", "X", "DP", "B", "A", "CC"  // bit 7 to bit 0
    };
    for (size_t i = 0; i < 8; ++i, registers <<= 1)
        if (registers & 0x80)
        {
            if (ss.tellp() > 0)
                ss << ",";
            ss << names[i];
        }
    return ss.str();
}


// Fills members 'read' and 'written' with bits representing the registers
// that are read or written by the instruction in 'e'.
// Those members remain zero if 'e' is not an instruction.
//
// N.B.: The CC bit of 'read' and 'written' is not set for every instruction that actually changes the flags.
//       It is set for branches and "TFR CC,__", because those instructions are involved in particular cases.
//       Example: !(k & m) gives { ANDB __; TFR CC,B } and we do not want stripOpToDeadReg() to remove the ANDB
//                even though B is dead.
//       Most of the code emitted by the compiler does not handle CC directly.
//
ASMText::InsEffects::InsEffects(const Element &e)
:   read(0), written(0)
{
    if (e.type != INSTR)
        return;

    const string &ins = e.fields[0];
    const string &arg = e.fields[1];

    bool disregardArgument = false;

    // Analyze opcode.
    //
    if (ins == "BITA" || ins == "TSTA")
        read |= A;
    else if (ins == "BITB" || ins == "TSTB")
        read |= B;
    else if (ins == "BSR" || ins == "LBSR" || ins == "JSR")
        read |= A | B | X | Y | U, written = read;  // be pessimistic
    else if (ins[0] == 'B')  // all other B instructions are conditional branches
        ;
    else if (ins == "LDD")
        written |= A | B;
    else if (ins == "LDA" || ins == "CLRA")
        written |= A;
    else if (ins == "LDB" || ins == "CLRB")
        written |= B;
    else if (ins == "LDX" || ins == "LEAX")
        written |= X;
    else if (ins == "LDY" || ins == "LEAY")
        written |= Y;
    else if (ins == "LDU" || ins == "LEAU")
        written |= U;
    else if (ins == "STD")
        read |= A | B;
    else if (ins == "STA")
        read |= A;
    else if (ins == "STB")
        read |= B;
    else if (ins == "STX")
        read |= X;
    else if (ins == "STY")
        read |= Y;
    else if (ins == "STU")
        read |= U;
    else if (ins == "SEX")
        read |= B, written |= A;
    else if (ins == "CMPD")
        read |= A | B;
    else if (ins == "CMPA")
        read |= A;
    else if (ins == "CMPB")
        read |= B;
    else if (ins == "CMPX")
        read |= X;
    else if (ins == "MUL")
        read |= A | B, written = A | B;
    else if (ins == "ADDD" || ins == "SUBD")
        read |= A | B, written |= A | B;
    else if (ins == "ADDA" || ins == "SUBA" || ins == "INCA"
            || ins == "COMA" || ins == "NEGA"
            || ins == "LSLA" || ins == "LSRA" || ins == "ASRA"
            || ins == "ROLA" || ins == "RORA"
            || ins == "ANDA" || ins == "ORA" || ins == "EORA")
        read |= A, written |= A;
    else if (ins == "ADDB" || ins == "SUBB" || ins == "INCB"
            || ins == "COMB" || ins == "NEGB"
            || ins == "LSLB" || ins == "LSRB" || ins == "ASRB"
            || ins == "ROLB" || ins == "RORB"
            || ins == "ANDB" || ins == "ORB" || ins == "EORB")
        read |= B, written |= B;
    else if (isConditionalBranch(ins.c_str()))
        read |= CC;
    else if (startsWith(ins, "LB"))  // LBRA or LBRN
        ;
    else if (ins == "PSHS")
        read |= parsePushPullArg(arg) & ~CC;
    else if (ins == "PULS")
        written |= parsePushPullArg(arg) & ~CC;
    else if (ins == "LDS" || ins == "LEAS" || ins == "INC" || ins == "DEC" || ins == "CLR")
        ;
    else if (ins == "RTS" || ins == "RTI")
        ;
    else if (ins == "TFR" || ins == "EXG" || ins == "TST")
        ;  // processed below
    else if (ins == "ABX")
        read = B | X, written = X;
    else if (ins == "ANDCC" || ins == "ORCC")
        ;
    else if (ins == "RMB" || ins == "ZMB" || ins == "FCB" || ins == "FDB" || ins == "FCC")
        disregardArgument = true;
    else if ((ins == "COM" || ins == "NEG" || ins == "CLR") && isConstantOffsetFromX(arg))
        read = X;
    else
        errormsg("failed to determine registers affected by opcode of %s %s", ins.c_str(), arg.c_str());

    // Analyze argument.
    //
    if (disregardArgument)
        ;
    else if (ins == "TFR" || ins == "EXG")
    {
        uint8_t firstReg, secondReg;
        getRegPairNames(arg, firstReg, secondReg);
        read    |= firstReg;
        written |= secondReg;
    }

    if (endsWith(arg, ",X"))
        read |= X;
    else if (endsWith(arg, ",Y"))
        read |= Y;
    else if (endsWith(arg, ",U"))
        read |= U;

    if (arg == ",X+" || arg == ",X++")
        read |= X;
    else if (arg == ",Y+" || arg == ",Y++")
        read |= Y;
    else if (arg == ",U+" || arg == ",U++")
        read |= U;

    if (ins != "PSHS" && ins != "PULS")
    {
        if (startsWith(arg, "D,"))
            read |= A | B;
        else if (startsWith(arg, "A,"))
            read |= A;
        else if (startsWith(arg, "B,"))
            read |= B;
    }

    if (arg[0] == '[' && (endsWith(arg, ",S]")
                               || endsWith(arg, ",X]")
                               || endsWith(arg, ",U]")
                               || endsWith(arg, ",Y]")  // relevant with OS-9
                               || endsWith(arg, ",PCR]")))
    {
        if (arg[2] == ',')  // if "[_,reg]", then look at "_"
        {
            switch (arg[1])
            {
            case 'A': read |= A; break;
            case 'B': read |= B; break;
            case 'D': read |= A | B; break;
            }
        }
        switch (arg[arg.length() - 2])  // look at index register used (do not care about S or PC)
        {
        case 'X': read |= X; break;
        case 'Y': read |= Y; break;
        case 'U': read |= U; break;
        }

    }
}


// Returns true iff arg =~ /^-?\d+,X$/.
bool
ASMText::isConstantOffsetFromX(const std::string &arg)
{
    if (!endsWith(arg, ",X"))
        return false;
    if (arg.length() == 2)
        return true;  // just ",X"
    // Require -\d+ until comma.
    size_t i = 0;
    if (arg[i] == '-')
        ++i;
    size_t numStart = i;
    while (isdigit(arg[i]))
        ++i;
    if (i == numStart)
        return false;  // no digits seen
    return i + 2 == arg.length();  // only ",X" after digits
}


string
ASMText::InsEffects::toString() const
{
    stringstream ss;
    ss << "(" << listRegisters(read) << "), (" << listRegisters(written) << ")";
    return ss.str();
}


uint8_t
ASMText::parseRegName(const char *name)
{
    switch (name[0])
    {
        case 'P': return PC;
        case 'U': return U;
        case 'Y': return Y;
        case 'X': return X;
        case 'B': return B;
        case 'A': return A;
        case 'C': return CC;
        case 'D': return name[1] != 'P' ? (A | B) : DP;
        default:  assert(!"unrecognized register name"); return 0;
    }
}


// arg: Must be a comma-separated pair of upper-case register names, e.g., "X,Y".
//
void
ASMText::getRegPairNames(const string &arg, uint8_t &firstReg, uint8_t &secondReg)
{
    size_t commaPos = arg.find(',');
    assert(commaPos > 0 && commaPos < string::npos);
    firstReg  = parseRegName(arg.c_str());
    secondReg = parseRegName(arg.c_str() + (commaPos + 1));
}


// Returns a bit field representation of the comma-separated list
// of register names in 'arg'.
//
uint8_t
ASMText::parsePushPullArg(const string &arg)
{
    uint8_t regs = 0;
    for (size_t len = arg.length(), i = 0; i < len; ++i)
    {
        switch (toupper(arg[i]))
        {
        case ',': break;
        case 'P': ++i; break;  // don't care about PC
        case 'U': regs |= U; break;
        case 'Y': regs |= Y; break;
        case 'X': regs |= X; break;
        case 'B': regs |= B; break;
        case 'A': regs |= A; break;
        case 'C': regs |= CC; ++i; break;
        case 'D':
            if (i + 1 < len && toupper(arg[i + 1]) == 'P')
                ;  // don't care about DP
            else
                regs |= A | B;
            break;
        }
    }
    return regs;
}


bool
ASMText::InsEffects::onlyDecimalDigits(const string &s)
{
    for (size_t len = s.length(), i = 0; i < len; ++i)
        if (!isdigit(s[i]))
            return false;
    return true;
}


// Starts checking 's' of the given character offset.
// Ignores preceding characters.
//
bool
ASMText::InsEffects::onlyHexDigits(const string &s, size_t offset)
{
    for (size_t len = s.length(), i = offset; i < len; ++i)
        if (!isxdigit(s[i]))
            return false;
    return true;
}


bool
ASMText::isYRegisterAllowed() const
{
    return yRegAllowed
            && ! TranslationUnit::instance().targetPlatformUsesY();
}


// Look for this:
//      LDD     #<constant>
//      TFR     D,X
//      LDX     #<constant>
// or:
//      LDB     #<constant>
//      SEX
//      TFR     D,X
// at the end of elements[], and replace with:
//      LDX     #<constant>     (sign-extended if SEX involved)
//
void
ASMText::optimizeXParameterLoad()
{
    const size_t sz = elements.size();
    if (sz < 3)
        return;

    if (isInstrWithImmedArg(sz - 3, "LDD") && isInstr(sz - 2, "TFR", "D,X") && isInstrWithImmedArg(sz - 1, "LDX"))
    {
        commentOut(sz - 3, "optim: optimizeXParameterLoad (case 1)");
        commentOut(sz - 2, "optim: optimizeXParameterLoad (case 1)");
        return;
    }

    if (isInstrWithImmedArg(sz - 3, "LDB") && isInstrAnyArg(sz - 2, "SEX") && isInstr(sz - 1, "TFR", "D,X"))
    {
        uint16_t argValue = extractImmedArg(sz - 3);
        if (argValue >= 0x80)  // sign-extend to replace SEX
            argValue |= 0xFF00;
        commentOut(sz - 3, "optim: optimizeXParameterLoad (case 2)");
        commentOut(sz - 2, "optim: optimizeXParameterLoad (case 2)");
        elements[sz - 1].fields[0] = "LDX";
        elements[sz - 1].fields[1] = "#" + wordToString(argValue, true);
        elements[sz - 1].fields[2] += " (optim: optimizeXParameterLoad, case 2)";
        return;
    }

    #if 0  /* Other potential case. To be activated if a test case can be found. */
    if (isInstrAnyArg(sz - 3, "CLRA") && isInstrWithImmedArg(sz - 2, "LDB") && isInstr(sz - 1, "TFR", "D,X"))
    {
        string immediateArg = elements[sz - 2].fields[1];  // e.g., "#$2A"
        commentOut(sz - 3, "optim: optimizeXParameterLoad (case 3)");
        commentOut(sz - 2, "optim: optimizeXParameterLoad (case 3)");
        elements[sz - 1].fields[0] = "LDX";
        elements[sz - 1].fields[1] = immediateArg;
        elements[sz - 1].fields[2] += " (optim: optimizeXParameterLoad, case 3)";
        return;
    }
    #endif
}


// Look for this:
//      LDD     <foo>
//      PSHS    B,A
//      LDD     <bar>
//      PSHS    B,A
// at the end of elements[], and replace with:
//      LDX     <foo>
//      LDD     <bar>
//      PSHS    X,B,A
// This is appropriate when pushing arguments to a function, knowing that
// X is a work register.
//
void
ASMText::optimizeConsecutiveFunctionArguments()
{
    const size_t sz = elements.size();
    if (sz < 4)
        return;

    if (   isInstrAnyArg(sz - 4, "LDD")
        && isInstr(      sz - 3, "PSHS", "B,A")
        && isInstrAnyArg(sz - 2, "LDD")
        && isInstr(      sz - 1, "PSHS", "B,A"))
    {
        elements[sz - 4].fields[0] = "LDX";  // replace 1st LDD
        elements[sz - 4].fields[2] += " (optim: optimizeConsecutiveFunctionArguments)";
        commentOut(sz - 3, "optim: optimizeConsecutiveFunctionArguments");  // remove 1st push
        elements[sz - 1].fields[1] = "X,B,A";
        elements[sz - 1].fields[2] += " (optim: optimizeConsecutiveFunctionArguments)";
        return;
    }
}


// Looks for this at the end of elements[]:
//      LDX     {foo}
//      TFR     X,D
// with the assumption that X is dead afterwards.
// Replaced with:
//      LDD     {foo}
//
void
ASMText::optimizeTFRXD()
{
    const size_t sz = elements.size();
    if (sz < 2)
        return;
    if (! isInstrAnyArg(sz - 2, "LDX"))
        return;
    if (! isInstr(sz - 1, "TFR", "X,D"))
        return;

    elements[sz - 2].fields[0] = "LDD";
    elements.pop_back();
}
