/*  $Id: SemanticsChecker.cpp,v 1.4 2017/06/25 21:04:37 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2016 Pierre Sarrazin <http://sarrazip.com/>

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

#include "SemanticsChecker.h"

#include "TranslationUnit.h"


SemanticsChecker::SemanticsChecker()
: currentFunctionDef(NULL)
{
    TranslationUnit::instance().pushScope(&TranslationUnit::instance().getGlobalScope());
}


SemanticsChecker::~SemanticsChecker()
{
    TranslationUnit::instance().popScope();  // pop global scope
}


bool
SemanticsChecker::open(Tree *t)
{
    // Push the scope of 't', if it has one.
    // This ensures that checkSemantics() looks up variable names in the right scope when needed.
    // An example is AssemblerStmt::checkSemantics().
    // NOTE: At this point, if 't' is a FunctionDef, it does not have a scope yet.
    //       This scope gets created by the call to checkSemantics().
    //       This is the reason for the patch after that call.
    //
    t->pushScopeIfExists();

    t->checkSemantics(*this);

    // PATCH: If 't' is a FunctionDef, no scope was pushed by the preceding call to pushScopeIfExists().
    //        We must push the scope here, now that checkSemantics() has created it.
    //        To avoid this patch, the use of ScopeCreator should be taken out of FunctionDef::checkSemantics()
    //        and the ScopeCreator should be invoked before using the SemanticsChecker.
    //
    if (dynamic_cast<FunctionDef *>(t))
        TranslationUnit::instance().pushScope(t->getScope());

    return true;
}


bool
SemanticsChecker::close(Tree *t)
{
    //std::cout << "# SemanticsChecker::close(" << typeid(*t).name() << ")\n";

    t->popScopeIfExists();

    if (const FunctionDef *fd = dynamic_cast<FunctionDef *>(t))
    {
        if (fd->getBody())  // if end of function body
            setCurrentFunctionDef(NULL);  // no more current function
    }
    return true;
}


void
SemanticsChecker::setCurrentFunctionDef(FunctionDef *fd)
{
    assert(currentFunctionDef == NULL || fd == NULL);
    currentFunctionDef = fd;
}


const FunctionDef *
SemanticsChecker::getCurrentFunctionDef() const
{
    return currentFunctionDef;
}
