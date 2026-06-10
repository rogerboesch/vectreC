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

#ifndef _H_CastExpr
#define _H_CastExpr

#include "Tree.h"


// Type coercion expression.
// The TypeDesc of this object is the type to which the sub-expression is cast.
// Example: For (int) x, the type of this tree is int, and getSubExpr() returns the tree for 'x'.
//
class CastExpr : public Tree
{
public:

    CastExpr(const TypeDesc *td, Tree *e);

    virtual ~CastExpr();

    const Tree *getSubExpr() const;

    Tree *getSubExpr();

    virtual void checkSemantics(Functor &f) override;

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const override;

    // The emitted code must be in B or D.
    // tree: Should designate the statement or expression that is emitting the cast.
    //       Used when an error or warning message must be issued.
    //
    static CodeStatus emitCastCode(ASMText &out, const TypeDesc *castTD, const TypeDesc *subTD, const Tree &tree);

    virtual bool iterate(Functor &f) override;

    virtual void replaceChild(Tree *existingChild, Tree *newChild) override
    {
        if (deleteAndAssign(subExpr, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    virtual bool isLValue() const override { return false; }

    static bool isZeroCastToVoidPointer(const Tree &tree);

private:

    bool emitCodeIfShiftingLongRight16Bits(ASMText &out, bool lValue) const;

    // Forbidden:
    CastExpr(const CastExpr &);
    CastExpr &operator = (const CastExpr &);

private:

    Tree *subExpr;  // owns the Tree object
    class Declaration *resultDeclaration;  // used when result is real number

};


#endif  /* _H_CastExpr */
