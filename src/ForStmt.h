/*  $Id: ForStmt.h,v 1.9 2018/02/02 02:55:59 sarrazip Exp $

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

#ifndef _H_ForStmt
#define _H_ForStmt

#include "Tree.h"


class ForStmt : public Tree
{
public:

    ForStmt(Tree *initExprList, Tree *cond, Tree *incrExprList, Tree *bodyStmt);

    virtual ~ForStmt();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    const Tree *getCondition() const { return condition; }

    const Tree *getInitializations() const;

    const Tree *getBody() const;

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(initializations, existingChild, newChild))
            return;
        if (deleteAndAssign(condition, existingChild, newChild))
            return;
        if (deleteAndAssign(increments, existingChild, newChild))
            return;
        if (deleteAndAssign(body, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    virtual bool isLValue() const { return false; }

private:

    CodeStatus emitInScope(ASMText &out,
                           const std::string &bodyLabel, const std::string &conditionLabel,
                           const std::string &incrementLabel, const std::string &endLabel) const;

    // Forbidden:
    ForStmt(const ForStmt &);
    ForStmt &operator = (const ForStmt &);

private:

    // These pointers own the pointed objects:
    Tree *initializations;
    Tree *condition;
    Tree *increments;
    Tree *body;
};


#endif  /* _H_ForStmt */
