/*  $Id: StringLiteralExpr.cpp,v 1.20 2019/04/05 03:01:02 sarrazip Exp $

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

#include "StringLiteralExpr.h"

#include "TranslationUnit.h"

using namespace std;


StringLiteralExpr::StringLiteralExpr(const string &literal)
  : Tree(TranslationUnit::getTypeManager().getArrayOfConstChar()),  // const char *
    stringLiteral(literal),
    stringValue(),
    asmLabel(),
    emitted(false)
{
    bool hexEscapeOutOfRange = false, octalEscapeOutOfRange = false;

    stringValue = decodeEscapedLiteral(hexEscapeOutOfRange, octalEscapeOutOfRange);

    if (hexEscapeOutOfRange)
        warnmsg("hex escape sequence out of range");
    if (octalEscapeOutOfRange)
        warnmsg("octal escape sequence out of range");
}


/*virtual*/
StringLiteralExpr::~StringLiteralExpr()
{
}


const string &
StringLiteralExpr::getLiteral() const
{
    return stringLiteral;
}


const string &
StringLiteralExpr::getValue() const
{
    return stringValue;
}


string
StringLiteralExpr::getLabel() const
{
    return asmLabel;
}


void
StringLiteralExpr::setLabel(const string &newLabel)
{
    assert(newLabel.length() > 0);
    asmLabel = newLabel;
}


string
StringLiteralExpr::getArg() const
{
    emitted = true;

    return asmLabel + TranslationUnit::instance().getLiteralIndexRegister(true);
}


string
StringLiteralExpr::getEscapedVersion() const
{
    assert(!asmLabel.empty());
    return TranslationUnit::instance().getEscapedStringLiteral(asmLabel);
}


/*virtual*/
CodeStatus
StringLiteralExpr::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    out.ins("LEAX", getArg(), getEscapedVersion());
    out.ins("TFR", "X,D");
    return true;
}


/*static*/
string
StringLiteralExpr::escape(const string &s)
{
    string::size_type len = s.length();
    string escaped = "\"";

    for (size_t i = 0; i < len; i++)
    {
        switch (s[i])
        {
            case '\0': escaped += "\\0"; break;
            case '\a': escaped += "\\a"; break;
            case '\b': escaped += "\\b"; break;
            case '\t': escaped += "\\t"; break;
            case '\n': escaped += "\\n"; break;
            case '\v': escaped += "\\v"; break;
            case '\f': escaped += "\\f"; break;
            case '\r': escaped += "\\r"; break;
            case '\'': escaped += "\\'"; break;
            case '\"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            default  :
            {
                unsigned char u = (unsigned char) s[i];
                if (u < 32 || u > 126)
                {
                    char t[128];
                    snprintf(t, sizeof(t), "\\x%02X", u);
                    escaped += t;
                }
                else
                    escaped += s[i];
            }
        }
    }

    return escaped + "\"";
}


// Interprets the character(s) at s + i.
// If '\0' is seen, returns false.
// Otherwise, 'out' receives the interpreted character,
// and 'i' is advanced by 0, 1, or more characters.
//
// Example: i = 1; interpretStringLiteralPosition("z\x41%", i, ch);
// This puts 'A' in ch, advances 'i' by 4 (so that i becomes 5)
// and returns true to indicate that 'ch' has been filled.
//
// hexEscapeOutOfRange, octalEscapeOutOfRange: Set to true if a \x or \0 escape sequence
//                                             is followed by an excessively large hex
//                                             or octal constant. The caller must initialize
//                                             those variables to false.
//
bool
StringLiteralExpr::interpretStringLiteralPosition(size_t &i, char &out,
                                                  bool &hexEscapeOutOfRange,
                                                  bool &octalEscapeOutOfRange) const
{
    if (i >= stringLiteral.size())
        return false;
    char c = stringLiteral[i];
    if (c == '\0')
        return false;  // unexpected
    if (c == '\\')
    {
        ++i;
        if (i >= stringLiteral.size())
        {
            out = '\\';
            return true;  // 'i' is not advanced here
        }
        c = stringLiteral[i];  // char following backslash
        switch (c)
        {
        case 'a' : out = '\a'; ++i; return true;
        case 'b' : out = '\b'; ++i; return true;
        case 't' : out = '\t'; ++i; return true;
        case 'n' : out = '\n'; ++i; return true;
        case 'v' : out = '\v'; ++i; return true;
        case 'f' : out = '\f'; ++i; return true;
        case 'r' : out = '\r'; ++i; return true;
        case '\'': out = '\''; ++i; return true;
        case '\"': out = '\"'; ++i; return true;
        case '\\': out = '\\'; ++i; return true;
        case 'x':
            out = 0;
            for (++i; i < stringLiteral.size() && isxdigit(stringLiteral[i]); ++i)
            {
                int hexDigit = tolower(stringLiteral[i]);
                char digit = char(hexDigit <= '9' ? hexDigit - '0' : hexDigit - 'a' + 10);
                if (out & 0xF0)
                    hexEscapeOutOfRange = true;
                out = (out << 4) | digit;
            }
            return true;
        case '0':
            out = '\0';
            for (++i; i < stringLiteral.size() && stringLiteral[i] >= '0' && stringLiteral[i] <= '7'; ++i)
            {
                char digit = stringLiteral[i] - '0';
                if (out & 0xE0)
                    octalEscapeOutOfRange = true;
                out = (out << 3) | digit;
            }
            return true;
        default:
            out = '\\';
            return true;  // 'i' is not advanced here
        }
    }

    // Ordinary character.
    out = c;
    ++i;
    return true;
}


// Returns the run-time value of the literal, i.e., after interpretation
// of the backslash escape sequences.
//
string
StringLiteralExpr::decodeEscapedLiteral(bool &hexEscapeOutOfRange,
                                        bool &octalEscapeOutOfRange) const
{
    hexEscapeOutOfRange = false, octalEscapeOutOfRange = false;

    string decoded;

    // Advance through 'stringLiteral', converting one or more input characters
    // into a single output character at each iteration.
    //
    char ch = '\0';
    for (size_t i = 0; interpretStringLiteralPosition(i, ch, hexEscapeOutOfRange, octalEscapeOutOfRange); )
        decoded += ch;

    //cout << "StringLiteralExpr::decodeEscapedLiteral: return {" << decoded << "}\n";
    return decoded;
}


size_t
StringLiteralExpr::getDecodedLength() const
{
    return stringValue.length();
}


/*static*/
void
StringLiteralExpr::emitStringLiteralDefinition(ASMText &out, const string &value)
{
    enum State { LINE_START, IN_FCC };

    State state = LINE_START;
    stringstream buffer;
    for (size_t i = 0; i < value.length(); i++)
    {
        unsigned char u = value[i];
        if (u < ' ' || u > 126 || u == '\"' || u == '\\')  // if unprintable/special character:
        {
            switch (state)
            {
                case IN_FCC:
                    buffer << '\"';
                    out.ins("FCC", buffer.str());
                    buffer.str(std::string());  // clear the buffer
                    state = LINE_START;

                    /* FALLTHROUGH */

                case LINE_START:
                    out.ins("FCB", wordToString(u, true));
                    break;
            }
        }
        else  // printable character:
        {
            switch (state)
            {
                case LINE_START:
                    buffer << '\"';
                    state = IN_FCC;
                    /* FALLTHROUGH */
                case IN_FCC:
                    buffer << u;
                    break;
            }
        }
    }

    if (state == IN_FCC)
    {
        buffer << '\"';
        out.ins("FCC", buffer.str());
    }

    out.ins("FCB", "0");
}


void
StringLiteralExpr::emitStringLiteralDefinition(ASMText &out) const
{
    emitStringLiteralDefinition(out, stringValue);
}


