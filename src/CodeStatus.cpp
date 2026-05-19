/*  $Id: CodeStatus.cpp,v 1.2 2015/05/31 23:43:23 sarrazip Exp $

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

#include "CodeStatus.h"

using namespace std;


CodeStatus::CodeStatus(bool successfulEmission)
  : success(successfulEmission)
{
}


CodeStatus::~CodeStatus()
{
}


CodeStatus::CodeStatus(const CodeStatus &x)
  : success(x.success)
{
}


CodeStatus &CodeStatus::operator = (const CodeStatus &x)
{
    if (&x != this)
        success = x.success;
    return *this;
}


CodeStatus::operator bool () const
{
    return success;
}
