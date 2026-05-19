/*  $Id: DWordConstantExpr.h,v 1.2 2018/02/21 00:44:36 sarrazip Exp $

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

#ifndef _H_DWordConstantExpr
#define _H_DWordConstantExpr

#include "Tree.h"


class DWordConstantExpr : public Tree
{
public:

    DWordConstantExpr(double _value, bool isSigned);

    virtual ~DWordConstantExpr();

    uint32_t getDWordValue() const;

    double getRealValue() const;

    void setLabel(const std::string &newLabel);

    bool isDoublePrecision() const;

    void negateValue() { value = - value; }

    // Returns a big endian representation of the value.
    //
    std::vector<uint8_t> getRepresentation() const;

    // Emits a definition of this constant, using the given representation,
    // of the type returned by getRepresentation().
    //
    static void emitDWordConstantDefinition(ASMText &out, const std::vector<uint8_t> &representation);

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool isLValue() const { return false; }

private:

    double value;  // value as seen by the parser
    std::string asmLabel;

};


#endif  /* _H_DWordConstantExpr */
