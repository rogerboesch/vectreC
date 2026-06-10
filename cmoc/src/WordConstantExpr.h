/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2023 Pierre Sarrazin <http://sarrazip.com/>

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

    virtual void checkSemantics(Functor &f) override;

    // Tell emitCode() to assume that the value to load fits a byte, so that
    // the emitted code will only load a byte instead of a word.
    //
    void forceAsByte();

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const override;

    CodeStatus emitCodeToLoadByte(ASMText &out) const;

    virtual bool isLValue() const override { return false; }

private:

    static bool hasUnsignedSuffix(const char *tokenText);
    static bool hasLongSuffix(const char *tokenText);
    CodeStatus emitRValue(ASMText &out, bool emitByte) const;

private:

    double wordValue;  // value (possibly out of range for uint16_t) as seen by the parser
    bool forcedAsByte;  // if true, emitCode() will emit code that loads a byte in B, instead of a word in D

};


#endif  /* _H_WordConstantExpr */
