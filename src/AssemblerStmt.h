/*  $Id: AssemblerStmt.h,v 1.11 2017/07/18 23:35:50 sarrazip Exp $

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

#ifndef _H_AssemblerStmt
#define _H_AssemblerStmt

#include "Declaration.h"


class AssemblerStmt : public Tree
{
public:

    // argIsVariable: true if 'arg' is a C variable name;
    // false if 'arg' is a string to be copied verbatim.
    //
    AssemblerStmt(const std::string &ins,
                  const std::string &arg,
                  bool argIsVariable);

    // asmText: text to be copied into the generated assembly language file.
    //
    AssemblerStmt(const char *_asmText);

    virtual ~AssemblerStmt();

    // Tells this statement that it is inside a function defined with modifier 'asm'.
    // Gives it a pointer to the scope of the containing function.
    //
    void setAssemblyOnly(const Scope *functionScope);

    virtual void checkSemantics(Functor &f);

    // Returns all names referred to with the :VAR notation, whether those names
    // are recognized or not as declared variables.
    //
    void getAllVariableNames(std::set<std::string> &varNames) const;

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool isLValue() const { return false; }

private:

    enum Token { NEWLINE, WHITESPACE, WORD, END };

    static Token getToken(const std::string &text, size_t &i, std::string &tokenText);
    static std::string removeComments(const std::string &text);
    static bool parseVariableNameAndOffset(const std::string &tokenText, std::string &variableName, int16_t &offset);
    static std::string resolveVariableReferences(const std::string &text,
                                                 const Scope &scope,
                                                 std::set<std::string> *recognizedVarNames,
                                                 std::set<std::string> *undeclaredNames,
                                                 bool requireAllocatedVariables);
    static bool isGlobalVariable(const std::string &varName);

private:

    // Forbidden operations:
    AssemblerStmt(const AssemblerStmt &);
    AssemblerStmt &operator = (const AssemblerStmt &);

private:

    std::string asmText;  // if empty, other fields contain instruction
    std::string instruction;
    std::string argument;
    const Scope *scopeOfAsmOnlyFunction;  // not null when stmt is in function defined with modifier 'asm'
    bool argIsVariable;

    static const char variableNameEscapeChar;  // precede var name w/ this to force ref to var instead of reg

};


#endif  /* _H_AssemblerStmt */
