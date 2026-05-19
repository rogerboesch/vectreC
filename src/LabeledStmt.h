/*  $Id: LabeledStmt.h,v 1.9 2019/03/10 18:29:45 sarrazip Exp $

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

#ifndef _H_LabeledStmt
#define _H_LabeledStmt

#include "Declaration.h"


class LabeledStmt : public Tree
{
public:

    LabeledStmt(Tree *_caseExpr, Tree *_statement);

    LabeledStmt(Tree *_defaultStatement);

    LabeledStmt(const char *_id, const std::string &_asmLabel, Tree *_statement);

    virtual ~LabeledStmt();

    virtual bool iterate(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    bool isCase() const { return id.empty() && expression; }

    bool isDefault() const { return id.empty() && !expression; }

    bool isCaseOrDefault() const { return id.empty(); }

    bool isId() const { return !id.empty(); }

    const std::string getId() const { return id; }

    const Tree *getExpression() const { return expression; }

    const Tree *getStatement() const { return statement; }

    Tree *getStatement() { return statement; }

    std::string getAssemblyLabelIfIDEqual(const std::string &id) const;

    virtual bool isLValue() const { return false; }

private:

    LabeledStmt(const LabeledStmt&);
    LabeledStmt &operator = (const LabeledStmt&);

    std::string id;        // when ID_LABEL (empty otherwise)
    std::string asmLabel;  // when ID_LABEL (empty otherwise)
    Tree *expression;  // when CASE_LABEL (null otherwise)
    Tree *statement;

};


#endif  /* _H_LabeledStmt */
