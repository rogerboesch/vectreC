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

#ifndef _H_Tree
#define _H_Tree

#include "util.h"
#include "CodeStatus.h"
#include "ASMText.h"

class Scope;
class VariableExpr;


class Tree
{
public:

    virtual ~Tree();

    void setScope(Scope *s);
    const Scope *getScope() const;
    Scope *getScope();
    void pushScopeIfExists() const;
    void popScopeIfExists() const;

    class Functor
    {
    public:
        virtual ~Functor() {}
        virtual bool open(Tree *)  { return true; }
        virtual bool close(Tree *) { return true; }
    };

    virtual bool iterate(Functor &f);

    // Tells an object to replace a direct child with 'newChild'
    // and to call delete on the replaced child.
    // newChild: Must come from 'new'.
    //
    virtual void replaceChild(Tree * /*existingChild*/, Tree * /*newChild*/) {}

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    // Does not keep a copy of the given pointer.
    // no: Source file line number (>= 1).
    //
    void setLineNo(const char *srcFilename, int no);

    void setLineNo(const std::string &srcFilename, int no);

    std::string getLineNo() const;
    void setIntLineNo(int no) { lineno = no; }
    int getIntLineNo() const { return lineno; }
    void copyLineNo(const Tree &tree);

    void writeLineNoComment(ASMText &out, const std::string &text = "") const;

    const TypeDesc *getTypeDesc() const;
    BasicType getType() const;
    int16_t getTypeSize() const;
    int16_t getPointedTypeSize() const;
    const TypeDesc *getFinalArrayElementType() const;
    int16_t getFinalArrayElementTypeSize() const;
    bool isSigned() const { assert(typeDesc); return typeDesc->isSigned; }
    bool isUnsignedOrPositiveConst() const;
    bool isNumerical() const { assert(typeDesc); return typeDesc->isNumerical(); }
    bool isIntegral() const { assert(typeDesc); return typeDesc->isIntegral(); }
    bool isReal() const { assert(typeDesc); return typeDesc->isReal(); }
    bool isSingle() const { assert(typeDesc); return typeDesc->isSingle(); }
    bool isDouble() const { assert(typeDesc); return typeDesc->isDouble(); }
    bool isLong() const { assert(typeDesc); return typeDesc->isLong(); }
    bool isRealOrLong() const { assert(typeDesc); return isReal() || isLong(); }
    bool isPtrOrArray() const { assert(typeDesc); return typeDesc->isPtrOrArray(); }
    const char *getConvToWordIns() const { return typeDesc->getConvToWordIns(); }
    const char *getLoadIns() const  { return getType() == BYTE_TYPE ? "LDB" : "LDD"; }
    const char *getStoreIns() const { return getType() == BYTE_TYPE ? "STB" : "STD"; }

    /** Returns an empty string if there is no class name.
    */
    const std::string &getClassName() const;

    void setTypeDesc(const TypeDesc *td);
    void setTypeToPointedType(const Tree &tree);
    void setPointerType(const Tree &treeOfPointedType);

    // Evaluates this tree and if it is a constant expression, stores the value
    // in 'result' and returns true.
    // Returns false otherwise, including if there is a division by zero or other
    // invalid operation.
    // A string literal is not considered constant: because the program is assumed
    // to be relocatable, the address of the character array is not predictable.
    //
    // The ExpressionTypeSettings must already have been run on this tree.
    //
    bool evaluateConstantExpr(uint16_t &result) const;

    // True if this tree is a long literal, a real literal, or if evaluateConstantExpr() succeeds.
    //
    bool isNumericalLiteral() const;

    bool isNumericalLiteralCastToOtherType() const;

    bool isStringLiteralCastToPtrToAnyChar() const;

    // Returns true iff this tree is a constant according to evaluateConstantExpr()
    // and this constant fits in a byte (even is the tree's type is not BYTE_TYPE).
    // valuePtr: If not null, AND the tree is a constant, then *valuePtr receives this constant.
    //
    bool is8BitConstant(uint16_t *valuePtr = NULL) const;

    bool fits8Bits() const;

    // Indicates if this expression fits 8 bits while taking the signedness into account.
    // For example, if this tree is a WordConstantExpr that represents 128..255, then this value
    // is a signed int. A value in 128..255 fits 8 bits, but not a signed 8-bit type, so false is returned.
    //
    bool fits8BitsWithSignedness() const;

    bool isExpressionAlwaysTrue() const;
    bool isExpressionAlwaysFalse() const;

    // Calls out.ins("LBSR", utilitySubRoutine, comment, "") and remembers that
    // the body of the named sub-routine will be needed at assembly time.
    //
    static void callUtility(ASMText &out, const std::string &utilitySubRoutine, const std::string &comment = "");

    void errormsg(const char *fmt, ...) const;
    void warnmsg(const char *fmt, ...) const;

    // isError: If true, equivalent to errormsg(). If false, equivalent to warnmsg().
    //
    void diagnose(bool isError, const char *fmt, ...) const;

    // Issues the error message on optionalTree if not null, otherwise uses
    // globals sourceFilename and lineno.
    //
    static void errormsg(const Tree *optionalTree, const char *fmt, ...);

    // Issues the warning message on optionalTree if not null, otherwise uses
    // globals sourceFilename and lineno.
    //
    static void warnmsg(const Tree *optionalTree, const char *fmt, ...);

    // In general, this should be the only way to cast a Tree * to VariableExpr *,
    // because it checks for the case where the Tree is an IdentifierExpr that
    // contains a VariableExpr.
    //
    const VariableExpr *asVariableExpr() const;

    // Indicates if this tree represents an expression that has an address in memory.
    //
    virtual bool isLValue() const = 0;

    // Returns true iff the l-value to initialize (of type varTypeDesc) is char[][], or char[][][], etc.,
    // and this tree only contains an array of string literals, or an array of arrays of string literals, etc.
    // varTypeDesc: Type of the l-value to be initialized.
    //
    bool definesOnlyAMatrixOfCharsAndHasInitializer(const TypeDesc &varTypeDesc) const;

    // Returns true if this tree only contains integer values, and no string literals.
    // Also returns true if this tree is a string literal and varTypeDesc is an array of characters.
    // varTypeDesc: Type of the l-value to be initialized.
    //
    bool isArrayWithOnlyNumericalLiteralInitValues(const TypeDesc &varTypeDesc) const;

    // Returns true only if varTypeDesc is a struct or union this tree only contains integer values,
    // and no string literals.
    //
    bool isStructWithOnlyNumericalLiteralInitValues(const TypeDesc &varTypeDesc) const;

    // Determines if this tree represents an expression or sequence of expressions
    // that can be initialized statically, i.e., without running any code.
    // varTypeDesc: Type of the l-value to be initialized.
    // allowStringLiterals: By default, a StringLiteralExpr in this tree causes this method to return false.
    //                      Passing true for this parameter makes this method accept a StringLiteralExpr.
    //
    bool isStaticallyInitializable(const TypeDesc &varTypeDesc, bool allowStringLiterals = false) const;

    // Indicates that the final value of the expression represented by this tree does not need
    // to be left in the B or D register.
    //
    void setYieldedValueNeeded(bool needed);

    bool isYieldedValueNeeded() const;

protected:

    Tree();  // void type
    Tree(const TypeDesc *td);

    static bool deleteAndAssign(Tree *&member, Tree *oldAddr, Tree *newAddr);

private:

    uint16_t evaluateConstantExpr() const;  // may throw int
    bool isCastToMultiByteType() const;
    bool isExpressionAlwaysTrueOrFalse(bool boolToReturnIfZero) const;

    // Forbidden:
    Tree(const Tree &);
    Tree &operator = (const Tree &);

private:

    Scope *scope;  // may be null; does not own the object
    std::string sourceFilename;  // valid only when lineno >= 1
    int lineno;  // valid only when >= 1
    const TypeDesc *typeDesc;
    bool yieldedValueNeeded;

};


#endif  /* _H_Tree */
