/*  CMOC - A C-like cross-compiler
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

#include "TranslationUnit.h"

#include "Parameters.h"
#include "TreeSequence.h"
#include "DeclarationSequence.h"
#include "FunctionDef.h"
#include "SemanticsChecker.h"
#include "AncestorFunctor.h"
#include "FunctionCallExpr.h"
#include "Scope.h"
#include "StringLiteralExpr.h"
#include "RealConstantExpr.h"
#include "DWordConstantExpr.h"
#include "ClassDef.h"
#include "Pragma.h"
#include "IdentifierExpr.h"
#include "VariableExpr.h"
#include "SwitchStmt.h"
#include "LabeledStmt.h"
#include "AssemblerStmt.h"
#include "WhileStmt.h"
#include "ForStmt.h"
#include "JumpStmt.h"
#include "ExpressionTypeSetter.h"

#include <errno.h>

using namespace std;


/*static*/ TranslationUnit *TranslationUnit::theInstance = NULL;


TranslationUnit::TranslationUnit(const Parameters &_params)
  : params(_params),
    typeManager(),
    globalScope(NULL),
    definitionList(NULL),
    functionDefs(),
    callGraph(),
    globalVariables(),
    scopeStack(),
    breakableStack(),
    functionEndLabel(),
    labelGeneratorIndex(0),
    callConventionStack(),
    stringLiteralLabelToValue(),
    stringLiteralValueToLabel(),
    realConstantLabelToValue(),
    realConstantValueToLabel(),
    dwordConstantLabelToValue(),
    dwordConstantValueToLabel(),
    builtInFunctionDescs(),
    warnedAboutUnsupportedFloats(false),
    warnedAboutVolatile(false),
    neededUtilitySubRoutines(),
    vxTitle("CMOC"),
    vxMusic("vx_music_1"),
    vxTitleSizeWidth(80),
    vxTitleSizeHeight(-8),
    vxTitlePosX(-0x56),
    vxTitlePosY(0x20),
    vxCopyright("2015"),
    prerequisiteFilenamesSeen()
{
    assert(theInstance == NULL);
    theInstance = this;  // instance() needed by Scope constructor
    typeManager.createBasicTypes();
    globalScope = new Scope(NULL, string());  // requires 'void', i.e., must come after createBasicTypes()
    typeManager.createInternalStructs(*globalScope, params.targetPlatform, params.floatingPointLibrary);  // global scope receives internal structs

    //typeManager.dumpTypes(cout); // dumps all predefined types in C notation
}


void
TranslationUnit::setDefinitionList(TreeSequence *defList)
{
    delete definitionList;
    definitionList = defList;
}


void
TranslationUnit::addFunctionDef(FunctionDef *fd)
{
    if (fd == NULL)
        return;
    assert(fd->getTypeDesc()->isPtrToFunction() || ! fd->getTypeDesc()->isTypeWithCallingConventionFlags());
    functionDefs[fd->getId()] = fd;
}


void
TranslationUnit::removeFunctionDef(FunctionDef *fd)
{
    if (fd == NULL)
        return;
    functionDefs.erase(fd->getId());
}


// Adjusts 'fd' so that it has the same interrupt and calling convention as 'preexistingFD'.
// They are assumed to have the same formal parameters.
// Returns true if the return type of 'fd' becomes equal to that of 'preexistingFD'
// after making this adjustment.
//
bool
TranslationUnit::adjustFunctionDefReturnTypeFromPrototype(FunctionDef &fd,
                                                          FunctionDef &preexistingFD)
{
    int result = TypeDesc::compare(*preexistingFD.getTypeDesc(), *fd.getTypeDesc());
    if (result == -1)
        return false;  // difference other than interrupt or call conv.: reject

    // Difference, but only on interrupt or call conv. modifiers.
    // Give each FunctionDef the attributes that the other has.
    // However, reject if there are two non-default calling conventions.
    //
    if (fd.isInterruptServiceRoutine() != preexistingFD.isInterruptServiceRoutine())
    {
        fd.setInterruptServiceRoutine(true);
        preexistingFD.setInterruptServiceRoutine(true);
    }
    if (fd.getCallConvention() != preexistingFD.getCallConvention())
    {
        if (fd.getCallConvention() != DEFAULT_CMOC_CALL_CONV
                && preexistingFD.getCallConvention() != DEFAULT_CMOC_CALL_CONV)
            return false;

        if (fd.getCallConvention() == DEFAULT_CMOC_CALL_CONV)
            fd.setCallConvention(preexistingFD.getCallConvention());
        else
            preexistingFD.setCallConvention(fd.getCallConvention());
    }

    assert(preexistingFD.hasSameReturnTypeAndModifiers(fd));
    return true;
}


// The FunctionDef may be modified.
//
void
TranslationUnit::registerFunction(FunctionDef *fd)
{
    if (fd == NULL)
        return;

    if (fd->getFormalParamList() == NULL)
    {
        fd->errormsg("function %s() has no formal parameter list", fd->getId().c_str());
        return;
    }

    FunctionDef *preexisting = getFunctionDef(fd->getId());
    if (preexisting != NULL)
    {
        bool sameRetType = preexisting->hasSameReturnTypeAndModifiers(*fd);
        bool sameParams  = preexisting->hasSameFormalParams(*fd);
        if (!sameRetType || !sameParams)
        {
            const char *msg = NULL, *be = "are";
            if (!sameRetType && !sameParams)
                msg = "return type and formal parameters";
            else if (!sameRetType)
            {
                if (!adjustFunctionDefReturnTypeFromPrototype(*fd, *preexisting))
                    msg = "return type", be = "is";
            }
            else if (!sameParams)
            {
                const FormalParamList *preexistingFPList = preexisting->getFormalParamList();
                if (preexistingFPList && preexistingFPList->size() == 0)
                {
                    // No error because we tolerate this, to compile old K&R programs:
                    // int foo();
                    // int foo(int x) { ... }
                }
                else
                    msg = "formal parameters";
            }

            if (msg)
                fd->errormsg("%s for %s() %s different from previously declared at %s: `%s' vs `%s'",
                         msg, fd->getId().c_str(), be, preexisting->getLineNo().c_str(),
                         fd->getPrototypeWithFunctionId().c_str(),
                         preexisting->getPrototypeWithFunctionId().c_str());

        }
        if (preexisting->getBody() != NULL && fd->getBody() != NULL)
            fd->errormsg("%s() already has a body at %s",
                         fd->getId().c_str(), preexisting->getLineNo().c_str());
        if (preexisting->getBody() == NULL && fd->getBody() != NULL)  // if have prototype but now have body
        {
            removeFunctionDef(preexisting);
            addFunctionDef(fd);
        }
        return;
    }

    if (fd->getId() == "main")
    {
        const TypeDesc *returnType = fd->getTypeDesc();
        assert(returnType->isValid());
        if (returnType->type != WORD_TYPE || !returnType->isSigned)
            fd->warnmsg("return type of main() must be int");
        if (fd->getNumFormalParams() != 0)
        {
            if (params.targetPlatform == OS9 || params.targetPlatform == FLEX)
            {
                // Check that main() receives the expected parameters.
                bool ok = true;
                if (fd->getNumFormalParams() != 2)
                    ok = false;
                else
                {
                    const FormalParamList *params = fd->getFormalParamList();
                    vector<Tree *>::const_iterator it = params->begin();
                    const TypeDesc *firstParamTD = (*it)->getTypeDesc();
                    ++it;
                    const TypeDesc *secondParamTD = (*it)->getTypeDesc();
                    if (firstParamTD->type != WORD_TYPE || ! firstParamTD->isSigned
                        || (secondParamTD->type != POINTER_TYPE && secondParamTD->type != ARRAY_TYPE)
                        || secondParamTD->pointedTypeDesc->type != POINTER_TYPE
                        || secondParamTD->pointedTypeDesc->pointedTypeDesc->type != BYTE_TYPE
                        || ! secondParamTD->pointedTypeDesc->pointedTypeDesc->isSigned)
                    {
                        ok = false;
                    }
                }
                if (!ok)
                    fd->errormsg("main() must receive (int, char **)");
            }
            else
                fd->warnmsg("main() does not receive parameters when targeting this platform");
        }
    }

    addFunctionDef(fd);
}


void
TranslationUnit::registerFunctionCall(const string &callerId, const string &calleeId)
{
    pushBackUnique(callGraph[callerId], calleeId);
}


// Checks function prototypes, definitions and calls.
//
class FunctionChecker : public Tree::Functor
{
public:
    FunctionChecker(TranslationUnit &tu, bool _isCallToUndefinedFunctionAllowed)
    :   translationUnit(tu),
        declaredFunctions(), undefinedFunctions(), definedFunctions(), calledFunctions(),
        isCallToUndefinedFunctionAllowed(_isCallToUndefinedFunctionAllowed)
    {
    }
    virtual bool open(Tree *t) override
    {
        if (FunctionDef *fd = dynamic_cast<FunctionDef *>(t))
        {
            processFunctionDef(fd);
        }
        else if (FunctionCallExpr *fc = dynamic_cast<FunctionCallExpr *>(t))
        {
            if (!fc->isCallThroughPointer())
            {
                string funcId = fc->getIdentifier();
                calledFunctions.insert(funcId);
                if (declaredFunctions.find(funcId) == declaredFunctions.end())  // if unknown ID
                    fc->warnmsg("calling undeclared function %s()", funcId.c_str());
            }
        }
        return true;
    }
    virtual bool close(Tree * /*t*/) override
    {
        return true;
    }
    void reportErrors() const
    {
        for (map<string, const FunctionDef *>::const_iterator it = undefinedFunctions.begin();
                                                             it != undefinedFunctions.end(); ++it)
        {
            const string& funcId = it->first;

            // Import the function name (so that another module can provide the function body).
            //
            if (calledFunctions.find(funcId) != calledFunctions.end())
                translationUnit.registerNeededUtility("_" + funcId);
        }
    }
private:
    void processFunctionDef(FunctionDef *fd)
    {
        string funcId = fd->getId();
        declaredFunctions.insert(funcId);

        if (fd->getBody() == NULL)  // if prototype:
        {
            if (definedFunctions.find(funcId) == definedFunctions.end())  // if body not seen:
                undefinedFunctions[funcId] = fd;  // remember that this function is declared and might lack a definition
        }
        else  // if body:
        {
            definedFunctions.insert(funcId);  // remember that this function is defined
            map<string, const FunctionDef *>::iterator it = undefinedFunctions.find(funcId);
            if (it != undefinedFunctions.end())  // if prototype seen:
                undefinedFunctions.erase(it);  // not undefined anymore
        }
    }

    TranslationUnit &translationUnit;
    set<string> declaredFunctions;
    map<string, const FunctionDef *> undefinedFunctions;
    set<string> definedFunctions;
    set<string> calledFunctions;
    bool isCallToUndefinedFunctionAllowed;
};


Register
TranslationUnit::getFirstParameterRegister(bool isParam8Bits) const
{
    return isParam8Bits ? B : X;
}

void
TranslationUnit::pushCurrentDefaultCallConvention(CallConvention callConv)
{
    callConventionStack.push_back(callConv);
}


CallConvention
TranslationUnit::popCurrentDefaultCallConvention()
{
    if (callConventionStack.size() == 0)
        return DEFAULT_CMOC_CALL_CONV;
    CallConvention result = callConventionStack.back();
    callConventionStack.pop_back();
    return result;
}


CallConvention
TranslationUnit::getCurrentDefaultCallConvention() const
{
    if (callConventionStack.size() == 0)
        return DEFAULT_CMOC_CALL_CONV;
    return callConventionStack.back();
}


// Checks for labeled statements used outside of a switch() statement.
//
class LabeledStmtChecker : public Tree::Functor
{
public:
    LabeledStmtChecker()
    :   Tree::Functor(),
        switchLevel(0)
    {
    }
    virtual bool open(Tree *t) override
    {
        if (dynamic_cast<SwitchStmt *>(t))
            ++switchLevel;
        const LabeledStmt *ls = NULL;
        if (switchLevel == 0 && (ls = dynamic_cast<LabeledStmt *>(t)) && ls->isCaseOrDefault())
            t->errormsg("%s label not within a switch statement", ls->isCase() ? "case" : "default");
        return true;
    }
    virtual bool close(Tree *t) override
    {
        if (dynamic_cast<SwitchStmt *>(t))
            --switchLevel;
        return true;
    }
private:
    size_t switchLevel;
};


// Checks that 'continue' is only used in a loop and that 'break'
// is only used in a loop or a switch.
//
class BreakAndContinueChecker : public Tree::Functor
{
public:
    BreakAndContinueChecker()
    :   Tree::Functor(),
        loopLevel(0),
        switchLevel(0)
    {
    }

    bool open(Tree *t) override
    {
        if (dynamic_cast<WhileStmt *>(t) || dynamic_cast<ForStmt *>(t))  // if while, do-while or for
            ++loopLevel;
        else if (dynamic_cast<SwitchStmt *>(t))
            ++switchLevel;
        else if (auto *js = dynamic_cast<JumpStmt *>(t))
        {
            switch (js->getJumpType())
            {
            case JumpStmt::CONT:
                if (loopLevel == 0)
                    t->errormsg("continue statement not within a loop");
                break;
            case JumpStmt::BRK:
                if (loopLevel == 0 && switchLevel == 0)
                    t->errormsg("break statement not within loop or switch");
                break;
            default:
                ;
            }
        }
        return true;
    }

    bool close(Tree *t) override
    {
        if (dynamic_cast<WhileStmt *>(t) || dynamic_cast<ForStmt *>(t))
            --loopLevel;
        else if (dynamic_cast<SwitchStmt *>(t))
            --switchLevel;
        return true;
    }

private:
    size_t loopLevel;
    size_t switchLevel;
};


// Functor that checks that all members of a struct or union are of a defined type,
// e.g., detect struct A { struct B b; } where 'B' is not defined.
//
struct ClassChecker
{
    bool operator()(const ClassDef &cl)
    {
        for (size_t i = 0; i < cl.getNumDataMembers(); ++i)
        {
            const ClassDef::ClassMember *member = cl.getDataMember(i);
            if (member == NULL)
                continue;  // unexpected
            const TypeDesc *memberTD = member->getTypeDesc();
            if (memberTD->type == CLASS_TYPE
                    && ! TranslationUnit::instance().getClassDef(memberTD->className))
            {
                member->errormsg("member `%s' of `%s' is of undefined type `%s'",
                                member->getName().c_str(), cl.getName().c_str(), memberTD->className.c_str());
            }
        }
        return true;
    }
};


// Detects the use of a global variable before its declarator has been seen.
//
class UndeclaredGlobalVariableChecker : public Tree::Functor
{
public:
    UndeclaredGlobalVariableChecker() : globalsEncountered() {}

    virtual bool open(Tree *t) override
    {
        if (const Declaration *decl = dynamic_cast<Declaration *>(t))
        {
            if (decl->isGlobal())
            {
                assert(!decl->getVariableId().empty());
                globalsEncountered.insert(decl->getVariableId());
            }
        }
        else if (const IdentifierExpr *ie = dynamic_cast<IdentifierExpr *>(t))
        {
            const VariableExpr *ve = ie->getVariableExpr();
            if (ve && !ve->isFuncAddrExpr())
            {
                decl = ve->getDeclaration();
                assert(decl);
                if (decl->isGlobal())
                {
                    const string &id = ve->getId();
                    if (globalsEncountered.find(id) == globalsEncountered.end())
                        ie->errormsg("global variable `%s' undeclared", id.c_str());
                }
            }

        }
        return true;
    }

private:
    std::set<std::string> globalsEncountered;
};


// Search for string literals and register them in the TranslationUnit,
// to give them an assembly label.
// Also register the names of functions where __FUNCTION__ or __func__ is used.
//
class StringLiteralRegistererererer : public Tree::Functor
{
public:
    StringLiteralRegistererererer() : currentFunctionDef(NULL) {}
    virtual bool open(Tree *t) override
    {
        if (StringLiteralExpr *sle = dynamic_cast<StringLiteralExpr *>(t))
        {
            sle->setLabel(TranslationUnit::instance().registerStringLiteral(*sle));
            return true;
        }
        if (FunctionDef *fd = dynamic_cast<FunctionDef *>(t))
        {
            currentFunctionDef = fd;
            return true;
        }
        if (IdentifierExpr *ie = dynamic_cast<IdentifierExpr *>(t))
        {
            string id = ie->getId();
            if (id == "__FUNCTION__" || id == "__func__")
            {
                string literal;
                if (currentFunctionDef)
                    literal = currentFunctionDef->getId();
                StringLiteralExpr *sle = ie->setFunctionNameStringLiteral(literal);
                sle->setLabel(TranslationUnit::instance().registerStringLiteral(*sle));
            }
            return true;
        }
        return true;
    }
    virtual bool close(Tree *t) override
    {
        if (currentFunctionDef == t)
            currentFunctionDef = NULL;
        return true;
    }
private:
    FunctionDef *currentFunctionDef;

    StringLiteralRegistererererer(const StringLiteralRegistererererer &);
    StringLiteralRegistererererer &operator = (const StringLiteralRegistererererer &);
};


class DeclarationFinisher : public Tree::Functor
{
public:
    DeclarationFinisher() {}
    virtual bool open(Tree *t) override
    {
        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(t);
        if (declSeq)
        {
            // Set the type of the expression used by enumerators, if any, e.g., enum { A = sizeof(v) }.
            //
            std::vector<Enumerator *> *enumeratorList = declSeq->getEnumeratorList();
            if (enumeratorList)  // if enum { ... }, with or without declarators
            {
                for (size_t i = 0; i < enumeratorList->size(); ++i)  // for each name in enum { ... }
                {
                    const Enumerator &enumerator = *(*enumeratorList)[i]; // e.g., A = 42 or B = sizeof(v)
                    Tree *valueExpr = enumerator.valueExpr;  // e.g., 42 or sizeof(v) or null if no expr
                    if (valueExpr)
                    {
                        ExpressionTypeSetter ets;
                        valueExpr->iterate(ets);

                        // valueExpr->getType() will still be VOID_TYPE here in the case where
                        // valueExpr contains undefined enumerated names, e.g.,
                        // enum { B = A + 1 }, with A undefined. Since we know it is an
                        // enumerated name, we use int as a fallback.
                        //
                        if (valueExpr->getType() == VOID_TYPE)
                            valueExpr->setTypeDesc(TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true));
                    }
                }
            }
            return true;
        }

        Declaration *decl = dynamic_cast<Declaration *>(t);
        if (!decl)
            return true;
        /*cout << "# DeclarationFinisher: decl " << decl << ": '" << decl->getVariableId()
             << "', isGlobal=" << decl->isGlobal() << ", needsFinish=" << decl->needsFinish
             << ", line " << decl->getLineNo() << endl;*/
        if (!decl->needsFinish)
            return true;

        const TypeDesc *varTypeDesc = decl->getTypeDesc();

        vector<uint16_t> arrayDimensions;

        // arrayDimensions will contain dimensions from varTypeDesc as well as from this declarator.
        // Example: typedef char A[10]; A v[5];
        // varTypeDesc is A, which is char[10], and this declarator contains dimension 5.
        // We start arrayDimensions with the 5, because the type of v is as if v had been
        // declared as char v[5][10].

        /*cout << "# DeclarationFinisher:   arraySizeExprList=[" << vectorToString(decl->arraySizeExprList) << "]"
             << ", varTypeDesc=" << varTypeDesc << " '" << varTypeDesc->toString() << "'\n";*/
        if (decl->arraySizeExprList.size() > 0)
        {
            if (!Declarator::computeArrayDimensions(arrayDimensions,
                                                    decl->isExtern,
                                                    decl->arraySizeExprList,
                                                    decl->getVariableId(),
                                                    decl->initializationExpr,
                                                    decl))
                return true;
        }

        size_t numDimsDueToDeclarator = arrayDimensions.size();
        /*cout << "# DeclarationFinisher:   arrayDimensions=[" << vectorToString(arrayDimensions)
                                << "], numDimsDueToDeclarator=" << numDimsDueToDeclarator << endl;*/

        if (varTypeDesc->type == ARRAY_TYPE)
        {
            // varTypeDesc may contain dimensions, if the variable is being declared using
            // a typedef of an array, e.g.,
            //   typedef A a[2][3];
            //   A someVar;
            // Then varTypeDesc->appendDimensions() will put 2 and 3 in arrayDimensions.
            // In the case of
            //   A someArray[4][5];
            // then the 4 and 5 are in decl->arraySizeExprList, and it is the call to
            // computeArrayDimensions() that will have inserted 4 and 5 in arrayDimensions.
            //
            varTypeDesc->appendDimensions(arrayDimensions);
        }

        // Here, arrayDimensions is empty if non-array.

        const TypeDesc *finalTypeDesc = TranslationUnit::getTypeManager().getArrayOf(varTypeDesc, numDimsDueToDeclarator);

        decl->setTypeDesc(finalTypeDesc);
        decl->arrayDimensions = arrayDimensions;
        decl->needsFinish = false;

        /*cout << "# DeclarationFinisher:   final arrayDimensions=" << vectorToString(arrayDimensions) << ", finalTypeDesc="
             << finalTypeDesc << " '" << finalTypeDesc->toString() << "'\n";*/

        if (decl->isGlobal())
        {
            TranslationUnit::instance().declareGlobal(decl);
        }

        if (finalTypeDesc->type == VOID_TYPE)
            decl->errormsg("variable `%s` is declared with type void", decl->getVariableId().c_str());

        uint16_t sizeInBytes = uint16_t(-1);
        if (TranslationUnit::instance().warnArraySizeZero()
                    && finalTypeDesc->type == ARRAY_TYPE
                    && decl->getVariableSizeInBytes(sizeInBytes)
                    && sizeInBytes == 0)
            decl->warnmsg("array variable `%s` has size zero", decl->getVariableId().c_str());

        return true;
    }
private:
    DeclarationFinisher(const DeclarationFinisher &);
    DeclarationFinisher &operator = (const DeclarationFinisher &);
};


// Call setGlobal(true) on each Declaration at the global scope.
//
void
TranslationUnit::markGlobalDeclarations()
{
    assert(scopeStack.size() == 0);  // ensure current scope is global one
    assert(definitionList);
    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
    {
        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(*it);
        if (!declSeq)
            continue;

        for (std::vector<Tree *>::iterator jt = declSeq->begin(); jt != declSeq->end(); ++jt)
        {
            if (Declaration *decl = dynamic_cast<Declaration *>(*jt))
            {
                decl->setGlobal(true);
            }
        }
    }
}


void
TranslationUnit::setTypeDescOfGlobalDeclarationClasses()
{
    assert(scopeStack.size() == 0);  // ensure current scope is global one
    assert(definitionList);
    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
    {
        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(*it);
        if (!declSeq)
            continue;
        if (declSeq->getType() != CLASS_TYPE)
            continue;

        const string &className = declSeq->getTypeDesc()->className;
        assert(!className.empty());
        ClassDef *cl = getClassDef(className);
        if (cl)  // cl will be null in case like: struct T *f(); where struct T is left undefined
        {
            cl->setTypeDesc(declSeq->getTypeDesc());
            cl->check();  // do more checks on this struct
        }
    }
}


void
TranslationUnit::declareGlobal(Declaration *decl)
{
    assert(decl);
    assert(decl->isGlobal());
    if (!decl->isExtern)
        globalVariables.push_back(decl);

    /*cout << "# TranslationUnit::declareGlobal: globalScope->declareVariable() for "
            << decl->getVariableId() << endl;*/
    if (!globalScope->declareVariable(decl))
    {
        const Declaration *existingDecl = globalScope->getVariableDeclaration(decl->getVariableId(), false);
        assert(existingDecl);
        if (!decl->isExtern && !existingDecl->isExtern)
            decl->errormsg("global variable `%s' already declared at global scope at %s",
                        decl->getVariableId().c_str(), existingDecl->getLineNo().c_str());
        else if (decl->getTypeDesc() != existingDecl->getTypeDesc())
            decl->errormsg("global variable `%s' declared with type `%s' at `%s' but with type `%s' at `%s'",
                        decl->getVariableId().c_str(),
                        decl->getTypeDesc()->toString().c_str(),
                        decl->getLineNo().c_str(),
                        existingDecl->getTypeDesc()->toString().c_str(),
                        existingDecl->getLineNo().c_str());
    }
}


void
TranslationUnit::setGlobalDeclarationLabels()
{
    /*  Global variables are stored in globalVariables in the order in which
        they are declared. This is necessary to emit correct initializers.
        Example: int a = 42; int b = a;
    */
    assert(scopeStack.size() == 0);  // ensure current scope is global one
    assert(definitionList);
    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
    {
        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(*it);
        if (!declSeq)
            continue;

        for (std::vector<Tree *>::iterator jt = declSeq->begin(); jt != declSeq->end(); ++jt)
        {
            if (Declaration *decl = dynamic_cast<Declaration *>(*jt))
            {
                // Set the assembly label on this global variable.
                uint16_t size = 0;
                if (decl->needsFinish)
                    ;  // error message already issued
                else if (!decl->isExtern && !decl->getVariableSizeInBytes(size))
                    decl->errormsg("invalid dimensions for array `%s'", decl->getVariableId().c_str());
                else
                    decl->setLabelFromVariableId();
            }
        }
    }
}


void
TranslationUnit::declareFunctions()
{
    assert(scopeStack.size() == 0);  // ensure current scope is global one
    assert(definitionList);
    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
    {
        if (FunctionDef *fd = dynamic_cast<FunctionDef *>(*it))
        {
            #if 0
            cerr << "Registering function " << fd->getId() << " at "
                    << fd << " which has "
                    << (fd->getBody() ? "a" : "no") << " body\n";
            #endif
            registerFunction(fd);
            continue;
        }

        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(*it);
        if (declSeq != NULL)
        {
            for (std::vector<Tree *>::iterator jt = declSeq->begin(); jt != declSeq->end(); ++jt)
            {
                if (FunctionDef *fd = dynamic_cast<FunctionDef *>(*jt))
                {
                    assert(fd->getBody() == NULL);  // only prototype expected here, not function definition
                    registerFunction(fd);
                }
            }
        }
    }
}


// Functions that have a body must have received their Scope object.
//
void
TranslationUnit::declareFunctionLocalStaticVariables()
{
    if (definitionList == NULL)
        return;

    class SetLocalStaticLabelFunctor : public Scope::DeclarationFunctor
    {
        string currentFunctionId;
    public:
        SetLocalStaticLabelFunctor() : currentFunctionId() {}

        virtual bool operator()(Declaration &decl) override
        {
            if (decl.isLocalStatic())
            {
                if (decl.initializationExpr != NULL
                        && ! decl.initializationExpr->isStaticallyInitializable(*decl.getTypeDesc()))
                {
                    if (decl.initializationExpr->isStaticallyInitializable(*decl.getTypeDesc(), true))
                        decl.forceDynamicInitializer();  // string literals require run-time init; see Declaration::emitCode()
                    else
                        decl.errormsg("initializer for local static variable `%s' is not constant", decl.getVariableId().c_str());
                }
                
                decl.setLocalStaticLabel(currentFunctionId);
            }
            return true;
        }

        virtual ~SetLocalStaticLabelFunctor() {}

        void setCurrentFunctionId(const string &functionId) { currentFunctionId = functionId; }
    } functor;

    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
        if (FunctionDef *fd = dynamic_cast<FunctionDef *>(*it))
        {
            functor.setCurrentFunctionId(fd->getId());
            Scope *topFuncScope = fd->getScope();
            if (topFuncScope && ! topFuncScope->iterateDeclarations(functor, true))
            {
                assert(!"unexpected stop");
                break;
            }
        }
}



// This is the method where global variables get declared, with a call to Scope::declareVariable().
//
void
TranslationUnit::checkSemantics()
{
    if (!definitionList)
        return;

    markGlobalDeclarations();

    setTypeDescOfGlobalDeclarationClasses();

    // Finish Declaration objects that have been created by Declarator::declareVariable()
    // but that could not be initialized completely because that method is called
    // during parsing.
    // Also set the type of the expression used by enumerators, if any, e.g., enum { A = sizeof(v) }.
    {
        DeclarationFinisher df;
        definitionList->iterate(df);
    }

    vector<const Declaration *> constDataDeclarationsToCheck;
    constDataDeclarationsToCheck.reserve(32);

    setGlobalDeclarationLabels();

    declareFunctions();


    // Check that all members of structs and unions are of a defined type,
    // e.g., detect struct A { struct B b; } where 'B' is not defined.
    //
    ClassChecker cc;
    globalScope->forEachClassDef(cc);

    StringLiteralRegistererererer r;
    definitionList->iterate(r);

    // Among other things:
    // - The ExpressionTypeSetter is run over the function bodies during the following step.
    // - For each FunctionDef encountered, this creates a Scope for it and calls setScope() on the FunctionDef.
    {
        SemanticsChecker sc;
        definitionList->iterate(sc);
    }  // destroy ScopeCreator here so that it pops all scopes it pushed onto the TranslationUnit's stack

    // Detect expression statements whose final value is not needed in B or D, e.g., void f() { n++; }
    {
        AncestorFunctor af;
        definitionList->iterate(af);
    }

    // This sets the assembly language label of the local static variables of the functions.
    // This must be done after the functions scopes have been created.
    //
    declareFunctionLocalStaticVariables();


    {
        UndeclaredGlobalVariableChecker checker;
        definitionList->iterate(checker);
    }


    // Call setReadOnly() on global declarations that are suitable for the rodata section,
    // which may be in ROM.
    //
    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); it++)
    {
        DeclarationSequence *declSeq = dynamic_cast<DeclarationSequence *>(*it);
        if (declSeq == NULL)
            continue;
        for (std::vector<Tree *>::iterator it = declSeq->begin(); it != declSeq->end(); ++it)
        {
            Declaration *decl = dynamic_cast<Declaration *>(*it);
            if (decl == NULL)
                continue;
            determineIfGlobalIsReadOnly(*decl);
        }
    }

    // Same for local static declarations.
    {
        class DetermineIfLocalStaticIsReadOnlyFunctor : public Scope::DeclarationFunctor
        {
        public:
            virtual bool operator()(Declaration &decl) override
            {
                if (decl.isLocalStatic())
                    TranslationUnit::instance().determineIfGlobalIsReadOnly(decl);
                return true;
            }

            virtual ~DetermineIfLocalStaticIsReadOnlyFunctor() {}
        } functor;

        for (vector<Tree *>::iterator it = definitionList->begin();
                                      it != definitionList->end(); it++)
            if (FunctionDef *fd = dynamic_cast<FunctionDef *>(*it))
                fd->getScope()->iterateDeclarations(functor, true);
    }


    // Check function prototypes, definitions and calls.
    // The following step assumes that the ExpressionTypeSetter has been run
    // over the function bodies, so that function calls that use a function pointer
    // can be differentiated from standard calls.
    //
    FunctionChecker ufc(*this, params.callToUndefinedFunctionAllowed);
    definitionList->iterate(ufc);
    ufc.reportErrors();


    LabeledStmtChecker lsc;
    definitionList->iterate(lsc);


    BreakAndContinueChecker bcc;
    definitionList->iterate(bcc);
}


void
TranslationUnit::determineIfGlobalIsReadOnly(Declaration &decl) const
{
    // Determine if this declaration goes into the 'rodata' section, which may be in ROM,
    // in the case of a cartridge program.
    // This must be done after the SemanticsChecker pass, i.e., after the ExpressionTypeSetter.
    //
    bool typeCanGoInRO = decl.getTypeDesc()->canGoInReadOnlySection(isRelocatabilitySupported());
    bool initializerAllowsRO = (decl.isExtern || decl.hasOnlyNumericalLiteralInitValues())
                               && decl.getTypeDesc()->isConstant();
    decl.setReadOnly(typeCanGoInRO && initializerAllowsRO);

    if (decl.isReadOnly())
        checkConstDataDeclarationInitializer(decl);  // do check after ExpressionTypeSetter
}


// This method assumes that the ExpressionTypeSetter has been run.
// This ensures that an initializer like -1, which may be represented as a UnaryOpExpr,
// is typed as WORD_TYPE, for example.
//
void
TranslationUnit::checkConstDataDeclarationInitializer(const Declaration &decl) const
{
    if (decl.initializationExpr == NULL)
    {
        if (!decl.isExtern)
            decl.errormsg("global variable '%s' defined as constant but has no initializer",
                          decl.getVariableId().c_str());
    }
}


TargetPlatform
TranslationUnit::getTargetPlatform() const
{
    return params.targetPlatform;
}


FloatingPointLibrary
TranslationUnit::getFloatingPointLibrary() const
{
    return params.floatingPointLibrary;
}


bool
TranslationUnit::isCharSignedByDefault() const
{
    return params.isCharSignedByDefault;
}


bool
TranslationUnit::targetPlatformUsesY() const
{
    return getTargetPlatform() == OS9;
}


// Under OS-9, Y points to the current process's writable data segment,
// but read-only globals are still next to the code, thus PC-relative.
//
const char *
TranslationUnit::getDataIndexRegister(bool prefixWithComma, bool readOnly) const
{
    return (targetPlatformUsesY() && !readOnly ? ",Y" : ",PCR") + (prefixWithComma ? 0 : 1);
}


// String, long and float literals are always next to the code.
//
const char *
TranslationUnit::getLiteralIndexRegister(bool prefixWithComma) const
{
    return &",PCR"[prefixWithComma ? 0 : 1];
}


// Calls setCalled() on each function that is assumed to be called.
//
void TranslationUnit::detectCalledFunctions()
{
    // Accumulate a list of functions that are called directly or indirectly from
    // any function that has external linkage.
    //
    StringVector calledFunctionIds;
    calledFunctionIds.push_back("main");
    for (FunctionDefTable::const_iterator it = functionDefs.begin(); it != functionDefs.end(); ++it)
    {
        const FunctionDef *fd = it->second;
        if (fd->getBody() && !fd->hasInternalLinkage())
            calledFunctionIds.push_back(fd->getId());
    }

    size_t index = 0, initSize = 0;
    do
    {
        // Process each function in calledFunctionIds[] starting at 'index',
        // up to the current size of calledFunctionIds.
        //
        initSize = calledFunctionIds.size();

        for ( ; index < initSize; ++index)  // for each caller to process
        {
            const string callerId = calledFunctionIds[index];  // copied instead of using reference, b/c calledFunctionIds gets modified by this loop

            // Get the list of functions called by 'callerId'.
            //
            StringGraph::const_iterator it = callGraph.find(callerId);
            if (it == callGraph.end())
                continue;  // not expected

            // For each function called by 'callerId', add that function to
            // 'calledFunctionIds'. Note that 'size' marks the end of the
            // functions that were in this list before the for() loop.
            // Any new name added by pushBackUnique() will be beyond 'size'.
            //
            const StringVector &callees = it->second;
            for (StringVector::const_iterator jt = callees.begin(); jt != callees.end(); ++jt)
            {
                const string &calleeId = *jt;
                pushBackUnique(calledFunctionIds, calleeId);
            }
        }

        // If the preceding for() loop added new names to 'calledFunctionIds',
        // then we have new callers to process, so we do another iteration.
        // Note that 'index' is now 'initSize', which means the next iteration
        // will only process the newly added names.
        assert(index == initSize);

    } while (calledFunctionIds.size() > initSize);

    // 'calledFunctionIds' is now the total list of functions called from the entry point.
    // Mark each of them as called. Assembly code will be emitted for those functions.
    //
    for (StringVector::const_iterator jt = calledFunctionIds.begin(); jt != calledFunctionIds.end(); ++jt)
    {
        FunctionDef *fd = getFunctionDef(*jt);
        if (fd)
            fd->setCalled();
    }
}


void
TranslationUnit::allocateLocalVariables()
{
    for (map<string, FunctionDef *>::iterator kt = functionDefs.begin(); kt != functionDefs.end(); kt++)
        kt->second->allocateLocalVariables();
}


void
TranslationUnit::emitAssembler(ASMText &out, uint16_t dataAddress,
                               int32_t initialStackRegValue,
                               uint16_t stackSpace, uint16_t extraStackSpace,
                               bool emitBootLoaderMarker)
{
    detectCalledFunctions();

    out.emitComment("6809 assembly program generated by " + string(PACKAGE) + " " + string(VERSION));

    // Find the function named 'main':
    //
    map<string, FunctionDef *>::iterator kt = functionDefs.find("main");
    const FunctionDef *mainFunctionDef = (kt == functionDefs.end() ? NULL : kt->second);

    // Start the section that goes at the beginning of the binary if we have a main() function to generate.
    //
    bool needStartSection = (mainFunctionDef != NULL);
    if (needStartSection)
    {
        out.startSection("start");
    }

    if (params.targetPlatform == VECTREX && needStartSection)
    {
        out.emitComment("Vectrex header, positioned at address 0.");

        string resolvedMusicAddress = resolveVectrexMusicAddress(vxMusic);

        out.ins("FCC", "'g GCE " + vxCopyright + "'");
        out.ins("FCB", "$80");
        out.ins("FDB", resolvedMusicAddress);
        out.ins("FCB", int8ToString(vxTitleSizeHeight, true));
        out.ins("FCB", int8ToString(vxTitleSizeWidth, true));
        out.ins("FCB", int8ToString(vxTitlePosY, true));
        out.ins("FCB", int8ToString(vxTitlePosX, true));
        out.ins("FCC", "'" + vxTitle + "'");
        out.ins("FCB", "$80");
        out.ins("FCB", "0");
    }

    // If we are generating main(), also generate the program_start routine that calls main().
    //
    if (mainFunctionDef != NULL)
    {
        out.emitExport("program_start");
        assert(mainFunctionDef);
        out.emitImport(mainFunctionDef->getLabel().c_str());
        out.emitImport("INILIB");
        out.emitImport("_exit");
        out.emitLabel("program_start");

        if (params.targetPlatform == COCO_BASIC && emitBootLoaderMarker)
            out.ins("FCC", "\"OS\"", "marker for CoCo DECB DOS command");
    }

    if (mainFunctionDef != NULL)
    {
        // Start the program by initializing the global variables, then
        // jumping to the main() function's label.

        if (params.targetPlatform == OS9)
        {
            // OS-9 launches a process by passing it the start and end addresses of
            // its data segment in U and Y respectively. The OS9PREP transfers U (the start)
            // to Y because CMOC uses U to point to the current C function's stack frame.
            // Every reference to a writable global variable under OS-9 and CMOC thus has the form FOO,Y.
            // Variable FOO must have been declared with an RMB directive in a section
            // that starts with ORG 0. This way, FOO represents an offset from the
            // start of the data segment of the current process.
            // This convention is used by Declaration::getFrameDisplacementArg().
            //
            // CAUTION: All of the code emitted after the OS9PREP call must be careful
            //          to preserve Y.

            out.emitImport("OS9PREP");
            out.ins("LBSR", "OS9PREP", "init data segment; sets Y to data segment; parse cmd line");

            out.ins("PSHS", "X,B,A", "argc, argv for main()");
        }
        else
        {
            if (initialStackRegValue >= 0 && initialStackRegValue <= 0xFFFF)
                out.ins("LDS", "#" + wordToString(uint16_t(initialStackRegValue), true));
            out.ins("LDD", "#-" + wordToString(stackSpace, false), "stack space in bytes");
        }

        out.ins("LBSR", "INILIB", "initialize standard library and global variables");  // inits INISTK, for exit()
        if (mainFunctionDef != NULL)
        {
            out.ins("LBSR", mainFunctionDef->getLabel(), "call main()");

            out.emitLabel("main_return_address");  // useful when determining the call stack

            if (params.targetPlatform == OS9)
                out.ins("LEAS", "4,S", "discard argc, argv");
        }

        if (params.targetPlatform != VECTREX)
        {
            if (mainFunctionDef == NULL)
            {
                out.ins("CLRA", "", "no main() to call: use 0 as exit status");
                out.ins("CLRB");
            }
            out.ins("PSHS", "B,A", "send main() return value to exit()");
        }
        out.ins("LBSR", "_exit", "use LBSR to respect calling convention");
    }

    if (needStartSection)
    {
        out.endSection();
    }

    if (mainFunctionDef != NULL && params.targetPlatform == OS9 && extraStackSpace != 0)
    {
        out.startSection("__os9");
        out.emitInlineAssembly("stack\tEQU\t" + wordToString(extraStackSpace) + "\t""extra stack space");
        out.endSection();
    }

    out.startSection("code");

    // Find the global variables. Import the name when "extern". Export the non-static globals.
    //
    if (definitionList)
        for (vector<Tree *>::const_iterator it = definitionList->begin(); it != definitionList->end(); ++it)
        {
            if (const DeclarationSequence *ds = dynamic_cast<DeclarationSequence *>(*it))
            {
                for (vector<Tree *>::const_iterator jt = ds->begin(); jt != ds->end(); ++jt)
                {
                    if (const Declaration *decl = dynamic_cast<Declaration *>(*jt))
                    {
                        if (decl->isExtern)
                        {
                            assert(!decl->getLabel().empty());
                            out.emitImport(decl->getLabel().c_str());
                        }
                        else if (!decl->hasStaticKeyword)
                        {
                            assert(!decl->getLabel().empty());
                            out.emitExport(decl->getLabel().c_str());
                        }
                    }
                }
            }
        }

    // Generate code for each function that is called at least once
    // or has its address taken at least once (see calls to FunctionDef::setCalled()).

    if (definitionList)
    {
        // Import label of each non-static function prototype.
        for (auto it : functionDefs)
        {
            const FunctionDef *fd = it.second;
            if (fd->getBody() == NULL && !fd->hasInternalLinkage())
                out.emitImport(fd->getLabel());
        }

        set<string> emittedFunctions;

        // This iterator will point to the first AssemblerStmt in definitionList
        // that precedes by NO FunctionDef.
        vector<Tree *>::const_iterator postFuncAsmIter = definitionList->end();

        for (vector<Tree *>::const_iterator it = definitionList->begin(); it != definitionList->end(); ++it)
        {
            if (const FunctionDef *fd = dynamic_cast<const FunctionDef *>(*it))
            {
                postFuncAsmIter = definitionList->end();

                if (fd->getBody() == NULL)
                    continue;

                bool emit = fd->isCalled() || !fd->hasInternalLinkage();

                if (emit)
                {
                    if (!emitPrecedingVerbatimAssemblyBlocks(out, it))
                        errormsg("failed to emit assembler statements that precede function %s()", fd->getId().c_str());

                    if (!fd->hasInternalLinkage())
                        out.emitExport(fd->getLabel());
                    if (!fd->emitCode(out, false))
                        errormsg("failed to emit code for function %s()", fd->getId().c_str());
                    emittedFunctions.insert(fd->getId());  // remember that this func has been emitted
                }
            }
            else if (dynamic_cast<const AssemblerStmt *>(*it))
                postFuncAsmIter = it;
        }

        // Second pass in case some inline assembly has referred to a C function.
        // This means that functions are not necessarily emitted in the order in which they are defined.

        for (vector<Tree *>::const_iterator it = definitionList->begin(); it != definitionList->end(); ++it)
        {
            if (const FunctionDef *fd = dynamic_cast<const FunctionDef *>(*it))
            {
                if (fd->getBody() == NULL)
                    continue;

                if (fd->isCalled() && emittedFunctions.find(fd->getId()) == emittedFunctions.end())
                {
                    if (!emitPrecedingVerbatimAssemblyBlocks(out, it))
                        errormsg("failed to emit assembler statements that precede function %s()", fd->getId().c_str());

                    if (!fd->hasInternalLinkage())
                        out.emitExport(fd->getLabel());
                    if (!fd->emitCode(out, false))
                        errormsg("failed to emit code for function %s() in 2nd pass", fd->getId().c_str());
                    emittedFunctions.insert(fd->getId());  // remember that this func has been emitted
                }
            }
        }

        // Emit the AssemblerStmts that precede no FunctionDefs, if any.
        if (!emitAssemblerStmts(out, postFuncAsmIter, definitionList->end()))
            errormsg("failed to emit assembler statements at end of translation unit");
    }

    // Issue a warning for uncalled static functions.
    for (kt = functionDefs.begin(); kt != functionDefs.end(); kt++)
    {
        const FunctionDef *fd = kt->second;
        if (params.isUncalledStaticFunctionWarningEnabled && fd->getBody() && !fd->isCalled() && fd->hasInternalLinkage())
            fd->warnmsg("static function %s() is not called", fd->getId().c_str());
    }

    out.endSection();

    if (mainFunctionDef != NULL)
    {
        out.startSection("initgl_start");
        out.emitExport("INITGL");  // called by INILIB
        out.emitLabel("INITGL");
        out.endSection();
    }

    {
        // Generate code for global variables that are not initialized statically.
        //
        out.startSection("initgl");

        out.emitSeparatorComment();
        out.emitComment("Initialize global variables.");
        assert(scopeStack.size() == 0);  // ensure current scope is global one
        for (vector<Declaration *>::iterator jt = globalVariables.begin();
                                            jt != globalVariables.end(); jt++)
        {
            Declaration *decl = *jt;
            assert(decl);
            if (!decl->initializationExpr)
                continue;
            if (!decl->initializationExpr->isStaticallyInitializable(*decl->getTypeDesc()))
            {
                if (!decl->emitCode(out, false))
                    errormsg("failed to emit code for declaration of %s", decl->getVariableId().c_str());
            }
        }

        out.endSection();
    }

    out.startSection("rodata");

    // Generate the string literals:

    out.emitLabel("string_literals_start");
    if (stringLiteralLabelToValue.size() > 0)
    {
        out.emitSeparatorComment();
        out.emitComment("STRING LITERALS");
        for (StringLiteralToExprMap::const_iterator it = stringLiteralLabelToValue.begin();
                                                    it != stringLiteralLabelToValue.end(); it++)
        {
            const StringLiteralExpr *sle = it->second;
            out.emitLabel(it->first);
            sle->emitStringLiteralDefinition(out);
        }
    }
    out.emitLabel("string_literals_end");

    // Generate the real constants:

    if (realConstantLabelToValue.size() > 0)
    {
        // This must be done after emitting the initgl section, because the latter
        // may need to register real constants, which will then be emitted here.
        //
        out.emitLabel("real_constants_start");
        out.emitSeparatorComment();
        out.emitComment("REAL CONSTANTS");
        for (map< std::string, std::vector<uint8_t> >::const_iterator it = realConstantLabelToValue.begin();
                                                                     it != realConstantLabelToValue.end(); it++)
        {
            out.emitLabel(it->first);
            RealConstantExpr::emitRealConstantDefinition(out, it->second);
        }
        out.emitLabel("real_constants_end");
    }

    // Generate the dword constants:

    if (dwordConstantLabelToValue.size() > 0)
    {
        out.emitLabel("dword_constants_start");
        out.emitSeparatorComment();
        out.emitComment("DWORD CONSTANTS");
        for (map< std::string, std::vector<uint8_t> >::const_iterator it = dwordConstantLabelToValue.begin();
                                                                     it != dwordConstantLabelToValue.end(); it++)
        {
            out.emitLabel(it->first);
            DWordConstantExpr::emitDWordConstantDefinition(out, it->second);
        }
        out.emitLabel("dword_constants_end");
    }

    out.emitSeparatorComment();
    out.emitComment("READ-ONLY GLOBAL VARIABLES");

    if (!emitGlobalVariables(out, true, true))
        return;
    if (!emitLocalStaticVariables(out, true, true))
        return;

    out.endSection();

    // If no separate data section, then emit the writable globals after the code.
    // In this case, nothing other than INITGL should come after
    // the 'program_end' label, because sbrk() uses the memory
    // that starts at that label.
    //
    if (dataAddress == 0xFFFF)
    {
        emitWritableGlobals(out);
    }

    if (mainFunctionDef != NULL)
    {
        out.startSection("initgl_end");
        out.ins("RTS", "", "end of global variable initialization");
        out.endSection();

        emitProgramEnd(out);
    }

    // Here, we are not in any section.

    if (dataAddress != 0xFFFF)  // if data section is separate from the code/read-only section
    {
        // Start of data section, if separate from code.
        //
        out.emitSeparatorComment();
        out.emitComment("WRITABLE DATA SECTION");
        emitWritableGlobals(out);
    }
}


void
TranslationUnit::finishEmittingAssembler(ASMText &out)
{
    // Import all needed utility routines.
    //
    out.emitSeparatorComment();
    out.emitComment("Importing " + dwordToString(uint32_t(neededUtilitySubRoutines.size()), false) + " utility routine(s).");
    for (set<string>::const_iterator it = neededUtilitySubRoutines.begin();
                                    it != neededUtilitySubRoutines.end(); ++it)
        out.emitImport(it->c_str());

    out.emitSeparatorComment();

    out.emitEnd();
}


// it: Must refer to a FunctionDef pointer in the definitionList sequence.
// Emits code for the AssemblerStmts that precedes 'it' in definitionList
// but after any FunctionDef that may precede 'it'.
// Example: asm { clra }  void f() {}  asm { nop }  asm { rts }  void g() {}
//          In this case, if 'it' designates g(), then nop and rts are emitted,
//          but not clra, because the search backwards stops at f().
//
CodeStatus
TranslationUnit::emitPrecedingVerbatimAssemblyBlocks(ASMText &out,
                                                     vector<Tree *>::const_iterator it)
{
    if (definitionList == NULL)
        return true;

    assert(dynamic_cast<const FunctionDef *>(*it));

    // Iterate backwards as long as AssemblerStmt are found before 'it',
    // but stop at the function that precedes 'it'.
    vector<Tree *>::const_iterator jt;
    for (jt = it; jt != definitionList->begin(); )
    {
        --jt;
        if (dynamic_cast<const FunctionDef *>(*jt))
        {
            ++jt;
            break;
        }
    }

    return emitAssemblerStmts(out, jt, it);
}


// Emits code for each AssemblerStmt found in the [begin, end) range, in order of appearance.
// The AssemblerStmts are allowed to refer to global C variables.
//
CodeStatus
TranslationUnit::emitAssemblerStmts(ASMText &out,
                                    vector<Tree *>::const_iterator begin,
                                    vector<Tree *>::const_iterator end)
{
    // Allow the inline assembly to refer to global variables.
    pushScope(&getGlobalScope());  // const_cast should be removed...

    for (auto it = begin; it != end; ++it)
        if (const AssemblerStmt *as = dynamic_cast<const AssemblerStmt *>(*it))
            if (!as->emitCode(out, false))
                return false;

    popScope();
    return true;
}


// Resolves the given Vectrex music symbol.
// If it is of the form "vx_music_N", where N is in 1..13, returns
// the corresponding hex address in the form "$xxxx".
// Otherwise, the symbol is returned as is.
//
string
TranslationUnit::resolveVectrexMusicAddress(const string &symbol)
{
    static const uint16_t vxMusicAddresses[] =
    {
        0xFD0D, 0xFD1D, 0xFD81, 0xFDD3,
        0xFE38, 0xFE76, 0xFEC6, 0xFEF8,
        0xFF26, 0xFF44, 0xFF62, 0xFF7A, 0xFF8F
    };

    if (!startsWith(symbol, "vx_music_"))
        return symbol;
    char *endptr = NULL;
    unsigned long n = strtoul(symbol.c_str() + 9, &endptr, 10);
    if (errno == ERANGE || n < 1 || n > 13)
        return symbol;

    return wordToString(vxMusicAddresses[size_t(n) - 1], true);
}


// Emits the program_end label in an assembler section also called program_end.
// This section name is used by createLinkScript().
//
void
TranslationUnit::emitProgramEnd(ASMText &out) const
{
    out.emitSeparatorComment();
    out.startSection("program_end");
    out.emitExport("program_end");  // needed by INILIB
    out.emitLabel("program_end");
    out.endSection();
}


// Starts and ends a section.
//
CodeStatus
TranslationUnit::emitWritableGlobals(ASMText &out) const
{
    out.startSection("rwdata");

    out.emitComment("Statically-initialized global variables");
    if (!emitGlobalVariables(out, false, true))
        return false;
    out.emitComment("Statically-initialized local static variables");
    if (!emitLocalStaticVariables(out, false, true))
        return false;

    out.endSection();

    out.startSection("bss");

    out.emitLabel("bss_start");
    out.emitComment("Uninitialized global variables");
    if (!emitGlobalVariables(out, false, false))
        return false;
    out.emitComment("Uninitialized local static variables");
    if (!emitLocalStaticVariables(out, false, false))
        return false;

    out.emitLabel("bss_end");

    out.endSection();

    return true;
}


// readOnlySection: Selects which globals get emitted: true means the read-only globals,
//                  false means the writable globals.
// withStaticInitializer: If true, selects only globals that have a static initializer,
//                        i.e., are initialized with FCB/FDB directives.
//                        If false, selects RMB-defined globals.
//
CodeStatus
TranslationUnit::emitGlobalVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const
{
    bool success = true;

    for (vector<Declaration *>::const_iterator jt = globalVariables.begin();
                                              jt != globalVariables.end(); jt++)
    {
        Declaration *decl = *jt;
        assert(decl);
        if (! decl->emitGlobalVariables(out, readOnlySection, withStaticInitializer))
            success = false;
    }


    return success;
}


// Emits assembly directives that define compile-time initial values,
// or run-time initialization code, for all the local static variables.
//
CodeStatus
TranslationUnit::emitLocalStaticVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const
{
    if (definitionList == NULL)
        return true;

    class EmitLocalStaticVariableFunctor : public Scope::DeclarationFunctor
    {
        ASMText &out;
        bool readOnlySection;
        bool withStaticInitializer;
    public:
        EmitLocalStaticVariableFunctor(ASMText &_out, bool _readOnlySection, bool _withStaticInitializer)
            : out(_out), readOnlySection(_readOnlySection), withStaticInitializer(_withStaticInitializer) {}

        virtual bool operator()(Declaration &decl) override
        {
            if (!decl.isLocalStatic())
                return true;

            return decl.emitGlobalVariables(out, readOnlySection, withStaticInitializer);
        }

        virtual ~EmitLocalStaticVariableFunctor() {}
    } functor(out, readOnlySection, withStaticInitializer);

    // Emit local static variables, which reside in the heap like globals.
    for (vector<Tree *>::iterator it = definitionList->begin();
                                    it != definitionList->end(); it++)
        if (FunctionDef *fd = dynamic_cast<FunctionDef *>(*it))
            if (! fd->getScope()->iterateDeclarations(functor, true))
                return false;

    return true;
}


TranslationUnit::~TranslationUnit()
{
    assert(theInstance != NULL);
    assert(scopeStack.size() == 0);

    // Scope tree must be destroyed after the TreeSequences in definitionList.
    delete definitionList;
    delete globalScope;

    theInstance = NULL;
}


void
TranslationUnit::pushScope(Scope *scope)
{
    assert(scope != NULL);
    scopeStack.push_back(scope);
}


Scope *
TranslationUnit::getCurrentScope()
{
    return (scopeStack.size() > 0 ? scopeStack.back() : NULL);
}


void
TranslationUnit::popScope()
{
    assert(scopeStack.size() > 0);
    scopeStack.pop_back();
}


Scope &
TranslationUnit::getGlobalScope()
{
    return *globalScope;
}


void
TranslationUnit::pushBreakableLabels(const string &brkLabel,
                                        const string &contLabel)
{
    breakableStack.push_back(BreakableLabels());
    breakableStack.back().breakLabel = brkLabel;
    breakableStack.back().continueLabel = contLabel;
}


const BreakableLabels *
TranslationUnit::getCurrentBreakableLabels()
{
    if (breakableStack.size() > 0)
        return &breakableStack.back();
    return NULL;
}


void
TranslationUnit::popBreakableLabels()
{
    assert(breakableStack.size() > 0);
    breakableStack.pop_back();
}


void
TranslationUnit::setCurrentFunctionEndLabel(const string &label)
{
    functionEndLabel = label;
}


string
TranslationUnit::getCurrentFunctionEndLabel()
{
    return functionEndLabel;
}


/*static*/
string
TranslationUnit::genLabel(char letter)
{
    return instance().generateLabel(letter);
}


string
TranslationUnit::generateLabel(char letter)
{
    char label[7];
    snprintf(label, sizeof(label), "%c%05u",
                        letter, (unsigned) ++labelGeneratorIndex);
    return label;
}


FunctionDef *
TranslationUnit::getFunctionDef(const string &functionId)
{
    map<string, FunctionDef *>::iterator it = functionDefs.find(functionId);
    return (it == functionDefs.end() ? (FunctionDef *) 0 : it->second);
}


string
TranslationUnit::getFunctionLabel(const string &functionId)
{
    FunctionDef *fd = getFunctionDef(functionId);
    return (fd != NULL ? fd->getLabel() : "");
}


const FunctionDef *
TranslationUnit::getFunctionDefFromScope(const Scope &functionScope) const
{
    for (map<string, FunctionDef *>::const_iterator it = functionDefs.begin();
                                                   it != functionDefs.end(); ++it)
    {
        const FunctionDef *fd = it->second;
        if (fd && fd->getScope() == &functionScope)
            return fd;
    }
    return NULL;
}

string
TranslationUnit::registerStringLiteral(const StringLiteralExpr &sle)
{
    const string &stringValue = sle.getValue();
    map<string, string>::iterator it = stringLiteralValueToLabel.find(stringValue);
    if (it != stringLiteralValueToLabel.end())
        return it->second;

    string asmLabel = generateLabel('S');
    stringLiteralLabelToValue[asmLabel] = &sle;
    stringLiteralValueToLabel[stringValue] = asmLabel;
    return asmLabel;
}


string
TranslationUnit::getEscapedStringLiteral(const string &stringLabel)
{
    assert(!stringLabel.empty());
    StringLiteralToExprMap::iterator it = stringLiteralLabelToValue.find(stringLabel);
    if (it != stringLiteralLabelToValue.end())
        return StringLiteralExpr::escape(it->second->getValue());
    assert(!"unknown string literal label");
    return "";
}


string
TranslationUnit::registerRealConstant(const RealConstantExpr &rce)
{
    const vector<uint8_t> rep = rce.getRepresentation();  // length depends on single or double precision
    if (rep.size() == 0)
        return string();  // constant cannot be represented

    std::map< std::vector<uint8_t>, std::string >::iterator it = realConstantValueToLabel.find(rep);
    if (it != realConstantValueToLabel.end())
        return it->second;

    string asmLabel = generateLabel('F');
    realConstantLabelToValue[asmLabel] = rep;
    realConstantValueToLabel[rep] = asmLabel;
    return asmLabel;
}


string
TranslationUnit::registerDWordConstant(const DWordConstantExpr &dwce)
{
    const vector<uint8_t> rep = dwce.getRepresentation();
    std::map< std::vector<uint8_t>, std::string >::iterator it = dwordConstantValueToLabel.find(rep);
    if (it != dwordConstantValueToLabel.end())
        return it->second;

    string asmLabel = generateLabel('D');
    dwordConstantLabelToValue[asmLabel] = rep;
    dwordConstantValueToLabel[rep] = asmLabel;
    return asmLabel;
}


// In bytes. Returns 0 for an undefined struct or union.
//
int16_t
TranslationUnit::getTypeSize(const TypeDesc &typeDesc) const
{
    assert(typeDesc.isValid());

    if (typeDesc.type == CLASS_TYPE)
    {
        const ClassDef *cl = getClassDef(typeDesc.className);
        return cl ? cl->getSizeInBytes() : 0;
    }

    if (typeDesc.type == ARRAY_TYPE)
    {
        assert(typeDesc.numArrayElements != TypeDesc::NO_DIMENSION);
        size_t size = typeDesc.numArrayElements * (size_t) getTypeSize(*typeDesc.pointedTypeDesc);
        assert(size <= INT16_MAX);
        return int16_t(size);
    }

    return ::getTypeSize(typeDesc.type);
}


const ClassDef *
TranslationUnit::getClassDef(const std::string &className) const
{
    if (className.empty())
    {
        assert(!"empty class name passed to TranslationUnit::getClassDef()");
        return NULL;
    }
    for (vector<Scope *>::const_reverse_iterator it = scopeStack.rbegin();
                                                it != scopeStack.rend();
                                                it++)
    {
        const ClassDef *cl = (*it)->getClassDef(className);
        if (cl != NULL)
            return cl;
    }
    return globalScope->getClassDef(className);

}


ClassDef *
TranslationUnit::getClassDef(const std::string &className)
{
    return const_cast<ClassDef *>(static_cast<const TranslationUnit *>(this)->getClassDef(className));
}


void
TranslationUnit::registerNeededUtility(const std::string &utilitySubRoutine)
{
    neededUtilitySubRoutines.insert(utilitySubRoutine);
}


const set<string> &
TranslationUnit::getNeededUtilitySubRoutines() const
{
    return neededUtilitySubRoutines;
}


bool
TranslationUnit::isRelocatabilitySupported() const
{
    return params.relocatabilitySupported;
}


FramePointerOption
TranslationUnit::getFramePointerOption() const
{
    return params.framePointerOption;
}


void
TranslationUnit::processParseTimePragma(Pragma &pragma)
{
    bool processed = true;
    CallConvention callConv = DEFAULT_CMOC_CALL_CONV;

    if (pragma.isPushCallConvention(callConv))
        pushCurrentDefaultCallConvention(callConv);
    else if (pragma.isPopCallConvention())
        popCurrentDefaultCallConvention();
    else
        processed = false;
    
    if (processed)
        pragma.setProcessed();  // so that processPragmas() won't see it as invalid
}


void
TranslationUnit::processPragmas(uint16_t &codeAddress, bool codeAddressSetBySwitch,
                                uint16_t &codeLimitAddress, bool codeLimitAddressSetBySwitch,
                                uint16_t &dataAddress, bool dataAddressSetBySwitch,
                                uint16_t &stackSpace,
                                bool compileOnly)
{
    if (! definitionList)
        return;

    for (vector<Tree *>::iterator it = definitionList->begin();
                                 it != definitionList->end(); ++it)
        if (const Pragma *pragma = dynamic_cast<Pragma *>(*it))
        {
            if (pragma->isCodeOrg(codeAddress))  // if #pragma org ADDRESS
            {
                if (params.targetPlatform == VECTREX)
                    pragma->errormsg("#pragma org is not permitted for Vectrex");
                else if (compileOnly)
                    pragma->errormsg("#pragma org is not permitted with -c (use --org)");
                else if (codeAddressSetBySwitch)
                    pragma->warnmsg("#pragma org and --org (or --dos) both used");
            }
            else if (pragma->isCodeLimit(codeLimitAddress))  // if #pragma limit ADDRESS
            {
                if (compileOnly)
                    pragma->errormsg("#pragma limit is not permitted with -c (use --limit)");
                else if (codeLimitAddressSetBySwitch)
                    pragma->warnmsg("#pragma limit and --limit both used");
            }
            else if (pragma->isDataOrg(dataAddress))  // if #pragma data ADDRESS
            {
                if (params.targetPlatform == VECTREX)
                    pragma->errormsg("#pragma data is not permitted for Vectrex");
                else if (compileOnly)
                    pragma->errormsg("#pragma data is not permitted with -c (use --data)");
                else if (dataAddressSetBySwitch)
                    pragma->warnmsg("#pragma data and --data both used");
            }
            else if (pragma->isVxTitle(vxTitle))
            {
            }
            else if (pragma->isVxMusic(vxMusic))
            {
            }
            else if (pragma->isVxTitleSize(vxTitleSizeHeight, vxTitleSizeWidth))
            {
            }
            else if (pragma->isVxTitlePos(vxTitlePosY, vxTitlePosX))
            {
            }
            else if (pragma->isVxCopyright(vxCopyright))
            {
            }
            else if (pragma->isStackSpace(stackSpace))
            {
                if (params.targetPlatform == VECTREX)
                    pragma->errormsg("#pragma stack_space is not permitted for Vectrex");
            }
            else if (! pragma->wasProcessed())
                pragma->warnmsg("invalid pragma directive: %s", pragma->getDirective().c_str());
        }
}


bool
TranslationUnit::isNullPointerCheckingEnabled() const
{
    return params.nullPointerCheckingEnabled;
}


bool
TranslationUnit::isStackOverflowCheckingEnabled() const
{
    return params.stackOverflowCheckingEnabled;
}


// Destroys the DeclarationSpecifierList, the vector of Declarators
// and the Declarators.
// May return null in the case of a typedef.
//
DeclarationSequence *
TranslationUnit::createDeclarationSequence(DeclarationSpecifierList *dsl,
                                           std::vector<Declarator *> *declarators)
{
    DeclarationSequence *ds = NULL;

    const TypeDesc *td = dsl->getTypeDesc();
    assert(td->type != SIZELESS_TYPE);
    TypeManager &tm = TranslationUnit::getTypeManager();

    if (dsl->isTypeDefinition())
    {
        if (dsl->isAssemblyOnly())
            errormsg("modifier `asm' cannot be used on typedef");
        if (dsl->hasNoReturnInstruction())
            errormsg("modifier `__norts__' cannot be used on typedef");
        if (! declarators || declarators->size() == 0)
            errormsg("empty typename");
        else
            for (vector<Declarator *>::iterator it = declarators->begin(); it != declarators->end(); ++it)
                (void) tm.addTypeDef(td, *it);  // destroys the Declarator object
        ds = NULL;
    }
    else if (!declarators)
    {
        vector<Enumerator *> *enumeratorList = dsl->detachEnumeratorList();  // null if not an enum
        if (td->type != CLASS_TYPE && ! enumeratorList)
        {
            errormsg("declaration specifies a type but no declarator name");
        }

        // We have taken the enumeratorList out of the DeclarationSpecifierList
        // to give it to the created DeclarationSequence, whose checkSemantics()
        // method is responsible for checking that this enum is global.
        // (Enums local to a function are not supported.)
        //
        ds = new DeclarationSequence(td, enumeratorList);
    }
    else if (!params.callToUndefinedFunctionAllowed && dsl->isExternDeclaration())
    {
        // Ignore the declarators in an 'extern' declaration because
        // separate compilation is not supported.
        if (! declarators || declarators->size() == 0)
            errormsg("extern declaration defines no names");
        else
            deleteVectorElements(*declarators);
        ds = NULL;
    }
    else
    {
        bool isEnumType = dsl->hasEnumeratorList();
        ds = new DeclarationSequence(td, dsl->detachEnumeratorList());

        bool undefClass = (td->type == CLASS_TYPE && getClassDef(td->className) == NULL);

        assert(declarators->size() > 0);
        for (vector<Declarator *>::iterator it = declarators->begin(); it != declarators->end(); ++it)
        {
            Declarator *d = *it;
            if (undefClass && d->getPointerLevel() == 0)
            {
                errormsg("declaring `%s' of undefined type struct `%s'",
                         d->getId().c_str(), td->className.c_str());
            }
            else if (d->getFormalParamList() != NULL && isEnumType)
            {
                errormsg("enum with enumerated names is not supported in a function prototype's return type");
            }

            ds->processDeclarator(d, *dsl);  // destroys the Declarator object
        }
    }

    delete declarators;
    delete dsl->detachEnumeratorList();  // delete enumerator list if not used
    delete dsl;

    return ds;
}


void
TranslationUnit::checkForEllipsisWithoutNamedArgument(const FormalParamList *formalParamList)
{
    if (formalParamList && formalParamList->endsWithEllipsis() && ! formalParamList->isEllipsisImplied() && formalParamList->size() == 0)
        errormsg("named argument is required before `...'");  // as in GCC
}


bool
TranslationUnit::areAllWarningMessagesInhibited() const
{
    return params.inhibitAllWarningMessages;
}


bool
TranslationUnit::isWarningOnSignCompareEnabled() const
{
    return params.warnSignCompare;
}


bool
TranslationUnit::isWarningOnPassingConstForFuncPtr() const
{
    return params.warnPassingConstForFuncPtr;
}


void
TranslationUnit::addPrerequisiteFilename(const char *filename)
{
    if (filename[0] == '<')  // if preprocessor-generated filename, e.g., "<command-line>"
        return;
    if (find(prerequisiteFilenamesSeen.begin(), prerequisiteFilenamesSeen.end(), filename) != prerequisiteFilenamesSeen.end())
        return;  // already seen
    prerequisiteFilenamesSeen.push_back(filename);
}


void
TranslationUnit::writePrerequisites(ostream &out,
                                    const string &dependenciesFilename,
                                    const string &outputFilename,
                                    const string &pkgdatadir) const
{
    if (prerequisiteFilenamesSeen.size() == 0)  // if no files are depended on
        return;

    out << outputFilename;
    if (!dependenciesFilename.empty())
        out << ' ' << dependenciesFilename;
    out << " :";

    for (vector<string>::const_iterator it = prerequisiteFilenamesSeen.begin();
                                       it != prerequisiteFilenamesSeen.end(); ++it)
    {
        const string &fn = *it;
        if (!strncmp(fn.c_str(), pkgdatadir.c_str(), pkgdatadir.length()))  // exclude system header files
            continue;
        out << ' ' << fn;
    }

    out << '\n';
}


bool
TranslationUnit::warnOnConstIncorrect() const
{
    return params.isConstIncorrectWarningEnabled;
}


void
TranslationUnit::warnIfFloatUnsupported()
{
    if (warnedAboutUnsupportedFloats)
        return;
    if (!isFloatingPointSupported())
        warnmsg("floating-point arithmetic is not supported on the targeted platform");
    warnedAboutUnsupportedFloats = true;
}


bool
TranslationUnit::warnOnBinaryOpGivingByte() const
{
    return params.isBinaryOpGivingByteWarningEnabled;
}


bool
TranslationUnit::warnOnLocalVariableHidingAnother() const
{
    return params.isLocalVariableHidingAnotherWarningEnabled;
}


bool
TranslationUnit::warnOnNonLiteralPrintfFormat() const
{
    return params.isNonLiteralPrintfFormatWarningEnabled;
}


bool
TranslationUnit::warnOnUncalledStaticFunction() const
{
    return params.isUncalledStaticFunctionWarningEnabled;
}


bool
TranslationUnit::warnOnMissingFieldInitializers() const
{
    return params.isMissingFieldInitializersWarningEnabled;
}


bool
TranslationUnit::warnAboutInlineAsmArrayIndexes() const
{
    return params.inlineAsmArrayIndexesWarningEnabled;
}


void
TranslationUnit::enableVolatileWarning()
{
    if (warnedAboutVolatile)
        return;
    warnmsg("the `volatile' keyword is not supported by this compiler");
    warnedAboutVolatile = true;
}


bool
TranslationUnit::warnIfForConditionComparesDifferentSizes() const
{
    return params.forConditionComparesDifferentSizesWarningEnabled;
}


bool
TranslationUnit::warnArrayWithUnknownFirstDimension() const
{
    return params.warnArrayWithUnknownFirstDimension;
}


bool
TranslationUnit::warnArraySizeZero() const
{
    return params.warnArraySizeZero;
}


bool
TranslationUnit::warnTooManyElementsInInitializer() const
{
    return params.warnTooManyElementsInInitializer;
}


bool
TranslationUnit::warnShiftAlwaysZero() const
{
    return params.warnShiftAlwaysZero;
}


bool
TranslationUnit::warnLabelOnDeclaration() const
{
    return params.warnLabelOnDeclaration;
}


bool
TranslationUnit::warnAssignmentInCondition() const
{
    return params.warnAssignmentInCondition;
}


bool
TranslationUnit::onlyWarningOnWordSizedReturnTypeMismatch() const
{
    return params.onlyWarningOnWordSizedReturnTypeMismatch;
}


bool
TranslationUnit::isFloatingPointSupported() const
{
    return    params.floatingPointLibrary == FloatingPointLibrary::NATIVE_LIB
           || params.floatingPointLibrary == FloatingPointLibrary::MC6839_LIB
           || params.targetPlatform == COCO_BASIC
           || params.targetPlatform == DRAGON;
}


bool
TranslationUnit::isProgramMultiThreaded() const
{
    return true;
}


string
removeLeadingDigits(const string &s)
{
    string result;
    result.reserve(s.length());
    bool inPrefix = true;
    for (char c : s)
    {
        if (!isdigit(c))
            inPrefix = false;
        if (!inPrefix)
            result += c;
    }
    return result;
}


static const char *
getJSONBool(bool value)
{
    return value ? "true" : "false";
}


static bool
emitTypeDescIR(ostream &irFile, const TypeDesc &td)
{
    if (td.type == SIZELESS_TYPE)
        return false;  // unexpected
    irFile << "{\n";

    const char *type = NULL;
    if (td.type == VOID_TYPE)
        type = "void";
    else if (td.type == POINTER_TYPE)
        type = "pointer";
    else if (td.type == ARRAY_TYPE)
        type = "array";
    else if (td.type == CLASS_TYPE)
        type = "struct";
    else if (td.type == FUNCTION_TYPE)
        type = "function";
    else if (td.isIntegral())
        type = "integral";
    else if (td.isReal())
        type = "float";
    else
        return false;
    irFile << "\"type\": \"" << type << "\"";

    if (td.isIntegral())
        irFile << ",\n\"signed\": " << getJSONBool(td.isSigned) << "\n";
    else if (td.type != VOID_TYPE)
        return false;  // TODO: dump addition info about this type

    if (td.type != VOID_TYPE)
        irFile << ",\n\"size\": " << TranslationUnit::instance().getTypeSize(td) << "\n";
    irFile << "\n}";
    return true;
}


static bool
emitTreeIR(ostream &irFile, const Tree *tree)
{
    if (const FunctionDef *fd = dynamic_cast<const FunctionDef *>(tree))
    {
        irFile << ",\n\"id\": \"" << fd->getId() << "\"\n";
        irFile << ",\n\"returnType\": ";
        if (!emitTypeDescIR(irFile, *fd->getTypeDesc()))
            return false;
        irFile << "\n";
    }
    // TODO: Support other tree types.
    return true;
}


bool
TranslationUnit::emitInternalRepresentation(ostream &irFile)
{
    irFile << "{\n";
    irFile << "\"cmocVersion\": \"" VERSION "\",\n";
    irFile << "\"definitionList\": [\n";

    if (definitionList)
    {
        const char *punct = "";
        for (const Tree *tree : *definitionList)
        {
            if (tree)
            {
                irFile << punct << "{\n";
                irFile << "\"type\": \"" << removeLeadingDigits(typeid(*tree).name()) << "\"";
                if (!emitTreeIR(irFile, tree))
                    return false;  // stop on failure
                irFile << "}";
                punct = ",\n";
            }
        }
    }

    irFile << "]\n";  // definitionList
    irFile << "}\n";  // main JSON object
    return true;
}
