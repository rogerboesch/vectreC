/*  AncestorFunctor.cpp

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2022 Pierre Sarrazin <http://sarrazip.com/>

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

#include "AncestorFunctor.h"

#include "CompoundStmt.h"
#include "IfStmt.h"
#include "WhileStmt.h"
#include "ForStmt.h"
#include "LabeledStmt.h"


AncestorFunctor::AncestorFunctor()
  : ancestors()
{
    ancestors.reserve(64);
}


AncestorFunctor::~AncestorFunctor()
{
}


bool
AncestorFunctor::doesParentNeedValueOfChild(const Tree *parent, const Tree *child)
{
    if (dynamic_cast<const CompoundStmt *>(parent))
    {
        // N.B.: CompoundStmt derives from TreeSequence.
        //       A TreeSequence is used (namely) to contain the arguments to a function call.
        //       Here, we use CompoundStmt specifically to refer to a function body, a loop body, etc.
        //       A TreeSequence containing function arguments must let its elements yield values,
        //       because those values are passed to the function being called.
        return false;
    }

    if (const IfStmt *p = dynamic_cast<const IfStmt *>(parent))
        return p->getCondition() == child;

    if (const WhileStmt *p = dynamic_cast<const WhileStmt *>(parent))
        return p->getCondition() == child;

    if (const ForStmt *p = dynamic_cast<const ForStmt *>(parent))
        return p->getCondition() == child;

    if (dynamic_cast<const LabeledStmt *>(parent))
        return false;

    return true;
}


bool
AncestorFunctor::open(Tree *t)
{
    if (ancestors.size() == 0 || !doesParentNeedValueOfChild(ancestors.back(), t))
        t->setYieldedValueNeeded(false);
    ancestors.push_back(t);
    return true;
}


bool
AncestorFunctor::close(Tree *)
{
    ancestors.pop_back();
    return true;
}
