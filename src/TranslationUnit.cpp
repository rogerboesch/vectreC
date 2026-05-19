/*  $Id: TranslationUnit.cpp,v 1.166 2020/05/07 01:12:36 sarrazip Exp $

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

#include "TranslationUnit.h"

#include "TreeSequence.h"
#include "DeclarationSequence.h"
#include "FunctionDef.h"
#include "SemanticsChecker.h"
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
#include "ExpressionTypeSetter.h"

#include <assert.h>
#include <errno.h>
#include <fstream>

using namespace std;


/*static*/ TranslationUnit *TranslationUnit::theInstance = NULL;


void
TranslationUnit::createInstance(TargetPlatform targetPlatform,
                                bool callToUndefinedFunctionAllowed,
                                bool warnSignCompare,
                                bool warnPassingConstForFuncPtr,
                                bool isConstIncorrectWarningEnabled,
                                bool isBinaryOpGivingByteWarningEnabled,
                                bool isLocalVariableHidingAnotherWarningEnabled,
                                bool relocatabilitySupported)
{
    assert(theInstance == NULL);
    new TranslationUnit(targetPlatform,
                        callToUndefinedFunctionAllowed,
                        warnSignCompare,
                        warnPassingConstForFuncPtr,
                        isConstIncorrectWarningEnabled,
                        isBinaryOpGivingByteWarningEnabled,
                        isLocalVariableHidingAnotherWarningEnabled,
                        relocatabilitySupported);
    assert(theInstance);
}


void
TranslationUnit::destroyInstance()
{
    assert(theInstance);
    delete theInstance;
    theInstance = NULL;
}


TranslationUnit::TranslationUnit(TargetPlatform _targetPlatform,
                                 bool _callToUndefinedFunctionAllowed,
                                 bool _warnSignCompare,
                                 bool _warnPassingConstForFuncPtr,
                                 bool _isConstIncorrectWarningEnabled,
                                 bool _isBinaryOpGivingByteWarningEnabled,
                                 bool _isLocalVariableHidingAnotherWarningEnabled,
                                 bool _relocatabilitySupported)
  : typeManager(),
    globalScope(NULL),
    definitionList(NULL),
    functionDefs(),
    callGraph(),
    globalVariables(),
    scopeStack(),
    breakableStack(),
    functionEndLabel(),
    labelGeneratorIndex(0),
    stringLiteralLabelToValue(),
    stringLiteralValueToLabel(),
    realConstantLabelToValue(),
    realConstantValueToLabel(),
    dwordConstantLabelToValue(),
    dwordConstantValueToLabel(),
    builtInFunctionDescs(),
    relocatabilitySupported(_relocatabilitySupported),
    isProgramExecutableOnlyOnce(false),
    nullPointerCheckingEnabled(false),
    stackOverflowCheckingEnabled(false),
    callToUndefinedFunctionAllowed(_callToUndefinedFunctionAllowed),
    warnSignCompare(_warnSignCompare),
    warnPassingConstForFuncPtr(_warnPassingConstForFuncPtr),
    warnedAboutUnsupportedFloats(false),
    isConstIncorrectWarningEnabled(_isConstIncorrectWarningEnabled),
    isBinaryOpGivingByteWarningEnabled(_isBinaryOpGivingByteWarningEnabled),
    isLocalVariableHidingAnotherWarningEnabled(_isLocalVariableHidingAnotherWarningEnabled),
    warnedAboutVolatile(false),
    neededUtilitySubRoutines(),
    targetPlatform(_targetPlatform),
    vxTitle("CMOC"),
    vxMusic("vx_music_1"),
    vxTitleSizeWidth(80),
    vxTitleSizeHeight(-8),
    vxTitlePosX(-0x56),
    vxTitlePosY(0x20),
    vxCopyright("2015"),
    sourceFilenamesSeen()
{
    theInstance = this;  // instance() needed by Scope constructor
    typeManager.createBasicTypes();
    globalScope = new Scope(NULL, string());  // requires 'void', i.e., must come after createBasicTypes()
    typeManager.createInternalStructs(*globalScope, targetPlatform);  // global scope must be created; receives internal structs

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
    assert(fd->getTypeDesc()->isPtrToFunction() || fd->getTypeDesc()->isTypeWithoutCallingConventionFlags());
    functionDefs[fd->getId()] = fd;
}


void
TranslationUnit::removeFunctionDef(FunctionDef *fd)
{
    if (fd == NULL)
        return;
    functionDefs.erase(fd->getId());
}


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
        bool sameRetType = preexisting->hasSameReturnType(*fd);
        bool sameParams  = preexisting->hasSameFormalParams(*fd);
        if (!sameRetType || !sameParams)
        {
            const char *msg = NULL, *be = "are";
            if (!sameRetType && !sameParams)
                msg = "return type and formal parameters";
            else if (!sameRetType)
                msg = "return type", be = "is";
            else if (!sameParams)
                msg = "formal parameters";

            fd->errormsg("%s for %s() %s different from previously declared at %s",
                         msg, fd->getId().c_str(), be, preexisting->getLineNo().c_str());
        }
        if (preexisting->getBody() != NULL && fd->getBody() != NULL)
            fd->errormsg("%s() already has a body at %s",
                         fd->getId().c_str(), preexisting->getLineNo().c_str());
        if (preexisting->getBody() == NULL && fd->getBody() != NULL)
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
            if (targetPlatform == OS9)
            {
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
                    fd->errormsg("main() must receive (int, char **) or no parameters");
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
    virtual bool open(Tree *t)
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
                    fc->errormsg("calling undeclared function %s()", funcId.c_str());
            }
        }
        return true;
    }
    virtual bool close(Tree * /*t*/)
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
    virtual bool open(Tree *t)
    {
        if (dynamic_cast<SwitchStmt *>(t))
            ++switchLevel;
        const LabeledStmt *ls = NULL;
        if (switchLevel == 0 && (ls = dynamic_cast<LabeledStmt *>(t)) && ls->isCaseOrDefault())
            t->errormsg("%s label not within a switch statement", ls->isCase() ? "case" : "default");
        return true;
    }
    virtual bool close(Tree *t)
    {
        if (dynamic_cast<SwitchStmt *>(t))
            --switchLevel;
        return true;
    }
private:
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

    virtual bool open(Tree *t)
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
    virtual bool open(Tree *t)
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
    virtual bool close(Tree *t)
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
    virtual bool open(Tree *t)
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
             << "', isGlobal=" << decl->isGlobal() << ", needsFinish=" << decl->needsFinish << ", line " << decl->getLineNo() << endl;*/
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
        //cout << "# DeclarationFinisher:   arrayDimensions=[" << arrayDimensions << "], " << numDimsDueToDeclarator << endl;

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
            cl->setTypeDesc(declSeq->getTypeDesc());
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

    // Among other things, the ExpressionTypeSetter is run over the function bodies
    // during the following step.
    //
    {
        SemanticsChecker sc;
        definitionList->iterate(sc);
    }  // destroy ScopeCreator here so that it pops all scopes it pushed onto the TranslationUnit's stack


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

            // Determine if this global goes into the 'rodata' section, which may be in ROM,
            // in the case of a cartridge program.
            // This must be done after the SemanticsChecker pass, i.e., after the ExpressionTypeSetter.
            //
            bool typeCanGoInRO = decl->getTypeDesc()->canGoInReadOnlySection(relocatabilitySupported);
            bool initializerAllowsRO = (decl->isExtern && decl->isConst()) || decl->hasOnlyNumericalLiteralInitValues();
            decl->setReadOnly(typeCanGoInRO && initializerAllowsRO);

            if (decl->isReadOnly())
                checkConstDataDeclarationInitializer(*decl);  // do check after ExpressionTypeSetter
        }
    }


    // Check function prototypes, definitions and calls.
    // The following step assumes that the ExpressionTypeSetter has been run
    // over the function bodies, so that function calls that use a function pointer
    // can be differentiated from standard calls.
    //
    FunctionChecker ufc(*this, callToUndefinedFunctionAllowed);
    definitionList->iterate(ufc);
    ufc.reportErrors();


    LabeledStmtChecker lsc;
    definitionList->iterate(lsc);
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


void
TranslationUnit::setTargetPlatform(TargetPlatform platform)
{
    targetPlatform = platform;
}


TargetPlatform
TranslationUnit::getTargetPlatform() const
{
    return targetPlatform;
}


// Under OS-9, Y points to the current process's writable data segment,
// but read-only globals are still next to the code, thus PC-relative.
//
const char *
TranslationUnit::getDataIndexRegister(bool prefixWithComma, bool readOnly) const
{
    return (getTargetPlatform() == OS9 && !readOnly ? ",Y" : ",PCR") + (prefixWithComma ? 0 : 1);
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
            const string &callerId = calledFunctionIds[index];

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


// setTargetPlatform() must have been called before calling this method.
// Stops short if an error is detected.
//
void
TranslationUnit::emitAssembler(ASMText &out, uint16_t dataAddress, uint16_t stackSpace, bool emitBootLoaderMarker)
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

    if (targetPlatform == VECTREX && needStartSection)
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

        if (emitBootLoaderMarker)
            out.ins("FCC", "\"OS\"", "marker for CoCo DECB DOS command");
    }

    if (false || mainFunctionDef != NULL)
    {
        // Start the program by initializing the global variables, then
        // jumping to the main() function's label.

        if (targetPlatform == OS9)
        {
            // OS-9 launches a process by passing it the start and end addresses of
            // its data segment in U and Y respectively. The OS9PREP transfers U (the start)
            // to Y because CMOC uses U to point to the stack frame. Every reference
            // to a writable global variable under OS-9 and CMOC thus has the form FOO,Y.
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
            out.ins("LDD", "#-" + wordToString(stackSpace, false), "stack space in bytes");

        out.ins("LBSR", "INILIB", "initialize standard library and global variables");  // inits INISTK, for exit()
        if (mainFunctionDef != NULL)
        {
            out.ins("LBSR", mainFunctionDef->getLabel(), "call main()");

            if (targetPlatform == OS9)
                out.ins("LEAS", "4,S", "discard argc, argv");
        }

        if (targetPlatform != VECTREX)
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
                        else if (!decl->isStatic)
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

    set<string> emittedFunctions;
    for (kt = functionDefs.begin(); kt != functionDefs.end(); kt++)
    {
        const FunctionDef *fd = kt->second;
        if (fd->getBody() == NULL)
        {
            // Function prototype, so import its label.
            if (!fd->hasInternalLinkage())
                out.emitImport(fd->getLabel());
            continue;
        }

        bool emit = fd->isCalled() || !fd->hasInternalLinkage();

        if (emit)
        {
            if (!fd->hasInternalLinkage())
                out.emitExport(fd->getLabel());
            if (!fd->emitCode(out, false))
                errormsg("failed to emit code for function %s()", fd->getId().c_str());
            emittedFunctions.insert(fd->getId());  // remember that this func has been emitted
        }
    }
    // Second pass in case some inline assembly has referred to a C function.
    for (kt = functionDefs.begin(); kt != functionDefs.end(); kt++)
    {
        const FunctionDef *fd = kt->second;
        if (fd->getBody() == NULL)
            continue;

        if (fd->isCalled() && emittedFunctions.find(fd->getId()) == emittedFunctions.end())
        {
            if (!fd->hasInternalLinkage())
                out.emitExport(fd->getLabel());
            if (!fd->emitCode(out, false))
                errormsg("failed to emit code for function %s() in 2nd pass", fd->getId().c_str());
            emittedFunctions.insert(fd->getId());  // remember that this func has been emitted
        }
    }

    // Issue a warning for uncalled static functions.
    for (kt = functionDefs.begin(); kt != functionDefs.end(); kt++)
    {
        const FunctionDef *fd = kt->second;
        if (fd->getBody() && !fd->isCalled() && fd->hasInternalLinkage())
            fd->warnmsg("static function %s() is not called", fd->getId().c_str());
    }

    out.endSection();

    if (mainFunctionDef != NULL)
    {
        out.startSection("initgl_start");

        // Initial program break, for use by sbrk().
        // INITGL is placed just before or after the break.
        // If #pragma exec_once was given, then after INITGL has been executed,
        // its memory can be made available to sbrk().
        //
        if (isProgramExecutableOnlyOnce)
            emitProgramEnd(out);

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
            if (! decl->isArrayWithOnlyNumericalLiteralInitValues() && ! decl->isStructWithOnlyNumericalLiteralInitValues())
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

    // Generate global variables.
    //
    out.emitSeparatorComment();
    out.emitComment("READ-ONLY GLOBAL VARIABLES");

    if (!emitGlobalVariables(out, true, true))
        return;

    out.endSection();

    // If no data section, then emit the writable globals after the code.
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

        if (! isProgramExecutableOnlyOnce)
            emitProgramEnd(out);

        out.endSection();
    }

    // Here, we are not in any section.

    if (dataAddress != 0xFFFF)
    {
        // Start of data section, if separate from code.
        //
        out.emitSeparatorComment();
        out.emitComment("WRITABLE DATA SECTION");
        emitWritableGlobals(out);
    }

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


void
TranslationUnit::emitProgramEnd(ASMText &out) const
{
    out.emitSeparatorComment();
    out.emitExport("program_end");  // needed by INILIB
    out.emitLabel("program_end");
}


// Starts and ends a section.
//
CodeStatus
TranslationUnit::emitWritableGlobals(ASMText &out) const
{
    out.startSection("rwdata");

    out.emitComment("Globals with static initializers");
    if (!emitGlobalVariables(out, false, true))
        return false;

    out.endSection();

    out.startSection("bss");

    out.emitComment("Uninitialized globals");
    out.emitLabel("bss_start");
    if (!emitGlobalVariables(out, false, false))
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
        uint16_t size = 0;
        if (!decl->getVariableSizeInBytes(size))
        {
            success = false;
            continue;
        }

        if (decl->isReadOnly() != readOnlySection)
            continue;

        if (decl->isArrayWithOnlyNumericalLiteralInitValues())
        {
            if (withStaticInitializer)  // if selecting FCB/FDB globals
                decl->emitStaticArrayInitializer(out);
        }
        else if (readOnlySection)
        {
            if (withStaticInitializer)
            {
                out.emitLabel(decl->getLabel(), decl->getVariableId() + ": " + decl->getTypeDesc()->toString());
                decl->emitStaticValues(out, decl->initializationExpr, decl->getTypeDesc());
            }
        }
        else if (!withStaticInitializer)  // if selecting RMB globals
        {
            // We do not emit an FCB or FDB because these globals are initialized
            // at run-time by INITGL, so that they are re-initialized every time
            // the program is run.
            // This re-initialization does not happen for constant integer arrays,
            // for space saving purposes.
            //
            out.emitLabel(decl->getLabel());
            out.ins("RMB", wordToString(size), decl->getVariableId());
        }
    }

    return success;
}


TranslationUnit::~TranslationUnit()
{
    assert(scopeStack.size() == 0);

    // Scope tree must be destroyed after the TreeSequences in definitionList.
    delete definitionList;
    delete globalScope;

    theInstance = NULL;
}


void
TranslationUnit::pushScope(Scope *scope)
{
    //cout << "# TU::pushScope(" << scope << ")\n";
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
    //cout << "# TU::popScope: " << scopeStack.back() << "\n";
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
    vector<uint8_t> rep = rce.getRepresentation();  // length depends on single or double precision
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
    vector<uint8_t> rep = dwce.getRepresentation();
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
        assert(typeDesc.numArrayElements != uint16_t(-1));
        return typeDesc.numArrayElements * getTypeSize(*typeDesc.pointedTypeDesc);
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
    return relocatabilitySupported;
}


// Processes #pragma directives that need to be processed right after parsing.
//
// Changes 'codeAddress' iff a '#pragma org' directive is seen.
// codeAddressSetBySwitch: Indicates if the code address was set by a command-line argument.
//
// Changes 'codeLimitAddress' iff a '#pragma limit' directive is seen.
// codeLimitAddressSetBySwitch: Indicates if the code limit address was set by a command-line argument.
//
// Changes 'dataAddress' iff a '#pragma data' directive is seen.
//
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
        if (Pragma *pragma = dynamic_cast<Pragma *>(*it))
        {
            if (pragma->isCodeOrg(codeAddress))  // if #pragma org ADDRESS
            {
                if (targetPlatform == VECTREX)
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
                if (targetPlatform == VECTREX)
                    pragma->errormsg("#pragma data is not permitted for Vectrex");
                else if (compileOnly)
                    pragma->errormsg("#pragma data is not permitted with -c (use --data)");
                else if (dataAddressSetBySwitch)
                    pragma->warnmsg("#pragma data and --data both used");
            }
            else if (pragma->isExecOnce())
            {
                isProgramExecutableOnlyOnce = true;  // see emitAssembler()
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
                if (targetPlatform == VECTREX)
                    pragma->errormsg("#pragma stack_space is not permitted for Vectrex");
            }
            else
                pragma->errormsg("invalid pragma directive: %s", pragma->getDirective().c_str());
        }
}


void
TranslationUnit::enableNullPointerChecking(bool enable)
{
    nullPointerCheckingEnabled = enable;
}


bool
TranslationUnit::isNullPointerCheckingEnabled() const
{
    return nullPointerCheckingEnabled;
}


void
TranslationUnit::enableStackOverflowChecking(bool enable)
{
    stackOverflowCheckingEnabled = enable;
}


bool
TranslationUnit::isStackOverflowCheckingEnabled() const
{
    return stackOverflowCheckingEnabled;
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
    else if (!callToUndefinedFunctionAllowed && dsl->isExternDeclaration())
    {
        // Ignore the declarators in an 'extern' declaration because
        // separate compilation is not supported.
        if (! declarators || declarators->size() == 0)
            errormsg("extern declaration defines no names");
        else
            for (vector<Declarator *>::iterator it = declarators->begin(); it != declarators->end(); ++it)
                delete *it;
        ds = NULL;
    }
    else
    {
        bool isEnumType = dsl->hasEnumeratorList();
        ds = new DeclarationSequence(td, dsl->detachEnumeratorList());  // don't detach enumerator list from dsl yet

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

            ds->processDeclarator(d, *dsl);  // destroys the Declarator object; may need to check dsl's enumerator list
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
    if (formalParamList && formalParamList->endsWithEllipsis() && formalParamList->size() == 0)
        errormsg("named argument is required before `...'");  // as in GCC
}


bool
TranslationUnit::isCallToUndefinedFunctionAllowed() const
{
    return callToUndefinedFunctionAllowed;
}


bool
TranslationUnit::isWarningOnSignCompareEnabled() const
{
    return warnSignCompare;
}


bool
TranslationUnit::isWarningOnPassingConstForFuncPtr() const
{
    return warnPassingConstForFuncPtr;
}


void
TranslationUnit::addPrerequisiteFilename(const char *filename)
{
    if (filename[0] == '<')  // if preprocessor-generated filename, e.g., "<command-line>"
        return;
    if (find(sourceFilenamesSeen.begin(), sourceFilenamesSeen.end(), filename) != sourceFilenamesSeen.end())
        return;  // already seen
    sourceFilenamesSeen.push_back(filename);
}


void
TranslationUnit::writePrerequisites(ostream &out,
                                    const string &dependenciesFilename,
                                    const string &outputFilename,
                                    const string &pkgdatadir) const
{
    if (sourceFilenamesSeen.size() == 0)
        return;

    out << outputFilename;
    if (!dependenciesFilename.empty())
        out << ' ' << dependenciesFilename;
    out << " :";

    for (vector<string>::const_iterator it = sourceFilenamesSeen.begin();
                                       it != sourceFilenamesSeen.end(); ++it)
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
    return isConstIncorrectWarningEnabled;
}


bool
TranslationUnit::warnOnBinaryOpGivingByte() const
{
    return isBinaryOpGivingByteWarningEnabled;
}


bool
TranslationUnit::warnOnLocalVariableHidingAnother() const
{
    return isLocalVariableHidingAnotherWarningEnabled;
}


void
TranslationUnit::warnAboutVolatile()
{
    if (warnedAboutVolatile)
        return;
    warnmsg("the `volatile' keyword is not supported by this compiler");
    warnedAboutVolatile = true;
}
