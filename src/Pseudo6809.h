/*  $Id: Pseudo6809.h,v 1.3 2017/03/05 19:54:23 sarrazip Exp $

    CMOC - A C-like cross-compiler
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

#ifndef _H_Pseudo6809
#define _H_Pseudo6809

#include "util.h"

#include <stack>


/** represent a value that may or may not be known */
template <typename T> struct PossiblyKnownVal
{
    template <typename T1>
    PossiblyKnownVal(const PossiblyKnownVal<T1> &v)
        : index(v.index), index2(-1), val((T)v.val), known(v.known) {}

    template <typename T1>
    PossiblyKnownVal(const PossiblyKnownVal<T1> &v, int idx)
        : index(idx), index2(-1), val((T)v.val), known(v.known) {}

    template <typename T1>
    PossiblyKnownVal(const PossiblyKnownVal<T1> &v1, const PossiblyKnownVal<T1> &v2)
        : index(v1.index), index2(v1.index),
          val((T)((((int)v1.val) << 8) | v2.val)),
          known(v1.known && v2.known) {}

    PossiblyKnownVal() : index(-1), index2(-1), val(0), known(false) {}

    PossiblyKnownVal(T startval, bool isKnown=true, int idx1=-1)
        : index(idx1), index2(-1), val(startval), known(isKnown) {}

    PossiblyKnownVal<T> asl() const
    {
        return PossiblyKnownVal<T>((0xff & (val << 1)), known, index);
    }

    PossiblyKnownVal<T> asr() const
    {
        return PossiblyKnownVal<T>((val & 0x80) | (0xff & (val >> 1)), known, index);
    }

    PossiblyKnownVal<T> lsl() const
    {
        return PossiblyKnownVal<T>((0xff & (val << 1)), known, index);
    }

    PossiblyKnownVal<T> lsr() const
    {
        return PossiblyKnownVal<T>((0xff & (val >> 1)), known, index);
    }

    int index;
    int index2;
    T val;
    bool known;
};

template <typename T>
bool operator==(const PossiblyKnownVal<T> &a, const PossiblyKnownVal<T> &b) {
  return a.known == b.known
      && a.val == b.val
      && a.index == b.index
      && a.index2 == b.index2;
}

template <typename T>
bool operator!=(const PossiblyKnownVal<T> &a, const PossiblyKnownVal<T> &b) {
  return a.known != b.known
      || a.val != b.val
      || a.index != b.index
      || a.index2 != b.index2;
}


template<typename T> PossiblyKnownVal<T> operator+(const PossiblyKnownVal<T> &v1, const PossiblyKnownVal<T> &v2) {
  return PossiblyKnownVal<T>(v1.val + v2.val, v1.known && v2.known, v1.index == v2.index ? v1.index : -1);
}

template<typename T> PossiblyKnownVal<T> operator+(const PossiblyKnownVal<T> &v1, const T &v2) {
  return PossiblyKnownVal<T>(v1.val + v2, v1.known, v1.index);
}

template<typename T> PossiblyKnownVal<T> operator-(const PossiblyKnownVal<T> &v1, const PossiblyKnownVal<T> &v2) {
  return PossiblyKnownVal<T>(v1.val - v2.val, v1.known && v2.known, v1.index == v2.index ? v1.index : -1);
}

template<typename T> PossiblyKnownVal<T> operator-(const PossiblyKnownVal<T> &v1, const T &v2) {
  return PossiblyKnownVal<T>(v1.val - v2, v1.known, v1.index);
}

template<typename T> PossiblyKnownVal<T> operator&(const PossiblyKnownVal<T> &v1, const PossiblyKnownVal<T> &v2) {
  return PossiblyKnownVal<T>(v1.val & v2.val,
                            (v1.val == 0 && v1.known) || (v2.val == 0 && v2.known),
                            v1.index == v2.index ? v1.index : -1);
}

template<typename T> PossiblyKnownVal<T> operator|(const PossiblyKnownVal<T> &v1, const PossiblyKnownVal<T> &v2) {
  return PossiblyKnownVal<T>(v1.val | v2.val,
                            (v1.val == 0xff && v1.known) || (v2.val == 0xff && v2.known),
                            v1.index == v2.index ? v1.index : -1);
}

template<typename T> PossiblyKnownVal<T> operator^(const PossiblyKnownVal<T> &v1, const PossiblyKnownVal<T> &v2) {
  return PossiblyKnownVal<T>(v1.val ^ v2.val, v1.known && v2.known, v1.index == v2.index ? v1.index : -1);
}


template<typename T> PossiblyKnownVal<T> operator~(const PossiblyKnownVal<T> &v1) {
  return PossiblyKnownVal<T>(~v1.val, v1.known, v1.index);
}

template<typename T> PossiblyKnownVal<T> operator-(const PossiblyKnownVal<T> &v1) {
  return PossiblyKnownVal<T>(-v1.val, v1.known, v1.index);
}


/** represent the 6809 d register */
struct dreg_t {
  PossiblyKnownVal<uint8_t> a;
  PossiblyKnownVal<uint8_t> b;

  dreg_t() : a(), b() {}

  /** @return current d value */
  uint16_t dval() const { return ((uint16_t)a.val) << 8 | b.val; }

  /** sets the d register */
  void setdval(const PossiblyKnownVal<uint16_t> &val) {
    a.val = (uint8_t)(val.val >> 8);
    b.val = (uint8_t)(val.val & 0xff);
    a.known = b.known = val.known;
    a.index = b.index = val.index;
  }

  /** @return whether or not the d register is known */
  bool dknown() const { return a.known && b.known; }
};


/** represent the full see of 6809 registers */
struct Pseudo6809Registers {

  Pseudo6809Registers() : accum(), dp(), cc(), x(), y(), u(), s(), pc() {}

  /** reset all registers to the unknown state */
  void reset() {
    accum.a.known = accum.b.known = dp.known = cc.known
      = x.known = y.known = u.known = s.known = false;
  }

  /** @return the value for the given register */
  PossiblyKnownVal<int> getVal(Register reg) const
  {
    switch (reg)
    {
    case A:
        return accum.a;
    case B:
        return accum.b;
    case D:
        return PossiblyKnownVal<int>(accum.a, accum.b);
    case X:
        return x;
    case Y:
        return y;
    case S:
        return s;
    case DP:
        return dp;
    case PC:
        return pc;
    case CC:
        return cc;
    default:
        return PossiblyKnownVal<int>(0, false);
    }
  }

  /** sets the given register value */
  void setVal(Register reg, const PossiblyKnownVal<int> &val)
  {
    switch (reg)
    {
    case A:
        accum.a = val; break;
    case B:
        accum.b = val; break;
    case D:
        accum.setdval(val); break;
    case X:
        x = val; break;
    case Y:
        y = val; break;
    case U:
        u = val; break;
    case S:
        s = val; break;
    case DP:
        dp = val; break;
    case PC:
        pc = val; break;
    case CC:
        cc = val; break;
    default:
        break;
    }
  }

  template<typename T>
  void loadVal(Register reg, const PossiblyKnownVal<T> &val, int index)
  {
      setVal(reg, PossiblyKnownVal<int>(val, index));
  }

  /** @return mask of known registers */
  uint8_t knownRegisters() const {
    uint8_t mask = 0;
    mask |= accum.a.known ? A  : 0;
    mask |= accum.b.known ? B  : 0;
    mask |= dp.known      ? DP : 0;
    mask |= cc.known      ? CC : 0;
    mask |= x.known       ? X  : 0;
    mask |= y.known       ? Y  : 0;
    mask |= u.known       ? U  : 0;
    mask |= pc.known      ? PC : 0;
    return mask;
  }

  dreg_t accum;
  PossiblyKnownVal<uint8_t> dp, cc;
  PossiblyKnownVal<uint16_t> x, y, u, s, pc;
};


/** Represents the state of the processor at a given point in time */
typedef std::pair<Pseudo6809Registers, std::stack<PossiblyKnownVal<uint8_t> > > Pseudo6809State;


/**
 * A very simple 6809 simulator that keeps track of known register
 * values and known values on the stack.
 */
class Pseudo6809 {
public:
  Pseudo6809()
  : stack(), regs(), indexToReferences(), indexToConstantVals(), indexToState(), pushedConstant(false)
  {
  }

  /** @return the value of the given register, updating its reference state */
  PossiblyKnownVal<int> getVal(Register reg, int index)
  {
    if (reg == D)
    {
        getVal(A, index);
        getVal(B, index);
        return regs.getVal(reg);
    }
    PossiblyKnownVal<int> val = regs.getVal(reg);
    refVal(val, index);
    return val;
  }

  /** reference a value */
  template<typename N>
  void refVal(const PossiblyKnownVal<N> &val, int index) {
    std::vector<int> &refs = indexToReferences[val.index];
    if (find(refs.begin(), refs.end(), index) == refs.end())
      refs.push_back(index);
    std::vector<int> &refs2 = indexToReferences[val.index2];
    if (find(refs2.begin(), refs2.end(), index) == refs2.end())
      refs2.push_back(index);
  }

  /** Loads the register with the given value. */
  void loadVal(Register reg, const PossiblyKnownVal<int> &val, int index) {
    regs.loadVal(reg, val, index);
    if (!val.known)
      return;
    std::vector<PossiblyKnownVal<int> > &vals = indexToConstantVals[val.index];
    vals.push_back(val);
  }

  void addVal(Register reg1, Register reg2, int index)
  {
    loadVal(reg1, getVal(reg1, index) + getVal(reg2, index), index);
  }

  template<typename T>
  void addVal(Register reg, const PossiblyKnownVal<T> &val, int index)
  {
    loadVal(reg, getVal(reg, index) + val, index);
  }

  void addVal(Register reg, int val, int index)
  {
    loadVal(reg, getVal(reg, index) + val, index);
  }

  void exg(Register reg1, Register reg2, int index)
  {
    loadVal(reg1, getVal(reg1, index) - getVal(reg2, index), index);
  }

  void tfr(Register reg1, Register reg2, int index)
  {
    loadVal(reg2, getVal(reg1, index), index);
  }

  /**
   * Processes the given instruction.
   * @return false if it does not know how to deal with the instruction.
   */
  bool process(const std::string &instr, const std::string &operand, int index,
               bool ignoreStackErrors = false);

  /** processes a push instruction */
  void processPush(Register stackReg, const std::string &operand, int index);

  /** processes a pull instruction */
  bool processPull(Register stackReg, const std::string &operand, int index);

  /** @return number of pushed/pulled bytes */
  int numBytesPushedOrPulled(const std::string &operand);

  /** reset the processor to all registers unknown and an empty stack */
  void reset() {
    regs.reset();
    indexToReferences.clear();
    while(stack.size() > 0)
      stack.pop();
  }

  /** peeks at top 16-bit value on the stack */
  PossiblyKnownVal<int> peek16(int index) {
    PossiblyKnownVal<int> val = pull16(index);
    stack.push(PossiblyKnownVal<uint8_t>((uint8_t)(val.val & 0xff), val.known, val.index));
    stack.push(PossiblyKnownVal<uint8_t>((uint8_t)((val.val >> 8) & 0xff), val.known, val.index));
    return val;
  }

  /** peeks at top 8-bit value on the stack */
  PossiblyKnownVal<int> peek8(int index) {
    PossiblyKnownVal<int> val = pull8(index);
    stack.push(val);
    return val;
  }

  /** pushes a 16 bit value to the stack */
  void push16(const PossiblyKnownVal<int> &val) {
    stack.push(PossiblyKnownVal<uint8_t>((uint8_t)(val.val & 0xff), val.known, val.index));
    pushedConstant |= val.known;
    stack.push(PossiblyKnownVal<uint8_t>((uint8_t)((val.val >> 8) & 0xff), val.known, val.index));
    pushedConstant |= val.known;
  }

  /** pop a 16-bit value from the stack */
  PossiblyKnownVal<int> pull16(int index) {
    PossiblyKnownVal<uint8_t> a = stack.top();
    refVal(a, index);
    stack.pop();
    PossiblyKnownVal<uint8_t> b = stack.top();
    refVal(b, index);
    stack.pop();
    return PossiblyKnownVal<int>((((int)a.val) << 8) | b.val, a.known && b.known,
                                 a.index == b.index ? a.index : -1);
  }

  /** pushes an 8 bit value to the stack */
  void push8(const PossiblyKnownVal<int> &val) {
    pushedConstant |= val.known;
    stack.push(val);
  }

  /** returns whether or not reg is 16-bit */
  bool regIs16Bit(Register reg) const
  {
      switch (reg)
      {
      case X:
      case Y:
      case D:
      case U:
      case S:
      case PC:
          return true;
      default:
          return false;
      }
  }

  /** pop a 16-bit value from the stack */
  PossiblyKnownVal<int> pull8(int index) {
    PossiblyKnownVal<uint8_t> a = stack.top();
    refVal(a, index);
    stack.pop();
    return a;
  }

  std::stack<PossiblyKnownVal<uint8_t> > stack;
  Pseudo6809Registers regs;

  /** maps index of an instruction that loads a value to a vector
   *  that contains the indices of instructions that reference that
   *  value */
  std::map<int, std::vector<int> > indexToReferences;

  /** maps index of an instrction that loads a value to a vector that
   *  that contains all the values generated by that instruction */
  std::map<int, std::vector<PossiblyKnownVal<int> > > indexToConstantVals;

  /** maps instruction index to the system state before it was run. */
  std::map<int, Pseudo6809State> indexToState;

  /** true iff one or more constants were pushed on the stack */
  bool pushedConstant;
};


#endif  /* _H_ASMText */
