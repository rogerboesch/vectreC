/*  $Id: WordConstantExpr.cpp,v 1.13 2019/10/14 23:27:33 sarrazip Exp $

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

#include "WordConstantExpr.h"

#include "TranslationUnit.h"

using namespace std;


WordConstantExpr::WordConstantExpr(double value, bool isWord, bool isSigned)
  : Tree(TranslationUnit::getTypeManager().getIntType(isWord ? WORD_TYPE : BYTE_TYPE, isSigned)),
    wordValue(value)
{
}


// tokenText: Assumed to be valid as per lexer.ll.
//
inline bool
WordConstantExpr::hasUnsignedSuffix(const char *tokenText)
{
    return strchr(tokenText, 'U') || strchr(tokenText, 'u');
}


// tokenText: Assumed to be valid as per lexer.ll.
//
inline bool
WordConstantExpr::hasLongSuffix(const char *tokenText)
{
    return strchr(tokenText, 'L') || strchr(tokenText, 'l');
}


WordConstantExpr::WordConstantExpr(double value, const char *tokenText)
  : Tree(TranslationUnit::getTypeManager().getIntType(WORD_TYPE, !hasUnsignedSuffix(tokenText) && value <= 0x7FFF)),
    wordValue(value)
{
    if (hasLongSuffix(tokenText))
        warnmsg("long constant is not supported (`%s')", tokenText);
}


/*virtual*/
WordConstantExpr::~WordConstantExpr()
{
}


uint16_t
WordConstantExpr::getWordValue() const
{
    assert(wordValue > -32769.0 && wordValue < 65536.0);
    if (wordValue >= 0.0)
        return uint16_t(wordValue);
    // Convert double to non-negative value, so that conversion to uint16_t is portable.
    // Then do a 2's complement to obtain a 16-bit unsigned representation of the actual negative value.
    return ~ uint16_t(- wordValue) + uint16_t(1);
}


/*virtual*/
void
WordConstantExpr::checkSemantics(Functor &)
{
    if (wordValue < -32768 || wordValue > 65535)
    {
        errormsg("invalid numerical constant %f (must be 16-bit integer)", wordValue);
    }
}


/*virtual*/
CodeStatus
WordConstantExpr::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
    {
        errormsg("cannot emit l-value for word constant expression");
        return false;
    }

    uint16_t uValue = getWordValue();
    if (uValue == 0)
    {
        out.ins("CLRA");
        out.ins("CLRB");
    }
    else
        out.ins("LDD", "#" + wordToString(uValue, true),
               "decimal " + (isSigned()
                               ? intToString(int16_t(uValue), false) + " signed"
                               : wordToString(uValue, false) + " unsigned"));
    return true;
}
