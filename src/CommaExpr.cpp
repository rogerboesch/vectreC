/*  $Id: CommaExpr.cpp,v 1.1 2017/12/02 02:45:36 sarrazip Exp $

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

#include "CommaExpr.h"

using namespace std;


CommaExpr::CommaExpr(Tree *subExpr0, Tree *subExpr1)
:   TreeSequence()
{
    assert(subExpr0);
    assert(subExpr1);
    addTree(subExpr0);
    addTree(subExpr1);
}


/*virtual*/
CommaExpr::~CommaExpr()
{
}


bool
CommaExpr::isLValue() const
{
    if (size() == 0)
        return false;
    const Tree *last = *rbegin();
    return last->isLValue();
}
