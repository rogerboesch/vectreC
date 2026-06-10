/*  $Id: LabeledStmt.cpp,v 1.10 2025/09/27 23:44:50 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2025 Pierre Sarrazin <http://sarrazip.com/>

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

#include "LabeledStmt.h"

#include "TranslationUnit.h"
#include "DeclarationSequence.h"

using namespace std;


LabeledStmt::LabeledStmt(Tree *_caseExpr, Tree *_statement)
  : Tree(),
    id(),
    asmLabel(TranslationUnit::instance().generateLabel('L')),
    expression(_caseExpr),
    statement(_statement)
{
    assert(statement);
}


LabeledStmt::LabeledStmt(Tree *_defaultStatement)
  : Tree(),
    id(),
    asmLabel(TranslationUnit::instance().generateLabel('L')),
    expression(NULL),
    statement(_defaultStatement)
{
}


LabeledStmt::LabeledStmt(const char *_id, Tree *_statement)
  : Tree(),
    id(_id),
    asmLabel(TranslationUnit::instance().generateLabel('L')),
    expression(NULL),
    statement(_statement)
{
}


LabeledStmt::~LabeledStmt()
{
    delete expression;
    delete statement;
}


std::string
LabeledStmt::getAssemblyLabelIfIDEqual(const std::string &_id) const
{
    if (isId() && id == _id)
        return asmLabel;
    return string();
}


/*virtual*/
CodeStatus
LabeledStmt::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    assert(statement);
    const char *comment = NULL;
    if (isId())
        comment = "labeled statement";
    else if (isCase())
        comment = "case statement";
    else
        comment = "default statement";
    statement->writeLineNoComment(out, comment);

    out.emitLabel(asmLabel, "label " + id + ", declared at " + getLineNo());

    if (! statement->emitCode(out, lValue))
        return false;

    return true;
}


bool
LabeledStmt::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (expression && !expression->iterate(f))
        return false;
    if (!statement->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}


void
LabeledStmt::checkSemantics(Functor &)
{
    if (TranslationUnit::instance().warnLabelOnDeclaration()
        && dynamic_cast<const DeclarationSequence *>(statement) != NULL)
    {
        // With GCC 9.4.0, this would be an error.
        warnmsg("a label can only be part of a statement and a declaration is not a statement");
    }
}
