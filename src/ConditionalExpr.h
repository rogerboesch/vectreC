/*  $Id: ConditionalExpr.h,v 1.7 2016/09/15 03:34:56 sarrazip Exp $

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

#ifndef _H_ConditionalExpr
#define _H_ConditionalExpr

#include "Tree.h"


class ConditionalExpr : public Tree
{
public:

    // _condition, _trueExpr, _falseExpr: must not be null and must come
    // from operator new. The Tree objects are owned by this object and
    // ~ConditionalExpr() will call operator delete on them.
    //
    ConditionalExpr(Tree *_condition, Tree *_trueExpr, Tree *_falseExpr);

    // Destroys the three subtrees with operator delete.
    //
    virtual ~ConditionalExpr();

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;
    
    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild);

    const Tree *getTrueExpression() const;
    const Tree *getFalseExpression() const;

    virtual bool isLValue() const { return trueExpr->isLValue() && falseExpr->isLValue(); }

private:

    static void promoteIfNeeded(ASMText &out, const Tree &exprToPromote, const Tree &otherExpr);

    // Forbidden:
    ConditionalExpr(const ConditionalExpr &);
    ConditionalExpr &operator = (const ConditionalExpr &);

private:

    Tree *condition;
    Tree *trueExpr;
    Tree *falseExpr;

};


#endif  /* _H_ConditionalExpr */
