/*  $Id: FormalParamList.cpp,v 1.2 2017/12/25 22:36:17 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2017 Pierre Sarrazin <http://sarrazip.com/>

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

#include "FormalParamList.h"

using namespace std;


FormalParamList::FormalParamList()
:   TreeSequence(),
    ellipsis(false)
{
}


FormalParamList::~FormalParamList()
{
}


void
FormalParamList::endWithEllipsis()
{
    ellipsis = true;
}


bool
FormalParamList::endsWithEllipsis() const
{
    return ellipsis;
}

bool
FormalParamList::hasSingleVoidParam() const
{
    return size() == 1 && (*begin())->getTypeDesc()->type == VOID_TYPE;
}


bool
FormalParamList::isAcceptableNumberOfArguments(size_t numArguments) const
{
    if (endsWithEllipsis())
        return numArguments >= size();

    return numArguments == size();
}
