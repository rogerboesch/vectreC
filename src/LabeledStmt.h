/*  $Id: LabeledStmt.h,v 1.12 2025/09/27 06:16:03 sarrazip Exp $

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

    // For a statement labeled with an identifier, e.g., foo: x = 1;
    //
    LabeledStmt(const char *_id, Tree *_statement);

    virtual ~LabeledStmt();

    virtual bool iterate(Functor &f) override;

    virtual void checkSemantics(Functor &f) override;

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const override;

    bool isCase() const { return id.empty() && expression; }

    bool isDefault() const { return id.empty() && !expression; }

    bool isCaseOrDefault() const { return id.empty(); }

    bool isId() const { return !id.empty(); }

    const std::string getId() const { return id; }

    const Tree *getExpression() const { return expression; }

    const Tree *getStatement() const { return statement; }

    Tree *getStatement() { return statement; }

    const std::string &getAssemblyLabel() const { return asmLabel; }

    std::string getAssemblyLabelIfIDEqual(const std::string &id) const;

    virtual bool isLValue() const override { return false; }

private:

    LabeledStmt(const LabeledStmt&);
    LabeledStmt &operator = (const LabeledStmt&);

    std::string id;        // empty if 'case' or 'default' statement
    std::string asmLabel;  // empty if 'case' or 'default' statement
    Tree *expression;  // when 'case' statement (null otherwise)
    Tree *statement;  // sub-statement

};


#endif  /* _H_LabeledStmt */
