/*  $Id: FormalParameter.h,v 1.6 2017/07/22 01:54:11 sarrazip Exp $

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

#ifndef _H_FormalParameter
#define _H_FormalParameter

#include "Tree.h"


class FormalParameter : public Tree
{
public:

    // _paramId: Allowed to be empty.
    // Makes a deep copy of '_arrayDimensions'.
    //
    FormalParameter(const TypeDesc *_td,
                    const std::string &_paramId,
                    const std::vector<uint16_t> &_arrayDimensions,
                    const std::string &_enumTypeName);

    virtual ~FormalParameter();

    // Empty in the case of an unnamed function parameter.
    //
    std::string getId() const;

    const std::vector<uint16_t> &getArrayDimensions() const;

    const std::string &getEnumTypeName() const;

    virtual bool isLValue() const { return false; }

private:

    std::string paramId;  // allowed to be empty
    std::vector<uint16_t> arrayDimensions;
    std::string enumTypeName;  // used when parameter is of named enum type

};


#endif  /* _H_FormalParameter */
