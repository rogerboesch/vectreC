/*  $Id: ScopeCreator.h,v 1.5 2016/10/05 02:28:24 sarrazip Exp $

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

#ifndef _H_ScopeCreator
#define _H_ScopeCreator

#include "Tree.h"

class TranslationUnit;
class Scope;
class VariableExpr;
class IdentifierExpr;


/*  Functor to iterate in a function body to find all subtrees
    for which a Scope object must be created (e.g., compound statement,
    for statement, while statement).
    Also, for each Declaration seen, declares it to the innermost scope
    that contains it.
    For each VariableExpr, finds its related Declaration and sets its type.
    For each FunctionCallExpr, finds the related Declaration if a function
    pointer variable is used.
*/
class ScopeCreator : public Tree::Functor
{
public:
    ScopeCreator(TranslationUnit &tu, Scope *ancestorScope);

    virtual ~ScopeCreator();

    virtual bool open(Tree *t);

    virtual bool close(Tree *t);

    void processIdentifierExpr(IdentifierExpr &ie);

private:

    bool privateOpen(Tree *t);
    bool privateClose(Tree *t);

    TranslationUnit &translationUnit;
    std::vector<Tree *> ancestors;  // [0] is top ancestor

};


#endif  /* _H_ScopeCreator */
