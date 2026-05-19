/*  $Id: WordConstantExpr.h,v 1.8 2016/12/29 02:12:48 sarrazip Exp $

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

#ifndef _H_WordConstantExpr
#define _H_WordConstantExpr

#include "Tree.h"


class WordConstantExpr : public Tree
{
public:

    // isWord: If false, type is BYTE_TYPE.
    //
    WordConstantExpr(double value, bool isWord, bool isSigned);

    // tokenText: String stored by parser in yytext[]. Used to interpret suffixes
    //            (U for unsigned, L for long).
    //
    WordConstantExpr(double value, const char *tokenText);

    virtual ~WordConstantExpr();

    uint16_t getWordValue() const;

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool isLValue() const { return false; }

private:

    static bool hasUnsignedSuffix(const char *tokenText);
    static bool hasLongSuffix(const char *tokenText);

private:

    double wordValue;  // value (possibly out of range for uint16_t) as seen by the parser

};


#endif  /* _H_WordConstantExpr */
