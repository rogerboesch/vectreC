/*  $Id: FormalParamList.h,v 1.7 2025/08/28 02:28:18 sarrazip Exp $

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

#ifndef _H_FormalParamList
#define _H_FormalParamList

#include "TreeSequence.h"


// List of FormalParameter objects.
//
class FormalParamList : public TreeSequence
{
public:
    FormalParamList();

    virtual ~FormalParamList();

    void endWithEllipsis(bool ellipsisImplied = false);

    bool endsWithEllipsis() const;

    bool isEllipsisImplied() const;

    bool hasSingleVoidParam() const;

    bool isAcceptableNumberOfArguments(size_t numArguments) const;

private:
    bool ellipsis;
    bool ellipsisImplied;  // used for K&R functions whose parameter list is an empty parenthesis, e.g., f()
};


#endif  /* _H_FormalParamList */
