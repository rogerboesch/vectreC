/*  $Id: RealConstantExpr.h,v 1.3 2018/02/21 00:44:37 sarrazip Exp $

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

#ifndef _H_RealConstantExpr
#define _H_RealConstantExpr

#include "Tree.h"


class RealConstantExpr : public Tree
{
public:

    // tokenText: String stored by parser in yytext[]. Used to interpret f suffix (for float).
    //
    RealConstantExpr(double value, const char *tokenText);

    virtual ~RealConstantExpr();

    void setLabel(const std::string &newLabel);

    bool isDoublePrecision() const;

    double getRealValue() const { return realValue; }

    uint32_t getDWordValue() const;

    void negateValue() { realValue = - realValue; }

    // Returns an IEEE-754-ish representation of the real value,
    // or an empty vector if the value cannot be represented on the target platform
    // (e.g., too large).
    //
    std::vector<uint8_t> getRepresentation() const;

    // Emits a definition of this constant, using the given representation,
    // of the type returned by getRepresentation().
    //
    static void emitRealConstantDefinition(ASMText &out, const std::vector<uint8_t> &representation);

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool isLValue() const { return false; }

private:

    double realValue;  // value as seen by the parser
    std::string asmLabel;

};


#endif  /* _H_RealConstantExpr */
