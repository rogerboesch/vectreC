/*  $Id: UnaryOpExpr.h,v 1.12 2017/07/25 01:21:53 sarrazip Exp $

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

#ifndef _H_UnaryOpExpr
#define _H_UnaryOpExpr

#include "Tree.h"

class VariableExpr;


class UnaryOpExpr : public Tree
{
public:

    enum Op
    {
        IDENTITY, NEG,
        POSTINC, POSTDEC, PREINC, PREDEC,
        ADDRESS_OF, INDIRECTION,
        BOOLEAN_NEG, BITWISE_NOT,
        SIZE_OF
    };

    static const char *getOperatorName(Op op);

    UnaryOpExpr(Op op, Tree *e);

    UnaryOpExpr(const TypeDesc *_typeDesc);  // sizeof(type) (sizeof(expr) uses preceding ctor)

    virtual ~UnaryOpExpr();

    Op getOperator() const;

    const Tree *getSubExpr() const;

    // Returns null in the case of the SIZE_OF operator.
    Tree *getSubExpr();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    // Determines the type of the sizeof argument if it has not already been determined.
    // Can be called more than once.
    //
    void setSizeofArgTypeDesc();

    // If this is a sizeof(type), emits an error message if 'type' is an unknown struct.
    //
    void checkForSizeOfUnknownStruct();

    // If successful, 'size' receives the size in bytes of the sizeof() argument,
    // and the function returns true.
    // Otherwise, the function returns false.
    //
    bool getSizeOfValue(uint16_t &size) const;

    CodeStatus emitSimplerIfIncrement(ASMText &out);

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild);

    void allowDereferencingVoid() { dereferencingVoidAllowed = true; }

    virtual bool isLValue() const { return oper == INDIRECTION || oper == PREINC || oper == POSTINC || oper == POSTINC || oper == POSTDEC; }

private:

    static const VariableExpr *isPostIncOfPtrToSmallType(const Tree &tree);

    // Forbidden:
    UnaryOpExpr(const UnaryOpExpr &);
    UnaryOpExpr &operator = (const UnaryOpExpr &);

private:

    Op oper;
    bool dereferencingVoidAllowed;  // applies to INDIRECTION only
    Tree *subExpr;  // owns the Tree object (not used by sizeof(type) operator)
    const TypeDesc *sizeofArgTypeDesc;   // used by sizeof operator
    class Declaration *resultDeclaration;  // used when result is real number

};


#endif  /* _H_UnaryOpExpr */
