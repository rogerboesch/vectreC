/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2016 Pierre Sarrazin <http://sarrazip.com/>

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

#include "AssemblerStmt.h"

#include "TranslationUnit.h"
#include "SemanticsChecker.h"
#include "ASMText.h"
#include "ClassDef.h"

using namespace std;


const char AssemblerStmt::variableNameEscapeChar = ':';


AssemblerStmt::AssemblerStmt(const std::string &ins,
                                const std::string &arg,
                                bool _argIsVariable)
  : Tree(),
    asmText(),
    instruction(ins),
    argument(arg),
    scopeOfAsmOnlyFunction(NULL),
    argIsVariable(_argIsVariable)
{
    if (instruction.length() < 3 || instruction.length() > 5)
        errormsg("invalid assembler instruction");
}


AssemblerStmt::AssemblerStmt(const char *_asmText)
  : Tree(),
    asmText(_asmText),
    instruction(),
    argument(),
    scopeOfAsmOnlyFunction(NULL),
    argIsVariable(false)
{
    // Fix the source line number so that it points to the start of the text
    // instead of the end.
    int numNewlines = 0;
    for (const char *s = _asmText; (s = strchr(s, '\n')) != NULL; ++s)
        ++numNewlines;
    setIntLineNo(getIntLineNo() - numNewlines);
}


AssemblerStmt::~AssemblerStmt()
{
}


void
AssemblerStmt::setAssemblyOnly(const Scope *functionScope)
{
    assert(functionScope);
    scopeOfAsmOnlyFunction = functionScope;
}


// Parse a newline, whitespace sequence or word at offset 'i' in 'text'.
// Returns END if 'i' is at the end of the string.
// Returns the text of the parsed token (empty string for END).
// Advances 'i' to the character past the parsed token.
// (No advancement in the case of END.)
//
// Quirk: text of the form [var] is considered as 3 tokens: "[", "var" and "]",
// but foo[bar] is considered as a single token.
// The first rule facilitates the parsing of indirect addresses,
// e.g., "jsr [someVariable]".
// The second rule allows to parse an array reference as a single token, e.g.,
// "ldd someIntegerArray[12]".
//
// A variable name may be preceded by an escape character (given by variableNameEscapeChar).
// This is useful when the C variable name is also the name of of a register.
// The escape character forces the name to refer to the C variable instead of the register.
//
// A semi-colon introduces a comment. The text from that semi-colon (inclusively)
// up to the end of the line (excluding the newline character) is considered as
// a comment to be ignored. WHITESPACE is returned, and the comment text (including
// the semi-colon) is stored in 'tokenText'.
// Note that the assembler may give special meaning to the presence of a semi-colon
// comment. With LWASM in particular, as of 2017, such a comment line can be used
// to avoid breaking the scope of local labels (those with an initial @).
//
AssemblerStmt::Token
AssemblerStmt::getToken(const string &text, size_t &i, string &tokenText)
{
    tokenText.clear();

    if (i >= text.length())
    {
        tokenText.clear();
        return END;
    }
    if (text[i] == '\n')
    {
        ++i;  // pass the newline
        tokenText = '\n';
        return NEWLINE;
    }

    tokenText = text[i];

    // Parse a sequence of white space, or of non-space characters.
    // Such a sequence stops before a ']', which is considered a word by itself.
    //
    int isFirstCharSpace = isspace(text[i]);
    ++i;  // pass 1st char of token
    if (isFirstCharSpace)
    {
        for ( ; i < text.length() && text[i] != '\n' && isspace(text[i]); ++i)  // accumulate sequence of spaces
            tokenText += text[i];
        return WHITESPACE;
    }

    // Return a comment introduced by a semi-colon or asterisk as white space.
    // A space or start-of-line is required before the comment character.
    //
    assert(i != 0);
    if ((i == 1 || isspace(text[i - 2])) && (text[i - 1] == ';' || text[i - 1] == '*'))
    {
        for ( ; i < text.length() && text[i] != '\n'; ++i)  // advance until end of line
            tokenText += text[i];
        return WHITESPACE;
    }

    if (text[i - 1] == '\"')  // if string literal (e.g., argument for FCC directive)
    {
        for ( ; i < text.length() && text[i] != '\"'; ++i)
            tokenText += text[i];
        if (i < text.length())
        {
            tokenText += '\"';
            ++i;  // pass closing quote
        }
        return WORD;
    }

    if (!isAssemblyIdentifierChar(text[i - 1]) && text[i - 1] != variableNameEscapeChar)
    {
        // The character cannot be part of an identifier, so it is considered as a single-character token.
        // Note that '@' can be part of an assembler identifier (this is supported by LWASM).
        return WORD;
    }

    // If '[' is encountered, we want to accumulate all chars up to and including "]",
    // because this is the "foo[bar]" case.
    // If no '[' is encountered, we want to stop before the "]",
    // because this is the "[var]" case.
    // Dots are accepted; they are used to introduce a struct field name.
    //
    bool gotOpeningBracket = false;
    for ( ; ; ++i)
    {
        if (i == text.length())  // stop on end of text
            break;
        if (isspace(text[i]))  // stop on white space
            break;
        if (text[i] == ']' && !gotOpeningBracket)
            break;
        if (text[i] == '[')  // remember this
            gotOpeningBracket = true;
        else if (!gotOpeningBracket && !isAssemblyIdentifierChar(text[i]) && text[i] != '.')  // stop on '+', '-' etc.
            break;
        tokenText += text[i];
    }

    return WORD;
}


// Removes C++-style comments (ends of lines that start with two slashes).
//
string
AssemblerStmt::removeComments(const string &text)
{
    string result;
    size_t origLen = text.length();
    for (size_t i = 0; i < origLen; ++i)
    {
        if (text[i] == '/' && i + 1 < origLen && text[i + 1] == '/')  // if '//' comment
        {
            for (i += 2; i < origLen && text[i] != '\n'; ++i)
                ;
            if (i == origLen)
                break;
            --i;  // compensate for for-loop increment; next iteration will append \n to 'result'
        }
        else
            result += text[i];
    }
    return result;
}


// If an identifier is found at s + offset, returns its length.
// Returns 0 if no identifier was seen at s + offset.
//
static size_t
getIdentifier(const string s, size_t offset)
{
    if (offset >= s.length() || !isCIdentifierStartingChar(s[offset]))
        return 0;
    size_t start = offset;
    do
        ++offset;
    while (offset < s.length() && isCIdentifierChar(s[offset]));
    return offset - start;
}


// Support decimal only. Does not support hex or octal.
//
static size_t
getIntegerString(const string s, size_t offset)
{
    if (offset >= s.length() || (!isdigit(s[offset]) && s[offset] != '-'))
        return 0;
    size_t start = offset;
    do
        ++offset;
    while (offset < s.length() && isdigit(s[offset]));
    return offset - start;
}


// Parse a C expression of the form var or var.field, or var.field[index].field[index]...field[index], etc.
// result: Upon success, receives strings for variable names and struct field names,
//         a decimal strings for array indexes. Example: obj.v[42] gives { "obj", "v", "42" }.
// Returns: size_t(-1) upon success.
//          Upon failure, returns the character index in tokenText where an error was detected.
//
static size_t
parseCVariableExpression(vector<string> &result, const string &tokenText)
{
    // Expect an identifier first.
    //
    size_t idLength = getIdentifier(tokenText, 0);
    if (idLength == 0)
        return 0;  // error: expected an identifier at start

    result.push_back(string(tokenText, 0, idLength));

    // Accept sequence of ".field" or "[index]".
    //
    for (size_t offset = idLength; ; )
    {
        if (tokenText[offset] == '.')
        {
            ++offset;
            idLength = getIdentifier(tokenText, offset);
            if (idLength == 0)
                return offset;  // error: expected struct field name after '.'
            result.push_back(string(tokenText, offset, idLength));
            offset += idLength;
        }
        else if (tokenText[offset] == '[')  // if integer index into an array
        {
            ++offset;
            size_t intLength = getIntegerString(tokenText, offset);
            if (intLength == 0)
                return offset;  // error: expected integer after '['
            result.push_back(string(tokenText, offset, intLength));
            offset += intLength;
            if (tokenText[offset] != ']')
                return offset;  // error: expected closing ']'
            ++offset;
        }
        else
        {
            if (offset != tokenText.length())
                return offset;  // error: unexpected chars at end
            break;
        }
    }
    return size_t(-1);  // success
}


// scope: Declaration scope of the asm{} statement.
// errorMessageTree: If not null, error messages are issued by calling errormsg() on this.
// Returns: 0 if tokenText refers to a C variable (possibly with array indexes and/or struct fields).
//          -1 if tokenText does not refer to a C variable.
//          -2 if tokenText refers to a C variable but is partly invalid (an error message is issued).
// variableDecl: When 0 is returned, this receives the address of the declaration of the C variable referred to.
// byteOffsetIntoVar: When 0 is returned, this receives the offset in bytes into the start of the C variable layout.
//
static int
parseCVariableRefAndByteIndex(const string &tokenText,
                              const Scope &scope,
                              const Tree *errorMessageTree,
                              const Declaration *&variableDecl,
                              int16_t &byteOffsetIntoVar)
{
    variableDecl = NULL;
    byteOffsetIntoVar = 0;

    vector<string> expr;
    size_t errorOffset = parseCVariableExpression(expr, tokenText);
    if (errorOffset != size_t(-1))
        return -1;  // tokenText is not a C var ref

    variableDecl = scope.getVariableDeclaration(expr[0], true);
    if (!variableDecl)
    {
        // Check if enum name.
        //
        uint16_t enumValue = 0;
        if (TranslationUnit::getTypeManager().getEnumeratorValue(expr[0], enumValue))
        {
            if (expr.size() > 1)
            {
                if (errorMessageTree)
                    errorMessageTree->errormsg("invalid characters after enum name `%s' in inline assembler statement", expr[0].c_str());
                return -2;
            }
        }

        return -1;  // tokenText is not a C var ref; might be a hex value starting with letter, e.g., "ff00"
    }

    const TypeDesc *currentTD = variableDecl->getTypeDesc();

    // Interpret the rest of 'expr'.
    //
    string lastName = expr[0];
    for (size_t i = 1; i < expr.size(); ++i)
    {
        const string &s = expr[i];
        if (isdigit(s[0]) || s[0] == '-')  // if array index
        {
            long intValue = strtol(s.c_str(), NULL, 10);  // decimal assumed
            if (intValue < -32768 || intValue > 32767)
                return -2;  // error: invalid array index

            // Check that we are indexing an array, and get the array element size.
            if (!currentTD->isArray())
            {
                // When [index] is applied to non-array, it is a byte offset and it ends the expression.
                if (i + 1 != expr.size())
                {
                    if (errorMessageTree)
                        errorMessageTree->errormsg("extra characters in C variable reference after [%s]",
                                                    s.c_str());
                }
                else
                    byteOffsetIntoVar += intValue;
                break;
            }
            else
            {
                if (intValue != 0 && TranslationUnit::instance().warnAboutInlineAsmArrayIndexes())
                {
                    if (errorMessageTree)
                        errorMessageTree->warnmsg("index `%s' into array `%s' in inline assembly statement taken as element index, not byte index",
                                                    s.c_str(), lastName.c_str());
                }

                const TypeDesc *arrayElemTD = currentTD->getPointedTypeDesc();
                size_t arrayElemSize = TranslationUnit::instance().getTypeSize(*arrayElemTD);
                currentTD = arrayElemTD;
                
                byteOffsetIntoVar += int16_t(intValue * arrayElemSize);
            }
        }
        else  // variable name or struct field name
        {
            lastName = s;

            if (!currentTD->isStruct())
            {
                if (errorMessageTree)
                    errorMessageTree->errormsg("referring to field `%s' of non-struct expression", s.c_str());
                return -2;
            }
            const ClassDef *cl = TranslationUnit::instance().getClassDef(currentTD->className);
            if (!cl)
            {
                if (errorMessageTree)
                    errorMessageTree->errormsg("unknown struct or union `%s'", currentTD->className.c_str());
                return -2;
            }
            const ClassDef::ClassMember *member = NULL;
            int16_t memberOffset = cl->getDataMemberOffset(s, member);
            if (memberOffset < 0)
            {
                if (errorMessageTree)
                    errorMessageTree->errormsg("struct or union `%s' has no field named `%s'",
                                            cl->getName().c_str(), s.c_str());
                return -2;
            }
            byteOffsetIntoVar += memberOffset;
            currentTD = member->getTypeDesc();
        }
    }

    return 0;
}


// Processes assembly language text.
// Resolves each 3rd column name to a program variable, if any.
// If a name in the 3rd column is not a program variable, it is left as is,
// without error or warning.
// Calls FunctionDef::setCalled() on each recognized function referenced by the text.
//
// Stores the resulting text in resolvedAsmText,.
//
// text: Must not contain comments.
// scope: Scope object to use to resolve variable names.
// recognizedVarNames: If not null, the set accumulates the names of declared
//                     C variables that were referred to in 'text'.
//                     (Does not accumulate enumerated names.)
// unrecognizedNames: If not null, accumulates the names that refer to undeclared
//                    C variables or enumerators.
// requireAllocatedVariables: Require the variables used by the assembly code
//                            to have received a valid frame displacement.
//                            Passing false is useful when the caller only wants
//                            wants the referenced variable names or register
//                            the called functions.
// errorMessageTree: If not null, error messages are issued by calling errormsg() on this.
//
bool
AssemblerStmt::processInlineAsmText(const string &text,
                                    const Scope &scope,
                                    string &resolvedAsmText,
                                    set<string> *recognizedVarNames,
                                    set<string> *unrecognizedNames,
                                    bool requireAllocatedVariables,
                                    const Tree *errorMessageTree)
{
    string result;
    result.reserve(text.length() * 2);

    size_t colNum = 1;  // 1 = label column, 2 = instr. col., 3 = arg. col.
    Token tok = END;
    string tokenText, lastCol2;
    tokenText.reserve(256);
    lastCol2.reserve(256);
    size_t i = 0;  // offset in 'text'
    bool currentInstructionCanRefVariables = false;
    bool varRefRequiresEscapeChar = false;

    while ((tok = getToken(text, i, tokenText)) != END)
    {
        switch (tok)
        {
        case NEWLINE:
            colNum = 1;
            result += tokenText;
            varRefRequiresEscapeChar = false;  // this setting is specific to each line
            break;

        case WHITESPACE:
            ++colNum;
            result += tokenText;
            break;

        case WORD:
            if (colNum == 3 && tokenText == "#")  // if immediate mode arg, rest of arg only allowed to refer to C var/enum with ':' prefix
            {
                // With this, LDD #foo will only refer to asm label foo, because referring to C var foo
                // would require using ":foo".
                // However, an immediate arg can still refer to a C enum as long as it uses a colon:
                //   enum { FOO = 42 };
                //   asm { LDB #1+:FOO*2 }
                //
                varRefRequiresEscapeChar = true;
            }

            if (colNum == 3 && currentInstructionCanRefVariables)  // if instruction argument that could refer to variable or enum
            {
                int16_t offset = 0;  // offset into the C variable, if tokenText refers to a C variable
                bool escapeCharUsed = (tokenText.length() > 0 && tokenText[0] == variableNameEscapeChar);
                string unescapedVarRef = (escapeCharUsed ? string(tokenText, 1) : tokenText);
                const Declaration *variableDecl = NULL;
                int code = -1;

                // Take unescapedVarRef as a reference to a C variable/enum UNLESS:
                // - it is a CPU register name (a, b, x, etc.), or
                // - it must be preceded by a ':' but is not.
                //
                if (!isRegisterName(tokenText) && (!varRefRequiresEscapeChar || escapeCharUsed))
                {
                    code = parseCVariableRefAndByteIndex(unescapedVarRef, scope, errorMessageTree, variableDecl, offset);
                    if (code == -2)  // if error message issued re: bad C var ref
                        return false;
                }
                bool gotVariableAndOffset = (code == 0);

                uint16_t enumValue = 0;

                if (!gotVariableAndOffset && isRegisterName(tokenText))
                {
                    result += tokenText;  // no substitution allowed on register name
                }
                else if (variableDecl != NULL)  // if tokenText is a C ref var
                {
                    if (requireAllocatedVariables)
                        result += variableDecl->getFrameDisplacementArg(offset);

                    if (recognizedVarNames != NULL)
                        recognizedVarNames->insert(variableDecl->getVariableId());
                }
                else if (FunctionDef *fd = TranslationUnit::instance().getFunctionDef(tokenText))
                {
                    // Replace the function name with its assembly label.
                    //
                    result += fd->getLabel();
                    if (lastCol2 != "bsr" && lastCol2 != "lbsr"
                            && lastCol2 != "bra" && lastCol2 != "lbra"
                            && lastCol2 != "brn" && lastCol2 != "lbrn"
                            && ! ASMText::isConditionalBranch(lastCol2.c_str()))
                        result += ",pcr";

                    // Find which function is calling 'fd', i.e., the function containing the asm{}.
                    // First, go up the parents of 'scope' until a direct child of the global scope.
                    // This child is the scope of a function. Find the function whose scope this is.
                    //
                    const FunctionDef *caller = NULL;
                    const Scope &globalScope = TranslationUnit::instance().getGlobalScope();
                    const Scope *callerScope = &scope;
                    assert(callerScope);
                    if (callerScope != &globalScope)  // callerScope can be global scope if processing global asm{}
                    {
                        while (callerScope->getParent() != &globalScope)
                        {
                            callerScope = callerScope->getParent();
                            assert(callerScope);
                        }
                        caller = TranslationUnit::instance().getFunctionDefFromScope(*callerScope);
                        assert(caller);
                    }

                    // Register the function call.
                    //
                    fd->setCalled(); // make sure the code for 'fd' gets emitted
                    if (caller)
                        TranslationUnit::instance().registerFunctionCall(caller->getId(), fd->getId());
                }
                else if (TranslationUnit::getTypeManager().getEnumeratorValue(unescapedVarRef, enumValue))
                {
                    result += wordToString(enumValue);
                }
                else if (escapeCharUsed && !variableDecl)
                {
                    if (errorMessageTree)
                        errorMessageTree->errormsg("undeclared identifier `%s' in assembly language statement",
                                                    unescapedVarRef.c_str());
                }
                else  // no match: keep text as is
                {
                    result += tokenText;

                    if (escapeCharUsed && unrecognizedNames != NULL)
                        unrecognizedNames->insert(unescapedVarRef);
                }
            }
            else
            {
                result += tokenText;

                if (colNum == 2)  // if instruction field
                {
                    stringToLower(tokenText);
                    lastCol2 = tokenText;

                    // A push/pull instruction's argument must never be interpreted as
                    // a C variable name. A C function could have an argument called 'x'
                    // but "pshs x" still refers to register X.
                    //
                    currentInstructionCanRefVariables = (tokenText != "pshs"
                                                      && tokenText != "puls"
                                                      && tokenText != "pshu"
                                                      && tokenText != "pulu"
                                                      && tokenText != "tfr"
                                                      && tokenText != "exg");
                }
            }
            break;

        case END:
        default:
            assert(!"unexpected");
        }
    }

    resolvedAsmText = result;
    return true;
}


bool
AssemblerStmt::isGlobalVariable(const string &varName)
{
    const Scope &globalScope = TranslationUnit::instance().getGlobalScope();
    const Declaration *varDecl = globalScope.getVariableDeclaration(varName, false);
    return varDecl != NULL;
}


/*virtual*/
void
AssemblerStmt::checkSemantics(Functor &f)
{
    // Get the parent function.
    //
    SemanticsChecker &checker = dynamic_cast<SemanticsChecker &>(f);
    const FunctionDef *parentFunctionDef = checker.getCurrentFunctionDef();
    assert(parentFunctionDef == NULL || parentFunctionDef->getScope() != NULL);  // asm{} can appear at global scope

    if (asmText.empty())  // if single-instruction asm() directive
    {
        if (parentFunctionDef && parentFunctionDef->isAssemblyOnly() && argIsVariable && !isGlobalVariable(argument))
            errormsg("assembly-only function refers to local C variable `%s'", argument.c_str());
    }
    else  // if multi-line assembly language text
    {
        // Resolve variable (and function) references in the text.
        // This is necessary to register function calls made in that text
        // (via FunctionDef::setCalled() and TranslationUnit::registerFunctionCall()).
        //
        set<string> recognizedVarNames;
        const Scope *sc = (scopeOfAsmOnlyFunction ? scopeOfAsmOnlyFunction : TranslationUnit::instance().getCurrentScope());
        assert(sc);
        string resolvedAsmText;
        (void) processInlineAsmText(removeComments(asmText), *sc, resolvedAsmText, &recognizedVarNames, NULL, false);

        // An assembly-only function is not allowed to refer to local C variables because such a function
        // has no stack frame. (It is allowed to call functions however.)
        //
        if (parentFunctionDef && parentFunctionDef->isAssemblyOnly() && recognizedVarNames.size() > 0)
        {
            // Create and issue an error message.
            stringstream varNames;
            size_t numLocals = 0;
            for (set<string>::const_iterator it = recognizedVarNames.begin(); it != recognizedVarNames.end(); ++it)
            {
                const string &varName = *it;
                if (isGlobalVariable(varName))
                    continue;  // allow asm{} text to refer to globals
                if (numLocals > 0)
                    varNames << ", ";
                varNames << "`" << varName << "'";
                ++numLocals;
            }
            if (numLocals > 0)
                errormsg("assembly-only function refers to local C variable%s %s",
                         numLocals > 1 ? "s" : "", varNames.str().c_str());
        }
    }
}


void
AssemblerStmt::getAllVariableNames(set<string> &varNames) const
{
    set<string> unrecognizedNames;
    const Scope *sc = (scopeOfAsmOnlyFunction ? scopeOfAsmOnlyFunction : TranslationUnit::instance().getCurrentScope());
    assert(sc);
    string resolvedAsmText;
    (void) processInlineAsmText(removeComments(asmText), *sc, resolvedAsmText, &varNames, &unrecognizedNames, false);
    varNames.insert(unrecognizedNames.begin(), unrecognizedNames.end());
}


/*virtual*/
CodeStatus
AssemblerStmt::emitCode(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;

    if (!asmText.empty())  // if multi-line assembly language text instead of single instruction:
    {
        writeLineNoComment(out, "inline assembly");

        const Scope *cs = TranslationUnit::instance().getCurrentScope();
        assert(cs);
        string resolvedAsmText;
        if (!processInlineAsmText(removeComments(asmText), *cs, resolvedAsmText, NULL, NULL, true, this))
            return false;
        out.emitInlineAssembly(resolvedAsmText);
        return true;
    }

    Declaration *variableDecl = NULL;
    if (argIsVariable)
    {
        Scope *cs = TranslationUnit::instance().getCurrentScope();
        variableDecl = cs->getVariableDeclaration(argument, true);

        if (variableDecl == NULL)
            errormsg("undeclared variable `%s' used in inline assembler statement",
                        argument.c_str());
    }

    string text = "\t" + instruction + "\t"
                       + (variableDecl ? variableDecl->getFrameDisplacementArg() : argument)
                       + "\t" + getLineNo();
    if (variableDecl)
        text += " (re: variable " + argument + ")";
    out.emitInlineAssembly(text);

    return true;
}
