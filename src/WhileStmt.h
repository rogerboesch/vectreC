/*  $Id: WhileStmt.h,v 1.8 2017/07/22 15:36:11 sarrazip Exp $

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

#ifndef _H_WhileStmt
#define _H_WhileStmt

#include "Tree.h"


class WhileStmt : public Tree
{
public:

    WhileStmt(Tree *cond, Tree *bodyStmt, bool isDoWhile);

    virtual ~WhileStmt();

    bool isDoStatement() const { return isDo; }

    const Tree *getCondition() const { return condition; }

    const Tree *getBody() const { return body; }

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(condition, existingChild, newChild))
            return;
        if (deleteAndAssign(body, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    virtual bool isLValue() const { return false; }

private:

    // Forbidden:
    WhileStmt(const WhileStmt &);
    WhileStmt &operator = (const WhileStmt &);

public:

    Tree *condition;  // owns the pointed object
    Tree *body;  // owns the pointed object
    bool isDo;  // true: do {} while (cond); false: while (cond) {}

};


#endif  /* _H_WhileStmt */
