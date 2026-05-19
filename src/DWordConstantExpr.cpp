/*  $Id: DWordConstantExpr.cpp,v 1.7 2019/01/26 19:46:22 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2018 Pierre Sarrazin <http://sarrazip.com/>

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

#include "DWordConstantExpr.h"

#include "TranslationUnit.h"

#include <algorithm>
#include <iomanip>

using namespace std;


DWordConstantExpr::DWordConstantExpr(double _value, bool isSigned)
  : Tree(TranslationUnit::getTypeManager().getLongType(isSigned)),
    value(_value),
    asmLabel()
{
}


/*virtual*/
DWordConstantExpr::~DWordConstantExpr()
{
}


uint32_t
DWordConstantExpr::getDWordValue() const
{
    assert(value > -2147483649.0 && value < 4294967296.0);
    if (value >= 0.0)
        return uint32_t(value);
    // Convert double to non-negative value, so that conversion to uint32_t is portable.
    // Then do a 2's complement to obtain a 32-bit unsigned representation of the actual negative value.
    return ~ uint32_t(- value) + uint32_t(1);
}


double
DWordConstantExpr::getRealValue() const
{
    return value;
}


void
DWordConstantExpr::setLabel(const string &newLabel)
{
    assert(newLabel.length() > 0);
    asmLabel = newLabel;
}


vector<uint8_t>
DWordConstantExpr::getRepresentation() const
{
    uint32_t d = getDWordValue();
    vector<uint8_t> rep;
    for (int shift = 24; shift >= 0; shift -= 8)
        rep.push_back(uint8_t((d >> shift) & 0xFF));
    return rep;
}


void
DWordConstantExpr::emitDWordConstantDefinition(ASMText &out, const std::vector<uint8_t> &representation)
{
    stringstream arg;
    for (size_t len = representation.size(), i = 0; i < len; ++i)
    {
        if (i > 0)
            arg << ',';
        arg << wordToString(representation[i], true);
    }
    out.ins("FCB", arg.str());
}


/*virtual*/
void
DWordConstantExpr::checkSemantics(Functor &)
{
    if (value < -2147483648.0 || value >= 4294967296.0)
    {
        errormsg("invalid numerical constant %f (must be 32-bit integer)", value);
    }
}


/*virtual*/
CodeStatus
DWordConstantExpr::emitCode(ASMText &out, bool lValue) const
{
    if (!lValue)
    {
        errormsg("cannot emit a 32-bit number as an r-value");  // doesn't fit in D
        return false;
    }
    if (asmLabel.empty())
    {
        // Somewhat ugly hack to register this constant now that we know that it is used.
        // This will cause the constant and its label to be emitted in the rodata section.
        const_cast<DWordConstantExpr *>(this)->setLabel(TranslationUnit::instance().registerDWordConstant(*this));
    }

    out.ins("LEAX", asmLabel + TranslationUnit::instance().getLiteralIndexRegister(true),
                    "32-bit constant: " + doubleToString(value));
    return true;
}
