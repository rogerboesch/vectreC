/*  $Id: FormalParameter.cpp,v 1.5 2016/07/24 23:03:06 sarrazip Exp $

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

#include "FormalParameter.h"

#include "Declarator.h"

using namespace std;


FormalParameter::FormalParameter(const TypeDesc *_td,
                                 const string &_paramId,
                                 const vector<uint16_t> &_arrayDimensions,
                                 const std::string &_enumTypeName)
  : Tree(_td),
    paramId(_paramId),
    arrayDimensions(_arrayDimensions),
    enumTypeName(_enumTypeName)
{
}


FormalParameter::~FormalParameter()
{
}


string
FormalParameter::getId() const
{
    return paramId;
}


const vector<uint16_t> &
FormalParameter::getArrayDimensions() const
{
    return arrayDimensions;
}


const string &
FormalParameter::getEnumTypeName() const
{
    return enumTypeName;
}
