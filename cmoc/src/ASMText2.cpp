/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2026 Pierre Sarrazin <http://sarrazip.com/>
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

///////////////////////////////////////////////////////////////////////////////
//
// This file contains optimizing methods, i.e., those that go in the
// ASMText::level1OptimFuncs[] and ASMText::level2OptimFuncs[] arrays.
//
///////////////////////////////////////////////////////////////////////////////


#include "ASMText.h"

#include "Pseudo6809.h"
#include "TranslationUnit.h"
#include "FunctionDef.h"

#include <iomanip>
#include <climits>

using namespace std;


bool
ASMText::branchToNextLocation(size_t index)
{
    if (index + 1 >= elements.size())  // pattern has 2 instructions
        return false;
    if (!isInstrAnyArg(index, "LBRA") && !isInstrAnyArg(index, "BRA"))  // require uncond. branch
        return false;
    if (!isLabel(index + 1, elements[index].fields[1]))  // require next element to be a label equal to branch argument
        return false;
    commentOut(index, "optim: branchToNextLocation");
    return true;
}


#if 0  /* Suspended because it can break switch() statements, as observed with the self tests. */
bool
ASMText::instrFollowingUncondBranch(size_t index)
{
    if (!isInstrAnyArg(index, "LBRA") && !isInstrAnyArg(index, "BRA"))  // require uncond. branch
        return false;
    bool modified = false;
    for (;;)
    {
        index = findNextInstrBeforeLabel(index + 1);  // find next instr. unless label/non-instr. is seen first
        if (index == size_t(-1))
            break;
        commentOut(index, "optim: instrFollowingUncondBranch");
        modified = true;
    }
    return modified;
}
#endif


bool
ASMText::lddToLDB(size_t index)
{
    if (!isInstrAnyArg(index, "LDD"))
        return false;
    Element &load = elements[index];
    if (load.fields[1].find("#$") != 0)  // if LDD is not immediate and in hex
        return false;
    size_t nextInstrIndex = findNextInstr(index + 1);  // find next instr., even over a label
    if (nextInstrIndex == size_t(-1))
        return false;
    if (!isInstr(nextInstrIndex, "SEX", "") && !isInstr(nextInstrIndex, "CLRA", ""))
        return false;

    // Register A is dead, so only load B.
    load.fields[0] = "LDB";

    // Make sure immediate value is 8 bits.
    char *end;
    unsigned long n = strtoul(load.fields[1].c_str() + 2, &end, 16);
    load.fields[1] = "#" + wordToString(uint16_t(n) & 0xFF, true);
    load.fields[2] = "optim: lddToLDB";
    return true;
}


// Determine if we have this pattern starting at elements[index]:
//    PSHS    B,A
//    LDD     #$__
//    LEAS    1,S
//    ADDB    ,S+
// This can be replaced with ADDB #$__.
//
bool
ASMText::pushLoadDiscardAdd(size_t index)
{
    if (index + 3 >= elements.size())  // pattern has 4 instructions
        return false;
    if (   isInstr(index, "PSHS", "B,A")
        && isInstrWithImmedArg(index + 1, "LDD")
        && isInstr(index + 2, "LEAS", "1,S")
        && isInstr(index + 3, "ADDB", ",S+"))
    {
        replaceWithInstr(index, "ADDB", "#" + wordToString(extractImmedArg(index + 1) & 0xFF, true), "optim: pushLoadDiscardAdd");
        commentOut(index + 1);
        commentOut(index + 2);
        commentOut(index + 3);
        return true;
    }
    return false;
}


// Check for this pattern:
//    PSHS    B
//    LDB     immediate/,U/,PCR
//    ADDB    ,S+
//
bool
ASMText::pushBLoadAdd(size_t index)
{
    if (index + 2 >= elements.size())  // pattern has 3 instructions
        return false;
    if (   isInstr(index, "PSHS", "B")
        && isInstrWithVarArg(index + 1, "LDB")
        && isInstr(index + 2, "ADDB", ",S+"))
    {
        replaceWithInstr(index, "ADDB", elements[index + 1].fields[1], "optim: pushBLoadAdd");
        commentOut(index + 1, "optim: pushBLoadAdd");
        commentOut(index + 2, "optim: pushBLoadAdd");
        return true;
    }
    return false;
}


// Check for this pattern:
//    PSHS    B,A
//    LDD     <foo>
//    ADDD    ,S++
// Change it to this:
//    ADDD    <foo>
//
bool
ASMText::pushDLoadAdd(size_t index)
{
    if (! isInstr(index, "PSHS", "B,A"))
        return false;
    size_t ldd = findNextInstrBeforeLabel(index + 1);
    if (! isInstrAnyArg(ldd, "LDD"))
        return false;
    const string &lddArg = elements[ldd].fields[1];
    if (lddArg == ",S")  // reject if LDD ,S
        return false;
    size_t addd = findNextInstrBeforeLabel(ldd + 1);
    if (! isInstr(addd, "ADDD", ",S++"))
        return false;

    // Apply.
    replaceWithInstr(addd, "ADDD", lddArg, "optim: pushDLoadAdd");
    commentOut(index, "optim: pushDLoadAdd");
    commentOut(ldd, "optim: pushDLoadAdd");
    return true;
}


// Check for this pattern:
//    PSHS    B,A
//    LDD     immediate/,U/,PCR  ; arg does not depend on X
//    LDX     ,S++
//
bool
ASMText::pushLoadDLoadX(size_t index)
{
    if (index + 2 >= elements.size())  // pattern has 3 instructions
        return false;
    if (   isInstr(index, "PSHS", "B,A")
        && (isInstrWithVarArg(index + 1, "LDD") || isInstrWithImmedArg(index + 1, "LDD"))
        && isInstr(index + 2, "LDX", ",S++"))
    {
        replaceWithInstr(index, "TFR", "D,X", "optim: pushLoadDLoadX");
        commentOut(index + 2);
        return true;
    }
    return false;
}


// Check for this pattern:
//   PSHS B,A
//   LDX ,S++
//   LDB/LDD #...
//   LBSR MUL168/MUL16
// Replace PSHS & LDX with TFR D,X.
// This is safe because the LBSR will overwrite D, so we know the original D won't be needed.
//
bool
ASMText::pushDLoadX(size_t index)
{
    if (index + 3 >= elements.size())  // pattern has 4 instructions
        return false;
    if (!isInstr(index,     "PSHS", "B,A"))
        return false;
    if (!isInstr(index + 1, "LDX", ",S++"))
        return false;
    if ((isInstrWithImmedArg(index + 2, "LDB") && isInstr(index + 3, "LBSR", "MUL168"))
        || (isInstrWithImmedArg(index + 2, "LDD") && isInstr(index + 3, "LBSR", "MUL16" )))
    {
        replaceWithInstr(index, "TFR", "D,X", "optim: pushDLoadX");  // replace PSHS
        commentOut(index + 1, "optim: pushDLoadX");  // remove LDX
        return true;
    }
    return false;
}


// Check for this pattern:
//    PSHS    B,A
//    LDX     immediate/,U/,PCR
//    LDD     ,S++
//
bool
ASMText::pushDLoadXLoadD(size_t index)
{
    if (index + 2 >= elements.size())  // pattern has 3 instructions
        return false;
    if (   isInstr(index, "PSHS", "B,A")
        && (isInstrWithVarArg(index + 1, "LDX") || isInstrWithImmedArg(index + 1, "LDX"))
        && isInstr(index + 2, "LDD", ",S++"))
    {
        commentOut(index, "optim: pushDLoadXLoadD");
        commentOut(index + 2);
        return true;
    }    
    return false;
}


bool
ASMText::replaceLDDZero(size_t index)
{
    if (!isInstrAnyArg(index, "LDD"))
        return false;
    const string &arg = elements[index].fields[1];
    if (arg != "#$00" && arg != "#0")
        return false;

    insertInstr(index, "CLRB");
    // Replace the LDD, which the insertion has moved forward:
    replaceWithInstr(index + 1, "CLRA");
    return true;
}

// Optimize two consecutive LDr instructions, where r is A, B or D,
// with no label in between. Remove the 1st load.
// r must not be X, U or S, because we could have LDX ____; LDX ,X, where
// the 1st load is used by the 2nd.
//
bool
ASMText::stripConsecutiveLoadsToSameReg(size_t index)
{
    if (index + 1 >= elements.size())  // pattern uses 2 or 3 elements
        return false;
    Element &e = elements[index];
    if (! (e.type == INSTR && e.fields[0].find("LD") == 0 && strchr("ABD", e.fields[0][2])))  // require LD{A,B,D} instruction
        return false;

    // Check if next element is a comment or instruction.
    size_t nextInstrIndex = findNextInstrBeforeLabel(index + 1);  // skips comments, not labels
    if (nextInstrIndex == size_t(-1))
        return false;
    Element &nextInstr = elements[nextInstrIndex];

    // Require same instruction.
    if (! (nextInstr.type == INSTR && nextInstr.fields[0] == e.fields[0]))
        return false;

    // Case that can pop up
    if ((nextInstr.fields[0][2] == 'B') &&
        (nextInstr.fields[1] == "D,X" || nextInstr.fields[1] == "B,X" ||
         nextInstr.fields[1] == "[D,X]" || nextInstr.fields[1] == "[B,X]"))
      return false;

    // Optimize.
    commentOut(index, "optim: stripConsecutiveLoadsToSameReg");

    // Indicate that the optimization was applied.
    return true;
}


// Check for this pattern:
//    STB <arg>
//    LDB <arg>
// Removes the load.
// Accepts comments between the two instructions.
//
bool
ASMText::storeLoad(size_t index)
{
    const Element &e = elements[index];
    if (! (e.type == INSTR && e.fields[0].find("ST") == 0 && strchr("ABD", e.fields[0][2]) != NULL))  // require ST{A,B,D}
        return false;
    size_t nextInstrIndex = findNextInstrBeforeLabel(index + 1);  // skips comments, not labels
    if (nextInstrIndex == size_t(-1))
        return false;
    Element &nextInstr = elements[nextInstrIndex];
    if (! (nextInstr.fields[0].find("LD") == 0 && nextInstr.fields[0][2] == e.fields[0][2]))  // require LD of same register
        return false;
    if (nextInstr.fields[1] != e.fields[1])  // if not same argument
        return false;
    if (isAbsoluteAddress(e.fields[1]))
        return false;  // assume content at address is volatile (e.g., I/O port at $FFxx)
    commentOut(nextInstrIndex, "optim: storeLoad");
    return true;
}


// Check for this pattern:
//      LBxx foo
//      LBRA bar
// foo:
// Replace LBxx with 'LB!xx bar' and remove LBRA.
//
bool
ASMText::condBranchOverUncondBranch(size_t index)
{
    size_t condBranchIndex = index;
    char inverseBranchInstr[INSTR_NAME_BUFSIZ];
    if (! isConditionalBranch(condBranchIndex, inverseBranchInstr, true))  // require cond. branch as 1st instr.
        return false;

    size_t uncondBranchIndex = findNextInstrBeforeLabel(condBranchIndex + 1);
    if (!isInstrAnyArg(uncondBranchIndex, "LBRA") && !isInstrAnyArg(uncondBranchIndex, "BRA"))  // require uncond. branch before label
        return false;

    Element &condBranch = elements[condBranchIndex];
    size_t labelIndex = uncondBranchIndex + 1;
    if (!isLabel(labelIndex, condBranch.fields[1]))  // require a label next, and that the cond. branch jump to this label
        return false;

    // Apply the optimization.
    condBranch.fields[0] = inverseBranchInstr;
    condBranch.fields[1] = elements[uncondBranchIndex].fields[1];
    condBranch.fields[2] += " (optim: condBranchOverUncondBranch)";
    commentOut(uncondBranchIndex, "optim: condBranchOverUncondBranch");
    return true;
}


// If 'index' is a long branch, try to convert it to a short branch.
// This is done when the target label is no farther than 28 instructions
// from the branch. Assuming at most 4 bytes per instruction, this means
// at most a 112-byte offset, which is well below the limit of 127.
// No shortening is done if inline assembly appears between the branch
// and its destination. (This optimization does not try to measure
// the machine language produced by the inline assembly.)
//
bool
ASMText::shortenBranch(size_t index)
{
    assert(index < elements.size());
    Element &e = elements[index];
    if (! (e.type == INSTR && e.fields[0].find("LB") == 0))  // require long branch
        return false;

    size_t targetLabelIndex = findLabelIndex(e.fields[1]);
    if (targetLabelIndex == size_t(-1))
        return false;  // unexpected

    size_t begin = (index <= targetLabelIndex ? index : targetLabelIndex);
    size_t end   = (index >  targetLabelIndex ? index : targetLabelIndex);
    size_t numInstr = 0;
    for (size_t i = begin; i <= end; ++i)
    {
        const Element &t = elements[i];
        if (t.type == INLINE_ASM || t.type == INCLUDE || t.type == SEPARATOR)
            return false;  // do not optimize if these are in range
        if (t.type == INSTR)
            ++numInstr;
    }
    if (numInstr > 36)  // was 28 before CMOC 0.1.78
        return false;  // too far: short branch may not be able to reach

    e.fields[0].erase(0, 1);  // remove 'L'
    return true;
}


// Optimize indexing into fixed pointers. Optimize the following sequence:
//  LDD XXXX
//  TFR D,X
//  LDD YYYY
//  LEAX D,X
//
// To
//  LDX #$XXXX
//  LDD YYYY
//  LEAX D,X
//
bool
ASMText::fasterPointerIndexing(size_t index)
{
    if (index + 3 >= elements.size())  // pattern uses 4 elements
        return false;

    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "LDD"))   // require LDD
        return false;

    Element &e2 = elements[index + 1];
    if (! (e2.type == INSTR && e2.fields[0] == "TFR" &&  // require TFR D,X
           e2.fields[1] == "D,X"))
        return false;

    Element &e3 = elements[index + 2];
    if (! (e3.type == INSTR && e3.fields[0] == "LDD"))   // require LDD
        return false;

    Element &e4 = elements[index + 3];
    if (! (e4.type == INSTR && e4.fields[0] == "LEAX" && // require LEAX D,X
           e4.fields[1] == "D,X"))
        return false;

    replaceWithInstr(index, "LDX", e1.fields[1], "optim: fasterPointerIndexing");
    commentOut(index + 1);

    return true;
}


// Optimize pushing pointers onto the stack. Optimize the following sequence:
//  LEAX XXXX,U
//  TFR X,D
//  ADDD #YYYY
//  PSHS B,A
//
// To
//  LEAX #XXXX+YYYY,U
//  PSHS X
//
// Note that this assumes that the #YYYY values in the D register is not
// used later on. Testing so far has confirmed that this is a safe assumption.
bool
ASMText::fasterPointerPushing(size_t index)
{
    if (index + 3 >= elements.size())  // pattern uses 43 elements
        return false;

    const Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "LEAX" && // require LEAX XXXX,U
           e1.fields[1].find(",U") != string::npos))
        return false;

    const Element &e2 = elements[index + 1];
    if (! (e2.type == INSTR && e2.fields[0] == "TFR" &&  // require TFR D,X
           e2.fields[1] == "X,D"))
        return false;

    const Element &e3 = elements[index + 2];
    if (! (e3.type == INSTR && e3.fields[0] == "ADDD" && // require ADDD #YYYY
           e3.fields[1].find("#") == 0))
        return false;

    const Element &e4 = elements[index + 3];
    if (! (e4.type == INSTR && e4.fields[0] == "PSHS" && // require PSHS B,A
           e4.fields[1] == "B,A"))
        return false;

    // Add the offset from the LEAX to the addened in the ADDD
    const char *strOffset = e1.fields[1].c_str();
    long offset = (strOffset[0] == '$' ? strtol(strOffset + 1, NULL, 16) : strtol(strOffset, NULL, 10));
    if (e3.fields[1][1] == '$')
      offset += strtol(e3.fields[1].c_str() + 2, NULL, 16);
    else
      offset += strtol(e3.fields[1].c_str() + 1, NULL, 10);

    replaceWithInstr(index, "LEAX", intToString(int16_t(offset)) + ",U", "optim: fasterPointerPushing");
    replaceWithInstr(index + 1, "PSHS", "X", "optim: fasterPointerPushing");
    commentOut(index + 2, "optim: fasterPointerPushing");
    commentOut(index + 3, "optim: fasterPointerPushing");

    return true;
}


// Remove all CLR[A/B] after a CLR[A/B] but before other instructions
// that might change those registers.
//
bool
ASMText::stripExtraClrA_B(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && (e1.fields[0] == "CLRA" || e1.fields[0] == "CLRB"))) // require CLR[A/B]
        return false;

    const string &ins = e1.fields[0];
    const uint8_t mask = uint8_t((ins == "CLRA") ? A : B);
    const string andInstr = (ins == "CLRA") ? "ANDA" : "ANDB";

    bool madeChanges = false;
    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if ((e.type != INSTR) && (e.type != COMMENT)) {
        break;
      } else if (e.type == INSTR && e.fields[0] == ins) {
        commentOut(index, "optim: stripExtraClrA_B");
        madeChanges = true;
      } else if (e.type == INSTR) {
        // Replace AND_ ,S+ with LEAS 1,S. Not any fater, buy opens up more optimizations
        if ((e.fields[0] == andInstr) && (e.fields[1] == ",S+")) {
          replaceWithInstr(index, "LEAS", "1,S", "optim: stripExtraClrA_B");
          madeChanges = true;
        } else {
          const InsEffects insEffects(e);
          if (isBasicBlockEndingInstruction(e) || (insEffects.written & mask))
            break;
        }
      }
    }

    return madeChanges;
}


// Remove PSHS/PULS X when the PSHS is either a PSHS B,A or PSHS X
// and there are no instructions in between that can change X.
//
bool
ASMText::stripExtraPulsX(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR &&
           (e1.fields[0] == "PSHS" &&
            (e1.fields[1] == "B,A" ||
             e1.fields[1] == "X"))))
        return false;

    // Find matching PULS X
    const size_t startIndex = index;
    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if ((e.type != INSTR) && (e.type != COMMENT)) {
        return false;
      } else if (e.type == INSTR && e.fields[0] == "PULS" && e.fields[1] == "X") {
        break;
      } else if (e.type == INSTR) {
        const InsEffects insEffects(e);
        if (isBasicBlockEndingInstruction(e) || (insEffects.written & X) ||
            ((insEffects.read & X) && (e1.fields[1] == "B,A")) ||
            e.fields[0] == "BSR" || e.fields[0] == "LBSR" ||
            e.fields[0] == "PSHS" || e.fields[0] == "PULS" || e.fields[0] == "LEAS" ||
            e.fields[1].find(",S") != string::npos) {
          return false;
        }
      }
    }
    const size_t endIndex = index;

    // Remove the PSHS
    if (e1.fields[1] == "B,A")
      replaceWithInstr(startIndex, "TFR", "D,X", "optim: stripExtraPulsX");
    else
      commentOut(startIndex, "optim: stripExtraPulsX");

    // Remove the PULS
    commentOut(endIndex, "optim: stripExtraPulsX");

    return true;
}


// Remove PSHS B/LDB ,S+ when there are no instructions in between
// that can modify the B.
//
bool
ASMText::stripExtraPushPullB(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR &&
           e1.fields[0] == "PSHS" && e1.fields[1] == "B"))
        return false;

    // Find matching LDB ,S+
    const size_t startIndex = index;
    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if ((e.type != INSTR) && (e.type != COMMENT)) {
        return false;
      } else if (e.type == INSTR && e.fields[0] == "LDB" && e.fields[1] == ",S+") {
        commentOut(index, "optim: stripExtraPushPullB");
        break;
      } else if (e.type == INSTR) {
        const InsEffects insEffects(e);
        if (isBasicBlockEndingInstruction(e) || (insEffects.written & B) ||
            e.fields[0] == "PSHS" || e.fields[0] == "PULS" || e.fields[0] == "LEAS" ||
            e.fields[1].find(",S") != string::npos) {
          return false;
        }
      }
    }
    const size_t endIndex = index;

    // Remove the PSHS and LDB
    commentOut(startIndex, "optim: stripExtraPushPullB");
    commentOut(endIndex, "optim: stripExtraPushPullB");

    return true;
}


// Changes ANDA/B #$00 to CLRA/B
//
bool
ASMText::andA_B0(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR &&
           (e1.fields[0] == "ANDA" || e1.fields[0] == "ANDB") &&
          (e1.fields[1] == "#$00")))
        return false;

    const char instr[] = { 'C', 'L', 'R', e1.fields[0][3], '\0' };
    replaceWithInstr(index, instr, "", "optim: andA_B0");
    return true;
}


// Change LDD instruction after a CLRA that load only 8 bit literals
// to LDB.
//
bool
ASMText::changeLoadDToLoadB(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "CLRA")) // require CLRA
        return false;

    bool madeChanges = false;
    for (index++; index < elements.size(); index++)
    {
        Element &e = elements[index];
        if (e.type != INSTR)
            break;
        if (e.fields[1].length() <= 4
            && e.fields[0] == "LDD"
            && startsWith(e.fields[1], "#$"))
        {
            replaceWithInstr(index, "LDB", e.fields[1], "optim: changeLoadDToLoadB");
            madeChanges = true;
        }
        else if (isBasicBlockEndingInstruction(e) || (InsEffects(e).written & A))
            break;
    }

    return madeChanges;
}


// Change ADDD instruction before a CLRA to an ADDB
//
bool
ASMText::changeAddDToAddB(size_t index)
{
    if (index + 1 >= elements.size())  // pattern has 2 instructions
        return false;

    // Note that this excludes LDD #0 which is used for branching
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "ADDD" &&
           e1.fields[1].length() > 2 &&
           startsWith(e1.fields[1], "#$")))
        return false;

    // Next instruction must be CLRA
    const Element &e2 = elements[index + 1];
    if (! (e2.type == INSTR && e2.fields[0] == "CLRA"))
        return false;

    // New operand is last 2 digits of ADDD operand.
    const char *digits = e1.fields[1].c_str() + (e1.fields[1].length() - 2);
    const char operand[] = { '#', '$', digits[0], digits[1], '\0' };
    replaceWithInstr(index, "ADDB", operand, "optim: changeAddDToAddB");

    return true;
}


// Remove PUSH A,B/LEAS 1,S when possible
//
bool
ASMText::stripPushLeas1(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "PSHS" && e1.fields[1] == "B,A"))
        return false;
    const size_t startIndex = index;

    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if ((e.type != INSTR) && (e.type != COMMENT)) {
        return false;
      } else if (e.type == INSTR &&
                 e.fields[0] == "LEAS" &&
                 e.fields[1] == "1,S") {
        replaceWithInstr(startIndex, "PSHS", "B", "optim: stripPushLeas");
        commentOut(index, "optim: stripPushLeas1");
        return true;
      } else if (e.type == INSTR) {
        const InsEffects insEffects(e);
        if (isBasicBlockEndingInstruction(e) || 
            e.fields[0] == "LBSR" || e.fields[0] == "BSR" ||
            e.fields[0] == "PSHS" || e.fields[0] == "PULS" ||
            e.fields[0] == "LEAS" || e.fields[1].find(",S") != string::npos) {
          return false;
        }
      }
    }

    return false;
}


// When there is a CLR[A/B] followed by a PSHS B,A and a corresponding
// OR[A/B] ,S+ or AND[A/B], S+ and no instruction that modifies [A/B],
// and no other instructions that manipulates the stack, then it is
// possible to reduce or remove the PSHS and eliminate the OR[A/B] or
// AND[A/B]
//
bool
ASMText::orAndA_B(size_t index)
{
    // Whether or not A/B are known to be zero
    bool aKnown = false, bKnown = false;

    // Start with a CLRA or CLRB
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && (e1.fields[0] == "CLRA" || e1.fields[0] == "CLRB")))
        return false;
    aKnown = (e1.fields[0] == "CLRA");
    bKnown = !aKnown;
    vector< pair<bool, size_t> > stack;  // value: index in elements[]
    stack.reserve(32);

    for(index++; index<elements.size(); index++ && (aKnown || bKnown)) {
      Element &e = elements[index];
      if ((e.type != INSTR) && (e.type != COMMENT)) {
        return false;
      } else if (e.type == INSTR) {
        // Deal with popping off a value
        if (e.fields[0] == "LEAS" && e.fields[1] == "1,S") {
          if (stack.size() == 0)
            return false;
          stack.pop_back();
        }

        // Deal with [AND/OR]A ,S+
        else if ((e.fields[0] == "ANDA" || e.fields[0] == "ORA") && (e.fields[1] == ",S+") &&
            aKnown && (stack.size() > 0) && stack.back().first) {
          Element pshsElement = elements[stack.back().second];
          if (pshsElement.fields[1] == "B,A") {
            pshsElement.fields[1] = "B";
            replaceWithInstr(index, "LEAS", "1,S", "optim: orAndA_B");
            return false;
          } else {
            commentOut(stack.back().second, "optim: orAndA_B");
            commentOut(index, "optim: orAndA_B");
          }
          return true;
        // Deal with [AND/OR]A ,S+
        } else if ((e.fields[0] == "ANDB" || e.fields[0] == "ORB") && (e.fields[1] == ",S+") &&
            bKnown && (stack.size() > 0) && stack.back().first) {
          Element pshsElement = elements[stack.back().second];
          if (pshsElement.fields[1] == "B,A") {
            pshsElement.fields[1] = "B";
            replaceWithInstr(index, "LEAS", "1,S", "optim: orAndA_B");
            return false;
          }
          commentOut(stack.back().second, "optim: orAndA_B");
          commentOut(index, "optim: orAndA_B");
          return true;
        }

        // If it is a push instruction, push current known state of A,B
        else if (e.fields[0] == "PSHS")  {
          if (e.fields[1] == "B,A") {
            stack.push_back(pair<bool, size_t>(bKnown, index));
            stack.push_back(pair<bool, size_t>(aKnown, index));
          } else if (e.fields[1] == "B") {
            stack.push_back(pair<bool, size_t>(bKnown, index));
          } else if (e.fields[1] == "A") {
            stack.push_back(pair<bool, size_t>(aKnown, index));
          } else {
            // Don't deal with other registers
            return false;
          }
        } else {
          const InsEffects insEffects(e);
          if (isBasicBlockEndingInstruction(e) || 
              e.fields[0] == "PULS" || e.fields[0] == "LEAS" ||
              e.fields[1].find(",S") != string::npos) {
            return false;
          }

          // Update the current state of A and B
          else {
            if (insEffects.written & A)
              aKnown = false;
            if (insEffects.written & B)
              bKnown = false;
            if (e.fields[0] == "CLRA")
              aKnown = true;
            if (e.fields[0] == "CLRB")
              bKnown = true;
          }
        }
      }
    }

    return false;
}


// Transforms an 8-bit LDD to CLRA followed by LDB. In some cases this makes
// it possible to optimize out the CLRA.
//
bool
ASMText::loadDToClrALoadB(size_t index)
{
    Element &e1 = elements[index];
    const string &field1 = e1.fields[1];
    if (e1.type == INSTR && e1.fields[0] == "LDD" &&
        field1.size() == 4 &&
        startsWith(field1, "#$") &&
        field1.find("-") == string::npos)
    {
      elements[index].fields[0] = "LDB";
      vector<Element>::iterator it = elements.insert(elements.begin() + index, Element());
      Element &clrA = *it;
      clrA.type = INSTR;
      clrA.fields[0] = "CLRA";
      clrA.liveRegs = 0;
      return true;
    }

    return false;
}


// Sometimes a constant is pushed on the stack via the A or B registers.
// Via the stack, this constant is then ADDed, ORed, ANDed or SUBed.
// When this occurs, the stack operation can be optimized away and
// the constant can be applied to the appropriate register.
//
// This optimization starts by looking for an instruction with a known
// value. If it is destroyed before being placed on a stack, then the
// optimization exits without changes. Otherwise, this optimization
// will continue to run until the end of a basic block is hit or there
// are no known constants on the stack. This optimization will keep
// track of the known constant on the stack until the point it gets
// consumed. When it is consumed, the push will be removed and the
// OP[A/B] ,S+ will be replaced with OP[A/B] #CONSTANT.
bool
ASMText::optimizeStackOperations1(size_t index)
{
    const size_t startIndex = index;
    Pseudo6809 simulator;
    bool firstInstr = true;
    bool canGoOn = true;

    do {
      const Element &e = elements[index];
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      firstInstr = false;

      // Only process non basic block ending instructions
      if (e.type == LABEL)
        break;
      if (e.type != INSTR)
        continue;
      if (isBasicBlockEndingInstruction(e))
        break;

      // Output the instruction
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while(canGoOn && ++index<elements.size() &&
            (simulator.pushedConstant || (simulator.regs.knownRegisters() != 0)));

    // Simulator hit an error
    if (!canGoOn) {
      return false;
    }

    // Go through each line
    for(size_t ii=startIndex; ii<index; ii++) {
      // We can only deal with lines with at least 2 references
      const size_t numRefs = simulator.indexToReferences[ii].size();
      if (numRefs < 2)
        continue;

      // We can only deal with lines that produce 1 constant
      const vector<PossiblyKnownVal<int> > &constantVals = simulator.indexToConstantVals[ii];
      if (constantVals.size() != 1)
        continue;

      // Get the index of the pshs instruction
      const vector<int> &refs = simulator.indexToReferences[ii];
      int foundPushIndex = -1;
      size_t numPushes = 0;
      for(size_t pushIndex=0; pushIndex<numRefs; pushIndex++) {
        if (elements[refs[pushIndex]].fields[0] == "PSHS") {
          numPushes++;
          foundPushIndex = refs[pushIndex];
        }
      }
      if (numPushes != 1)
        continue;
      Element &e1 = elements[foundPushIndex];

      // Must be 1 or 2 byte push
      const int numBytesPushed = simulator.numBytesPushedOrPulled(e1.fields[1]);
      if ((numBytesPushed < 1) || (numBytesPushed > 2))
        continue;

      // Get all the ,S+/,S++ elements
      vector<int> stackRefs;
      stackRefs.reserve(32);
      int numStackBytesRef = 0;
      int lowestRef = INT_MAX;
      for(size_t jj=0; jj<numRefs; jj++) {
        Element &e0 = elements[refs[jj]];
        if (e0.fields[1] == ",S") {
          return false;
        }
        if (e0.fields[1] == ",S+") {
          numStackBytesRef++;
          stackRefs.push_back(refs[jj]);
          if (refs[jj] < lowestRef)
            lowestRef = jj;
        }
        if (e0.fields[1] == ",S++") {
          numStackBytesRef += 2;
          stackRefs.push_back(refs[jj]);
          if (refs[jj] < lowestRef)
            lowestRef = jj;
        }
      }

      // The stack references must be the last items referenced
      if (numRefs - stackRefs.size() != (size_t) lowestRef)
        continue;

      // Must have 1 or 2 refs
      if ((stackRefs.size() == 0) || (stackRefs.size() > 2))
        continue;

      // The number of bytes refed must be <= number of bytes pushed
      if (numStackBytesRef > numBytesPushed)
        continue;

      // If the stack push is not B,A, then the number of bytes
      // pushed have to equal the number of bytes refed
      if ((e1.fields[1] != "B,A") && (numBytesPushed != numStackBytesRef))
        continue;

      // If the stack push is B,A and the number of bytes pushed
      // > the number of bytes refed then we have to transform
      // B,A to either B or A
      bool transformPushBAToA = false;
      bool transformPushBAToB = false;
      if ((e1.fields[1] == "B,A") && (numBytesPushed != numStackBytesRef)) {
        // If the instruction previous to lowestRef is a ,S+, we keep A.
        // If the instruction after lowestRef is a ,S+, we keep B
        const Element &eBefore = elements[refs[lowestRef]-1];
        const Element &eAfter= elements[refs[lowestRef]+1];
        transformPushBAToA = eBefore.fields[1] == ",S+";
        transformPushBAToB = eAfter.fields[1] == ",S+";
 
        // Not sure what todo when they both are stack ops or neither
        if (transformPushBAToA && transformPushBAToB)
          continue;
        if (!transformPushBAToA && !transformPushBAToB)
          continue;
      }

      // Make sure the bytes we pushed are the bytes we pull
      PossiblyKnownValStack pushStackState = simulator.indexToState[foundPushIndex + 1].second;
      const PossiblyKnownValStack &pullStackState0 = simulator.indexToState[stackRefs[0]].second;
      if (pushStackState.empty() || pullStackState0.empty() ||
          pushStackState.top() != pullStackState0.top() || !pushStackState.top().known)
        continue;

      // There was a single 16-bit reference. Make sure both values
      // were constants
      if ((numStackBytesRef == 2) && (stackRefs.size() == 1)) {
        if ((pushStackState.size() < 2) ||
            (pullStackState0.size() < 2))
          continue;
        PossiblyKnownValStack pushStackStateCopy = pushStackState;
        PossiblyKnownValStack pullStackState0Copy = pullStackState0;
        pushStackStateCopy.pop();
        pullStackState0Copy.pop();
        if (pushStackStateCopy.top() != pullStackState0Copy.top() ||
            !pushStackStateCopy.top().known)
          continue;
      }

      if (stackRefs.size() > 1) {
        const PossiblyKnownValStack &pullStackState1 = simulator.indexToState[stackRefs[1]].second;
        pushStackState.pop();
        if (pushStackState.empty() || pullStackState1.empty() ||
            pushStackState.top() != pullStackState1.top() ||
            !pushStackState.top().known)
          continue;
      }

      // We can comment out the instr at index when the number of references
      // = 1 + numStackRefs. Note that if two instructions were used to
      // generate a 16-bit value (clra; ldb), then the second instruction
      // will perform a useless load. We'll clean this up later.
      if (numRefs == (1 + stackRefs.size())) {
        Element &loadElement = elements[ii];
        const string &field0 = loadElement.fields[0];
        const Register targetReg = getRegisterFromName(field0.c_str() + field0.size() - 1);
        if (targetReg == D || numStackBytesRef != 2)
          commentOut(ii,
                     loadElement.fields[0] + " " + loadElement.fields[1] +
                       " optim: optimizeStackOperations1");
      }

      // We can either remove or transform the push
      if (transformPushBAToA) {
        e1.fields[1] = "A";
        e1.fields[2] = "optim: optimizeStackOperations1";
      } else if (transformPushBAToB) {
        e1.fields[1] = "B";
        e1.fields[2] = "optim: optimizeStackOperations1";
      } else {
        commentOut(foundPushIndex, e1.fields[0] + " " + e1.fields[1] + " optim: optimizeStackOperations1");
      }

      // Remove the stack references
      pushStackState = simulator.indexToState[foundPushIndex + 1].second;
      int stackVal = pushStackState.top().val;
      if ((numStackBytesRef == 2) && (stackRefs.size() == 1)) {
        pushStackState.pop();
        stackVal = (stackVal << 8) | pushStackState.top().val;
      }
      elements[stackRefs[0]].fields[1] = "#" + wordToString(int16_t(stackVal));
      elements[stackRefs[0]].fields[2] = "optim: optimizeStackOperations1";
      if (stackRefs.size() > 1) {
        pushStackState.pop();
        elements[stackRefs[1]].fields[1] = "#" + wordToString(int16_t(pushStackState.top().val));
        elements[stackRefs[1]].fields[2] = "optim: optimizeStackOperations1";
      }

      return true;
    }

    return false;
}


// Sometimes an unknown value is pushed on the stack from B register followed
// by loading a constant in the B register which is subsequently ADDed, ORed or
// ANDed with the value on the stack.  When this occurs, the stack push
// operation can be optimized away and the operation can be applied directly to
// the constant.
//
// This optimization starts by looking for a PSHS B instruction followed by a
// LOAD constant followed by an ADDB, ANDB or ORB with the stack value.
//
bool
ASMText::optimizeStackOperations2(size_t index)
{
    if (index + 3 >= elements.size()) {
      return false;
    }

    // First instruction must be a PSHS
    const size_t startIndex = index;
    const Element &pshs = elements[index++];
    if (!(pshs.type == INSTR && pshs.fields[0] == "PSHS" && pshs.fields[1] == "B")) {
      return false;
    }

    // Second instruction must be a LDB #
    const Element &ldb = elements[index++];
    if (!(ldb.type == INSTR && ldb.fields[0] == "LDB" && ldb.fields[1][0] == '#')) {
      return false;
    }

    // Ignore comments, look for ADDB/ORB/ANDB ,S+
    for(Element e = elements[index]; index < elements.size(); e = elements[++index]) {
      if (e.isCommentLike()) {
        continue;
      }
      if (e.type != INSTR) {
        return false;
      }
      if ((e.fields[0] == "ADDB" || e.fields[0] == "ORB" || e.fields[0] == "ANDB") &&
          (e.fields[1] == ",S+")) {
        break;
      }
      return false;
    }

    // Make sure we did not hit the end
    if (index >= elements.size()) {
      return false;
    }

    // We can remove the PSHS, the LD and transform the last op
    Element &lastOp = elements[index];
    lastOp.fields[1] = ldb.fields[1];
    lastOp.fields[2] = "optim: optimizeStackOperations2";
    commentOut(startIndex, "optim: optimizeStackOperations2");
    commentOut(startIndex + 1, "optim: optimizeStackOperations2");

    return true;
}


// Sometimes an unknown value is pushed on the stack from D register followed
// by loading a constant in the D register which is subsequently ADDed
// with the value on the stack.  When this occurs, the stack push
// operation can be optimized away and the operation can be applied directly to
// the constant.
//
// This optimization starts by looking for a PSHS B,A instruction followed by a
// LOAD constant followed by an ADDD.
//
// Note that this is symmteric to optimizeStackOperations2, but 16-bit. However
// it may be that this 16-bit version does not occur in practice.
  bool
ASMText::optimizeStackOperations3(size_t index)
{
    if (index + 3 >= elements.size()) {
      return false;
    }

    // First instruction must be a PSHS
    const size_t startIndex = index;
    const Element &pshs = elements[index++];
    if (!(pshs.type == INSTR && pshs.fields[0] == "PSHS" && pshs.fields[1] == "B,A")) {
      return false;
    }

    // Second instruction must be a LDD #
    const Element &ldd = elements[index++];
    if (!(ldd.type == INSTR && ldd.fields[0] == "LDD" && ldd.fields[1][0] == '#')) {
      return false;
    }

    // Ignore comments, look for ADDD ,S+
    for(Element e = elements[index]; index < elements.size(); e = elements[++index]) {
      if (e.isCommentLike()) {
        continue;
      }
      if (e.type != INSTR) {
        return false;
      }
      if (e.fields[0] == "ADDD" && e.fields[1] == ",S++") {
        break;
      }
      return false;
    }

    // Make sure we did not hit the end
    if (index >= elements.size()) {
      return false;
    }

    // We can remove the PSHS, the LD and transform the last op
    Element &lastOp = elements[index];
    lastOp.fields[1] = ldd.fields[1];
    lastOp.fields[2] = "optim: optimizeStackOperations3";
    commentOut(startIndex, "optim: optimizeStackOperations3");
    commentOut(startIndex + 1, "optim: optimizeStackOperations3");

    return true;
}


// Sometimes a value of the form #XXXX, (,R), (XXXX,R), ([,R]), ([XXXX,R]) is
// loaded to the D register and pushed to the stack when R is either U
// or PCR and XXXX is some offset. If there are no stack operations, references
// to the D register between the load and the subsequent PSHS
// and subsequent ,S++ that consumes the value, then the PSHS and LDD operations
// can be removed and the ,S++ can be replaced by the operand in the original
// load.
//
// This optimization starts by looking for a LDD instruction followed by a PSHS
// followed by an ,S++ instruction.
//
bool
ASMText::optimizeStackOperations4(size_t index)
{
    // First instruction must be a LDD with no pre or post decrement
    const size_t startIndex = index;
    Element &ldd = elements[index];
    if (!(ldd.type == INSTR && ldd.fields[0] == "LDD")
        || ldd.fields[1].find("D,") != string::npos
        || startsWith(ldd.fields[1], ",-")
        || startsWith(ldd.fields[1], "[,-")
        || startsWith(ldd.fields[1], ",X+")
        || startsWith(ldd.fields[1], ",Y+")
        || startsWith(ldd.fields[1], ",S")
        || startsWith(ldd.fields[1], ",U+")
        || startsWith(ldd.fields[1], "[,X+")
        || startsWith(ldd.fields[1], "[,Y+")
        || startsWith(ldd.fields[1], "[,S")
        || startsWith(ldd.fields[1], "[,U+")) {
      return false;
    }
    InsEffects lddEffects(ldd);

    // Next instruction must be a PSHS B,A
    if (++index >= elements.size()) {
      return false;
    }
    const Element &el = elements[index];
    if ((el.type != INSTR) || (el.fields[0] != "PSHS") || (el.fields[1] != "B,A")) {
      return false;
    }

    // Now we must find a ,S++ instruction
    size_t popIndex = startIndex;
    uint8_t currentKnown = A | B;
    while (++index < elements.size()) {
      const Element &e = elements[index];

      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        return false;
      if (e.fields[0].find("BSR") != string::npos ||
          e.fields[0].find("JSR") != string::npos ||
          e.fields[0].find("PSHS") != string::npos ||
          e.fields[0].find("PULS") != string::npos ||
          e.fields[1].find(",-S") != string::npos ||
          e.fields[1] == ",S+" ||
          e.fields[1] == ",S" ||
          e.fields[1].find("[,S") != string::npos)
        return false;

      // We can't do this if we changed any register we read during the ldd
      InsEffects effects(e);
      if (effects.written & lddEffects.read) {
        return false;
      }

      // We can't do this if we read the D register before it is written
      if (effects.read & currentKnown) {
        return false;
      }
      currentKnown = currentKnown & ~effects.written;

      // S++ instruction?
      if (e.fields[1] == ",S++") {
        popIndex = index;
        break;
      }
    }
    if (popIndex == startIndex) {
      return false;
    }

    // We can do the optimization
    Element &pop = elements[popIndex];
    pop.fields[1] = ldd.fields[1];
    pop.fields[2] = "optim: optimizeStackOperations4";
    commentOut(startIndex, "optim: optimizeStackOperations4");
    commentOut(startIndex + 1, "optim: optimizeStackOperations4");

    return true;
}


// Sometimes a constant value is loaded in the D register via a combination
// or CLRA, LDA, CLRB and LDB and the pushed to the stack.
// If there are no stack operations, references to the D register or changes
// U between the load and the subsequent PSHS and subsequent ,S++ that consumes
// the value, then the PSHS and LDD operations
// can be removed and the ,S++ can be replaced by the operand in the original
// load.
//
// This optimization starts by looking for a LDD instruction followed by a PSHS
// followed by an ,S++ instruction.
  bool
ASMText::optimizeStackOperations5(size_t index)
{
    size_t startIndex = index;
    if (index + 3 >= elements.size()) {
      return false;
    }

    Pseudo6809 simulator;
    for (size_t ii=index; ii<index + 2; ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        return false;
      if (e.fields[0].find("BSR") != string::npos ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      if (!(e.fields[0] == "CLRA" || e.fields[0] == "LDA" ||
            e.fields[0] == "CLRB" || e.fields[0] == "LDB"))
        return false;
      if (!simulator.process(e.fields[0], e.fields[1], (int)ii))
        return false;
    }
    if (!simulator.regs.accum.dknown()) {
      return false;
    }

    // Next instruction must be a PSHS B,A
    index = index + 2;
    const Element &el = elements[index];
    if ((el.type != INSTR) || (el.fields[0] != "PSHS") || (el.fields[1] != "B,A")) {
      return false;
    }

    // Now we must find a ,S++ instruction
    size_t popIndex = startIndex;
    uint8_t currentKnown = A | B;
    while (++index < elements.size()) {
      const Element &e = elements[index];

      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        return false;
      if (e.fields[0].find("BSR") != string::npos ||
          e.fields[0].find("JSR") != string::npos ||
          e.fields[0].find("PSHS") != string::npos ||
          e.fields[0].find("PULS") != string::npos ||
          e.fields[1].find(",-S") != string::npos ||
          e.fields[1] == ",S+" ||
          e.fields[1] == ",S" ||
          e.fields[1].find("[,S") != string::npos)
        return false;

      // We can't do this if we read the D register before it is written
      InsEffects effects(e);
      if (effects.read & currentKnown) {
        return false;
      }
      currentKnown = currentKnown & ~effects.written;

      // S++ instruction?
      if (e.fields[1] == ",S++") {
        popIndex = index;
        break;
      }
    }
    if (popIndex == startIndex) {
      return false;
    }

    // We can do the optimization
    Element &pop = elements[popIndex];
    pop.fields[1] = "#" + wordToString(simulator.regs.accum.dval(), true);
    pop.fields[2] = "optim: optimizeStackOperations5";
    commentOut(startIndex, "optim: optimizeStackOperations5");
    commentOut(startIndex + 1, "optim: optimizeStackOperations5");
    commentOut(startIndex + 2, "optim: optimizeStackOperations5");

    return true;
}


// Merge PSHS X; [LDD ___]; PSHS B,A into [LDD ___]; PSHS X,B,A.
// This works because the latter will push X first, then push D.
//
bool
ASMText::mergePushXPushD(size_t index)
{
    if (!isInstr(index, "PSHS", "X"))
        return false;
    size_t secondPushIndex = findNextInstrBeforeLabel(index + 1);
    size_t lddIndex = size_t(-1);
    if (isInstrAnyArg(secondPushIndex, "LDD"))  // if optional LDD found before PSHS B,A
    {
        lddIndex = secondPushIndex;
        secondPushIndex = findNextInstrBeforeLabel(lddIndex + 1);  // search again for PSHS B,A
    }
    if (!isInstr(secondPushIndex, "PSHS", "B,A"))
        return false;
    
    // Apply the optimization.
    commentOut(index, "optim: mergePushXPushD");  // remove PSHS X
    elements[secondPushIndex].fields[1] = "X,B,A";
    elements[secondPushIndex].fields[2] = "optim: mergePushXPushD";
    return true;
}


// Remove CLR[A/B] operations if A or B are already known to be zero.
bool
ASMText::removeClr(size_t index)
{
    Pseudo6809 simulator;
    bool madeChanges = false, canGoOn = false, firstInstr = true;

    do {
      const Element &e = elements[index];
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      firstInstr = false;

      // Only process non basic block ending instructions
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        break;
      if (e.type == INSTR && isBasicBlockEndingInstruction(e))
        break;

      const string &instr = e.fields[0];
      if (instr == "CLRA" || instr == "CLRB") {
        if ((instr[3] == 'A' && simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0) || 
            (instr[3] == 'B' && simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0)) {
          commentOut(index, "optim: removeClr");
          madeChanges = true;
        }
      }

      // Simulate
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);

    } while(canGoOn && ++index<elements.size() && (simulator.regs.knownRegisters() != 0));

    return madeChanges;
}


// Remove AND, OR, MUL or ADD operations if we can show that 
// they will not change performance of the program.
bool
ASMText::removeAndOrMulAddSub(size_t index)
{
    Pseudo6809 simulator;
    bool madeChanges = false, canGoOn = false, firstInstr = true;

    do {
      Element &e = elements[index];
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      firstInstr = false;

      // Only process non basic block ending instructions
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        break;
      if (e.type == INSTR && isBasicBlockEndingInstruction(e))
        break;

      // Get the preconditions
      const string &instr = e.fields[0];
      const string &oper = e.fields[1];
      const bool changesIndex = isInstrWithPreDecrOrPostIncr(index);

      int val = 0;
      if (instr == "ANDA" || instr == "ANDB") {
        if (!changesIndex &&
            ((instr[3] == 'A' && simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0) || 
             (instr[3] == 'B' && simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0))) {
          if (!isInstrWithPreDecrOrPostIncr(index)) {
            commentOut(index, "optim: removeAndOrMulAddSub");
            madeChanges = true;
          }
        } else if (extractConstantLiteral(e.fields[1], val)) {
          if (val == 0) {
            const char newInstr[] = { 'C', 'L', 'R', instr[3], '\0' };
            replaceWithInstr(index, newInstr, "", "optim: removeAndOrMulAddSub");
            madeChanges = true;
          } else if (val == 0xff) {
            commentOut(index, "optim: removeAndOrMulAddSub");
            madeChanges = true;
          }
        }
      } else if (instr == "ORA" || instr == ("ORB")) {
        if (!changesIndex &&
            ((instr[2] == 'A' && simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0xff) || 
             (instr[2] == 'B' && simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0xff))) {
          if (!isInstrWithPreDecrOrPostIncr(index)) {
            commentOut(index, "optim: removeAndOrMulAddSub");
            madeChanges = true;
          }
        } else if (extractConstantLiteral(e.fields[1], val)) {
          if (val == 0xff) {
            char newInstr[8];
            snprintf(newInstr, sizeof(newInstr), "LD%s", instr.c_str() + 2);
            replaceWithInstr(index, newInstr, "#$ff", "optim: removeAndOrMulAddSub");
            madeChanges = true;
          } else if (val == 0) {
            commentOut(index, "optim: removeAndOrMulAddSub");
            madeChanges = true;
          }
        }
      } else if (instr == "MUL") {
        if ((simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0) || 
            (simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0)) {
          commentOut(index, "optim: removeAndOrMulAddSub");
          madeChanges = true;
        }
      } else if ((instr == "ADDA") || (instr == "ADDB") || (instr == "ADDD")) {
        if ((instr[3] == 'A' && simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0) &&
            (instr[3] == 'B' && simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0) &&
            (instr[3] == 'D' && simulator.regs.accum.dknown() && simulator.regs.accum.dval() == 0)) {
          // We know that A, B or D are zero, so change to a LD
          const Element &e1 = elements[index + 1];
          if (e1.type != INSTR || !isBasicBlockEndingInstruction(e1)) {
            char newInstr[8];
            snprintf(newInstr, sizeof(newInstr), "LD%s", instr.c_str() + 3);
            replaceWithInstr(index, newInstr, oper, "optim: removeAndOrMulAddSub");
            madeChanges = true;
          }
        } else if (extractConstantLiteral(e.fields[1], val)) {
          // We know that the operand is a constant 0 and the side effects are not valued,
          // so we can comment it out
          const Element &e1 = elements[index + 1];
          if (val == 0 && (e1.type == INSTR && !isBasicBlockEndingInstruction(e1))) {
            InsEffects effects(e1);
            if (!(effects.read & CC)) {
              commentOut(index, "optim: removeAndOrMulAddSub");
              madeChanges = true;
            }
          } else if (val == 0 && e1.type == INSTR && isBasicBlockEndingInstruction(e1) &&
                     (((instr[3] == 'D') && (simulator.regs.accum.a.known && simulator.regs.accum.a.val == 0)) ||
                      (instr[3] == 'B'))) {
            // ADDD #0 and A is definitely zero or ADDB #0 so replace with TSTB
            replaceWithInstr(index, "TSTB", "", "optim: removeAndOrMulAddSub");
            madeChanges = true;
          } else if (val == 0 && e1.type == INSTR && isBasicBlockEndingInstruction(e1) &&
                     (((instr[3] == 'D') && (simulator.regs.accum.b.known && simulator.regs.accum.b.val == 0)) ||
                      (instr[3] == 'A'))) {
            // ADDD #0 and B is definitely zero or ADDA #0 so replace with TSTA
            replaceWithInstr(index, "TSTA", "", "optim: removeAndOrMulAddSub");
            madeChanges = true;
          } 
        }
      } else if ((instr == "LEAX") && (oper == "D,X")) {
        if (simulator.regs.accum.dknown() && simulator.regs.accum.dval() == 0) {
            commentOut(index, "optim: removeAndOrMulAddSub");
            madeChanges = true;
        } else if (simulator.regs.accum.a.known && (simulator.regs.accum.a.val == 0)) {
          e.fields[0] = "ABX";
          e.fields[1] = "";
          e.fields[2] = "optim: removeAndOrMulAddSub";
          madeChanges = true;
        }
      } else if ((instr == "LEAX") && endsWith(oper, ",X") && simulator.regs.x.known)  {
          if (!(oper[0] == 'A' || oper[0] == 'B' || oper[0] == 'D')) {
            e.fields[0] = "LDX";
            char temp[32];
            snprintf(temp, sizeof(temp), "#%d", int16_t(strtol(oper.c_str(), NULL, 10)) + simulator.regs.x.val);
            e.fields[1] = temp;
            e.fields[2] = "optim: removeAndOrMulAddSub";
            madeChanges = true;
          }
      } else if ((instr == "LDD") && extractConstantLiteral(e.fields[1], val)) {
          if ((val < 256) && simulator.regs.accum.a.known && (simulator.regs.accum.a.val == 0)) {
            e.fields[0] = "LDB";
            e.fields[2] = "optim: removeAndOrMulAddSub";
            madeChanges = true;
          }
      } else if (instr == "SEX" && simulator.regs.accum.b.known && simulator.regs.accum.b.val < 0x80
                 && simulator.regs.accum.a.known && simulator.regs.accum.b.val == 0x00) {
          commentOut(index, "optim: removeAndOrMulAddSub");
          madeChanges = true;
      } else if ((instr == "STB" || instr == "STD") && oper == ",X" && simulator.regs.x.known) {
          e.fields[1] = wordToString(simulator.regs.x.val, true);
          e.fields[2] = "optim: removeAndOrMulAddSub";
          madeChanges = true;
      } else if ((instr == "LDB" || instr == "LDD") && oper == ",X" && simulator.regs.x.known) {
          // Found a weird mess bug toggling HW registers, so avoid that
          if (simulator.regs.x.val < 0xff00) {
            e.fields[1] = wordToString(simulator.regs.x.val, true);
            e.fields[2] = "optim: removeAndOrMulAddSub";
            madeChanges = true;
          }
      }

      // Simulate
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while (canGoOn && ++index < elements.size() && simulator.regs.knownRegisters() != 0);

    return madeChanges;
}


bool
ASMText::replaceCMPWithTST(size_t index)
{
    if (isInstr(index, "CMPB", "#$00") ||
        isInstr(index, "CMPA", "#$00") ||
        isInstr(index, "CMPB", "#0") ||
        isInstr(index, "CMPA", "#0"))
    {
        // If the next instruction is BEQ or BNE, then only Z is needed,
        // so use TSTA/TSTB instead, which is one fewer byte.
        size_t branchIndex = findNextInstrBeforeLabel(index + 1);
        if (branchIndex != size_t(-1))
        {
            const string &field0 = elements[branchIndex].fields[0];
            if (field0.find("BEQ") != string::npos || field0.find("BNE") != string::npos)
            {
                char tstInstr[] = { 'T', 'S', 'T', elements[index].fields[0][3], '\0' };
                replaceWithInstr(index, tstInstr);
                return true;
            }
        }
    }
    return false;
}


bool
ASMText::optimizeLoadDX(size_t index)
{
    if (index + 1 >= elements.size())  // pattern has at least 2 instructions
        return false;

    if (!isInstr(index, "LEAX", "D,X"))
        return false;
    size_t loadIndex = findNextInstrBeforeLabel(index + 1);
    if (loadIndex == size_t(-1))
        return false;
    if (       !isInstr(loadIndex, "LDA", ",X")
            && !isInstr(loadIndex, "LDB", ",X")
            && !isInstr(loadIndex, "LDD", ",X"))
        return false;

    // Make sure there are no references to ,X.
    for(size_t ii = loadIndex + 1; ii < elements.size(); ii++)
    {
        const Element &e = elements[ii];
        if (e.isCommentLike())
            continue;
        if (e.type != INSTR || isBasicBlockEndingInstruction(e))
            break;
        InsEffects effects(e);
        if (effects.read & X)
            return false;
        if (effects.written & X)
            break;
    }

    // Apply.
    commentOut(index, "optim: optimizeLoadDX");
    elements[loadIndex].fields[1] = "D,X";
    elements[loadIndex].fields[2] += " (optim: optimizeLoadDX)";
    return true;
}


// Look for:
//      LEAX    D,X
//      LDX     ,X
// Replace with:
//      LDX     D,X
//
bool
ASMText::optimizeLoadXX(size_t index)
{
    if (!isInstr(index, "LEAX", "D,X"))
        return false;
    size_t loadIndex = findNextInstrBeforeLabel(index + 1);
    if (loadIndex == size_t(-1) || !isInstr(loadIndex, "LDX", ",X"))
        return false;

    // Apply.
    commentOut(index, "optim: optimizeLoadXX");
    elements[loadIndex].fields[1] = "D,X";
    elements[loadIndex].fields[2] += " (optim: optimizeLoadXX)";
    return true;
}


// Pattern: LDreg or STreg; CMPreg #0 or TSTreg; BEQ or BNE.
// Remove CMP because LD sets Z.
//
bool
ASMText::loadCmpZeroBeqOrBne(size_t index)
{
    const char *ins0 = getInstr(index);
    if (strcmp(ins0, "LDB") && strcmp(ins0, "LDD") && strcmp(ins0, "STB") && strcmp(ins0, "STD"))
        return false;

    size_t cmpIndex = findNextInstrBeforeLabel(index + 1);
    if (cmpIndex == size_t(-1))
        return false;
    const char *ins1 = getInstr(cmpIndex);
    if (strcmp(ins1, "CMPB") && strcmp(ins1, "CMPD") && strcmp(ins1, "ADDD") && strcmp(ins1, "TSTB"))
        return false;
    if (ins0[2] != ins1[3])  // if not same register
        return false;
    const char *arg1 = getInstrArg(cmpIndex);
    if (strcmp(ins1, "TSTB") && strcmp(arg1, "#0") && strcmp(arg1, "#$00"))
        return false;

    size_t branchIndex = findNextInstrBeforeLabel(cmpIndex + 1);
    if (branchIndex == size_t(-1))
        return false;
    const string &branchInstr = elements[branchIndex].fields[0];
    if (branchInstr.find("BEQ") == string::npos && branchInstr.find("BNE") == string::npos)
        return false;

    commentOut(cmpIndex, "optim: loadCmpZeroBeqOrBne");
    return true;
}


// Optimize this pattern:
//    PSHS B,A; LDB ...; CLRA; LEAS 1,S; CMPB ,S+
// No need to push and discard the MSB.
//
bool
ASMText::pushWordForByteComparison(size_t index)
{
    if (index + 4 >= elements.size())  // pattern has 5 instructions
        return false;

    if (!isInstr(index, "PSHS", "B,A"))
        return false;
    if (!isInstrAnyArg(index + 1, "LDB"))
        return false;
    if (!isInstr(index + 2, "CLRA", ""))
        return false;
    if (!isInstr(index + 3, "LEAS", "1,S"))
        return false;
    if (!isInstr(index + 4, "CMPB", ",S+"))
        return false;

    replaceWithInstr(index, "PSHS", "B", "optim: pushWordForByteComparison");  // don't push useless MSB
    commentOut(index + 2, "optim: pushWordForByteComparison");  // remove CLRA
    commentOut(index + 3, "optim: pushWordForByteComparison");  // no need to pop useless byte anymore

    return true;
}


// If TFR foo,bar followed by TFR bar,foo, remove 2nd TFR.
//
bool
ASMText::stripConsecOppositeTFRs(size_t index)
{
    if (!isInstrAnyArg(index, "TFR"))
        return false;
    size_t nextInstrIndex = findNextInstrBeforeLabel(index + 1);
    if (nextInstrIndex == size_t(-1))
        return false;
    if (!isInstrAnyArg(nextInstrIndex, "TFR"))
        return false;

    const string &arg0 = elements[index].fields[1];
    const string &arg1 = elements[nextInstrIndex].fields[1];

    uint8_t arg0FirstReg, arg0SecondReg, arg1FirstReg, arg1SecondReg;
    getRegPairNames(arg0, arg0FirstReg, arg0SecondReg);
    getRegPairNames(arg1, arg1FirstReg, arg1SecondReg);

    if (arg0FirstReg == arg1SecondReg && arg0SecondReg == arg1FirstReg)
    {
        commentOut(nextInstrIndex, "optim: stripConsecOppositeTFRs");
        return true;
    }

    return false;
}


// Example: If TFR foo,bar and next instruction is PULS bar, remove the TFR.
//
bool
ASMText::stripOpToDeadReg(size_t index)
{
    if (elements[index].type != INSTR)
        return false;
    if (isInstrAnyArg(index, "PSHS") || isInstrAnyArg(index, "PULS"))  // "PULS X; PULS X" would be useful to unstack 4 bytes
        return false;
    size_t nextInstrIndex = findNextInstrBeforeLabel(index + 1);
    if (nextInstrIndex == size_t(-1))
        return false;

    InsEffects ins0Effects(elements[index]);
    InsEffects ins1Effects(elements[nextInstrIndex]);

    // Do nothing if the 2nd instruction reads the flags (e.g., TFR CC,B).
    if (ins1Effects.read & CC)
        return false;

    // Do nothing if the 2nd instruction reads register(s) affected by the 1st instruction.
    if ((ins0Effects.written & ins1Effects.read) != 0)
        return false;

    // Do nothing if the two instructions do not write to the same register(s).
    //
    if ((ins0Effects.written & ins1Effects.written) == 0)
        return false;

    // Do nothing if the 1st instruction writes to a register that the 2nd instruction does not write to.
    if ((ins0Effects.written & ~ ins1Effects.written) != 0)
        return false;

    commentOut(index, "optim: stripOpToDeadReg");
    return true;
}


// If PSHS B,A; <ins>; PULS A,B and <ins> does not read D or access S,
// then remove the PSHS and PULS.
//
bool
ASMText::stripUselessPushPull(size_t index)
{
    if (elements[index].type != INSTR)
        return false;

    if (!isInstr(index, "PSHS", "B,A"))
        return false;

    size_t nextInstrIndex = findNextInstrBeforeLabel(index + 1);
    if (nextInstrIndex == size_t(-1))
        return false;
    if (isInstrAnyArg(nextInstrIndex, "PSHS") || isInstrAnyArg(nextInstrIndex, "PULS"))
        return false;
    if (elements[nextInstrIndex].fields[1].find(",S") != string::npos)  // if may access stacked D
        return false;
    size_t followingInstrIndex = findNextInstrBeforeLabel(nextInstrIndex + 1);
    if (followingInstrIndex == size_t(-1))
        return false;

    if (!isInstr(followingInstrIndex, "PULS", "A,B"))
        return false;

    InsEffects middleInsEffects(elements[nextInstrIndex]);
    if (middleInsEffects.read & D)
        return false;

    commentOut(index, "optim: stripUselessPushPull");
    commentOut(followingInstrIndex, "optim: stripUselessPushPull");
    return true;
}


bool
ASMText::stripUselessBitwiseOp(size_t index)
{
    const Element &e = elements[index];
    if (e.type != INSTR)
        return false;
    bool out = false;
    if (e.fields[0] == "ORA" || e.fields[0] == "EORA" || e.fields[0] == "ORB" || e.fields[0] == "EORB")
        out = (e.fields[1] == "#0");
    else if (e.fields[0] == "ANDA" || e.fields[0] == "ANDB")
        out = (e.fields[1] == "#$FF" || e.fields[1] == "#255");
    if (out)
      commentOut(index, "optim: stripUselessBitwiseOp");
    return out;
}


// Change TFR X,D PSHS B,A to PSHS X
//
bool
ASMText::optimizeTfrPush(size_t index)
{
    if (index + 1 >= elements.size())  // pattern has 2 instructions
        return false;
    if (! (isInstr(index, "TFR", "X,D") && isInstr(index + 1, "PSHS", "B,A")))
        return false;

    // Make sure there are no references to D
    for(size_t ii=index+3; ii<elements.size(); ii++) {
        const Element &e = elements[ii];
        if (e.isCommentLike())
            continue;
        if (e.type != INSTR || isBasicBlockEndingInstruction(e))
            break;
        InsEffects effects(e);
        if (effects.read & (A | B)) return false;
        if ((effects.written & (A | B)) == (A | B)) break;
    }

    replaceWithInstr(index, "PSHS", "X", "optim: optimizeTfrPush");
    commentOut(index + 1, "optim: optimizeTfrPush");
   
    return true;
}


// Change TFR X,D OPD to OPX
// Retired optimization as of CMOC 0.1.77: It assumes that D is dead
// after the sequence, which is not always the case.
// For example:
//   int a, b;
//   void f(void) { g(a = b); }
// This optimization would change { LEAX _b,PCR; TFR D,X; STD _a,PCR; PSHS B,A }
// to { LEAX _b,PCR; STX _a,PCR; PSHS B,A } and the pushed D would be undefined.
//
bool
ASMText::optimizeTfrOp(size_t index)
{
    if (index + 2 >= elements.size())  // pattern has 2 instructions
        return false;
    if (!(isInstr(index, "TFR", "X,D")))
        return false;

    if (elements[index + 1].type != INSTR)
        return false;

    InsEffects effects(elements[index + 1]);
    if (!(effects.read & (A | B)))
      return false;

    const string &instr = elements[index + 1].fields[0];
    if (! (instr == "CMPD" || instr == "STD"))
      return false;

    commentOut(index, "optim: optimizeTfrOp");
    elements[index + 1].fields[0] = (instr == "CMPD") ? "CMPX" : "STX";
    elements[index + 1].fields[2] = "optim: optimizeTfrOp";

    return true;
}


// Remove Push B ... OPB ,S+ when possible
//
bool
ASMText::removePushB(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "PSHS" && e1.fields[1] == "B"))
        return false;
    const size_t startIndex = index;

    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if (e.isCommentLike() && e.type == LABEL)
        continue;
      if (e.type != INSTR || isBasicBlockEndingInstruction(e))
        return false;

      const InsEffects insEffects(e);
      if (insEffects.written & B) {
        if (e.fields[1] != ",S+") return false;
        commentOut(startIndex, "optim: removePushB");
        commentOut(index, "optim: removePushB");
        return true;
      }

      if (e.fields[0] == "PSHS" || e.fields[1].find(",S") != string::npos)
        return false; 
    }

    return false;
}


// Remove Push B ... OPB ,S+ when possible
//
bool
ASMText::optimizeLdbTfrClrb(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "LDB"))
        return false;

    vector<size_t> instrs;
    for(index++; index<elements.size() && instrs.size()<2; index++) {
      Element &e = elements[index];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR || isBasicBlockEndingInstruction(e))
        return false;
      instrs.push_back(index);
    }
    if (! (isInstr(instrs[0], "TFR", "B,A") &&
           isInstr(instrs[1], "CLRB", "")) )
      return false;
    
    e1.fields[0] = "LDA";
    e1.fields[2] = "optim: optimizeLdbTfrClrb";
    commentOut(instrs[0], "optim: optimizeLdbTfrClrb");

    return true;
}


// Check for LDB followed by TSTB.
// Remove TSTB because LDB affects CC the same way.
//
bool
ASMText::optimizeLoadTest(size_t index)
{
    if (!isInstrAnyArg(index, "LDB"))
        return false;
    size_t testIndex = findNextInstrBeforeLabel(index + 1);
    if (!isInstr(testIndex, "TSTB", ""))
        return false;
    
    // Apply the optimization.
    commentOut(testIndex, "optim: optimizeLoadTest");
    return true;
}


// Change LDD ??? .... PSHS B,A .... LDD ,S++
// to LDD ???  .... OP ???,U
//
bool
ASMText::remove16BitStackOperation(size_t index)
{
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "LDD"))
        return false;

    // Step through the next ops until we find an op that
    // modifies D or does a PSHS B,A
    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR || isBasicBlockEndingInstruction(e))
        return false;

      const InsEffects insEffects(e);
      if (insEffects.written & (A | B))
        return false;
      if (isInstr(index, "PSHS", "B,A"))
        break;
    }
    if (index >= elements.size())
      return false;
    const size_t pushIndex = index;

    // Step through the next ops until we find an op that
    // modifies D or does a LDD ,S++
    for(index++; index<elements.size(); index++) {
      Element &e = elements[index];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR || isBasicBlockEndingInstruction(e))
        return false;

      if (isInstr(index, "LDD", ",S++"))
        break;

      const InsEffects insEffects(e);
      if (insEffects.written & (A | B))
        return false;
    }
    if (index >= elements.size())
      return false;
    const size_t popIndex = index;

    // We can remove the PSHS and LDD
    commentOut(pushIndex, "optim: remove16BitStackOperation");
    commentOut(popIndex, "optim: remove16BitStackOperation");

    return true;
}


// Try to optimize post increment operations
//
bool
ASMText::optimizePostIncrement(size_t index)
{
    if (index + 5 >= elements.size())
      return false;

    // first instr must reference a stack variable
    Element &e1 = elements[index];
    if (! (e1.type == INSTR && e1.fields[0] == "LDX" &&
           endsWith(e1.fields[1], ",U")) )
        return false;

    // next instr must increment X
    Element &e2 = elements[index + 1];
    if (! ((e2.type == INSTR && e2.fields[0] == "LEAX") &&
           (e2.fields[1] == "1,X" || e2.fields[1] == "2,X")) )
        return false;

    // next instr must save X
    Element &e3 = elements[index + 2];
    if (! ((e3.type == INSTR && e3.fields[0] == "STX") &&
           (e3.fields[1] == e1.fields[1])) )
        return false;

    // next instr must decrement X by the same amount
    Element &e4 = elements[index + 3];
    if (! ((e4.type == INSTR && e4.fields[0] == "LEAX") &&
           (e4.fields[1] == (string("-") + e2.fields[1]))) )
        return false;

    // Look for instructions until we find a ,X
    const size_t startIndex = index;
    for(index += 4; index<elements.size(); index++) {
      const Element &e = elements[index];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR || isBasicBlockEndingInstruction(e))
        return false;

      // This is the reference we are looking for
      if (e.fields[1] == ",X")
        break;

      // Watch for aliases
      if ((startsWith(e.fields[0], "LD") && !startsWith(e.fields[1], "#")) ||
          (startsWith(e.fields[0], "ST")))
        return false;

      // Make sure X does not get trashed
      const InsEffects insEffects(e);
      if (insEffects.written & X)
        return false;
    }
    if (index >= elements.size())
      return false;
    Element &e5 = elements[index];
    if (e5.type != INSTR)
        return false;

    // The indexed instruction will reference another register. We must find
    // any instruction that modifies this register between startIndex and
    // index
    const InsEffects indexInstrEffects(e5);
    uint8_t readRegs = indexInstrEffects.read & ~X;
    vector<size_t> loadIndices;
    for(size_t ii=startIndex + 4; ii<index; ii++) {
      const Element &ee = elements[ii];
      if (ee.type != INSTR)
        continue;
      const InsEffects eeEffects(ee);
      if ((eeEffects.written & readRegs) != 0) {
        loadIndices.push_back(ii);
      }
    }

    // Don't try to deal with situations with more than one load.
    if (loadIndices.size() > 1)
      return false;

    // Replace decrement with OP, X++ instr
    e4.fields[0] = e5.fields[0];
    e4.fields[1] = string(",X") + ((e2.fields[1] == "1,X") ? "+" : "++");
    e4.fields[2] = "optim: optimizePostIncrement";

    // Replace old OP, X instr with STX
    e5.fields[0] = "STX";
    e5.fields[1] = e1.fields[1];
    e5.fields[2] = "optim: optimizePostIncrement";

    // Either comment out or put the load instr at e2
    if (loadIndices.size() == 0)
      commentOut(startIndex + 1, "optim: optimizePostIncrement");
    else {
      const Element &ee = elements[loadIndices[0]];
      e2.fields[0] = ee.fields[0];
      e2.fields[1] = ee.fields[1];
      e2.fields[2] = "optim: optimizePostIncrement";
      commentOut(loadIndices[0], "optim: optimizePostIncrement");
    }

    // Comment out e3
    commentOut(startIndex + 2, "optim: optimizePostIncrement");

    return true;
}


// Remove operations that generate a value that does not get
// used
//
bool
ASMText::removeUselessOps(size_t index) {
    Pseudo6809 simulator;
    const size_t startIndex = index;
    size_t numInstrs = 0;
    bool firstInstr = true, canGoOn = true;
    do {
      const Element &e = elements[index];
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      if (firstInstr &&
          (e.fields[1].find("+") != string::npos ||
           !((e.fields[0] == "ADDD") ||
             (e.fields[0] == "ADDA") ||
             (e.fields[0] == "ADDB") ||
             (e.fields[0] == "SUBD") ||
             (e.fields[0] == "SUBA") ||
             (e.fields[0] == "SUBB") ||
             (e.fields[0] == "LDA") ||
             (e.fields[0] == "LDB") ||
             (e.fields[0] == "LDD"))))
        return false;
      firstInstr = false;

      // Don't optimize an instruction away if we reach the end of a function
      if (e.fields[1].find(",PC") != string::npos ||
          e.fields[0] == "LEAS" ||
          e.fields[0] == "RTS" ||
          e.fields[0] == "RTI")
        return false;
    
      // ... or a branch, because the target code might use the loaded register.
      if (isBranchOrCallAtIndex(index))
        return false;

      // Only process non basic block ending instructions
      if (e.type == LABEL || e.isCommentLike())
        continue;
      if (e.type == INSTR && isBasicBlockEndingInstruction(e))
        break;
      if (e.type != INSTR)
        break;

      // Don't optimize the instruction when a CC is read
      InsEffects effects(e);
      if (effects.read & CC)
        break;

      // Run the instruction
      numInstrs++;
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while(canGoOn && ++index<elements.size() &&
        (simulator.indexToReferences[startIndex].size() < 1));

    // There can be no instructions referencing this instruction
    if (!canGoOn || simulator.indexToReferences[startIndex].size() > 0) {
      return false;
    }

    // If index == startIndex + 1, chances are the change was just before the
    // end of a block, so it is probably needed
    if (numInstrs <= 2) {
      return false;
    }

    commentOut(startIndex, "optim: removeUselessOps: " + elements[startIndex].toString());
    return true;
}


// Optimize 16-bit stack operations of the form:
//   LD[X/D] ?,U
//     ...
//   PSHS [X/B,A]
//     ...
//   OP ,S++
//
bool
ASMText::optimize16BitStackOps1(size_t index) {
    Pseudo6809 simulator;
    const size_t startIndex = index;
    size_t numInstrs = 0;
    bool firstInstr = true, canGoOn = true;
    do {
      const Element &e = elements[index];

      // First instruction has to be "LD? *,U""
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      if (firstInstr &&
          (e.fields[0].find("LD") != 0 || e.fields[1].find(",U") == string::npos
           || e.fields[1].find("[") == 0))
        return false;
      firstInstr = false;

      // Only process non basic block ending instructions
      if (e.type == LABEL)
        return false;
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        break;
      if (isBasicBlockEndingInstruction(e))
        break;

      // Don't try to optimize when there are stores between references
      if (e.fields[0].find("ST") == 0 &&
          simulator.indexToReferences[startIndex].size() < 2) {
        return false;
      }

      // Don't try to optimize when there are bsrs
      if (e.fields[0].find("BSR") <= 2) {
        return false;
      }

      // Run the instruction
      numInstrs++;
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while(canGoOn && ++index<elements.size() &&
            (simulator.indexToReferences[startIndex].size() < 3));

    // Simulator hit a problem
    if (!canGoOn) {
      return false;
    }

    // We can only handle two references
    if (simulator.indexToReferences[startIndex].size() != 2) {
      return false;
    }

    // The first reference must be a PSHS [X|B,A]
    const char *targetReg = elements[startIndex].fields[0].c_str() + 2;
    const Element &pshs = elements[simulator.indexToReferences[startIndex][0]];
    if (pshs.fields[0] != "PSHS" ||
        !((pshs.fields[1] == "X" && !strcmp(targetReg, "X")) ||
          (pshs.fields[1] == "B,A" && !strcmp(targetReg, "D")))) {
      return false;
    }

    // The second reference must be OP ,S++
    Element &op = elements[simulator.indexToReferences[startIndex][1]];
    if (op.fields[1] != ",S++") {
      return false;
    }

    // Make sure that no instructions between startIndex and PSHS write
    // A or B
    if (!strcmp(targetReg, "D")) {
      for (int ii = startIndex + 1;
           ii<simulator.indexToReferences[startIndex][0];
           ii++) {
        if (elements[ii].type != INSTR)
          continue;
        InsEffects effects(elements[ii]);
        if (effects.written & (A | B))
          return false;
      }
    }

    // OP can directly refernce the LD value
    op.fields[1] = elements[startIndex].fields[1];
    op.fields[2] = "optim: optimize16BitStackOps1";

    // We can comment out the load value and pshs
    commentOut(startIndex, "optim: optimize16BitStackOps1");
    commentOut(simulator.indexToReferences[startIndex][0],
              "optim: optimize16BitStackOps1");

    return true;
}


// Optimize 16-bit stack operations of the form:
//   LDD ?
//   PSHS B,A
//   LDD ??
//   [ADDD/SUBD/CMPD] ,S++
//
//   to
//   LDD ??
//   [ADDD/SUBD/CMPD] ,S++
//
bool
ASMText::optimize16BitStackOps2(size_t index) {
    if (index + 4 >= elements.size()) {
      return false;
    }

    Element &ldd1 = elements[index];
    if (ldd1.type != INSTR || ldd1.fields[0] != "LDD" ||
        ldd1.fields[1].find(",S") != string::npos ||
        ldd1.fields[1].find(",PC") != string::npos ||
        ldd1.fields[1].find(",X") != string::npos) {
      return false;
    }

    Element &pshs = elements[index + 1];
    if (pshs.type != INSTR || pshs.fields[0] != "PSHS" ||
        pshs.fields[1] != "B,A") {
      return false;
    }

    size_t ii = index + 2;
    for(; ii<elements.size(); ii++) {
        Element &e = elements[ii];
        if (e.isCommentLike() || e.type == LABEL) {
          continue;
        }
        if (e.type != INSTR) {
          return false;
        }

        if (e.fields[0] == "LDD") {
          break;
        }

        // Pretty much anything that does not read or write 
        // D or S or does a store is OK 
        InsEffects effects(e);
        if (((effects.written & (A | B)) != 0) ||
            ((effects.read & (A | B)) != 0) ||
            startsWith(e.fields[0], "ST") ||
            e.fields[1].find(",S") != string::npos) {
          return false;
        }
    }

    Element &ldd2 = elements[ii];
    if (ldd2.type != INSTR || ldd2.fields[0] != "LDD" ||
        ldd2.fields[1].find("D,") != string::npos ||
        ldd2.fields[1].find(",S") != string::npos) {
      return false;
    }

    Element &op = elements[ii + 1];
    if ((op.type != INSTR || op.fields[1] != ",S++") ||
        !(op.fields[0] == "ADDD" || op.fields[0] == "SUBD" ||
          op.fields[0] == "CMPD")) {
      return false;
    }

    op.fields[1] = ldd1.fields[1];
    op.fields[2] = "optim: optimize16BitStackOps2";
    commentOut(index, "optim: optimize16BitStackOps2");
    commentOut(index + 1, "optim: optimize16BitStackOps2");

    return true;
}


// Optimize 8-bit stack operations of the form:
//   LD[A/B] ?,U
//     ...
//   PSHS [A/B]
//     ...
//   OP ,S+
//
bool
ASMText::optimize8BitStackOps(size_t index) {
    Pseudo6809 simulator;
    const size_t startIndex = index;
    size_t numInstrs = 0;
    bool firstInstr = true, canGoOn = true;
    do {
      const Element &e = elements[index];

      // First instruction has to be "LD? *,U""
      if (firstInstr && (e.type != INSTR || isBasicBlockEndingInstruction(e)))
        return false;
      if (firstInstr &&
          (e.fields[0].find("LD") != 0 || e.fields[1].find(",U") == string::npos
           || e.fields[1].find("[") == 0))
        return false;
      firstInstr = false;

      // Only process non basic block ending instructions
      if (e.type == LABEL)
        return false;
      if (e.isCommentLike())
        continue;
      if (e.type == INSTR && isBasicBlockEndingInstruction(e))
        break;
      if (e.type != INSTR)
        break;

      // Don't try to optimize when there are stores between references
      if (e.fields[0].find("ST") == 0 &&
          simulator.indexToReferences[startIndex].size() < 2) {
        return false;
      }

      // Don't try to optimize when there are bsrs
      if (e.fields[0].find("BSR") <= 2) {
        return false;
      }

      // Run the instruction
      numInstrs++;
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while(canGoOn && ++index<elements.size() &&
            (simulator.indexToReferences[startIndex].size() < 3));

    // Simulator hit a problem
    if (!canGoOn) {
      return false;
    }

    // We can only handle two references
    if (simulator.indexToReferences[startIndex].size() != 2) {
      return false;
    }

    // The first reference must be a PSHS [A/B]
    Element &pshs = elements[simulator.indexToReferences[startIndex][0]];
    if (pshs.fields[0] != "PSHS" ||
        !(pshs.fields[1] == "A" || pshs.fields[1] == "B" || pshs.fields[1] == "B,A")) {
      return false;
    }
    const bool doubleBytePush = pshs.fields[1] == "B,A";

    // The second reference must be OP ,S+
    Element &op = elements[simulator.indexToReferences[startIndex][1]];
    if (op.fields[1] != ",S+") {
      return false;
    }

    // If we pushed B,A there must be an LEAS 1,S just before op
    Element &leas = elements[simulator.indexToReferences[startIndex][1] - 1];
    if (doubleBytePush) {
      if (leas.fields[0] != "LEAS" || leas.fields[1] != "1,S") {
        return false;
      }
    }

    // OP can directly reference the LD value. The tricky thing is that if
    // the original load is the D register but the PSHS is on the B register
    // we have to bump the index by one.
    const string &field1 = elements[startIndex].fields[1];
    const char *targetReg = elements[startIndex].fields[0].c_str() + 2;
    if (!strcmp(targetReg, "D") && (doubleBytePush || pshs.fields[1] == "B")) {
      size_t commaIndex = field1.find(",");
      if (commaIndex == string::npos) {
        return false;
      }
      long offset = strtol(field1.c_str(), NULL, 10);  // extract integer up to commaIndex
      if (offset >= 0) {
        return false;
      }
      char temp[512];
      snprintf(temp, sizeof(temp), "%d%s", int16_t(offset + 1), field1.c_str() + commaIndex);
      op.fields[1] = temp;
    } else {
      op.fields[1] = field1;
    }
    op.fields[2] = "optim: optimize8BitStackOps";

    // We can comment out the load value and possibly the pshs
    commentOut(startIndex, "optim: optimize8BitStackOps");
    if (doubleBytePush) {
      commentOut(simulator.indexToReferences[startIndex][0],
                 "optim: optimize8BitStackOps");
      commentOut(simulator.indexToReferences[startIndex][1] - 1,
                 "optim: optimize8BitStackOps");
    } else if (pshs.fields[1] == "B,A") {
      pshs.fields[1] = !strcmp(targetReg, "A") ? "B" : "A";
      pshs.fields[2] = "optim: optimize8BitStackOps";
    } else {
      commentOut(simulator.indexToReferences[startIndex][0],
                 "optim: optimize8BitStackOps");
    }

    return true;
}


//  When possible, replace
//   LDD ?
//   TFR D,X
//
//  With
//   LDX ?
//
bool
ASMText::removeTfrDX(size_t index) {
    if (index + 2 >= elements.size())
      return false;

    const size_t startIndex = index;
    Element &e1 = elements[index++];
    if (e1.type != INSTR || e1.fields[0] != "LDD")
      return false;
    Element &e2 = elements[index++];
    if (e2.type != INSTR || e2.fields[0] != "TFR" || e2.fields[1] != "D,X")
      return false;

    short written = 0;
    do {
      const Element &e = elements[index];

      // Only process non basic block ending instructions
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;

      // Don't try to optimize when there are bsrs
      if (e.fields[0].find("BSR") <= 2) {
        return false;
      }

      // If we read A or B before writing, then we can't do this
      InsEffects effects(e1);
      if ((effects.read & (A | B)) & ~written) {
        return false;
      }
      written |= effects.written;

      // Run the instruction
    } while(++index<elements.size() && ((written & (A | B)) != (A | B)));

    // We can't do this if we have not written A and B
    if ((written & (A | B)) != (A | B)) {
      return false;
    }

    // Transform the LDD to LDX and remove TFR
    e1.fields[0] = "LDX";
    e1.fields[2] = "optim: removeTfrDX";
    commentOut(startIndex + 1, "optim: removeTfrDX");

    return true;
}


//  When possible, remove repeated
//   LEAX ?,U
//
bool
ASMText::removeUselessLeax(size_t index) {
    Element &e = elements[index];
    if (!(e.fields[0] == "LEAX" && e.fields[1].find(",U") != string::npos))
      return false;

    size_t numChanges = 0;
    for(index++; index<elements.size(); index++) {
        Element &e1 = elements[index];
        if (e1.isCommentLike())
            continue;
        if (e1.type != INSTR || isBasicBlockEndingInstruction(e1))
            break;

        InsEffects effects(e1);
        if (effects.read & X)  // if any following instruction reads X, the LEAX is useful
            break;
        if (e1.fields[0] == e.fields[0] && e1.fields[1] == e.fields[1]) {
            commentOut(index, "optim: removeUselessLeax");
            numChanges++;
            continue;
        }

        if (effects.written & X)
            break;
    }

    return numChanges > 0;
}


//  When possible, remove repeated
//   LDX ?,U
//
bool
ASMText::removeUselessLdx(size_t index) {
    Element &e = elements[index];
    if (!((e.fields[0] == "LDX" || e.fields[0] == "STX") &&
          e.fields[1].find(",U") != string::npos))
      return false;

    size_t numChanges = 0;
    for(index++; index<elements.size(); index++) {
        Element &e1 = elements[index];
        if (e1.isCommentLike())
            continue;
        if (e1.type != INSTR || isBasicBlockEndingInstruction(e1))
            break;

        // The value could change as a result of the STORE - reject if
        // we are storing to the same location, not relative to U or indirect
        if (e1.fields[0].find("ST") == 0) {
          if (e.fields[1].find(",U") == string::npos || 
              startsWith(e.fields[1], "[") || e1.fields[1] == e.fields[0]) {
            break;
          }

          // Reject if both the LD and ST offsets are within 1 byte of each other
          int offset, offset1;
          if (!parseRelativeOffset(e.fields[1], offset) ||
              !parseRelativeOffset(e1.fields[1], offset1)) {
            break;
          }
          if (abs(offset - offset1) < 2) {
            break;
          }
        }

        InsEffects effects(e1);
        if (effects.read & X)  // if any following instruction reads X, the LDX is useful
            break;
        if (e1.fields[0] == "LDX" && e1.fields[1] == e.fields[1]) {
            commentOut(index, "optim: removeUselessLdx");
            numChanges++;
            continue;
        }

        if (effects.written & X)
            break;
    }

    return numChanges > 0;
}


//  When possible, remove unused LEAX
//
bool
ASMText::removeUnusedLoad(size_t index) {
    if (index + 2 >= elements.size())
      return false;

    Pseudo6809 simulator;
    const size_t startIndex = index;

    Element &e1 = elements[index];
    if (!(e1.type == INSTR &&
         ((e1.fields[0] == "LEAX") ||
          (e1.fields[0].find("LD") == 0 &&
           e1.fields[1].find("#") == 0))))
      return false;

    bool canGoOn = true;
    size_t numInstrs = 0;
    do {
      const Element &e = elements[index];

      // Only process non basic block ending instructions
      if (e.type == LABEL || e.isCommentLike())
        continue;

      if (e.type == INSTR && isBasicBlockEndingInstruction(e)) {
        // Loads followed almost immediately by a branch are loading a value
        // that is likely required by the subsequent block. Disable the
        // optimization for this case
        if (index <= startIndex + 2)
          return false;
        break;
      }

      if (e.type != INSTR)
        break;

      // Don't try to optimize when there are bsrs
      if (e.fields[0].find("BSR") <= 2) {
        return false;
      }

      // Run the instruction
      numInstrs++;
      canGoOn &= simulator.process(e.fields[0], e.fields[1], (int)index, true);
    } while(++index<elements.size() &&
            (simulator.indexToReferences[startIndex].size() < 1));

    // Simulator hit a problem
    if (!canGoOn) {
      return false;
    }

    // Do not allow any references to the load
    if (simulator.indexToReferences[startIndex].size() > 0) {
      return false;
    }

    // If the instruction is a LOAD and there have been only 2
    // INSTRs, don't try this.
    if (e1.fields[0].find("LD") == 0 && numInstrs <= 2) {
      // Give a little extra leeway for LDX
      if (!(e1.fields[0] == "LDX" && numInstrs >= 2))
        return false;
    }

    // Transform the LDD to LDX and remove TFR
    commentOut(startIndex, "optim: removeUnusedLoad");
    return true;
}


// Remove TSTB in the following scenario
//  ANDB ???
//  TSTB
//
bool
ASMText::optimizeAndbTstb(size_t index) {
    const Element &e1 = elements[index];
    if (e1.fields[0] != "ANDB")
      return false;
    const Element &e2 = elements[index + 1];
    if (e2.fields[0] != "TSTB")
      return false;
    commentOut(index + 1, "optim: optimizeAndbTstb");
    return true;
}


// Optimize the following when possible:
//  LDX ?,U
//  LD? ,X
// To
//  LD? [?,U]
// When there are no other references to X
bool
ASMText::optimizeIndexedX(size_t index) {
    if (index + 2 >= elements.size())
      return false;

    Pseudo6809 simulator;
    const size_t startIndex = index;

    // index must point to LDX ?,U, but not LDX [?,U].
    Element &e1 = elements[index];
    if (!(e1.type == INSTR && e1.fields[0] == "LDX" &&
         e1.fields[1].find(",U") != string::npos &&
         e1.fields[1].find("[") == string::npos))
      return false;

    // index + 1 must point to LD? ,X.
    Element &e2 = elements[index + 1];
    if (!(e2.type == INSTR && e2.fields[0].find("LD") == 0 &&
         e2.fields[1] == ",X"))
      return false;

    size_t numInstrs = 0;
    bool canGoOn = true;
    do {
      const Element &e = elements[index];

      // Only process non basic block ending instructions
      if (e.type == LABEL || e.isCommentLike())
        continue;
      if (e.type != INSTR)
        break;

      // Don't try to optimize when there are bsrs
      if (e.fields[0].find("BSR") <= 2) {
        return false;
      }

      if (isBasicBlockEndingInstruction(e))
        break;

      if (index >= startIndex + 2) {
        InsEffects effects(e);
        if (effects.read & X) {
          return false;
        }

        if (effects.written & X) {
          break;
        }
      }

      // Run the instruction
      numInstrs++;
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);
    } while(canGoOn && ++index<elements.size() &&
            (simulator.indexToReferences[startIndex].size() < 2));

    // We can only handle one reference to X
    if (!canGoOn || simulator.indexToReferences[startIndex].size() != 1) {
      return false;
    }

    e2.fields[1] = string("[") + e1.fields[1] + string("]");
    e2.fields[2] = "optim: optimizeIndexedX";
    commentOut(startIndex, "optim: optimizeIndexedX");

    return true;
}


// Optimize the following when possible:
//  LEAX ?,U
//  LD? ,X
// To
//  LD? ?,U
// When there are no other references to X
bool
ASMText::optimizeIndexedX2(size_t index) {
    if (index + 2 >= elements.size())
      return false;

    Pseudo6809 simulator;
    const size_t startIndex = index;

    Element &e1 = elements[index];
    if (!(e1.type == INSTR && e1.fields[0] == "LEAX" &&
         e1.fields[1].find(",U") != string::npos))
      return false;
    Element &e2 = elements[index + 1];
    if (!(e2.type == INSTR && e2.fields[0].find("LD") == 0 &&
         e2.fields[1] == ",X"))
      return false;

    size_t numInstrs = 0;
    bool canGoOn = true;
    do {
      const Element &e = elements[index];

      // Only process non basic block ending instructions
      if (e.type == LABEL || e.isCommentLike())
        continue;
      if (e.type == INSTR && isBasicBlockEndingInstruction(e))
        break;
      if (e.type != INSTR)
        return false;

      if (index >= startIndex + 2) {
        InsEffects effects(e);
        if (effects.written & X) {
          break;
        }
        if (effects.read & X) {
          return false;
        }
      }

      // Don't try to optimize when there are calls.
      if (e.fields[0].find("BSR") <= 2 || e.fields[0] == "JSR")
        return false;

      // Run the instruction
      numInstrs++;
      canGoOn = simulator.process(e.fields[0], e.fields[1], (int)index);

    } while(canGoOn && ++index<elements.size() &&
            (simulator.indexToReferences[startIndex].size() < 2));

    // We can only handle one reference to X
    if (!canGoOn || simulator.indexToReferences[startIndex].size() != 1) {
      return false;
    }

    e2.fields[1] = e1.fields[1];
    e2.fields[2] += "; optim: optimizeIndexedX2";
    commentOut(startIndex, "optim: optimizeIndexedX2: "
                            + elements[startIndex].fields[0]
                            + " " + elements[startIndex].fields[1]);

    return true;
}


//  Transform 
//   LDD
//   PSHS B,A
//   LDD
//   PSHS B,A
//
//   to
//
//   LDX
//   PSHS X
//   LDD
//   PSHS B,A
//
bool
ASMText::transformPshsDPshsD(size_t index) {
    // Needs 4 elements
    if (index + 4 >= elements.size()) {
      return false;
    }

    // Make sure the first 4 instructions fit the pattern
    Element &e1 = elements[index];
    Element &e2 = elements[index + 1];
    const Element &e3 = elements[index + 2];
    const Element &e4 = elements[index + 3];
    if (e1.type != INSTR || e1.fields[0] != "LDD") {
      return false;
    }
    if (e2.type != INSTR || e2.fields[0] != "PSHS" || e2.fields[1] != "B,A") {
      return false;
    }
    if (e3.type != INSTR || e3.fields[0] != "LDD" ||
        e3.fields[1].find("D,") != string::npos ||
        e3.fields[1].find("B,") != string::npos) {
      return false;
    }
    if (e4.type != INSTR || e4.fields[0] != "PSHS" || e4.fields[1] != "B,A") {
      return false;
    }

    // Make sure no subsequent instructions read X before we hit
    // the end of the basic block or X is written
    for(index = index + 4; index < elements.size(); index++) {
      Element &e = elements[index];
      if (e.type == COMMENT) {
        continue;
      }
      if (e.type != INSTR) {
        return false;
      }
      InsEffects effects(e);
      if (effects.read & X) {
        return false;
      }
      if (effects.written & X) {
        break;
      }
    }

    // Transform the first instruction to use X. This will will help us to
    // remove a PSHS later on
    e1.fields[0][e1.fields[0].size() - 1] = 'X';
    e1.fields[2] = "optim: transformPshsDPshsD";
    e2.fields[1] = 'X';
    e2.fields[2] = "optim: transformPshsDPshsD";

    return true;
}


//  Transform 
//   LDX/LEAX
//   PSHS X
//   LDX/LEAX
//   PSHS X
//   (where the 2nd load does not itself read from X)
//
//   to
//
//   LDX/LEAY
//   PSHS Y
//   LD/LEAX
//   PSHS X
//
bool
ASMText::transformPshsXPshsX(size_t index) {
    // This optimization is not done for OS-9 because the Y register points to the global data.
    if (! isYRegisterAllowed()) {
      return false;  // avoid using Y
    }

    // Needs 6 elements
    if (index + 6 >= elements.size()) {
      return false;
    }

    // Make sure the first 4 instructions fit the pattern
    Element &e1 = elements[index];
    Element &e2 = elements[index + 1];
    const Element &e3 = elements[index + 2];
    const Element &e4 = elements[index + 3];
    if (e1.type != INSTR || !(e1.fields[0] == "LDX" || e1.fields[0] == "LEAX")) {
      return false;
    }
    if (e2.type != INSTR || e2.fields[0] != "PSHS" || e2.fields[1] != "X") {
      return false;
    }
    if (e3.type != INSTR || !(e3.fields[0] == "LDX" || e3.fields[0] == "LEAX")
        || e3.fields[1].find(",X") != string::npos) {
      return false;
    }
    if (e4.type != INSTR || e4.fields[0] != "PSHS" || e4.fields[1] != "X") {
      return false;
    }

    // Don't do this when we have 3 consecutive PSHS X instructions
    const Element &e6 = elements[index + 5];
    if (e6.type == INSTR && e6.fields[0] == "PSHS" && e6.fields[1] == "X") {
      return false;
    }

    // Transform the first instruction to use Y. These take more space usually
    // but will allow us to remove a PSHS later on
    e1.fields[0][e1.fields[0].size() - 1] = 'Y';
    e1.fields[2] = "optim: transformPshsXPshsX";
    e2.fields[1][e2.fields[1].size() - 1] = 'Y';
    e2.fields[2] = "optim: transformPshsXPshsX";

    return optimizePshsOps(index);
}


//  Optimize
//   LDY/LEAY
//   PSHS Y
//   LDX/LEAX
//   PSHS X
//   LDD
//   PSHS B,A
//
//   to
//
//   LDY/LEAY
//   LDX/LEAX
//   LDD
//   PSHS Y,X,B,A
//
bool
ASMText::optimizePshsOps(size_t index) {
    if (! isYRegisterAllowed())
        return false;  // avoid using Y

    StaticArray<size_t, 3> pshsIndices;  // the following code must push at most 3 elements in this array

    // First element has to be an instruction
    const Element *e = &elements[index++];
    if (e->type != INSTR)
      return false;

    // Look for LDY/LEAY followed by PSHS Y
    bool pshsY = false;
    if (e->fields[0] == "LDY" || e->fields[0] == "LEAY") {
      e = &elements[index++];

      if (e->type == INSTR && e->fields[0] == "PSHS" && e->fields[1] == "Y") {
        pshsIndices.push_back(index - 1);
        pshsY = true;
        e = &elements[index++];
      }
    }

    // Look for LDX/LEAX followed by PSHS X
    bool pshsX = false;
    if (e->type == INSTR && (e->fields[0] == "LDX" || e->fields[0] == "LEAX") &&
        e->fields[1].find(",S") == string::npos) {
      e = &elements[index++];

      if (e->type == INSTR && e->fields[0] == "PSHS" && e->fields[1] == "X") {
        pshsIndices.push_back(index - 1);
        pshsX = true;
        e = &elements[index++];
      }
    }

    // Should not happen   
    if (pshsY && !pshsX) {
      return false;
    }

    // Look for CLRA/CLRB/LDA/LDB/LDD followed by PSHS B,A 
    bool pshsD = false;
    if (e->type == INSTR &&
        (e->fields[0] == "CLRA" || e->fields[0] == "CLRB" ||
         e->fields[0] == "LDA" || e->fields[0] == "LDB" || e->fields[0] == "LDD") &&
        e->fields[1].find(",S") == string::npos) {
      e = &elements[index++];

      // Next instruction may be another CLRA/CLRB/LDA/LDB/LDDD
      if (e->type == INSTR && e->fields[0] != "PSHS") {
        if ((e->fields[0] == "CLRA" || e->fields[0] == "CLRB" ||
             e->fields[0] == "LDA" || e->fields[0] == "LDB" || e->fields[0] == "LDD") &&
            e->fields[1].find(",S") == string::npos) {
          e = &elements[index++];
        } else {
          return false;
        }
      }

      if (e->type == INSTR && e->fields[0] == "PSHS" && e->fields[1] == "B,A") {
        pshsIndices.push_back(index - 1);
        pshsD = true;
        e = &elements[index++];
      }
    }

    // Only worth doing if there are at least 2 Pushes
    if (pshsIndices.size() < 2) {
      return false;
    }

    // Generate the new PSHS instruction
    char regsToPush[16] = "";
    if (pshsY) strcat(regsToPush, ",Y");
    if (pshsX) strcat(regsToPush, ",X");
    if (pshsD) strcat(regsToPush, ",B,A");
    Element &pshs = elements[pshsIndices[pshsIndices.size() - 1]];
    pshs.fields[1] = regsToPush + 1;  // + 1 to ignore initial comma
    pshs.fields[2] = "optim: optimizePshsOps";

    // Comment out the old pshs instructions
    for(size_t ii = 0; ii < (pshsIndices.size() - 1); ii++) {
      commentOut(pshsIndices[ii], "optim: optimizePshsOps");
    }

    return true;
}


//  Optimize
//   PSHS B,A
//   LDD ?,U
//   CMPD ,S++
//   [L]B?? ?
//
//  To
//   LDD ?,U
//   CMPD ?,U
//   inverse([L]B??) ?
//
bool
ASMText::optimize16BitCompares(size_t index) {
    if (index + 4 >= elements.size()) {
      return false;
    }

    Element &pshs = elements[index];
    if (pshs.type != INSTR || pshs.fields[0] != "PSHS" ||
        pshs.fields[1] != "B,A") {
      return false;
    }

    Element &ldd = elements[index + 1];
    if (ldd.type != INSTR || ldd.fields[0] != "LDD" ||
        !endsWith(ldd.fields[1], ",U")) {
      return false;
    }

    Element &cmpd = elements[index + 2];
    if (cmpd.type != INSTR || cmpd.fields[0] != "CMPD" ||
        cmpd.fields[1] != ",S++") {
      return false;
    }

    char invertedOperandsBranchInstr[INSTR_NAME_BUFSIZ];
    if (!isRelativeSizeConditionalBranch(index + 3, invertedOperandsBranchInstr)) {
      return false;
    }
    Element &branch = elements[index + 3];

    commentOut(index, "optim: optimize16BitCompares");
    cmpd.fields[1] = ldd.fields[1];
    cmpd.fields[2] = "optim: optimize16BitCompares";
    commentOut(index + 1, "optim: optimize16BitCompares");
    branch.fields[0] = invertedOperandsBranchInstr;
    branch.fields[2] = "optim: optimize16BitCompares";

    return true;
}


//  Optimize consecutive ADDD and SUBD into a single op:
//   ADDD #2
//   SUBD #5
//
//  To
//   ADDD #$FFFA
//
bool
ASMText::combineConsecutiveOps(size_t index) {
    if (index + 2 > elements.size()) {
      return false;
    }

    Element &opd1 = elements[index];
    int n1;
    if (opd1.type != INSTR ||
        !(opd1.fields[0] == "ADDD" || opd1.fields[0] == "SUBD") ||
        !extractConstantLiteral(opd1.fields[1], n1)) {
      return false;
    }
    if (opd1.fields[0] == "SUBD") {
      n1 = -n1;
    }

    Element &opd2 = elements[index + 1];
    int n2;
    if (opd2.type != INSTR ||
        !(opd2.fields[0] == "ADDD" || opd2.fields[0] == "SUBD") ||
        !extractConstantLiteral(opd2.fields[1], n2)) {
      return false;
    }
    if (opd2.fields[0] == "SUBD") {
      n2 = -n2;
    }

    // Make sure that there is at least one instr before the
    // basic block ends
    for(size_t ii=index+2; ii < elements.size(); ii++) {
      const Element &e = elements[ii];
      if (e.type == COMMENT) {
        continue;
      }
      if (e.type != INSTR) {
        return false; // could be a label
      }
      if (isBasicBlockEndingInstruction(e)) {
        return false;
      }
      break;
    }

    // comment out opd1
    commentOut(index, "optim: combineConsecutiveOps");

    // patch up opd2
    const uint16_t n = (uint16_t)((n1 + n2) & 0xffff);
    opd2.fields[0] = "ADDD";
    opd2.fields[1] = "#" + wordToString(n, true);
    opd2.fields[2] = "optim: combineConsecutiveOps";

    return true;
}


//  Remove consecutive PSHS B,A and LDD	,S++
//
bool
ASMText::removeConsecutivePshsPul(size_t index) {
    if (index + 2 > elements.size()) {
      return false;
    }

    // First instruction must be PSHS B,A
    Element &opd1 = elements[index];
    if (opd1.type != INSTR || opd1.fields[0] != "PSHS" || opd1.fields[1] != "B,A") {
      return false;
    }

    // Next instr must be LDD ,S++
    size_t ii;
    for(ii = index+1; ii < elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.type == COMMENT) {
        continue;
      }
      if (e.type != INSTR) {
        return false; // could be a label
      }
      if (e.fields[0] != "LDD" || e.fields[1] != ",S++") {
        return false;
      }
      break;
    }
    if (ii >= elements.size()) {
      return false;
    }
    const size_t opd2Index = ii;

    // Make sure that there is at least one instr before the
    // basic block ends
    for(; ii < elements.size(); ii++) {
      Element &e = elements[ii++];
      if (e.type == COMMENT) {
        continue;
      }
      if (e.type != INSTR) {
        return false; // could be a label
      }
      if (isBasicBlockEndingInstruction(e)) {
        return false;
      }
      break;
    }

    // comment out opd1 and opd2
    commentOut(index, "optim: removeConsecutivePshsPul");
    commentOut(opd2Index, "optim: removeConsecutivePshsPul");

    return true;
}


// Coalesce LEAX N,X_or_PCR LEAX M,X to LEAX N+M,X, where N and M are constants.
//
bool
ASMText::coalesceConsecutiveLeax(size_t index) {
    if (index + 2 > elements.size()) {
      return false;
    }

    // First 2 instructions must be LEAX ????,????
    Element &e1 = elements[index];
    string &e1Field1 = e1.fields[1];
    if (e1.type != INSTR || e1.fields[0] != "LEAX" || startsWith(e1Field1, "A,")
        || startsWith(e1Field1, "B,") || startsWith(e1Field1, "D,")) {
      return false;
    }
    Element &e2 = elements[index + 1];
    const string &e2Field1 = e2.fields[1];
    if (e2.type != INSTR || e2.fields[0] != "LEAX" || !endsWith(e2Field1, ",X")  || startsWith(e2Field1, "A,")
        || startsWith(e2Field1, "B,") || startsWith(e2Field1, "D,")) {
      return false;
    }
    int offset1 = 0, offset2 = 0;
    bool isNumeric = true;
    if (isdigit(e1Field1[0]) || e1Field1[0] == '+' || e1Field1[0] == '-' || e1Field1[0] == ',')
      parseRelativeOffset(e1Field1, offset1);
    else
      isNumeric = false;
    if (isNumeric &&
        (isdigit(e2Field1[0]) || e2Field1[0] == '+' || e2Field1[0] == '-' || e2Field1[0] == ','))
      parseRelativeOffset(e2Field1, offset2);
    else
      isNumeric = false;

    size_t comma1Index = e1Field1.find(",");
    char temp[512];
    if (isNumeric) {
      snprintf(temp, sizeof(temp), "%u%s", uint16_t((offset1 + offset2) & 0xffff), e1Field1.c_str() + comma1Index);
    } else {
      size_t comma2Index = e2Field1.find(",");
      snprintf(temp, sizeof(temp), "%.*s%s%.*s%s",
                      int(comma1Index), e1Field1.c_str(),
                      comma2Index == 0 ? "" : "+",
                      int(comma2Index), e2Field1.c_str(),
                      e1Field1.c_str() + comma1Index);
    }
    e1Field1 = temp;
    commentOut(index + 1, "optim: coalesceConsecutiveLeax");
    return true;
}


//  Optimize LEAX AA,X
//           LDX CC,X
//  To       LDX AA+CC,X
//
bool
ASMText::optimizeLeaxLdx(size_t index) {
    if (index + 2 > elements.size()) {
      return false;
    }

    // First instruction must be LEAX AA,X
    Element &e1 = elements[index];
    if (e1.type != INSTR || e1.fields[0] != "LEAX" || !endsWith(e1.fields[1], ",X") 
        || startsWith(e1.fields[1], "A,") || startsWith(e1.fields[1], "B,") 
        || startsWith(e1.fields[1], "D,")) {
      return false;
    }

    // Next instruction must be LDX CC,X
    Element &e2 = elements[index + 1];
    if (e2.type != INSTR || e2.fields[0] != "LDX"
        || e2.fields[1].find("+") != string::npos
        || e2.fields[1].find("-") != string::npos
        || e2.fields[1].find("D") != string::npos
        || !endsWith(e2.fields[1], ",X")) {
      return false;
    }

    int offset1 = 0, offset2 = 0;
    parseRelativeOffset(e1.fields[1], offset1);
    parseRelativeOffset(e2.fields[1], offset2);
    e2.fields[1] = wordToString((offset1 + offset2) & 0xffff, false) + ",X";
    e2.fields[2] = "optim: optimizeLeaxLdx";
    commentOut(index, "optim: optimizeLeaxLdx");
    return true;
}


//  Optimize LEAX AA,X
//           LDD CC,X
//  To       LDD AA+CC,X
//
bool
ASMText::optimizeLeaxLdd(size_t index) {
    if (index + 2 > elements.size()) {
      return false;
    }

    // First instruction must be LEAX AA,X
    Element &e1 = elements[index];
    if (e1.type != INSTR || e1.fields[0] != "LEAX" || !endsWith(e1.fields[1], ",X") 
        || startsWith(e1.fields[1], "A,") || startsWith(e1.fields[1], "B,")
        || startsWith(e1.fields[1], "D,")) {
      return false;
    }

    // Next instruction must be LDD CC,X
    Element &e2 = elements[index + 1];
    if (e2.type != INSTR || e2.fields[0] != "LDD"
        || e2.fields[1].find("+") != string::npos
        || e2.fields[1].find("-") != string::npos
        || e2.fields[1].find("D") != string::npos
        || !endsWith(e2.fields[1], ",X")) {
      return false;
    }

    // Verify that the X value is not used anywhere
    for (size_t ii=index+2; ii<elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;
      if ((e.fields[0].find("BSR") != string::npos &&
           !startsWith(e.fields[1], "_")) ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      const InsEffects insEffects(e);
      if (insEffects.read & X)
        return false;
      if (insEffects.written & X)
        break;
    }

    int offset1 = 0, offset2 = 0;
    parseRelativeOffset(e1.fields[1], offset1);
    parseRelativeOffset(e2.fields[1], offset2);
    e2.fields[1] = wordToString((offset1 + offset2) & 0xffff, false) + ",X";
    e2.fields[2] = "optim: optimizeLeaxLdd";
    commentOut(index, "optim: optimizeLeaxLdd");
    return true;
}


//  Optimize LDX AA,BB
//           ???
//           ??? ,X
//  To       ???
//           ??? [AA,BB]
bool
ASMText::optimizeLdx(size_t index) {
    if (index + 3 > elements.size()) {
      return false;
    }

    // First instruction must be LDX AA,BB
    Element &e1 = elements[index];
    if (e1.type != INSTR || e1.fields[0] != "LDX" || endsWith(e1.fields[1], "]") 
        || startsWith(e1.fields[1], "A,") || startsWith(e1.fields[1], "B,")
        || startsWith(e1.fields[1], "D,")
        || e1.fields[1].find(",-") != string::npos
        || endsWith(e1.fields[1], "+") || e1.fields[1].find(",") == string::npos) {
      return false;
    }

    // Find the first usage of X
    size_t usageIndex = index;
    bool xUpdated = false;
    for (size_t ii=index+1; ii<elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;
      if ((e.fields[0].find("BSR") != string::npos &&
           !startsWith(e.fields[1], "_")) ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      const InsEffects insEffects(e);
      if (insEffects.read & X) {
        usageIndex = ii;
        xUpdated = (insEffects.written & X) ? true : false;
        break;
      }
      if (insEffects.written & (X | U))
        return false;
    }
    if (usageIndex == index) {
      return false;
    }

    // Verify that the X value is not used anywhere else
    for (size_t ii=usageIndex + 1; !xUpdated && ii<elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;
      if ((e.fields[0].find("BSR") != string::npos &&
           !startsWith(e.fields[1], "_")) ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      const InsEffects insEffects(e);
      if (insEffects.read & X)
        return false;
      if (insEffects.written & X)
        break;
    }

    // usage instruction must be ??? ,X
    Element &e2 = elements[usageIndex];
    if (e2.type != INSTR || startsWith(e2.fields[0], "LEA") ||
        e2.fields[1] != ",X") {
      return false;
    }

    e2.fields[1] = "[" + e1.fields[1] + "]";
    e2.fields[2] = "optim: optimizeLdx";
    commentOut(index, "optim: optimizeLdx");
    return true;
}


//  Optimize LEAX AA,BB
//           ???
//           ??? ,X
//  To       ???
//           ??? AA,BB
bool
ASMText::optimizeLeax(size_t index) {
    if (index + 3 > elements.size()) {
      return false;
    }

    // First instruction must be LEAX AA,BB
    Element &e1 = elements[index];
    if (e1.type != INSTR || e1.fields[0] != "LEAX" || endsWith(e1.fields[1], "]") 
        || startsWith(e1.fields[1], "A,") || startsWith(e1.fields[1], "B,")
        || startsWith(e1.fields[1], "D,")) {
      return false;
    }

    // Find the first usage of X
    size_t usageIndex = index;
    bool xUpdated = false;
    for (size_t ii=index+1; ii<elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike())
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;
      if ((e.fields[0].find("BSR") != string::npos &&
           !startsWith(e.fields[1], "_")) ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      const InsEffects insEffects(e);
      if (insEffects.read & X) {
        usageIndex = ii;
        xUpdated = (insEffects.written & X) ? true : false;
        break;
      }
      if (insEffects.written & (X | U))
        return false;
    }
    if (usageIndex == index) {
      return false;
    }

    // Verify that the X value is not used anywhere else
    for (size_t ii=usageIndex + 1; !xUpdated && ii<elements.size(); ii++) {
      Element &e = elements[ii];
      if (e.isCommentLike() || e.type == LABEL)
        continue;
      if (e.type != INSTR)
        return false;
      if (isBasicBlockEndingInstruction(e))
        break;
      if ((e.fields[0].find("BSR") != string::npos &&
           !startsWith(e.fields[1], "_")) ||
          e.fields[0].find("JSR") != string::npos)
        return false;
      const InsEffects insEffects(e);
      if (insEffects.read & X)
        return false;
      if (insEffects.written & X)
        break;
    }

    // usage instruction must be ??? ,X
    Element &e2 = elements[usageIndex];
    if (e2.type != INSTR || e2.fields[1] != ",X") {
      return false;
    }

    e2.fields[1] = e1.fields[1];
    e2.fields[2] = "optim: optimizeLeax";
    commentOut(index, "optim: optimizeLeax");
    return true;
}

 
//  Remove TFR ?,X when the X values is not used.
//
bool
ASMText::removeUselessTfr1(size_t index) {
    const size_t startIndex = index;
    Element &e = elements[index];
    if (e.fields[0] != "TFR" || e.fields[1].find(",X") == string::npos)
      return false;

    for(index++; index<elements.size(); index++) {
        Element &e1 = elements[index];
        if (e1.isCommentLike())
            continue;
        if (e1.type != INSTR)
            return false;
        if (isBasicBlockEndingInstruction(e1))
        {
            if (currentCallConvention == GCC6809_CALL_CONV)
                return false;  // keep TFR _,X b/c X has return value under GCC6809 convention
            break;
        }
        if (isFunctionEndingInstruction(e1))
        {
            if (currentCallConvention == GCC6809_CALL_CONV)
                return false;  // keep TFR _,X b/c X has return value under GCC6809 convention
            break;
        }
        if (e1.fields[0].find("BSR") != string::npos ||
            e1.fields[0].find("JSR") != string::npos)
            return false;

        const InsEffects effects(e1);
        if (effects.read & X)
            return false;
        if (effects.written & X)
            break;
    }

    commentOut(startIndex, "optim: removeUselessTfr1");
    return true;
}


//  optimize LDX ???
//           TFR X,D
//  to       LDD ???
//
bool
ASMText::removeUselessTfr2(size_t index) {
    const size_t startIndex = index;
    Element &e1 = elements[index++];
    if (e1.fields[0] != "LDX")
      return false;
    Element &e2 = elements[index++];
    if (e2.fields[0] != "TFR" || e2.fields[1] != "X,D")
      return false;

    for(index++; index<elements.size(); index++) {
        Element &e = elements[index];
        if (e.isCommentLike())
            continue;
        if (e.type != INSTR)
            return false;
        if (isBasicBlockEndingInstruction(e1))
            break;
        if (e.fields[0].find("BSR") != string::npos ||
            e.fields[0].find("JSR") != string::npos)
            return false;

        const InsEffects effects(e);
        if (effects.read & X)
            return false;
        if (effects.written & X)
            break;
    }

    e1.fields[0] = "LDD";
    commentOut(startIndex + 1, "optim: removeUselessTfr2");
    return true;
}


//  Remove CLRB when the B values is not used.
//
bool
ASMText::removeUselessClrb(size_t index) {
    const size_t startIndex = index;
    Element &e = elements[index];
    if (e.fields[0] != "CLRB")
      return false;

    for(index++; index<elements.size(); index++) {
        Element &e1 = elements[index];
        if (e1.isCommentLike())
            continue;
        if (e1.type != INSTR)
            return false;
        if (isBasicBlockEndingInstruction(e1))
            return false;
        if (e1.fields[0].find("BSR") != string::npos ||
            e1.fields[0].find("JSR") != string::npos)
            return false;

        const InsEffects effects(e1);
        if (effects.read & B)
            return false;
        if (effects.written & B)
            break;
    }

    commentOut(startIndex, "optim: removeUselessClrb");
    return true;
}


// Optimize repeated sequences of
//      TFR           X,D
//      (ADD/SUB)D    #XXXX
//
// This can be done when D is changed in a predictable way such that X,D
// remain a predictable different from each other.
bool
ASMText::optimizeDXAliases(size_t index) {
    if (index + 4 > elements.size()) {
      return false;
    }

    // Find the first group
    const Element &e1 = elements[index++];
    int accumOffset = 0;
    if (e1.fields[0] != "TFR" || e1.fields[1] != "X,D")
      return false;
    const Element &e2 = elements[index++];
    if (!(e2.fields[0] == "ADDD" || e2.fields[0] == "SUBD") ||
        !parseConstantLiteral(e2.fields[1], accumOffset))
      return false;
    accumOffset = (e2.fields[0] == "SUBD") ? -accumOffset : accumOffset;

    // Find subsequent groups
    bool madeChanges = false;
    do {
      // Find the next TFR
      for(index++; index<elements.size(); index++) {
          const Element &e3 = elements[index];
          if (e3.isCommentLike())
              continue;
          if (e3.type != INSTR)
              return madeChanges;
          if (isBasicBlockEndingInstruction(e3))
              return madeChanges;
          if (e3.fields[0].find("BSR") != string::npos ||
              e3.fields[0].find("JSR") != string::npos)
              return madeChanges;

          if (e3.fields[0] == "TFR" && e3.fields[1] == "X,D")
              break;

          const InsEffects effects(e3);
          if ((effects.written & (X | D)) != 0) {
              return madeChanges;
          }
          if (effects.read & D)
              return madeChanges;
      }
      if (index >= elements.size())
          return madeChanges;

      // Get the next ADDD or SUBD
      Element &e4 = elements[++index];
      int currentOffset = 0;
      if (!(e4.fields[0] == "ADDD" || e4.fields[0] == "SUBD") ||
          !parseConstantLiteral(e4.fields[1], currentOffset))
        return madeChanges;
      currentOffset = (e4.fields[0] == "SUBD") ? -currentOffset : currentOffset;
 
      // Next instr cannot be a conditional branch
      char buffer[INSTR_NAME_BUFSIZ];
      if (isConditionalBranch(index, buffer))
        return madeChanges;

      // By changing e4 we can comment out e3.
      e4.fields[0] = "ADDD";
      e4.fields[1] = "#" + wordToString((currentOffset - accumOffset) & 0xffff, true);
      e4.fields[2] = "optim: optimizeDXAliases";
      commentOut(index - 1, "optim: optimizeDXAliases");
      accumOffset = currentOffset;
      index++;
    } while(true);

    return madeChanges;
}


/*  An expression like 'c < ' ' || c > 127' can give this code:
        LDB	    5,U		  variable c
        CMPB	  #$20	
        BLO	    foo
        LDB	    5,U		  variable c
        CMPB    #$7F
    This function eliminates the 2nd LDB.
*/
bool
ASMText::removeLoadInComparisonWithTwoValues(size_t index)
{
    // Check for starting LDB.
    size_t firstLDBIndex = index;
    if (firstLDBIndex == size_t(-1))
        return false;
    const Element &firstLDB = elements[firstLDBIndex];
    if (firstLDB.fields[0] != "LDB")
        return false;

    // Require a CMPB after LDB.
    size_t firstCMPBIndex = findNextInstrBeforeLabel(firstLDBIndex + 1);
    if (firstCMPBIndex == size_t(-1))
        return false;
    const Element &firstCMPB = elements[firstCMPBIndex];
    if (firstCMPB.fields[0] != "CMPB")
        return false;

    // Require a condition branch instruction after CMPB.
    size_t branchIndex = findNextInstrBeforeLabel(firstCMPBIndex + 1);
    if (branchIndex == size_t(-1))
        return false;
    const Element &branch = elements[branchIndex];
    if (! isConditionalBranch(branch.fields[0].c_str()))
        return false;

    // Require an LDB after the branch.
    size_t secondLDBIndex = findNextInstrBeforeLabel(branchIndex + 1);
    if (secondLDBIndex == size_t(-1))
        return false;
    const Element &secondLDB = elements[secondLDBIndex];
    if (secondLDB.fields[0] != "LDB")
        return false;

    // Require the 2nd LDB to have the same argument as the 1st one.
    if (secondLDB.fields[1] != firstLDB.fields[1])
        return false;

    // Require a 2nd CMPB after the 2nd LDB.
    size_t secondCMPBIndex = findNextInstrBeforeLabel(secondLDBIndex + 1);
    if (secondCMPBIndex == size_t(-1))
        return false;
    const Element &secondCMPB = elements[secondCMPBIndex];
    if (secondCMPB.fields[0] != "CMPB")
        return false;

    // Remove the 2nd LDB.
    commentOut(secondLDBIndex, "optim: removeLoadInComparisonWithTwoValues");
    return true;  // modified the code
}


// Replace "LEA_ foo+n,PCR" with "LD_ #foo+n" if the compilation is not asked
// to produce position-independent code.
// Replace "LD_ foo+n,PCR" with "LD_ foo+n".
// Replace "ST_ foo+n,PCR" with "ST_ foo+n".
//
bool
ASMText::removePCRIfRelocatabilityNotSupported(size_t index)
{
    if (TranslationUnit::instance().isRelocatabilitySupported())
        return false;

    if (index == size_t(-1))
        return false;

    Element &el = elements[index];

    string &arg = el.fields[1];
    if (!endsWith(arg, ",PCR"))
        return false;

    string &ins = el.fields[0];
    if (startsWith(ins, "LEA"))
    {
        char reg = ins[3];
        ins.resize(3);
        ins[1] = 'D', ins[2] = reg;
        arg = "#" + string(arg, 0, arg.length() - 4);
        el.fields[2] += " (optim: removePCRIfRelocatabilityNotSupported)";
        return true;
    }
    if (startsWith(ins, "LD") || startsWith(ins, "ST") || ins == "CLR")  // if LD_ or ST_ foo+n,PCR
    {
        arg.erase(arg.length() - 4);  // remove ",PCR"
        el.fields[2] += " (optim: removePCRIfRelocatabilityNotSupported)";
        return true;
    }

    return false;
}


// If LDB ___; CMPB #$00; [L]BHI ___;
// then remove CMPB and replace branch with [L]BNE.
// Similarly if LDD ___; ADDD #0; [L]BHI ___.
// BHI is an unsigned comparison. It requires C and Z.
// Unsigned_value > 0 is equivalent to Unsigned_value != 0.
// BNE only requires Z, which is set by LD.
//
bool
ASMText::removeCMPZeroAfterLDIfBHI(size_t index)
{
    size_t loadIndex = index;
    bool isLDB = isInstrAnyArg(loadIndex, "LDB");
    if (!isLDB && !isInstrAnyArg(loadIndex, "LDD"))  // if index is size_t(-1), call gives false
        return false;
    size_t cmpIndex = findNextInstrBeforeLabel(loadIndex + 1);
    if (isLDB && !isInstr(cmpIndex, "CMPB", "#$00"))
        return false;
    if (!isLDB && !isInstr(cmpIndex, "ADDD", "#0"))
        return false;
    size_t branchIndex = findNextInstrBeforeLabel(cmpIndex + 1);
    if (!isInstrAnyArg(branchIndex, "LBHI") && !isInstrAnyArg(branchIndex, "BHI"))
        return false;

    // Apply the optimization.
    commentOut(cmpIndex, "optim: removeCMPZeroAfterLDIfBHI (BHI->BNE)");
    string &ins = elements[branchIndex].fields[0];
    if (ins[0] == 'L')
        ins = "LBNE";
    else
        ins = "BNE";
    return true;
}


// Check for Bxx foo; ...; foo EQU *; BRA bar.
// Replace with LBxx bar.
//
bool
ASMText::replaceBranchToUncondBranch(size_t index)
{
    size_t firstBranchIndex = index;
    if (!isBranchAtIndex(firstBranchIndex))
        return false;

    string &firstBranchDest = elements[firstBranchIndex].fields[1];  // we have Bxx firstBranchDest at 'index'

    size_t firstBranchDestIndex = findLabelAroundIndex(firstBranchDest, index, 100);
    if (firstBranchDestIndex == size_t(-1))
        return false;

    // Label firstBranchDest is at firstBranchDestIndex.
    // Check if next instruction is [L]BRA.
    size_t braIndex = findNextInstrBeforeLabel(firstBranchDestIndex + 1);
    if (!isInstrAnyArg(braIndex, "BRA") && !isInstrAnyArg(braIndex, "LBRA"))
        return false;

    // Nothing to do if the BRA label is the same. (Possible with "for (;;);")
    if (firstBranchDest == elements[braIndex].fields[1])
        return false;

    // Apply the optimization.
    // Make Bxx branch to the label at which the BRA branches.
    firstBranchDest = elements[braIndex].fields[1];

    // Make Bxx long in case that new label is far.
    // shortenBranch() is expected to shorten that branch if possible.
    string &firstBranchInstr = elements[firstBranchIndex].fields[0];
    if (firstBranchInstr[0] != 'L')
        firstBranchInstr = "L" + firstBranchInstr;

    return true;
}


// Looks for LDB foo; Bxx label; LDB foo; CMPB ___
// and removes the 2nd LDB.
// This optimization could be extended to a series of more than two LDBs.
// It could also be extended to LDD+CMPD.
//
bool
ASMText::ldbBranchLdbCompare(size_t index)
{
    if (!isInstrAnyArg(index, "LDB"))
        return false;
    size_t branchIndex = findNextInstrBeforeLabel(index + 1);
    if (!isConditionalBranchAtIndex(branchIndex))
        return false;
    size_t secondLoadIndex = findNextInstrBeforeLabel(branchIndex + 1);
    if (!isInstrAnyArg(secondLoadIndex, "LDB"))
        return false;
    if (elements[index].fields[1] != elements[secondLoadIndex].fields[1])  // both loads must have same arg
        return false;
    size_t cmpIndex = findNextInstrBeforeLabel(secondLoadIndex + 1);
    if (!isInstrAnyArg(cmpIndex, "CMPB"))
        return false;
    
    // Apply the optimization.
    commentOut(secondLoadIndex, "optim: ldbBranchLdbCompare");
    return true;
}


// Look for: LDB foo; PSHS B; LDB bar; CMPB ,S+.
// Change to: LDB bar; CMPB foo.
// LDB args must not involve S.
//
bool
ASMText::removePushBFromLdbPushLdbCmp(size_t index)
{
    if (!isInstrAnyArg(index, "LDB"))
        return false;
    if (elements[index].fields[1].find(",S") != string::npos)
        return false;

    size_t pushIndex = findNextInstrBeforeLabel(index + 1);
    if (!isInstr(pushIndex, "PSHS", "B"))
        return false;

    size_t secondLoadIndex = findNextInstrBeforeLabel(pushIndex + 1);
    if (!isInstrAnyArg(secondLoadIndex, "LDB"))
        return false;
    if (elements[secondLoadIndex].fields[1].find(",S") != string::npos)
        return false;
    
    size_t cmpIndex = findNextInstrBeforeLabel(secondLoadIndex + 1);
    if (!isInstr(cmpIndex, "CMPB", ",S+"))
        return false;

    // Apply the optimization.

    elements[cmpIndex].fields[1] = elements[index].fields[1];  // copy arg of 1st LDB
    commentOut(index, "optim: removePushBFromLdbPushLdbCmp");
    commentOut(pushIndex, "optim: removePushBFromLdbPushLdbCmp");
    return true;
}


// Look for: PSHS B,A; LDD ,S.
// Remove the LDD instruction.
//
bool
ASMText::removeLDDAfterPushingD(size_t index)
{
    if (!isInstr(index, "PSHS", "B,A"))
        return false;
    size_t lddIndex = findNextInstrBeforeLabel(index + 1);
    if (!isInstr(lddIndex, "LDD", ",S"))
        return false;

    commentOut(lddIndex, "optim: removeLDDAfterPushingD");
    return true;
}


// Look for:
//     LDD	<arg>
//     LDX	#nnn		with nnn <= 255 (typically an array element size in bytes re: BinaryOpExpr::emitArrayRef())
//     LBSR	MUL16		puts the product in D; also accept BSR/JSR
// Replace with:
//     LDX	<arg>
//     LDB	#nnn
//     LBSR	MUL168		puts the product in D
//
bool
ASMText::arrayIndexMul(size_t index)
{
    if (!isInstrAnyArg(index, "LDD"))
        return false;
    // Got an LDD at 'index'.
    size_t ldxIndex = findNextInstrBeforeLabel(index + 1);
    if (ldxIndex == size_t(-1) || !isInstrWithImmedArg(ldxIndex, "LDX"))
        return false;
    // Got an LDX #. Check that its immediate argument is <= 255.
    uint16_t ldxImmedArg = extractImmedArg(ldxIndex);
    if (ldxImmedArg > 255)
        return false;
    size_t bsrIndex = findNextInstrBeforeLabel(ldxIndex + 1);
    if (bsrIndex == size_t(-1) || (!isInstrAnyArg(bsrIndex, "LBSR") && !isInstrAnyArg(bsrIndex, "BSR") && !isInstrAnyArg(bsrIndex, "JSR")))
        return false;
    // Got an LBSR/BSR/JSR.
    if (elements[bsrIndex].fields[1] != "MUL16")
        return false;

    // Apply the optimization.
    elements[index].fields[0] = "LDX";  // replace LDD
    elements[ldxIndex].fields[0] = "LDB";  // replace LDX
    elements[ldxIndex].fields[2] += " (optim: arrayIndexMul)";
    elements[bsrIndex].fields[1] = "MUL168";  // replace routine name
    TranslationUnit::instance().registerNeededUtility("MUL168");
    return true;
}


// Look for:
//     CLRA
//     LDB     <arg>
//     LBSR    MUL16
// Remove the CLRA and use MUL168.
//
bool
ASMText::removeClrAFromArrayIndexMul(size_t index)
{
    if (!isInstr(index, "CLRA", ""))
        return false;
    size_t ldbIndex = findNextInstrBeforeLabel(index + 1);
    if (ldbIndex == size_t(-1) || !isInstrAnyArg(ldbIndex, "LDB"))
        return false;
    // Got an LDB.
    size_t bsrIndex = findNextInstrBeforeLabel(ldbIndex + 1);
    if (bsrIndex == size_t(-1) || (!isInstrAnyArg(bsrIndex, "LBSR") && !isInstrAnyArg(bsrIndex, "BSR") && !isInstrAnyArg(bsrIndex, "JSR")))
        return false;
    // Got an LBSR/BSR/JSR.
    if (elements[bsrIndex].fields[1] != "MUL16")
        return false;

    // Apply the optimization.
    commentOut(index, "optim: removeClrAFromArrayIndexMul");
    elements[bsrIndex].fields[1] = "MUL168";  // replace routine name
    TranslationUnit::instance().registerNeededUtility("MUL168");
    return true;
}


// Look for this:
//     LDX <arg>
//     LDA/LDB/etc. that does not change X
//     ...
//     LDX <same arg>
// Remove the second LDX.
// This optimization has been seen to allow other optimizations to find
// more instructions to remove.
//
bool
ASMText::removeRepeatedLDX(size_t index)
{
    if (!isInstrAnyArg(index, "LDX"))
        return false;
    const size_t firstLDXIndex = index;
    size_t secondLDXIndex = size_t(-1);
    ++index;
    for (;;)
    {
        size_t intermediateInstrIndex = findNextInstrBeforeLabel(index);
        if (intermediateInstrIndex == size_t(-1))
            break;  // end of instruction sequence
        Element &el = elements[intermediateInstrIndex];
        if (el.fields[0] == "LDX" && el.fields[1] == elements[firstLDXIndex].fields[1])
        {
            secondLDXIndex = intermediateInstrIndex;
            break;
        }
        static const char *acceptedInstructions[] = { "LDA", "LDB", "CLRA", "CLRB", "STA", "STB", "LDD", "STD" };
        static const auto end = acceptedInstructions + sizeof(acceptedInstructions) / sizeof(acceptedInstructions[0]);
        auto it = std::find_if(acceptedInstructions, end,
                               [&el](const char *s) { return el.fields[0] == s; });
        if (it == end)  // if not found
            return false;  // not an accepted instruction
        const string &arg = el.fields[1];
        if (arg.find("-X") != string::npos || arg.find("X+") != string::npos)  // if decrement or increment
            return false;  // X changed by intervening instructions: can't apply optimization
        
        index = intermediateInstrIndex + 1;  // for next search
    }

    if (secondLDXIndex == size_t(-1))
        return false;  // second indentical LDX not found
    
    // Apply the optimization.
    commentOut(secondLDXIndex, "optim: removeRepeatedLDX");
    return true;
}


// Replace this:
//   PSHS    B,A
//   LDD     {foo}
//   ADDD    ,S
//   ADDD    ,S++
// with this:
//   LSLB
//   ROLA
//   ADDD    {foo}
//
bool
ASMText::avoidPushingDoubledD(size_t index)
{
    if (!isInstr(index, "PSHS", "B,A"))
        return false;
    size_t lddIndex = findNextInstrBeforeLabel(index + 1);
    if (!isInstrAnyArg(lddIndex, "LDD"))
        return false;
    size_t firstAddIndex = findNextInstrBeforeLabel(lddIndex + 1);
    if (!isInstr(firstAddIndex, "ADDD", ",S"))
        return false;
    size_t secondAddIndex = findNextInstrBeforeLabel(firstAddIndex + 1);
    if (!isInstr(secondAddIndex, "ADDD", ",S++"))
        return false;

    string lddArg     = elements[lddIndex].fields[1];
    string lddComment = elements[lddIndex].fields[2];

    commentOut(index, "optim: avoidPushingDoubledD");
    replaceWithInstr(lddIndex,       "LSLB");
    replaceWithInstr(firstAddIndex,  "ROLA");
    replaceWithInstr(secondAddIndex, "ADDD", lddArg, lddComment);
    return true;
}


// Replace this:
//   PSHS    B
//   [CLRA]         ; optional
//   LDB    {foo}
//   PULS    A
//   MUL
// with this:
//   LDA    {foo}
//   MUL
//
bool
ASMText::removePushBPullABeforeMul(size_t index)
{
    if (!isInstr(index, "PSHS", "B"))
        return false;
    size_t ldbIndex = findNextInstrBeforeLabel(index + 1);
    size_t clraIndex = size_t(-1);
    if (isInstr(ldbIndex, "CLRA", ""))
    {
        clraIndex = ldbIndex;
        ldbIndex = findNextInstrBeforeLabel(ldbIndex + 1);
    }
    if (!isInstrAnyArg(ldbIndex, "LDB"))
        return false;
    size_t pulsIndex = findNextInstrBeforeLabel(ldbIndex + 1);
    if (!isInstr(pulsIndex, "PULS", "A"))
        return false;
    size_t mulIndex = findNextInstrBeforeLabel(pulsIndex + 1);
    if (!isInstr(mulIndex, "MUL", ""))
        return false;
    
    // Apply the optimization.
    commentOut(index, "optim: removePushBPullABeforeMul");  // remove the PSHS B
    if (clraIndex != size_t(-1))
        commentOut(clraIndex, "optim: removePushBPullABeforeMul");
    elements[ldbIndex].fields[0] = "LDA";
    commentOut(pulsIndex, "optim: removePushBPullABeforeMul");
    return true;
}


// Replace this:
//      PSHS    B
//      [CLRA]
//      LDB     {foo}
//      SUBB    S+
// with this:
//      [CLRA]
//      NEGB
//      ADDB    {foo}
//
bool
ASMText::removePushBBeforeSubB(size_t index)
{
    if (!isInstr(index, "PSHS", "B"))
        return false;
    size_t ldbIndex = findNextInstrBeforeLabel(index + 1);
    if (isInstr(ldbIndex, "CLRA", ""))
        ldbIndex = findNextInstrBeforeLabel(ldbIndex + 1);
    if (!isInstrAnyArg(ldbIndex, "LDB"))
        return false;
    size_t subIndex = findNextInstrBeforeLabel(ldbIndex + 1);
    if (!isInstr(subIndex, "SUBB", ",S+"))
        return false;

    // Apply the optimization.
    commentOut(index, "optim: removePushBBeforeSubB");  // remove PSHS
    elements[subIndex].fields[0] = "ADDB";
    elements[subIndex].fields[1] = elements[ldbIndex].fields[1];  // move LDB arg to ADDB
    elements[subIndex].fields[2] = "optim: removePushBBeforeSubB";
    elements[ldbIndex].fields[0] = "NEGB";
    elements[ldbIndex].fields[1] = "";
    elements[ldbIndex].fields[2] = "optim: removePushBBeforeSubB";
    return true;
}


// Look for this:
//      LDB/LDX #___        ; constant argument
//      STB/STX -__,U       ; <parameterSaveCommentPrefix> ...
//      ...
//      LDB/LDX -__,U       ; same arg as store instruction
// Replace the last LDB/LDX with this:
//      LDB/LDX #___        ; constant argument
// and remove the two previous instructions.
// This may serve on a call that uses the GCC6809 convention.
//
bool
ASMText::avoidSavingConstantRegisterPassedParameter(size_t index)
{
    // Look for LDB/LDX #___.
    //
    Element &load = elements[index];
    if (load.type != INSTR)
        return false;
    if (load.fields[0] != "LDB" && load.fields[0] != "LDX")
        return false;
    if (load.fields[1][0] != '#')
        return false;

    // Require next instruction to be a store that saves a parameter
    // for an upcoming call.
    //
    const size_t storeIndex = findNextInstrBeforeLabel(index + 1);
    if (storeIndex == size_t(-1))
        return false;
    if (!isInstrAnyArg(storeIndex, "STB") && !isInstrAnyArg(storeIndex, "STX"))
        return false;
    Element &store = elements[storeIndex];
    if (!startsWith(store.fields[2], parameterSaveCommentPrefix))
        return false;

    // Look for LDB/LDX with the same arg as the store instruction.
    //
    size_t reloadIndex = storeIndex;
    while ((reloadIndex = findNextInstrBeforeLabel(reloadIndex + 1)) != size_t(-1))
    {
        Element &reload = elements[reloadIndex];
        if (reload.fields[0] == load.fields[0] && reload.fields[1] == store.fields[1])
        {
            // Found. Apply the optimization.
            assert(reload.fields[0] != "STX");
            reload.fields[1] = load.fields[1];  // load immediate, instead of load from _,U
            reload.fields[2] = "optim: avoidSavingConstantRegisterPassedParameter"; 
            commentOut(index, "optim: avoidSavingConstantRegisterPassedParameter");  // remove initial load immediate
            commentOut(storeIndex, "optim: avoidSavingConstantRegisterPassedParameter");  // remove store
            return true;
        }
    }

    return false;
}


// Looks for consecutive immediate mode LDB/LDX instructions.
// When two of the same kind are found, the first one is removed.
// This can remove double loads that may precede a function call
// that uses the GCC convention.
//
bool
ASMText::removeUselessGCCCallLoads(size_t index)
{
    size_t iter = index;

    // foundIndexes[0]: List of indexes in elements[] where an LDB # was found.
    // foundIndexes[1]: List of indexes in elements[] where an LDX # was found.
    size_t foundIndexes[2][2] = { { size_t(-1), size_t(-1) },
                                  { size_t(-1), size_t(-1) } };

    do
    {
        // Check for LDB/X #___.
        Element &e = elements[iter];
        if (e.type != INSTR)
            break;
        if (e.fields[0] != "LDB" && e.fields[0] != "LDX")
            break;
        if (e.fields[1][0] != '#')
            break;

        size_t *list = foundIndexes[e.fields[0] == "LDX"];
        if (list[0] == size_t(-1))  // if no LD* instructions already found
            list[0] = iter;
        else if (list[1] == size_t(-1))  // if only one already found
            list[1] = iter;
        // otherwise, ignore 3rd cases, etc.

        // Go to next instruction in current basic block.
        iter = findNextInstrBeforeLabel(iter + 1);
    } while (iter != size_t(-1));

    bool applied = false;
    if (foundIndexes[0][1] != size_t(-1))  // if two LDBs found
    {
        commentOut(foundIndexes[0][0], "removeUselessGCCCallLoads (LDB #)");  // remove first LDB
        applied = true;
    }
    if (foundIndexes[1][1] != size_t(-1))  // if two LDXs found
    {
        commentOut(foundIndexes[1][0], "removeUselessGCCCallLoads (LDX #)");  // remove first LDX
        applied = true;
    }
    return applied;
}


// Look for:
//      PSHS    Y,X,B
//      LDX/LDD __,U
//      STX/STD 3,S
//      PULS	B,X
//      JSR     [,S++]
// Replace with:
//      JSR     [__,U]
//
bool
ASMText::functionPointerGCCCall1(size_t index)
{
    if (! isInstr(index, "PSHS", "Y,X,B"))
        return false;
    size_t ldxIndex = findNextInstrBeforeLabel(index + 1);
    if (ldxIndex == size_t(-1)
            || (! isInstrAnyArg(ldxIndex, "LDX") && ! isInstrAnyArg(ldxIndex, "LDD"))
            || ! endsWith(elements[ldxIndex].fields[1], ",U"))
        return false;
    size_t stxIndex = findNextInstrBeforeLabel(ldxIndex + 1);
    if (stxIndex == size_t(-1)
            || (! isInstr(stxIndex, "STX", "3,S") && ! isInstr(stxIndex, "STD", "3,S")))
        return false;
    size_t pulsIndex = findNextInstrBeforeLabel(stxIndex + 1);
    if (pulsIndex == size_t(-1)
            || ! isInstr(pulsIndex, "PULS", "B,X"))
        return false;
    size_t jsrIndex = findNextInstrBeforeLabel(pulsIndex + 1);
    if (jsrIndex == size_t(-1)
            || ! isInstr(jsrIndex, "JSR", "[,S++]"))
        return false;
    
    // Apply.
    const char *comment = "optim: functionPointerGCCCall1";
    elements[jsrIndex].fields[1] = "[" + elements[ldxIndex].fields[1] + "]";
    elements[jsrIndex].fields[2] += string("; ") + comment;
    commentOut(index, comment);
    commentOut(ldxIndex, comment);
    commentOut(stxIndex, comment);
    commentOut(pulsIndex, comment);
    return true;
}


// Also for loor for:
//      PSHS    Y,X,B
//      LDX     __,U
//      LDD	    foo,X
//      STD     3,S
//      PULS    B,X
//      JSR     [,S++]
// Replace with this:
//      PSHS    Y,X
//      LDX     __,U
//      LDX	    foo,X
//      STX     2,S
//      PULS    X
//      JSR     [,S++]
// or this if using Y is allowed:
//      LDY     __,U
//      JSR     [foo,Y]
//
bool
ASMText::functionPointerGCCCall2(size_t index)
{
    if (! isInstr(index, "PSHS", "Y,X,B"))
        return false;
    size_t ldxIndex = findNextInstrBeforeLabel(index + 1);
    if (ldxIndex == size_t(-1)
            || ! isInstrAnyArg(ldxIndex, "LDX")
            || ! endsWith(elements[ldxIndex].fields[1], ",U"))
        return false;
    size_t lddIndex = findNextInstrBeforeLabel(ldxIndex + 1);
    if (lddIndex == size_t(-1)
            || ! isInstrAnyArg(lddIndex, "LDD")
            || ! endsWith(elements[lddIndex].fields[1], ",X"))
        return false;
    size_t stdIndex = findNextInstrBeforeLabel(lddIndex + 1);
    if (stdIndex == size_t(-1)
            || ! isInstr(stdIndex, "STD", "3,S"))
        return false;
    size_t pulsIndex = findNextInstrBeforeLabel(stdIndex + 1);
    if (pulsIndex == size_t(-1)
            || ! isInstr(pulsIndex, "PULS", "B,X"))
        return false;
    size_t jsrIndex = findNextInstrBeforeLabel(pulsIndex + 1);
    if (jsrIndex == size_t(-1)
            || ! isInstr(jsrIndex, "JSR", "[,S++]"))
        return false;
    
    // Apply.
    const char *comment = "optim: functionPointerGCCCall2";
    if (isYRegisterAllowed())
    {
        commentOut(index, comment);  // remove PSHS
        elements[ldxIndex].fields[0] = "LDY";
        string location = elements[lddIndex].fields[1];  // foo,X
        location.back() = 'Y';  // now: foo,Y
        elements[jsrIndex].fields[1] = "[" + location + "]";  // [foo,Y]
        commentOut(lddIndex, comment);
        commentOut(stdIndex, comment);
        commentOut(pulsIndex, comment);
    }
    else
    {
        elements[index].fields[1] = "Y,X";  // no need to preserve B
        elements[index].fields[2] += string("; ") + comment;

        elements[lddIndex].fields[0] = "LDX" ; // stop trashing B
        elements[lddIndex].fields[2] += string("; ") + comment;

        elements[stdIndex].fields[0] = "STX";
        elements[stdIndex].fields[1] = "2,S";  // because B not pushed anymore
        elements[stdIndex].fields[2] += string("; ") + comment;

        elements[pulsIndex].fields[1] = "X";  // do not pull B anymore
        elements[pulsIndex].fields[2] += string("; ") + comment;
    }
    return true;
}


// Look for:
//      LEAS    <foo>,S
//      LEAS    <bar>,S
// Replace with:
//      LEAS    <foo+bar>,S
// if the sum is non-zero. Remove both instructions if zero.
//
bool
ASMText::removeConsecutiveLEASInstructions(size_t index)
{
    if (! isInstrAnyArg(index, "LEAS"))
        return false;
    const size_t secondLEAS = findNextInstrBeforeLabel(index + 1);
    if (! isInstrAnyArg(secondLEAS, "LEAS"))
        return false;

    // Check that both instructions have arguments of the form __,S
    // and extract those arguments.
    if (! endsWith(elements[index].fields[1], ",S"))
        return false;
    if (! endsWith(elements[secondLEAS].fields[1], ",S"))
        return false;

    // Apply the optimization.
    int16_t firstValue  = extractNumericalPrefix(elements[index].fields[1]);
    int16_t secondValue = extractNumericalPrefix(elements[secondLEAS].fields[1]);
    int16_t sum = firstValue + secondValue;
    if (sum == 0)  // if total is zero, both instructions can be removed:
    {
        commentOut(index,      "optim: removeConsecutiveLEASInstructions");
        commentOut(secondLEAS, "optim: removeConsecutiveLEASInstructions");

    }
    else  // combine the two instructions into a single one:
    {
        elements[index].fields[1] = intToString(sum) + ",S";
        elements[index].fields[2] = "optim: removeConsecutiveLEASInstructions";
        commentOut(secondLEAS,      "optim: removeConsecutiveLEASInstructions");
    }
    return true;
}


// Look for this:
//      PSHS    B,A
//      LEAX    __,[U or Y]
//      TFR     X,D
//      ADDD    ,S++
//      TFR     D,X
// Replace with:
//      LEAX    __,[U or Y]
//      LEAX    D,X
//
bool
ASMText::tightenArrayElementAddressComputation(size_t index)
{
    if (! isInstr(index, "PSHS", "B,A"))
        return false;
    const size_t leax = findNextInstrBeforeLabel(index + 1);
    if (! isInstrAnyArg(leax, "LEAX"))
        return false;
    if (elements[leax].fields[1].find(',') == string::npos)  // check for __,R
        return false;
    const size_t tfrXD = findNextInstrBeforeLabel(leax + 1);
    if (! isInstr(tfrXD, "TFR", "X,D"))
        return false;
    const size_t addd = findNextInstrBeforeLabel(tfrXD + 1);
    if (! isInstr(addd, "ADDD", ",S++"))
        return false;
    const size_t tfrDX = findNextInstrBeforeLabel(addd + 1);
    if (! isInstr(tfrDX, "TFR", "D,X"))
        return false;
    
    // Apply the optimization.
    commentOut(index, "optim: tightenArrayElementAddressComputation");  // remove PSHS
    elements[tfrXD].fields[0] = "LEAX";  // replace TFR X,D
    elements[tfrXD].fields[1] = "D,X";
    elements[tfrXD].fields[2] = "optim: tightenArrayElementAddressComputation";
    commentOut(addd, "optim: tightenArrayElementAddressComputation");
    commentOut(tfrDX, "optim: tightenArrayElementAddressComputation");
    return true;
}


// Look for:
//      LDB     ___
//      TFR     D,X		for MUL16; implies that A is 0
//      LDB     #constant
//      LBSR    MUL16   preserves X, but only product in D is needed
// Replace with:
//      LDB     ___
//      LDA     #constant
//      MUL
//
bool
ASMText::mul16ByConstant(size_t index)
{
    if (! isInstrAnyArg(index, "LDB"))
        return false;
    const size_t tfr = findNextInstrBeforeLabel(index + 1);
    if (! isInstr(tfr, "TFR", "D,X"))
        return false;
    const size_t ldbConst = findNextInstrBeforeLabel(tfr + 1);
    if (! isInstrAnyArg(ldbConst, "LDB"))
        return false;
    if (elements[ldbConst].fields[1][0] != '#')  // if not LDB immediate
        return false;
    const size_t mul16 = findNextInstrBeforeLabel(ldbConst + 1);
    if (! isInstr(mul16, "LBSR", "MUL16"))
        return false;

    // Apply the optimization.
    commentOut(tfr, "optim: mul16ByConstant");
    elements[ldbConst].fields[0] = "LDA";  // LDB# replaced with LDA#
    elements[ldbConst].fields[2] += " optim: mul16ByConstant";
    elements[mul16].fields[0] = "MUL";  // LBSR MUL16 replaced with MUL
    elements[mul16].fields[1].clear();
    elements[mul16].fields[2] += " optim: mul16ByConstant";

    return true;
}


// Look for:
//      LDB     {foo}
//      NEGB
//      ADDB    {bar}
// Replace with:
//      LDB     {bar}
//      SUBB    {foo}
// Reject if {foo} or {bar} ends with + or contains ",-",
// in case we have LDB ,X+; NEGB; ADDB ,-X.
// We want to avoid changing the side effects.
//
bool
ASMText::negbToSubb(size_t index)
{
    if (! isInstrAnyArg(index, "LDB"))
        return false;
    if (isInstrWithPreDecrOrPostIncr(index))
        return false;
    size_t negb = findNextInstrBeforeLabel(index + 1);
    if (! isInstr(negb, "NEGB", ""))
        return false;
    size_t addb = findNextInstrBeforeLabel(negb + 1);
    if (! isInstrAnyArg(addb, "ADDB"))
        return false;
    if (isInstrWithPreDecrOrPostIncr(addb))
        return false;

    // Apply the optimization.
    const string ldbArg = elements[index].fields[1];
    elements[index].fields[1] = elements[addb].fields[1];
    elements[index].fields[2] += " (optim: negbToSubb)";
    commentOut(negb, "optim: negbToSubb");
    elements[addb].fields[0] = "SUBB";
    elements[addb].fields[1] = ldbArg;
    elements[addb].fields[2] += " (optim: negbToSubb)";

    return true;
}


// Look for:
//      LEAS    {n},S
//      LEAS    ,U
// Remove the 1st LEAS.
// 
bool
ASMText::removeUnneededLEAS(size_t index)
{
    if (! isInstrAnyArg(index, "LEAS"))
        return false;
    if (! endsWith(elements[index].fields[1], ",S"))
        return false;
    size_t leasU = findNextInstrBeforeLabel(index + 1);
    if (! isInstr(leasU, "LEAS", ",U"))
        return false;

    // Apply the optimization.
    commentOut(index, "optim: removeUnneededLEAS");
    return true;
}
