/*  $Id: ObjectMemberExpr.h,v 1.8 2016/10/11 01:23:50 sarrazip Exp $

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

#ifndef _H_ObjectMemberExpr
#define _H_ObjectMemberExpr

#include "ClassDef.h"


class ObjectMemberExpr : public Tree
{
public:

    ObjectMemberExpr(Tree *e, const std::string &memberName, bool direct);

    virtual ~ObjectMemberExpr();

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(subExpr, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    // true means '.' operator is used, false means '->'.
    //
    bool isDirect() const;

    const Tree *getSubExpr() const;

    Tree *getSubExpr();

    const std::string &getClassName() const;

    // Returns null if the class name is not defined.
    //
    const ClassDef *getClass() const;

    const std::string &getMemberName() const;

    // Issues an error message if the class or member does not exist.
    //
    const ClassDef::ClassMember *getClassMember() const;

    virtual bool isLValue() const { return true; }

private:

    // Forbidden:
    ObjectMemberExpr(const ObjectMemberExpr &);
    ObjectMemberExpr &operator = (const ObjectMemberExpr &);

private:

    Tree *subExpr;  // owns the Tree object
    std::string memberName;
    bool direct;  // true means '.' operator is used, false means '->'

};


#endif  /* _H_ObjectMemberExpr */
