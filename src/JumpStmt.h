/*  $Id: JumpStmt.h,v 1.7 2016/09/15 03:34:57 sarrazip Exp $

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

#ifndef _H_JumpStmt
#define _H_JumpStmt

#include "Tree.h"
class FunctionDef;


class JumpStmt : public Tree
{
public:

    enum JumpType
    {
        BRK, CONT, RET, GO_TO
    };

    JumpStmt(JumpType jt, Tree *arg);

    JumpStmt(const char *_targetLabelID);  // goto statement

    JumpType getJumpType() const;

    const Tree *getArgument() const;

    virtual ~JumpStmt();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(argument, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    virtual bool isLValue() const { return false; }

private:

    // Forbidden:
    JumpStmt(const JumpStmt &);
    JumpStmt &operator = (const JumpStmt &);

public:

    JumpType jumpType;
    Tree *argument;  // relevant for RET only, NULL if RET has no argument; owns the pointed object
    std::string targetLabelID;  // relevant for GO_TO only
    const FunctionDef *currentFunctionDef;

};


#endif  /* _H_JumpStmt */
