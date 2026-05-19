/*  $Id: Scope.cpp,v 1.16 2020/05/06 02:40:26 sarrazip Exp $

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

#include "Scope.h"

#include "TreeSequence.h"
#include "Declaration.h"
#include "TranslationUnit.h"
#include "ClassDef.h"

#include <assert.h>
#include <algorithm>

using namespace std;


Scope::Scope(Scope *_parent, const string &_startLineNo)
  : parent(_parent),
    subScopes(),
    declTable(),
    classTable(),
    startLineNo(_startLineNo)
{
    if (parent != NULL)
        parent->addSubScope(this);
}


Scope::~Scope()
{
    for (vector<Scope *>::iterator its = subScopes.begin();
                                        its != subScopes.end();
                                        its++)
    {
        Scope *child = *its;
        assert(child != NULL);
        assert(child->getParent() == this);
        delete child;
    }

    for (std::map<std::string, ClassDef *>::iterator it = classTable.begin();
                                                    it != classTable.end(); ++it)
        delete it->second;
}



void
Scope::addSubScope(Scope *ss)
{
    assert(ss != NULL);

    // 'ss' must not already be in subScopes.
    assert(find(subScopes.begin(), subScopes.end(), ss) == subScopes.end());

    subScopes.push_back(ss);
}


const Scope *
Scope::getParent() const
{
    return parent;  // allowed to be null
}


Scope *
Scope::getParent()
{
    return parent;  // allowed to be null
}


int16_t
Scope::allocateLocalVariables(int16_t displacement, bool processSubScopes, size_t &numLocalVariablesAllocated)
{
    /*cerr << "# Scope(" << this << ")::allocateLocalVariables("
        << displacement << ", " << processSubScopes << "): " << subScopes.size() << " subscopes\n";*/
    for (DeclarationTable::reverse_iterator itd = declTable.rbegin(); itd != declTable.rend(); itd++)
    {
        Declaration *decl = itd->second;
        assert(decl != NULL);

        if (decl->hasFunctionParameterFrameDisplacement())
            continue;  // function parameter, i.e., already allocated by FunctionDef::declareFormalParams()

        if (decl->isExtern)
            continue;

        if (decl->isGlobal())
        {
            assert(!"global declaration in a Scope on which allocateLocalVariables() is called");
            continue;
        }

        uint16_t size = 0;
        if (!decl->getVariableSizeInBytes(size))
        {
            if (!decl->needsFinish)  // if needsFinish, then DeclarationFinisher failed, so error msg already issued there
                decl->errormsg("invalid dimensions for array `%s'", decl->getVariableId().c_str());
            continue;
        }
        if (size > 32767)
        {
            decl->errormsg("local variable `%s' exceeds maximum of 32767 bytes", decl->getVariableId().c_str());
            continue;
        }

        displacement -= int16_t(size);
        /*cerr << "  " << decl->getVariableId() << " is "
                << size
                << " byte(s), which puts displacement at "
                << displacement << "\n";*/
        decl->setFrameDisplacement(displacement);

        ++numLocalVariablesAllocated;
    }

    int16_t minDisplacement = displacement;

    if (processSubScopes)
        for (vector<Scope *>::iterator its = subScopes.begin();
                                      its != subScopes.end(); its++)
        {
            int16_t d = (*its)->allocateLocalVariables(displacement, true, numLocalVariablesAllocated);
            minDisplacement = min(minDisplacement, d);
        }

    /*std::cerr << "Scope(" << this << ")::allocateLocalVariables: "
                << "returning minDisplacement of " << minDisplacement << "\n";*/
    return minDisplacement;
}


bool
Scope::declareVariable(Declaration *d)
{
    assert(d != NULL);
    const string id = d->getVariableId();
    const Declaration *found = getVariableDeclaration(id, false);
    /*cout << "# Scope::declareVariable: [" << this << "] id='" << id << "' -> d=" << d
            << ", {" << d->getTypeDesc()->toString()
            << "}, isExtern=" << d->isExtern << ", lineno=" << (d ? d->getLineNo() : "")
            << ", found=" << found << ", scope start: " << startLineNo << endl;*/
    if (found != NULL)  // if already declared in this scope
    {
        if (found->getTypeDesc() != d->getTypeDesc())
            return false;

        if (found->isExtern && !d->isExtern)
        {
            // An "extern" declaration already exists and 'd' is a definition.
            // We destroy the extern and only keep 'd'.
            //
            declTable.erase(findInVectorOfPairsByKey(declTable, id));
            declTable.push_back(make_pair(id, d));
            return true;
        }

        // Accept two identical extern declarations.
        return found->isExtern && d->isExtern;
    }

    // Optionally warn if the declared variable is local and hides another local variable.
    //
    if (TranslationUnit::instance().warnOnLocalVariableHidingAnother())
    {
        found = getVariableDeclaration(id, true);  // look in ancestor Scopes
        if (found != NULL && ! found->isGlobal())
            d->warnmsg("Local variable `%s' hides local variable `%s' declared at %s",
                        id.c_str(), found->getVariableId().c_str(), found->getLineNo().c_str());
    }

    declTable.push_back(make_pair(id, d));
    return true;
}


Declaration *
Scope::getVariableDeclaration(const string &id, bool lookInAncestors) const
{
    for (DeclarationTable::const_iterator it = declTable.begin(); it != declTable.end(); ++it)
        if (it->first == id)
            return it->second;

    if (lookInAncestors && parent != NULL)
        return parent->getVariableDeclaration(id, lookInAncestors);

    return NULL;
}


void
Scope::destroyDeclarations()
{
    for (DeclarationTable::iterator it = declTable.begin(); it != declTable.end(); ++it)
        delete it->second;

    declTable.clear();
}


void
Scope::declareClass(ClassDef *cl)
{
    if (cl == NULL)
        return;
    const string &className = cl->getName();
    assert(!className.empty());

    if (classTable.find(className) != classTable.end())
    {
        cl->errormsg("struct %s already declared", className.c_str());
        return;
    }

    classTable[className] = cl;
}


const ClassDef *
Scope::getClassDef(const std::string &className) const
{
    map<string, ClassDef *>::const_iterator it = classTable.find(className);
    //cerr << "Scope[" << this << "]: getClassDef('" << className << "'): "
    //      << (it != classTable.end()) << "\n";
    return (it != classTable.end() ? it->second : (ClassDef *) 0);
}


void
Scope::getDeclarationIds(std::vector<std::string> &dest) const
{
    for (DeclarationTable::const_iterator it = declTable.begin(); it != declTable.end(); it++)
        dest.push_back(it->first);
}
