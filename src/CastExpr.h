/*  $Id: CastExpr.h,v 1.9 2018/09/22 05:46:35 sarrazip Exp $

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

#ifndef _H_CastExpr
#define _H_CastExpr

#include "Tree.h"


class CastExpr : public Tree
{
public:

    CastExpr(const TypeDesc *td, Tree *e);

    virtual ~CastExpr();

    const Tree *getSubExpr() const;

    Tree *getSubExpr();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    static CodeStatus emitCastCode(ASMText &out, const TypeDesc *castTD, const TypeDesc *subTD);

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(subExpr, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    virtual bool isLValue() const { return false; }

    static bool isZeroCastToVoidPointer(const Tree &tree);

private:

    // Forbidden:
    CastExpr(const CastExpr &);
    CastExpr &operator = (const CastExpr &);

private:

    Tree *subExpr;  // owns the Tree object
    class Declaration *resultDeclaration;  // used when result is real number

};


#endif  /* _H_CastExpr */
