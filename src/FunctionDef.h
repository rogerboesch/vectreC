/*  $Id: FunctionDef.h,v 1.27 2019/06/22 05:16:58 sarrazip Exp $

    CMOC - A C-like cross-compiler
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

#ifndef _H_FunctionDef
#define _H_FunctionDef

#include "FormalParamList.h"

class DeclarationSpecifierList;
class Declaration;
class Declarator;


// The return type of a function is the type of the Tree base object,
// i.e., it is obtained by calling getTypeDesc() on the FunctionDef.
//
class FunctionDef : public Tree
{
public:

    // Takes ownership of the FormalParamList of 'declarator', if any.
    // 'declarator' loses its FormalParamList in such a case.
    //
    FunctionDef(const DeclarationSpecifierList &dsl,
                Declarator &declarator);

    // Calls delete on the FormalParamList pointer received for the constructor, if any.
    //
    virtual ~FunctionDef();
    
    // If this FunctionDef already has a body, this method calls delete on 'body'
    // and issues a compiler error.
    // Otherwise, this FunctionDef becomes owner of the TreeSequence object,
    // and the FunctionDef destructor will call delete on it.
    // Does nothing if 'body' is null.
    //
    void setBody(TreeSequence *body);

    const TreeSequence *getBody() const;
    TreeSequence *getBody();

    std::string getId() const;
    std::string getLabel() const;
    std::string getEndLabel() const;

    bool hasSameReturnType(const FunctionDef &fd) const;
    bool hasSameFormalParams(const FunctionDef &fd) const;
    size_t getNumFormalParams() const;

    // Mark this function as called, for the purposes of not emitting code
    // for functions that are defined but not called.
    //
    void setCalled();

    // Indicates if this function is considered to be called at least once,
    // possibly through a function pointer.
    //
    bool isCalled() const;

    virtual void checkSemantics(Functor &f);

    // Must be called before calling emitCode().
    // Must only be called once.
    //
    void allocateLocalVariables();

    // declareFormalParams() and allocateLocalVariables() must have been called.
    //
    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    // May return NULL.
    const FormalParamList *getFormalParamList() const;

    bool isInterruptServiceRoutine() const;

    bool isFunctionReceivingFirstParamInReg() const;

    bool isAssemblyOnly() const;

    bool hasInternalLinkage() const;

    // Returns true if numArguments is exactly the number of formal parameters,
    // in the case of a non-variadic function, or if numArguments is at least
    // the number of named formal parameters, in the case of a variadic function
    // (one whose format parameter list ends with an ellipsis).
    //
    bool isAcceptableNumberOfArguments(size_t numArguments) const;

    // Returns an empty string if not found.
    //
    std::string findAssemblyLabelFromIDLabeledStatement(const std::string &id) const;

    virtual bool isLValue() const { return false; }

    // Returns an instruction argument.
    // Only relevant when a function receives a hidden parameter that points
    // to where the return value must be stored.
    //
    std::string getAddressOfReturnValue() const;

    // Number of bytes that a function is expected to use in addition to its local variables.
    // Useful when targeting OS-9.
    //
    static uint16_t getFunctionStackSpace();

    static void setFunctionStackSpace(uint16_t numBytes);

private:
    // Forbidden:
    FunctionDef(const FunctionDef &);
    FunctionDef &operator = (const FunctionDef &);

private:

    void declareFormalParams();
    bool hasHiddenParam() const;

private:

    std::string functionId;
    FormalParamList *formalParamList;  // null in erroneous case like "int f {}"; owned by this object; must come from new
    std::string functionLabel;
    std::string endLabel;
    TreeSequence *bodyStmts;  // owns the pointed object
    std::vector<Declaration *> formalParamDeclarations;  // owns the pointed objects
    Declaration *hiddenParamDeclaration;  // non null when hidden param received in reg but spilled in stack
    size_t numLocalVariablesAllocated;
    int16_t minDisplacement;  // set by allocateLocalVariables()
    bool isISR;
    bool isStatic;
    bool asmOnly;
    bool noReturnInstruction;
    bool called;  // true means at least one call or address-of seen on this function
    bool firstParamReceivedInReg;

    static uint16_t functionStackSpace;  // in bytes; 0 means no stack check

};


#endif  /* _H_FunctionDef */
