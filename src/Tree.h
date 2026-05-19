/*  $Id: Tree.h,v 1.30 2019/08/15 00:32:25 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2018 Pierre Sarrazin <http://sarrazip.com/>

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
    bool isConst() const { assert(typeDesc); return typeDesc->isConstant(); }
    bool isPtrToOrArrayOfConst() const { assert(typeDesc); return typeDesc->isPtrOrArray() && typeDesc->getPointedTypeDesc()->isConstant(); }
    const char *getConvToWordIns() const { return isSigned() ? "SEX" : "CLRA"; }
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

    // Returns true iff this tree is a constant according to evaluateConstantExpr()
    // and this constant fits in a byte (even is the tree's type is not BYTE_TYPE).
    //
    bool is8BitConstant() const;

    bool fits8Bits() const { return getType() == BYTE_TYPE || is8BitConstant(); }

    bool isExpressionAlwaysTrue() const;
    bool isExpressionAlwaysFalse() const;

    // Calls out.ins("LBSR", utilitySubRoutine, comment, "") and remembers that
    // the body of the named sub-routine will be needed at assembly time.
    //
    static void callUtility(ASMText &out, const std::string &utilitySubRoutine, const std::string &comment = "");

    void errormsg(const char *fmt, ...) const;
    void warnmsg(const char *fmt, ...) const;

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

protected:

    Tree();  // void type
    Tree(const TypeDesc *td);

    static bool deleteAndAssign(Tree *&member, Tree *oldAddr, Tree *newAddr);

private:

    uint16_t evaluateConstantExpr() const;  // may throw int
    bool isCastToMultiByteType() const;

    // Forbidden:
    Tree(const Tree &);
    Tree &operator = (const Tree &);

private:

    Scope *scope;  // may be null; does not own the object
    std::string sourceFilename;  // valid only when lineno >= 1
    int lineno;  // valid only when >= 1
    const TypeDesc *typeDesc;

};


#endif  /* _H_Tree */
