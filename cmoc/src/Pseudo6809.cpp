/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2015 Pierre Sarrazin <http://sarrazip.com/>
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

#include "Pseudo6809.h"

#include "ASMText.h"

using namespace std;


bool
Pseudo6809::process(const string &instr, const string &operand, int index, bool ignoreStackErrors)
{
  // Excluding limited stack tracking, this does not simulate memory operations.
  // It simply assumes that all memory values are unknown
  
  // Record the current state
  indexToState[index] = make_pair(regs, stack);

  // Determine whether operand is a constant literal
  int n = 0;
  const bool operandIsConstant = ASMText::extractConstantLiteral(operand, n);
  const uint16_t val16 = (operandIsConstant ? uint16_t(n) : 0);

  // Determine whether instr is a comma op that is not an indexed op
  const bool isStackOp = instr == "PSHS" || instr == "PULS" ||
                         instr == "PSHU" || instr == "PULU";
  const bool isOddCommaOp = isStackOp ||
                            instr == "TFR" || instr == "EXG";

  // Determine whether operand is indexed
  const size_t commaIndex = operand.find(",");
  const bool isIndexed = !isOddCommaOp && commaIndex != string::npos;
  const Register indexReg = isIndexed ? getRegisterFromName(operand.c_str() + commaIndex + 1) : NO_REGISTER;

  // Determine whether operand is indirect
  const bool isIndirect = operand.find("[") != string::npos;

  // Determine whether operand is post increment
  const bool postIncrement1 = isIndexed && operand.find("+") != string::npos;
  const bool postIncrement2 = isIndexed && operand.find("++") != string::npos;

  // Determine whether operand is pre decrement
  const bool preDecrement1 = isIndexed && operand.find("-") != string::npos;
  const bool preDecrement2 = isIndexed && operand.find("--") != string::npos;

  // Determine whether there is an offset
  const bool hasOffset = isIndexed && ((commaIndex > 1) || (commaIndex > 0 && !isIndirect));
  const char *offsetStr = operand.c_str();
  const Register offsetStrReg = getRegisterFromName(offsetStr);
  const bool isConstantOffset = hasOffset && !(offsetStrReg == D || offsetStrReg == A || offsetStrReg == B);
  const int offsetVal = isConstantOffset ? (offsetStr[0] == '$' ? strtol(offsetStr + 1, NULL, 16)
                                                                : (offsetStr[0] == '-' || isdigit(offsetStr[0])) ? strtol(offsetStr, NULL, 10) : 0)
                                         : 0;

  // Run basic stack ops
  if (isStackOp) {
    const Register stackReg = getRegisterFromName(instr.c_str() + 3);
    if (instr.find("PSH") == 0)
      processPush(stackReg, operand, index);
    else
      return processPull(stackReg, operand, index);
    return true;
  }

  // Tests A or B for zero
  if (instr == "TSTA" || instr == "TSTB") {
    getVal(instr[3] == 'A' ? A : B, index);
    return true;
  }

  // This instruction adds B to X
  if (instr == "ABX") {
    getVal(B, index);
    getVal(X, index);
    addVal(X, B, index);
    return true;
  }

  // These instructions have no dependencies or side effects
  if (instr == "CWAI" || instr == "SYNC" || 
      instr == "NOP" || instr.find("BRN") != string::npos) {
    return true;
  }

  // Thie instruction converts A into a decimal equivalent
  if (instr == "DAA") {
    PossiblyKnownVal<int> val = getVal(A, index);
    if (val.known && val.val <= 100)
      loadVal(A, PossiblyKnownVal<int>(((val.val / 10)<<4) + (val.val % 10), true, index), index);
    else
      loadVal(A, PossiblyKnownVal<int>(0, false, index), index);
    return true;
  }

  // Transfer and exchange registgers
  if (instr == "TFR" || instr == "EXG") {
    Register reg1 = getRegisterFromName(operand.c_str());
    Register reg2 = getRegisterFromName(operand.c_str() + commaIndex + 1);
    getVal(reg1, index);
    if (instr == "TFR") {
      tfr(reg1, reg2, index);
    } else {
      getVal(reg2, index);
      exg(reg1, reg2, index);
    }
    return true;
  }

  // This instruction multiplies AxB and puts the result in D.
  if (instr == "MUL") {
    getVal(D, index);
    if ((regs.accum.a.val == 0 && regs.accum.a.known) ||
        (regs.accum.b.val == 0 && regs.accum.b.known))
      loadVal(D, PossiblyKnownVal<uint16_t>(0, true, index), index);
    else
      loadVal(D, PossiblyKnownVal<uint16_t>(regs.accum.a.val * regs.accum.b.val,
                                              regs.accum.dknown(), index), index);
    return true;
  }

  // If B >= 0x80, make A 0xFF, otherwise make it 0x00.
  if (instr == "SEX") {
    PossiblyKnownVal<int> val = getVal(B, index);
    if (val.known)
      loadVal(A, PossiblyKnownVal<int>(val.val > 128 ? 0xff : 0, true, index), index);
    else
      loadVal(A, PossiblyKnownVal<int>(0, false, index), index);
  }

  // Don't bother with branching ops for now.
  if (instr == "JSR" || instr == "JMP" ||
      instr.find("BRA") != string::npos ||
      instr.find("BCC") != string::npos ||
      instr.find("BCS") != string::npos ||
      instr.find("BEQ") != string::npos ||
      instr.find("BGE") != string::npos ||
      instr.find("BGT") != string::npos ||
      instr.find("BHI") != string::npos ||
      instr.find("BHS") != string::npos ||
      instr.find("BLE") != string::npos ||
      instr.find("BLO") != string::npos ||
      instr.find("BLS") != string::npos ||
      instr.find("BLT") != string::npos ||
      instr.find("BLE") != string::npos ||
      instr.find("BMI") != string::npos ||
      instr.find("BNE") != string::npos ||
      instr.find("BPL") != string::npos ||
      instr.find("BSR") != string::npos ||
      instr.find("BVC") != string::npos ||
      instr.find("BVS") != string::npos ||
      instr.find("SWI") != string::npos ||
      instr == "RTS" || instr == "RTI")
      return false;

  // Try to deal with the remaining instructions as generically as possible
  // First try to figure out the target register. If blank then this is
  // a memory op.
  Register targetRegister;
  if (instr.find("OR") == 0 || instr.find("LD") == 0 || instr.find("ST") == 0)
    targetRegister = getRegisterFromName(instr.c_str() + 2);
  else
    targetRegister = getRegisterFromName(instr.c_str() + 3);

  // All instructions except LEA and LD reference targetRegister
  PossiblyKnownVal<int> lhs;
  if (! (instr.find("LD") == 0 || instr.find("LEA"))) {
    lhs = getVal(targetRegister, index);
  }

  // The RHS may be constant (immediate), indexed (possibly constant),
  // indirect indexed, direct, extended or indirect extended.
  // Make sure we ascertain the RHS as well as possible
  PossiblyKnownVal<int> rhs;
  if (operandIsConstant) {
    rhs = PossiblyKnownVal<int>(val16, true, index);
  } else if (isIndexed) {
    // Get the register value of the RHS and prep the value that the
    // rhs will point to
    rhs = getVal(indexReg, index);
    PossiblyKnownVal<int> indexVal = PossiblyKnownVal<int>(0, false, index);

    // Deal with pre decrement
    if (preDecrement1) {
      rhs = rhs - 1;
      if (indexReg == S)
        return false;
      if (preDecrement2) {
        rhs = rhs - 1;
      }
      loadVal(indexReg, rhs, index);
    }

    // Deal with post increment
    if (!preDecrement1 && !isConstantOffset) {
      if (postIncrement1)
        rhs = rhs + 1;
      if (postIncrement2) 
        rhs = rhs + 1;
      if (postIncrement1) {
        loadVal(indexReg, rhs, index);
      } else {
        getVal(indexReg, index);
      }

      if (indexReg == S) {
        if (stack.size() < size_t((postIncrement2) ? 2 : 1)) {
          if (!ignoreStackErrors)
            return false;
        }
        else if (postIncrement2)
          indexVal = pull16(index);
        else if (postIncrement1)
          indexVal = pull8(index);
        else {
          if (stack.size() < size_t(regIs16Bit(targetRegister) ? 2 : 1)) {
            if (!ignoreStackErrors)
              return false;
          }
          indexVal = regIs16Bit(targetRegister) ? peek16(index) : peek8(index);
        }
      } else if (instr.find("LEA") != 0) {
        rhs = PossiblyKnownVal<int>(0, false, index);
      }
    }

    // Deal with constant offsets
    if (isConstantOffset)
      rhs = rhs + offsetVal;
    else if (hasOffset)
      rhs = rhs + getVal(offsetStrReg, index);

    // Instructions that are not LEA, load from memory
    if (instr.find("LEA") != 0)
      rhs = indexVal;

    // Indirect instructions load from memory
    rhs.known = rhs.known && !isIndirect;
  } else { // direct, extended, possibly being indirect
    rhs = PossiblyKnownVal<int>(0, false, index);
  }

  // Short cut - if there is no register than make the lhs = rhs.
  // This makes implementing instruction support easier.
  if (targetRegister == NO_REGISTER) {
    lhs = rhs;
  }

  // ADD with carry. We don't track CC, so don't bother
  if (instr.find("ADC") == 0) {
    loadVal(targetRegister, PossiblyKnownVal<int>(0, false, index), index);
    return true;
  }

  // Performs an add
  if (instr.find("ADD") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs + rhs, index);
    return true;
  }

  // ANDs register
  if (instr.find("AND") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs & rhs, index);
    return true;
  }

  // Shifts bits left, don't bother with CC bits for now
  if (instr.find("ASL") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs.asl(), index);
    return true;
  }

  // Shifts bits right, don't bother with CC bits for now
  if (instr.find("ASR") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs.asr(), index);
    return true;
  }

  // Clears the registers or memory
  if (instr.find("CLR") == 0) {
    loadVal(targetRegister, PossiblyKnownVal<int>(0, true, index), index);
    return true;
  }

  // Compares registers or memory.
  if (instr.find("CMP") == 0) {
    getVal(targetRegister, index);
    return true;
  }

  // Complements memory or register
  if (instr.find("COM") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, ~lhs, index);
    return true;
  }

  // Decrements memory or register
  if (instr.find("DEC") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs - 1, index);
    return true;
  }

  // Exclusive OR a register
  if (instr.find("EOR") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs ^ rhs, index);
    return true;
  }

  // Increments memory or register
  if (instr.find("INC") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs + 1, index);
    return true;
  }

  // Perform a load
  if (instr.find("LD") == 0) {
    if (operandIsConstant)
      loadVal(targetRegister, rhs, index);
    else
      loadVal(targetRegister, PossiblyKnownVal<int>(0, false, index), index);
    return true;
  }

  // Shifts bits left, don't bother with CC bits for now
  if (instr.find("LSL") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs.lsl(), index);
    return true;
  }

  // Shifts bits right, don't bother with CC bits for now
  if (instr.find("LSR") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs.lsr(), index);
    return true;
  }

  // Perform an LEA
  if (instr.find("LEA") == 0) {
    // Deal with the S register specially
    if (targetRegister == S) {
      // Avoid voodoo magic
      if (indexReg != S)
        return false;

      // Try to deal with constant offsets here
      if (offsetVal < 0) {
        for(int ii=0; ii<-offsetVal; ii++)
          push8(PossiblyKnownVal<int>(0, false, index));
      } else {
        for(int ii=0; ii<offsetVal; ii++) {
          if (stack.empty())
            return false;
          stack.pop();
        }
      }
    } else {
      loadVal(targetRegister, rhs, index);
    }
    return true;
  }

  // Negates the given value
  if (instr.find("NEG") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, -lhs, index);
    return true;
  }

  // ORs register
  if (instr.find("OR") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs | rhs, index);
    return true;
  }

  // Can't do much here without CC bits
  if (instr.find("ROR") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, PossiblyKnownVal<int>(0, false,index), index);
    return true;
  }

  // Can't do much here without CC bits
  if (instr.find("ROL") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, PossiblyKnownVal<int>(0, false,index), index);
    return true;
  }

  // Can't do much here without CC bits
  if (instr.find("SBC") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, PossiblyKnownVal<int>(0, false,index), index);
    return true;
  }

  // The main thing here is that we referenced the registers
  if (instr.find("ST") == 0) {
    getVal(targetRegister, index);
    return true;
  }

  // Performs an sub
  if (instr.find("SUB") == 0) {
    getVal(targetRegister, index);
    loadVal(targetRegister, lhs - rhs, index);
    return true;
  }

  // Compares registers or memory. Don't bother for now
  if (instr.find("TST") == 0) {
    getVal(targetRegister, index);
    return true;
  }

  // Some unknown weirdness
  return false;
}


void
Pseudo6809::processPush(Register stackReg, const string &operand, int index)
{
  const bool isS = stackReg == S;
  if (operand.find("PC") != string::npos) {
    if (isS) push16(getVal(PC, index));
    addVal(stackReg, -2, index);
  }
  if (operand.find("U") != string::npos) {
    if (isS) push16(getVal(U, index));
    addVal(stackReg, -2, index);
  }
  if (operand.find("S") != string::npos) {
    if (isS) push16(getVal(S, index));
    addVal(stackReg, -2, index);
  }
  if (operand.find("Y") != string::npos) {
    if (isS) push16(getVal(Y, index));
    addVal(stackReg, -2, index);
  }
  if (operand.find("X") != string::npos) {
    if (isS) push16(getVal(X, index));
    addVal(stackReg, -2, index);
  }
  if (operand.find("DP") != string::npos) {
    if (isS) push8(getVal(DP, index));
    addVal(stackReg, -1, index);
  }
  if (operand.find("B") != string::npos) {
    if (isS)  {
      push8(getVal(B, index));
    }
    addVal(stackReg, -1, index);
  }
  if (operand.find("A") != string::npos) {
    if (isS) {
      push8(getVal(A, index));
    }
    addVal(stackReg, -1, index);
  }
  if (operand.find("CC") != string::npos) {
    if (isS) push8(getVal(CC, index));
    addVal(stackReg, -1, index);
  }
}

bool
Pseudo6809::processPull(Register stackReg, const string &operand, int index)
{
  const bool isS = stackReg == S;

  if (operand.find("CC") != string::npos) {
    if (stack.empty()) return false;
    regs.setVal(CC, isS ? pull8(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 1, index);
  }
  if (operand.find("A") != string::npos) {
    if (stack.empty()) return false;
    regs.setVal(A, isS ? pull8(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 1, index);
  }
  if (operand.find("B") != string::npos) {
    if (stack.empty()) return false;
    regs.setVal(B, isS ? pull8(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 1, index);
  }
  if (operand.find("DP") != string::npos) {
    if (stack.empty()) return false;
    regs.setVal(DP, isS ? pull8(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 1, index);
  }
  if (operand.find("X") != string::npos) {
    if (stack.size() < 2) return false;
    regs.setVal(X, isS ? pull16(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 2, index);
  }
  if (operand.find("S") != string::npos) {
    if (stack.size() < 2) return false;
    regs.setVal(S, isS ? pull16(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 2, index);
  }
  if (operand.find("Y") != string::npos) {
    if (stack.size() < 2) return false;
    regs.setVal(Y, isS ? pull16(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 2, index);
  }
  if (operand.find("U") != string::npos) {
    if (stack.size() < 2) return false;
    regs.setVal(U, isS ? pull16(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 2, index);
  }
  if (operand.find("PC") != string::npos) {
    if (stack.size() < 2) return false;
    regs.setVal(PC, isS ? pull16(index) : PossiblyKnownVal<int>(0, false));
    addVal(stackReg, 2, index);
  }

  return true;
}


int
Pseudo6809::numBytesPushedOrPulled(const string &operand) {
  int sum = 0;
  if (operand.find("CC") != string::npos) sum++;
  if (operand.find("A") != string::npos) sum++;
  if (operand.find("B") != string::npos) sum++;
  if (operand.find("DP") != string::npos) sum++;
  if (operand.find("X") != string::npos) sum += 2;
  if (operand.find("S") != string::npos) sum += 2;
  if (operand.find("Y") != string::npos) sum += 2;
  if (operand.find("U") != string::npos) sum += 2;
  if (operand.find("PC") != string::npos) sum += 2;
  return sum;
}
