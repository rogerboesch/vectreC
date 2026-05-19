/*  $Id: IdentifierExpr.cpp,v 1.9 2017/08/06 02:06:00 sarrazip Exp $

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

#include "IdentifierExpr.h"

#include "VariableExpr.h"
#include "TranslationUnit.h"
#include "WordConstantExpr.h"
#include "StringLiteralExpr.h"

using namespace std;


IdentifierExpr::IdentifierExpr(const char *id)
  : Tree(),
    identifier(id),
    variableExpr(NULL),
    functionNameStringLiteral(NULL)
{
}


/*virtual*/
IdentifierExpr::~IdentifierExpr()
{
    delete functionNameStringLiteral;
    delete variableExpr;
}


string
IdentifierExpr::getId() const
{
    return identifier;
}


void
IdentifierExpr::setVariableExpr(VariableExpr *ve)
{
    delete variableExpr;
    variableExpr = ve;

    if (variableExpr)
    {
        setTypeDesc(variableExpr->getTypeDesc());
        variableExpr->copyLineNo(*this);
    }
}


const VariableExpr *
IdentifierExpr::getVariableExpr() const
{
    return variableExpr;
}


const Declaration *
IdentifierExpr::getDeclaration() const
{
    return variableExpr ? variableExpr->getDeclaration() : NULL;
}


StringLiteralExpr *
IdentifierExpr::setFunctionNameStringLiteral(const std::string &newName)
{
    delete functionNameStringLiteral;
    functionNameStringLiteral = new StringLiteralExpr(newName);
    return functionNameStringLiteral;
}


const StringLiteralExpr *
IdentifierExpr::getFunctionNameStringLiteral() const
{
    return functionNameStringLiteral;
}


bool
IdentifierExpr::isFuncAddrExpr() const
{
    return variableExpr && variableExpr->isFuncAddrExpr();
}


bool
IdentifierExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (variableExpr)
        variableExpr->iterate(f);
    if (!f.close(this))
        return false;
    return true;
}


/*virtual*/
CodeStatus
IdentifierExpr::emitCode(ASMText &out, bool lValue) const
{
    if (variableExpr)
        return variableExpr->emitCode(out, lValue);

    if (functionNameStringLiteral != NULL)
        return functionNameStringLiteral->emitCode(out, lValue);

    uint16_t enumValue = 0;
    if (TranslationUnit::getTypeManager().getEnumeratorValue(identifier, enumValue))
    {
        if (lValue)
        {
            errormsg("cannot use enumerated name (`%s') as l-value", identifier.c_str());
            return true;
        }
        const TypeDesc *td = TranslationUnit::getTypeManager().getEnumeratorTypeDesc(identifier);
        WordConstantExpr wce(enumValue, td->type == WORD_TYPE, td->isSigned);
        return wce.emitCode(out, false);
    }

    assert(false);
    return false;
}
