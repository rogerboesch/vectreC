/*  $Id: FunctionPointerCast.h,v 1.2 2022/07/07 16:25:18 sarrazip Exp $

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

#ifndef _H_FunctionPointerCast
#define _H_FunctionPointerCast

#include "FormalParamList.h"


class FunctionPointerCast
{
public:

    // _pointerLevel: Must be positive.
    // _formalParamList: Must come from new.
    //
    FunctionPointerCast(size_t _pointerLevel, FormalParamList *_formalParamList)
    :   pointerLevel(_pointerLevel),
        formalParamList(_formalParamList)
    {
        assert(_pointerLevel > 0);
        assert(_formalParamList);
    }

    ~FunctionPointerCast()
    {
        delete formalParamList;
    }

    size_t getPointerLevel() const
    {
        return pointerLevel;
    }

    const FormalParamList *getFormalParamList() const
    {
        return formalParamList;
    }

private:

    // Forbidden:
    FunctionPointerCast(const FunctionPointerCast &);
    FunctionPointerCast operator =(const FunctionPointerCast &);

private:

    size_t pointerLevel;
    FormalParamList *formalParamList;

};


#endif  /* _H_FunctionPointerCast */
