/*  $Id: ObjectMemberExpr.cpp,v 1.11 2017/10/15 00:30:54 sarrazip Exp $

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

#include "ObjectMemberExpr.h"

#include "TranslationUnit.h"
#include "VariableExpr.h"
#include "Declaration.h"
#include "WordConstantExpr.h"
#include "VariableExpr.h"
#include "IdentifierExpr.h"
#include "ClassDef.h"

#include <assert.h>

using namespace std;


ObjectMemberExpr::ObjectMemberExpr(Tree *e,
                                    const string &_memberName,
                                    bool _direct)
  : Tree(),
    subExpr(e),
    memberName(_memberName),
    direct(_direct)
{
}


/*virtual*/
ObjectMemberExpr::~ObjectMemberExpr()
{
    delete subExpr;
}


void
ObjectMemberExpr::checkSemantics(Functor & /*f*/)
{
    // If subExpr is in error (e.g., undeclared variable), then its type is void
    // and we skip these checks.
    //
    string className;
    if (subExpr->getType() == CLASS_TYPE)
        className = subExpr->getTypeDesc()->className;
    else if (subExpr->getTypeDesc()->isPtrOrArray() && subExpr->getTypeDesc()->getPointedType() == CLASS_TYPE)  // if pointer to class
        className = subExpr->getTypeDesc()->pointedTypeDesc->className;

    if (className.empty())
        return;

    // No need to check if 'memberName' exists in 'className', because
    // ExpressionTypeSetter::close() already checks this.
}


/*virtual*/
CodeStatus
ObjectMemberExpr::emitCode(ASMText &out, bool lValue) const
{
    assert(subExpr != NULL);

    const ClassDef *cl = TranslationUnit::instance().getClassDef(getClassName());
    assert(cl != NULL);

    const ClassDef::ClassMember *member = NULL;
    int16_t offset = cl->getDataMemberOffset(memberName, member);
    assert(offset >= 0);
    assert(member != NULL);

    if (!lValue && member->getType() == CLASS_TYPE)
    {
        errormsg("cannot use member `%s' of struct `%s' as an r-value",
                 member->getName().c_str(), cl->getName().c_str());
        return true;
    }

    //cout << "ObjectMemberExpr::emitCode: lValue=" << lValue << ", member->isArray()=" << member->isArray() << endl;
    string opcode = (lValue || member->isArray()
                        ? "LEAX"
                        : getLoadInstruction(getType()));
    string arg = (offset > 0 ? wordToString(offset) : "");
    //cout << "ObjectMemberExpr::emitCode: opcode=" << opcode << ", arg=" << arg << endl;

    const VariableExpr *ve = subExpr->asVariableExpr();

    string memberComment = "member " + memberName + " of " + getClassName();

    bool checkNullPtr = TranslationUnit::instance().isNullPointerCheckingEnabled();

    if (direct)
    {
        if (ve != NULL)
        {
            out.ins(opcode, ve->getFrameDisplacementArg(offset), memberComment + ", via variable " + ve->getId());
        }
        else
        {
            if (!subExpr->emitCode(out, true))
                return false;

            if (checkNullPtr)
                callUtility(out, "check_null_ptr_x");

            if (!lValue || offset > 0)
                out.ins(opcode, arg + ",X", memberComment);
        }
    }
    else
    {
        //cout << "ObjectMemberExpr::emitCode: indirect: ve=" << ve << endl;
        if (ve != NULL)
            out.ins("LDX", ve->getFrameDisplacementArg(), "variable " + ve->getId());
        else
        {
            if (!subExpr->emitCode(out, false))
                return false;
            out.ins("TFR", "D,X", "X points to a struct " + cl->getName());
        }

        if (checkNullPtr)
            callUtility(out, "check_null_ptr_x");

        if (!lValue || offset > 0)
            out.ins(opcode, arg + ",X", memberComment);
    }

    // If producing r-value and member is array, then transfer array address to D.
    //
    if (!lValue && member->isArray())
        out.ins("TFR", "X,D");

    return true;
}


bool
ObjectMemberExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!subExpr->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}


bool
ObjectMemberExpr::isDirect() const
{
    return direct;
}


const Tree *
ObjectMemberExpr::getSubExpr() const
{
    return subExpr;
}


Tree *
ObjectMemberExpr::getSubExpr()
{
    return subExpr;
}


const std::string &
ObjectMemberExpr::getClassName() const
{
    if (direct)
        return subExpr->getTypeDesc()->className;
    return subExpr->getTypeDesc()->pointedTypeDesc->className;
}


const ClassDef *
ObjectMemberExpr::getClass() const
{
    const std::string &className = getClassName();
    return TranslationUnit::instance().getClassDef(className);
}



const std::string &
ObjectMemberExpr::getMemberName() const
{
    return memberName;
}


const ClassDef::ClassMember *
ObjectMemberExpr::getClassMember() const
{
    const ClassDef *cl = getClass();
    if (cl == NULL)
    {
        errormsg("reference to member `%s' of undefined class `%s'",
                 getMemberName().c_str(), getClassName().c_str());
        assert(!getClassName().empty());
        return NULL;
    }
    const ClassDef::ClassMember *mi = cl->getDataMember(getMemberName());
    if (mi == NULL)
    {
        errormsg("struct %s has no member named %s",
                 cl->getName().c_str(), getMemberName().c_str());
        return NULL;
    }
    return mi;
}
