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

#include "FunctionCallExpr.h"

#include "TranslationUnit.h"
#include "TreeSequence.h"
#include "IdentifierExpr.h"
#include "WordConstantExpr.h"
#include "VariableExpr.h"
#include "StringLiteralExpr.h"
#include "FunctionDef.h"
#include "SemanticsChecker.h"
#include "Declaration.h"
#include "ObjectMemberExpr.h"
#include "FormalParameter.h"
#include "UnaryOpExpr.h"
#include "CastExpr.h"


using namespace std;


FunctionCallExpr::FunctionCallExpr(Tree *func, TreeSequence *args)
  : Tree(),
    function(func),
    funcPtrVarDecl(NULL),
    arguments(args),
    returnValueDeclaration(NULL),
    tempDeclarationForFirstByteParam(),
    firstByteParamImmedValue(0),
    tempDeclarationForFirstWordParam(),
    firstWordParamImmedValue(0)
{
    assert(function != NULL);
    assert(arguments != NULL);
    if (auto ie = dynamic_cast<IdentifierExpr *>(function))
        ie->makeNameInAFunctionCall(true);  // remember that this ID is used to call a function, i.e., foo()
}


/*virtual*/
FunctionCallExpr::~FunctionCallExpr()
{
    delete function;
    delete arguments;
    delete returnValueDeclaration;
}


string
FunctionCallExpr::getIdentifier() const
{
    const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(function);
    return ie ? ie->getId() : string();
}


bool
FunctionCallExpr::isCallThroughPointer() const
{
    const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(function);
    return !ie || funcPtrVarDecl;
}


void 
FunctionCallExpr::setFunctionPointerVariableDeclaration(Declaration *_funcPtrVarDecl)
{
    funcPtrVarDecl = _funcPtrVarDecl;
}


// Determines if an expression of type 'argTree' can be used in a context that
// requires 'paramTD'.
//
FunctionCallExpr::Diagnostic
FunctionCallExpr::paramAcceptsArg(const TypeDesc &paramTD, const Tree &argTree)
{
    const TypeDesc &argTD = *argTree.getTypeDesc();

    assert(paramTD.isValid());
    assert(argTD.isValid());
    switch (paramTD.type)
    {
    case BYTE_TYPE:
        if (!argTD.isNumerical())
            return ERROR_MSG;
        if (argTD.type != BYTE_TYPE && argTree.is8BitConstant())
            return NO_PROBLEM;  // argument larger than byte, but actual value is known and fits byte
        if (argTree.getTypeSize() <= TranslationUnit::instance().getTypeSize(paramTD))  // if argument NOT larger than expected by function
            return NO_PROBLEM;
        return WARN_ARGUMENT_TOO_LARGE;
    case WORD_TYPE:
    case SIZELESS_TYPE:
        if (paramTD.isIntegral() && argTD.isReal())  // e.g., short <- float
            return WARN_REAL_FOR_INTEGRAL;
        if (paramTD.isIntegral() && argTD.isPtrOrArray())  // e.g., int <- int *
            return WARN_PTR_FOR_INTEGRAL;
        if (!argTD.isNumerical() && !argTD.isPtrOrArray())
            return ERROR_MSG;
        if (argTree.getTypeSize() > 2)
            return WARN_ARGUMENT_TOO_LARGE;
        return NO_PROBLEM;
    case CLASS_TYPE:
        if (paramTD.isNumerical())
        {
            if (paramTD.isReal() && argTD.isPtrOrArray())  // e.g., float <- float *
                return ERROR_MSG;
            if (paramTD.isIntegral() && argTD.isPtrOrArray())  // e.g., long <- long *
                return WARN_PTR_FOR_INTEGRAL;
            if (paramTD.isIntegral() && argTD.isReal())  // e.g., long <- float
                return WARN_REAL_FOR_INTEGRAL;
            return argTD.isNumerical() || argTD.isPtrOrArray() ? NO_PROBLEM : ERROR_MSG;
        }
        // The parameter is a user struct.
        return argTD.isStruct() && paramTD.className == argTD.className ? NO_PROBLEM : ERROR_MSG;
    case POINTER_TYPE:
    case ARRAY_TYPE:
        if (paramTD.isPtrToFunction() && argTD.isPtrToFunction())
        {
            return TypeDesc::sameFunctionTypesModuloKAndREmptyParamLilst(*paramTD.pointedTypeDesc, *argTD.pointedTypeDesc)
                        ? NO_PROBLEM : ERROR_MSG;
        }
        if (argTD.isNumerical())
        {
            uint16_t result = 0;
            if (!argTree.evaluateConstantExpr(result))
                return WARN_NON_PTR_ARRAY_FOR_PTR;
            if (argTree.getType() == BYTE_TYPE)
                return WARN_PASSING_CHAR_CONSTANT_FOR_PTR;
            if (result != 0 && TranslationUnit::instance().isWarningOnPassingConstForFuncPtr())  // re: -Wpass-const-for-func-pointer
                return WARN_PASSING_NON_ZERO_CONSTANT_FOR_PTR;
            return NO_PROBLEM;
        }
        if (!argTD.isPtrOrArray())
            return ERROR_MSG;
        // A void * parameter accepts a pointer of any type, except a function pointer.
        if (paramTD.pointedTypeDesc->type == VOID_TYPE && argTD.isPtrToFunction())
            return WARN_FUNC_PTR_FOR_PTR;
        if (CastExpr::isZeroCastToVoidPointer(argTree))
            return NO_PROBLEM;
        if (paramTD.pointedTypeDesc->isConstant())
            return paramTD.pointedTypeDesc->type == VOID_TYPE
                       || TypeDesc::sameTypesModuloConst(*paramTD.pointedTypeDesc, *argTD.pointedTypeDesc)
                   ? NO_PROBLEM
                   : ERROR_MSG;
        // The parameter is non-const T * or [].
        if (paramTD.pointedTypeDesc->type != VOID_TYPE
                && ! TypeDesc::sameTypesModuloConst(*paramTD.pointedTypeDesc, *argTD.pointedTypeDesc)
                && ! CastExpr::isZeroCastToVoidPointer(argTree))
        {
            if (TypeDesc::samePointerOrArrayTypesModuloSignedness(paramTD, argTD))
                return WARN_DIFFERENT_SIGNEDNESS;
            if (paramTD.getPointedType() == VOID_TYPE || argTD.getPointedType() == VOID_TYPE)
                return WARNING_VOID_POINTER;
            return ERROR_MSG;  // argument points to type incompatible with T
        }
        if (argTD.pointedTypeDesc->isConstant() && TranslationUnit::instance().warnOnConstIncorrect())  // reject argument if it is const T * or []
            return WARN_CONST_INCORRECT;
        return NO_PROBLEM;
    case VOID_TYPE:
        return ERROR_MSG;
    case FUNCTION_TYPE:
        return TypeDesc::sameFunctionTypesModuloKAndREmptyParamLilst(paramTD, argTD)
                    ? NO_PROBLEM : ERROR_MSG;
    }
    return ERROR_MSG;
}


// Abstract view of a list of function argument types.
// Used by FunctionCallExpr::checkCallArguments().
//
class Contraption
{
public:
    virtual ~Contraption() {}
    virtual bool hasNextParam() const = 0;
    virtual void nextParam() = 0;
    virtual bool isAcceptableNumberOfArguments(size_t numArguments) const = 0;
    virtual bool endsWithEllipsis() const = 0;
    virtual size_t size() const = 0;
    virtual const TypeDesc *getCurrentParamTypeDesc() const = 0;
    virtual const FormalParameter *getCurrentParamAsFormalParameter() const = 0;
};


class FormalParamListContraption : public Contraption
{
public:
    FormalParamListContraption(const FormalParamList &_formalParamList)
        : formalParamList(_formalParamList), it(formalParamList.begin()) {}
    virtual ~FormalParamListContraption() {}
    virtual bool hasNextParam() const override { return it != formalParamList.end(); }
    virtual void nextParam() override { assert(it != formalParamList.end()); ++it; }
    virtual bool isAcceptableNumberOfArguments(size_t numArguments) const override
        { return formalParamList.isAcceptableNumberOfArguments(numArguments); }
    virtual bool endsWithEllipsis() const override { return formalParamList.endsWithEllipsis(); }
    virtual size_t size() const override { return formalParamList.size(); }
    virtual const TypeDesc *getCurrentParamTypeDesc() const override
    {
        assert(it != formalParamList.end());
        assert(*it);
        return (*it)->getTypeDesc();
    }
    virtual const FormalParameter *getCurrentParamAsFormalParameter() const override
    {
        assert(it != formalParamList.end());
        assert(*it);
        return dynamic_cast<const FormalParameter *>(*it);
    }
private:
    const FormalParamList &formalParamList;
    vector<Tree *>::const_iterator it;
};


class TypeDescVectorContraption : public Contraption
{
public:
    TypeDescVectorContraption(const vector<const TypeDesc *> &_formalParamTypes, bool _ellipsis)
        : formalParamTypes(_formalParamTypes), it(formalParamTypes.begin()), ellipsis(_ellipsis) {}
    virtual ~TypeDescVectorContraption() {}
    virtual bool hasNextParam() const override { return it != formalParamTypes.end(); }
    virtual void nextParam() override { assert(it != formalParamTypes.end()); ++it; }
    virtual bool isAcceptableNumberOfArguments(size_t numArguments) const override
    {
        if (ellipsis)
            return numArguments >= size();
        return numArguments == size();
    }
    virtual bool endsWithEllipsis() const override { return ellipsis; }
    virtual size_t size() const override { return formalParamTypes.size(); }
    virtual const TypeDesc *getCurrentParamTypeDesc() const override
    {
        assert(it != formalParamTypes.end());
        assert(*it);
        return *it;
    }
    virtual const FormalParameter *getCurrentParamAsFormalParameter() const override { return NULL; }
private:
    const vector<const TypeDesc *> &formalParamTypes;
    vector<const TypeDesc *>::const_iterator it;
    bool ellipsis;
};


void
FunctionCallExpr::checkCallArguments(const string &functionId,
                                     Contraption &contraption,
                                     const TreeSequence &args) const
{
    extern int numErrors;
    int initNumErrors = numErrors;

    string temp = (functionId.empty() ? "call through function pointer" : "function " + functionId + "()");

    if (! contraption.isAcceptableNumberOfArguments(args.size()))
    {
        string funcLocation;
        const FunctionDef *calledFD = TranslationUnit::instance().getFunctionDef(getIdentifier());
        if (calledFD != NULL)
            funcLocation = " (declared at " + calledFD->getLineNo() + ")";
        errormsg("call %s passes %u argument(s) but function expects %s%u%s",
                 (functionId.empty() ? "through function pointer" : ("to " + functionId + "()").c_str()), args.size(),
                 contraption.endsWithEllipsis() ? "at least " : "",
                 contraption.size(),
                 funcLocation.empty() ? "" : funcLocation.c_str());
    }
    else
    {
        // Check the type of each argument against the corresponding formal parameter.
        // Arguments that are passed through the ellipsis of a variadic function are not checked.
        //
        size_t numParamsToCheck = contraption.size();
        std::vector<Tree *>::const_iterator a;
        for (a = args.begin(); numParamsToCheck > 0 && contraption.hasNextParam(); --numParamsToCheck, ++a, contraption.nextParam())
        {
            const Tree *argTree = *a;
            assert(argTree);
            const TypeDesc *argTD = argTree->getTypeDesc();
            const TypeDesc *paramTD = contraption.getCurrentParamTypeDesc();

            const FormalParameter *fp = contraption.getCurrentParamAsFormalParameter();
            unsigned fpIndex = unsigned(a - args.begin()) + 1;

            string paramNameStr = (fp && !fp->getId().empty() ? (" (" + fp->getId() + ")").c_str() : "");

            switch (paramAcceptsArg(*paramTD, *argTree))
            {
            case NO_PROBLEM:
                break;
            case WARN_CONST_INCORRECT:
                argTree->warnmsg("`%s' used as parameter %u%s of %s which is `%s' (not const-correct)",
                                    argTD->toString().c_str(),
                                    fpIndex, paramNameStr.c_str(), temp.c_str(),
                                    paramTD->toString().c_str());
                break;
            case WARN_NON_PTR_ARRAY_FOR_PTR:
                argTree->warnmsg("passing non-pointer/array (%s) as parameter %u%s of %s, which is `%s`",
                                     argTD->toString().c_str(),
                                     fpIndex, paramNameStr.c_str(), temp.c_str(),
                                     paramTD->toString().c_str());
                break;
            case WARN_PASSING_CHAR_CONSTANT_FOR_PTR:
                argTree->warnmsg("passing character constant as parameter %u%s of %s, which is `%s'",
                                     fpIndex, paramNameStr.c_str(), temp.c_str(),
                                     paramTD->toString().c_str());
                break;
            case WARN_PASSING_NON_ZERO_CONSTANT_FOR_PTR:
                argTree->warnmsg("passing non-zero numeric constant as parameter %u%s of %s, which is `%s'",
                                     fpIndex, paramNameStr.c_str(), temp.c_str(),
                                     paramTD->toString().c_str());
                break;
            case WARN_ARGUMENT_TOO_LARGE:
                argTree->warnmsg("`%s' argument is too large for parameter %u%s of %s, which is `%s`",
                                  argTD->toString().c_str(),
                                  fpIndex, paramNameStr.c_str(), temp.c_str(),
                                  paramTD->toString().c_str());
                break;
            case WARN_REAL_FOR_INTEGRAL:
                argTree->warnmsg("passing real type `%s' for parameter %u%s of %s, which is `%s`",
                                  argTD->toString().c_str(),
                                  fpIndex, paramNameStr.c_str(), temp.c_str(),
                                  paramTD->toString().c_str());
                break;
            case WARN_FUNC_PTR_FOR_PTR:
                argTree->warnmsg("passing function pointer `%s' for parameter %u%s of %s, which is `%s`",
                                  argTD->toString().c_str(),
                                  fpIndex, paramNameStr.c_str(), temp.c_str(),
                                  paramTD->toString().c_str());
                break;
            case WARN_DIFFERENT_SIGNEDNESS:
                argTree->warnmsg("`%s' used as parameter %u%s of %s which is `%s' (different signedness)",
                                    argTD->toString().c_str(),
                                    fpIndex, paramNameStr.c_str(), temp.c_str(),
                                    paramTD->toString().c_str());
                break;
            case WARNING_VOID_POINTER:
                argTree->warnmsg("passing `%s' for parameter of type `%s' (implicit cast of void pointer)",
                                    argTD->toString().c_str(),
                                    paramTD->toString().c_str());
                break;
            case WARN_PTR_FOR_INTEGRAL:
                argTree->warnmsg("passing pointer type `%s' for parameter %u%s of %s, which is `%s`",
                                  argTD->toString().c_str(),
                                  fpIndex, paramNameStr.c_str(), temp.c_str(),
                                  paramTD->toString().c_str());
                break;
            case ERROR_MSG:
                argTree->errormsg("`%s' used as parameter %u%s of %s which is `%s'",
                                    argTD->toString().c_str(),
                                    fpIndex, paramNameStr.c_str(), temp.c_str(),
                                    paramTD->toString().c_str());
                break;
            }


            // If parameter is named enum, check that argument is member of it.
            const string *enumTypeName = (fp ? &fp->getEnumTypeName() : NULL);
            if (enumTypeName && !enumTypeName->empty())
            {
                const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(argTree);
                if (!ie)
                    argTree->warnmsg("parameter %u of %s is not a member of enum %s",
                                      fpIndex, temp.c_str(), enumTypeName->c_str());
                else
                {
                    // Get the enumerator list of the named enum.
                    string id = ie->getId();
                    if (! TranslationUnit::getTypeManager().isIdentiferMemberOfNamedEnum(*enumTypeName, id))
                        argTree->warnmsg("`%s' used as parameter %u of %s but is not a member of enum %s",
                                          id.c_str(), fpIndex, temp.c_str(), enumTypeName->c_str());
                }
            }
        }
    }

    // Check printf() arguments unless an error has already been reported about this function call.
    if (numErrors == initNumErrors)
    {
        if (functionId == "printf" || functionId == "sprintf")
            checkPrintfArguments(args, functionId);
    }
}


const TypeDesc *
FunctionCallExpr::getCalledPrototypeTypeDesc() const
{
    if (isCallThroughPointer())
    {
        const TypeDesc *td = function->getTypeDesc();
        if (td->type == POINTER_TYPE && td->pointedTypeDesc->type == FUNCTION_TYPE)
            return td->pointedTypeDesc;
        if (td->type == FUNCTION_TYPE)
            return td;
        return NULL;
    }

    string fid = getIdentifier();
    const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(fid);
    if (!fd)
        return NULL;
    const TypeDesc *td = TranslationUnit::instance().getTypeManager().getFunctionPointerType(*fd);
    assert(td->type == POINTER_TYPE && td->pointedTypeDesc->type == FUNCTION_TYPE);
    return td->pointedTypeDesc;
}


// Called by the ExpressionTypeSetter.
//
bool
FunctionCallExpr::checkAndSetTypes()
{
    assert(arguments != NULL);

    if (isCallThroughPointer())
    {
        assert(function);
        const TypeDesc *funcTD = getCalledPrototypeTypeDesc();
        if (!funcTD)
        {
            function->errormsg("function pointer call through expression of invalid type (`%s')",
                                function->getTypeDesc()->toString().c_str());
            return false;
        }
        assert(funcTD->type == FUNCTION_TYPE);

        const TypeDesc *retTD = funcTD->getReturnTypeDesc();
        assert(retTD);

        setTypeDesc(retTD);

        if (funcTD->isInterruptServiceRoutine())
        {
            errormsg("calling an interrupt service routine is forbidden");
            return false;
        }

        TypeDescVectorContraption contraption(funcTD->getFormalParamTypeDescList(), funcTD->endsWithEllipsis());
        checkCallArguments(string(), contraption, *arguments);

        return true;
    }

    string fid = getIdentifier();
    const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(fid);
    if (!fd)
    {
        setTypeDesc(TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true));  // assume call returns int
        return false;  // undeclared function: let FunctionChecker handle this
    }

    if (fd->isInterruptServiceRoutine())
    {
        errormsg("calling function %s() is forbidden because it is an interrupt service routine",
                 fid.c_str());
        return false;
    }

    FormalParamListContraption contraption(*fd->getFormalParamList());
    checkCallArguments(fid, contraption, *arguments);

    setTypeDesc(fd->getTypeDesc());
    return true;
}


static bool
isValidPrintfConversionSpecifier(char c)
{
    return strchr("cdfpsuxX", c) != NULL;
}


// Allowed: %ld, %lu, %lx, %lX.
//
static bool
isValidLongPrintfConversionSpecifier(char c)
{
    return strchr("duxX", c) != NULL;
}


void
FunctionCallExpr::checkPrintfArguments(const TreeSequence &args, const string &functionId) const
{
    std::vector<Tree *>::const_iterator a = args.begin();

    if (a == args.end())  // no arguments: already reported
        return;

    if (functionId == "sprintf")
    {
        const Tree *arg = *a;
        if (arg->getType() != POINTER_TYPE && arg->getType() != ARRAY_TYPE)
        {
            warnmsg("first argument of sprintf""() should be pointer or array instead of `%s'",
                    arg->getTypeDesc()->toString().c_str());
            return;
        }
        if (dynamic_cast<const StringLiteralExpr *>(arg))
        {
            warnmsg("first argument of sprintf""() is a string literal", arg->getTypeDesc()->toString().c_str());
            return;
        }
        ++a;
        if (a == args.end())  // no 2nd argument: already reported
            return;
    }

    const StringLiteralExpr *formatArg = dynamic_cast<const StringLiteralExpr *>(*a);
    if (!formatArg)
    {
        if (TranslationUnit::instance().warnOnNonLiteralPrintfFormat())
            warnmsg("format argument of %s() is not a string literal", functionId.c_str());
        return;  // cannot check format if not a string literal
    }

    ++a;  // advance to first post-format argument

    const string formatStr = formatArg->getLiteral();
    size_t formatLen = formatStr.length();
    bool warningIssued = false;
    for (size_t i = 0; i < formatLen; ++i)  // for each character of the format string
        if (formatStr[i] == '%')
        {
            ++i;
            if (i < formatLen && formatStr[i] == '%')  // if double percent, an actual '%' is printed
                continue;

            // Look for end of placeholder.
            // If letter found, scan the letters of the placeholder.
            // Note whether the 'l' modifier is specified.
            bool runtimeWidthMarkerSeen = false;
            while (i < formatLen && !isalpha(formatStr[i]) && !isspace(formatStr[i]))
            {
                if (formatStr[i] == '*')
                    runtimeWidthMarkerSeen = true;
                ++i;
            }
            bool haveLongModifier = false;
            if (i < formatLen && isalpha(formatStr[i]))  // if on letter
            {
                if (formatStr[i] == 'l')
                {
                    haveLongModifier = true;
                    ++i;
                }
                if (i == formatLen || !isValidPrintfConversionSpecifier(formatStr[i]))
                {
                    formatArg->warnmsg("no valid conversion specifier follows %% in %s() format string", functionId.c_str());
                    warningIssued = true;
                    break;
                }
                if (haveLongModifier && !isValidLongPrintfConversionSpecifier(formatStr[i]))
                {
                    formatArg->warnmsg("unsupported use of `l' flag used on `%c' conversion specifier in %s() format string",
                                        formatStr[i], functionId.c_str());
                    warningIssued = true;
                    break;
                }
            }
            else
            {
                formatArg->warnmsg("no conversion specifier follows %% in %s() format string", functionId.c_str());
                warningIssued = true;
                break;
            }

            if (a == args.end())
            {
                formatArg->warnmsg("not enough arguments to %s() to match its format string", functionId.c_str());
                warningIssued = true;
                break;
            }

            if (runtimeWidthMarkerSeen)
            {
                if ((*a)->getType() != WORD_TYPE)
                    (*a)->warnmsg("argument %u of %s() is supposed to be a signed int runtime field width, but is of type `%s'",
                                    unsigned(a - args.begin()) + 1, functionId.c_str(), (*a)->getTypeDesc()->toString().c_str());

                ++a;  // point to next argument
                if (a == args.end())
                {
                    formatArg->warnmsg("not enough arguments to %s() to match its format string", functionId.c_str());
                    warningIssued = true;
                    break;
                }
            }

            const TypeDesc *argTD = (*a)->getTypeDesc();
            unsigned argIndex = unsigned(a - args.begin()) + 1;

            if (formatStr[i] == 'f' && ! argTD->isReal())
            {
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder is %%f",
                              argIndex, functionId.c_str(), argTD->toString().c_str());
                warningIssued = true;
            }
            else if (argTD->isReal() && formatStr[i] != 'f')
            {
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder is not %%f",
                              argIndex, functionId.c_str(), argTD->toString().c_str());
                warningIssued = true;
            }

            if (haveLongModifier && ! argTD->isLong())
            {
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder has the `l' modifier",
                              argIndex, functionId.c_str(), argTD->toString().c_str());
                warningIssued = true;
            }
            else if (argTD->isLong() && ! haveLongModifier)
            {
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder does not have the `l' modifier",
                              argIndex, functionId.c_str(), argTD->toString().c_str());
                warningIssued = true;
            }

            if (formatStr[i] == 's' && !argTD->isPtrOrArrayOfChar())
            {
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder is %%s",
                              argIndex, functionId.c_str(), argTD->toString().c_str());
                warningIssued = true;
            }

            if (argTD->isStruct())
            {
                (*a)->warnmsg("argument %u of %s() is a struct or union",
                              argIndex, functionId.c_str());
                warningIssued = true;
            }
            
            ++a;  // point to next argument
        }

    if (!warningIssued && a != args.end())
        formatArg->warnmsg("too many arguments for %s() format string", functionId.c_str());
}


/*virtual*/
void
FunctionCallExpr::checkSemantics(Functor &f)
{
    if (isCallThroughPointer())
    {
        UnaryOpExpr *u = dynamic_cast<UnaryOpExpr *>(function);
        if (u)
            u->allowDereferencingVoid();
    }
    else
    {
        // Register this function call for the purposes of determining which functions
        // are never called and do not need to have assembly code emitted for them.
        // When there is no calling function (as in a global variable's initialization
        // expression), we do as if main() were the caller. This is not actually the
        // case, because the caller is the INITGL routine, but it is close enough for
        // the purposes of TranslationUnit::detectCalledFunctions().
        //
        TranslationUnit &tu = TranslationUnit::instance();
        SemanticsChecker &sem = dynamic_cast<SemanticsChecker &>(f);
        const FunctionDef *caller = sem.getCurrentFunctionDef();
        string callerId;
        if (caller == NULL)
            callerId = "main";
        else
            callerId = caller->getId();
        tu.registerFunctionCall(callerId, getIdentifier());
    }

    // If the return type is struct/union, declare a hidden struct/union in the current scope.
    // This object will receive the return value. Its address will be passed in the call.
    //
    if (passesHiddenParam())
        returnValueDeclaration = Declaration::declareHiddenVariableInCurrentScope(*this);

    const TypeDesc *calledTD = getCalledPrototypeTypeDesc();
    if (!calledTD)
    {
        // An error message is assumed to be issued elsewhere for this.
        return;
    }

    if (calledTD->getCallConvention() == GCC6809_CALL_CONV)
    {
        // Declare some hidden local variables in the stack space
        // where the parameters received in registers will be spilled
        // at the start of the function body.
        // The signedness of the temporary locations does not matter.
        // However, do NOT delcare a local variable IF the value is known at compile time.
        //
        ssize_t firstByteArgIndex, firstWordArgIndex;
        calledTD->getGCCCallConventionInfo(firstByteArgIndex, firstWordArgIndex);
        auto &tm = TranslationUnit::getTypeManager();
        if (firstByteArgIndex != -1)
        {
            Tree *argTree = arguments->getTree(firstByteArgIndex);
            uint16_t result = 0;
            if (argTree && argTree->evaluateConstantExpr(result))  // if known at compile time
            {
                firstByteParamImmedValue = result & 0xFF;  // leave tempDeclarationForFirstByteParam null
                if (WordConstantExpr *wce = dynamic_cast<WordConstantExpr *>(argTree))
                    wce->forceAsByte();  // tells code emitter to use LDB instead of LDD
            }
            else
                tempDeclarationForFirstByteParam.reset(Declaration::declareHiddenVariableInCurrentScope(*this, tm.getIntType(BYTE_TYPE, false)));
        }
        if (firstWordArgIndex != -1)
        {
            const Tree *argTree = arguments->getTree(firstWordArgIndex);  // will be null if -2
            uint16_t result = 0;
            if (argTree && argTree->evaluateConstantExpr(result))  // if known at compile time
                firstWordParamImmedValue = result;  // leave tempDeclarationForFirstWordParam null
            else
                tempDeclarationForFirstWordParam.reset(Declaration::declareHiddenVariableInCurrentScope(*this, tm.getIntType(WORD_TYPE, false)));
        }
    }
}


bool
FunctionCallExpr::passesHiddenParam() const
{
    return getType() == CLASS_TYPE;
}


// Emits code to pass a function argument value currently contained in a register.
// Emits an instruction and increments numBytesPushed depending on passInReg,
// i.e., whether the argument must be passed in a register or pushed on the stack.
//
// gccCall: Indicates that the GCC6809 calling convention applies.
//          The value to be passed will not be pushed on the stack, but will instead
//          be stored in tempDeclarationForFirstByteParam or tempDeclarationForFirstWordParam
//          depending on its size. Just before the call to the function, the appropriate
//          register will be loaded from that location.
// regContainingArg: Supported: B, D, X.
// numBytesPushed: In/Out. Gets incremented if passInReg is false.
// pshsComment: Comment to use if passInReg is false.
// param: May be null.
//
bool
FunctionCallExpr::emitPushSingleArg(ASMText &out, bool passInReg, bool gccCall,
                                    Register regContainingArg, uint16_t &numBytesPushed,
                                    const string &pshsComment,
                                    const TypeDesc *paramTypeDesc) const
{
    assert(regContainingArg == B || regContainingArg == D || regContainingArg == X);
    const char *regNameContainingArg = getRegisterName(regContainingArg);

    if (!passInReg)
    {
        out.ins("PSHS", regContainingArg == D ? "B,A" : regNameContainingArg, pshsComment);
        numBytesPushed += regContainingArg == B ? 1 : 2;
        return true;
    }

    // If the parameter of the function is a byte, and regContainingArg is D, then
    // it is actually B that gets passed.
    //
    if (paramTypeDesc && paramTypeDesc->type == BYTE_TYPE && regContainingArg == D)
        regContainingArg = B;

    // If gccCall is true, getFirstParameterRegister() is appropriate, despite its name.
    //
    Register regToPassArgIn = TranslationUnit::instance().getFirstParameterRegister(regContainingArg == B);
    const char *regNameToPassArgIn = getRegisterName(regToPassArgIn);
    string comment = "function receives argument in " + string(regNameToPassArgIn);

    if (regContainingArg != regToPassArgIn)
        out.ins("TFR", regNameContainingArg + string(",") + regNameToPassArgIn, comment);
    else
        out.emitComment(comment);

    if (passInReg && gccCall)
    {
        const Declaration *tempDecl = (regToPassArgIn == B ? tempDeclarationForFirstByteParam.get() : tempDeclarationForFirstWordParam.get());
        if (tempDecl)
            out.emitParameterSaveInstruction(regNameToPassArgIn, tempDecl->getFrameDisplacementArg(0), function->getLineNo());
    }

    return true;
}


// If the called function receives its first parameter in a register, the emitted code
// leaves the value of that parameter in D or B.
//
// numBytesPushed: Must already be initialized. Gets incremented by the number of bytes
//                 pushed onto the system stack by the code emitted by this method.
// functionId: If not empty, appears in the comments. Not used for anything else.
// callConv: Determines if the argument will be pushed on the stack or in a register.
//
bool
FunctionCallExpr::emitArgumentPushCode(ASMText &out,
                                       const string &functionId,
                                       CallConvention callConv,
                                       uint16_t &numBytesPushed) const
{
    /*  Push the arguments in reverse order on the stack.
        Promote byte expressions to word.
    */

    const TypeDesc *calledTD = getCalledPrototypeTypeDesc();

    const bool calledFunctionReceivesFirstVisibleParamInReg = (callConv == GCC6809_CALL_CONV
                    ? false
                    : callConv == FIRST_PARAM_IN_REG_CALL_CONV && !passesHiddenParam());

    ssize_t firstByteArgIndex = -1, firstWordArgIndex = -1;  // zero-based indexes, when referring to 'arguments' array
    if (callConv == GCC6809_CALL_CONV && calledTD)
        calledTD->getGCCCallConventionInfo(firstByteArgIndex, firstWordArgIndex);

    const vector<const TypeDesc *> *paramTypeDescList = (calledTD ? &calledTD->getFormalParamTypeDescList() : NULL);

    size_t index = arguments->size();
    for (vector<Tree *>::reverse_iterator it = arguments->rbegin();
                                         it != arguments->rend(); it++, index--)
    {
        const Tree *expr = *it;
        string comment = ASMText::getArgumentPassingComment(uint16_t(index), functionId, *expr->getTypeDesc());

        // Determine which formal parameter this argument corresponds to, if any.
        // (A function taking an ellipsis may receive more arguments that it has declared parameters.)

        assert(index > 0);
        size_t fpIndex = index - 1;
        const TypeDesc *paramTypeDesc = NULL;
        if (paramTypeDescList && fpIndex < paramTypeDescList->size())
        {
            paramTypeDesc = (*paramTypeDescList)[fpIndex];
            assert(paramTypeDesc);
        }

        const bool passInReg = callConv == GCC6809_CALL_CONV
                                ? (ssize_t(index) == firstByteArgIndex + 1 || ssize_t(index) == firstWordArgIndex + 1)
                                : (calledFunctionReceivesFirstVisibleParamInReg && fpIndex == 0);

        // Emit code depending on the argument type.

        const VariableExpr *ve = expr->asVariableExpr();
        const UnaryOpExpr *unary = dynamic_cast<const UnaryOpExpr *>(expr);

        if (const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(expr))
        {
            out.ins("LEAX", sle->getArg(), sle->getEscapedVersion());
            emitPushSingleArg(out, passInReg, callConv == GCC6809_CALL_CONV, X, numBytesPushed, comment, paramTypeDesc);
        }
        else if (ve && ve->getType() == ARRAY_TYPE)  // if argument is an array
        {
            out.ins("LEAX", ve->getFrameDisplacementArg(), "address of array " + ve->getId());
            emitPushSingleArg(out, passInReg, callConv == GCC6809_CALL_CONV, X, numBytesPushed, comment, paramTypeDesc);
        }
        else if (unary && unary->getOperator() == UnaryOpExpr::ADDRESS_OF)
        {
            const Tree *subExpr = unary->getSubExpr();
            const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(subExpr);
            if (ie && ie->asVariableExpr() && ie->getType() == ARRAY_TYPE)
            {
                // Special case for array b/c it has no l-value.
                out.ins("LEAX", ie->asVariableExpr()->getFrameDisplacementArg(),
                                    "address of array " + ie->asVariableExpr()->getId());
            }
            else if (!subExpr->emitCode(out, true))  // emit l-value, to get address in X and avoid TFR X,D
                return false;
            emitPushSingleArg(out, passInReg, callConv == GCC6809_CALL_CONV, X, numBytesPushed, comment, paramTypeDesc);
        }
        else if (expr->getType() == CLASS_TYPE)  // if passing a struct by value
        {
            assert(!passInReg || callConv == GCC6809_CALL_CONV);  // under gcc conv., 1- or 2-byte struct can be passed in B or X

            // Emit the struct expression as an l-value, i.e., compute its address in X.
            if (!expr->emitCode(out, true))
                return false;

            if (paramTypeDesc && paramTypeDesc->isIntegral() && !paramTypeDesc->isLong() && expr->isRealOrLong())  // if passing real/long to short integral
            {
                assert(!passInReg);

                // Emit code to convert the number at X into the integral expected by the function.

                // Pass the address of the argument in D.
                out.ins("TFR", "X,D");

                // Push enough bytes in the stack to contain the integral argument.
                int16_t paramSize = TranslationUnit::instance().getTypeSize(*paramTypeDesc);
                int16_t passedSize = (paramSize == 1 ? 2 : paramSize);
                out.ins("LEAS", "-" + wordToString(passedSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", paramSize == 1 ? "1,S" : ",S");

                callUtility(out, "init" + string(expr->isLong() ? "" : (paramTypeDesc->isSigned ? "Signed" : "Unsigned"))
                                        + (paramTypeDesc->type == BYTE_TYPE ? "Byte" : "Word")
                                        + "From" + (expr->isLong()
                                                        ? "DWord"
                                                        : (expr->isSingle() ? "Single" : "Double")),
                                 "convert argument to l-value at X");

                numBytesPushed += uint16_t(passedSize);

                if (paramSize == 1)
                {
                    out.ins("LDB", "1,S", "LSB of argument");
                    out.ins(paramTypeDesc->getConvToWordIns(), "", "promoting byte argument to word");
                    out.ins("STA", ",S", "MSB of argument");
                }
            }
            else if ((paramTypeDesc && paramTypeDesc->isLong() && expr->isReal())  // if passing real to long
                  || (paramTypeDesc && paramTypeDesc->isReal() && expr->isLong()))  // or passing long to real
            {
                assert(!passInReg);

                // Emit code to convert the argument at X into the type expected by the function.

                // Pass the address of the argument in D.
                out.ins("TFR", "X,D");

                // Push enough bytes in the stack to contain the long argument.
                int16_t passedSize = TranslationUnit::instance().getTypeSize(*paramTypeDesc);
                out.ins("LEAS", "-" + wordToString(passedSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", ",S");

                if (paramTypeDesc->isLong())
                    callUtility(out, "init" + string(paramTypeDesc->isSigned ? "Signed" : "Unsigned") + "DWordFrom" + (expr->isSingle() ? "Single" : "Double"),
                                     "convert real argument to long at X");
                else
                    callUtility(out, "init" + string(paramTypeDesc->isSingle() ? "Single" : "Double") + "From" + (expr->isSigned() ? "Signed" : "Unsigned") + "DWord",
                                     "convert long argument to real at X");

                numBytesPushed += uint16_t(passedSize);
            }
            else
            {
                // Push the struct to the stack.
                // Call a specific utility routine for the 5-byte struct case,
                // which optimizes the case of Color Basic's 5-byte float.
                //
                uint16_t structSizeInBytes = uint16_t(expr->getTypeSize());
                if (structSizeInBytes > 0)
                {
                    if (passInReg)
                    {
                        assert(structSizeInBytes == 1 || structSizeInBytes == 2);
                        assert(callConv == GCC6809_CALL_CONV);
                        assert(tempDeclarationForFirstByteParam || tempDeclarationForFirstWordParam);
                        out.ins(structSizeInBytes == 1 ? "LDB" : "LDX", ",X", "load struct value");
                        out.ins(structSizeInBytes == 1 ? "STB" : "STX",
                                structSizeInBytes == 1 ? tempDeclarationForFirstByteParam->getFrameDisplacementArg(0)
                                                       : tempDeclarationForFirstWordParam->getFrameDisplacementArg(0),
                                "save register for call at " + function->getLineNo());
                    }
                    else
                    {
                        const char *u = NULL;
                        switch (structSizeInBytes)
                        {
                        case 4:  u = "push4ByteStruct"; break;
                        case 5:  u = "push5ByteStruct"; break;
                        default:
                            u = (structSizeInBytes < 256 ? "pushSmallStruct" : "pushStruct");
                            const char *ld = (structSizeInBytes < 256 ? "LDB" : "LDD");
                            out.ins(ld, "#" + wordToString(structSizeInBytes), "size of " + expr->getTypeDesc()->toString());
                        }
                        out.ins("LEAS", "-" + wordToString(structSizeInBytes) + ",S",
                                        "pass " + expr->getTypeDesc()->toString() + " by value");
                        callUtility(out, u, comment);
                        if (structSizeInBytes == 1)
                        {
                            out.ins("LEAS", "-1,S", "1-byte argument always pushed as 2 bytes");
                            ++numBytesPushed;
                        }

                        numBytesPushed += structSizeInBytes;
                    }
                }
            }
        }
        else
        {
            if (!expr->emitCode(out, false))  // get r-value in B or D
                return false;
            Register regContainingArg = (expr->getType() == BYTE_TYPE ? B : D);

            // Promote a single-byte value if needed.
            int16_t paramSize = (paramTypeDesc ? TranslationUnit::instance().getTypeSize(*paramTypeDesc) : 0);
            if (expr->getType() == BYTE_TYPE && (!passInReg || paramSize == 2))
            {
                out.ins(expr->getConvToWordIns(), "", "promoting byte argument to word");
                if (passInReg && callConv == GCC6809_CALL_CONV)
                {
                    out.ins("TFR", "D,X", "first parameter of " + (functionId.empty() ? "" : functionId + "() ") + "passed in X (gcc call)");
                    out.optimizeXParameterLoad();  // try to load directly in X instead if possible
                    regContainingArg = X;
                }
                else
                    regContainingArg = D;
            }

            // An int-sized parameter that is currently in D but must be passed to a __gcccall function
            // must be considered to be in B for the purposes of emitPushSingleArg().
            // This fixes a bug in CMOC 0.1.87
            //
            if (passInReg && callConv == GCC6809_CALL_CONV && paramSize == 1 && regContainingArg == D)
                regContainingArg = B;

            if (paramTypeDesc && paramTypeDesc->isRealOrLong())  // if passing basic type to real/long
            {
                assert(!passInReg);

                out.ins("LEAS", "-" + wordToString(paramSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", ",S");

                callUtility(out, "init" + string(paramTypeDesc->isLong() ? "DWord" : (paramTypeDesc->isSingle() ? "Single" : "Double"))
                                 + "From" + (expr->isSigned() ? "Signed" : "Unsigned")
                                          + (expr->isLong() ? "DWord" : "Word"));

                numBytesPushed += paramSize;
            }
            else
            {
                emitPushSingleArg(out, passInReg, callConv == GCC6809_CALL_CONV, regContainingArg, numBytesPushed, comment, paramTypeDesc);
            }
        }

        // Try to merge LDD <foo>; PSHS B,A; LDD <bar>; PSHS B,A into LDX <foo>; LDD <bar>; PSHS X,B,A.
        //
        out.optimizeConsecutiveFunctionArguments();
    }

    return true;
}


// Determines if 'function' is an expression of the form *v where v is
// a VariableExpr marked as a function address expression.
//
static string
isDereferenceOfFunctionId(const Tree &function)
{
    const UnaryOpExpr *unary = dynamic_cast<const UnaryOpExpr *>(&function);
    if (!unary || unary->getOperator() != UnaryOpExpr::INDIRECTION)  // if not (*v)()
        return string();
    const VariableExpr *ve = unary->getSubExpr()->asVariableExpr();
    if (!ve || !ve->isFuncAddrExpr())
        return string();  // not a function ID
    string funcId = ve->getId();
    const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(funcId);
    if (!fd)
        return string();
    return funcId;
}


// Emits true if 'function' is of the form ((FuncPtrType) constant),
// i.e., ((int (*)(void)) 0x1234)().
// In this case, returns the constant in jsrArg in hex, prefixed with '$'.
//
bool
FunctionCallExpr::isCallToConstantAddress(string &jsrArg) const
{
    const CastExpr *ce = dynamic_cast<const CastExpr *>(function);
    if (!ce)
        return false;
    const WordConstantExpr *wce = dynamic_cast<const WordConstantExpr *>(ce->getSubExpr());
    if (!wce)
        return false;
    jsrArg = wordToString(wce->getWordValue(), true);
    return true;
}


/*virtual*/
CodeStatus
FunctionCallExpr::emitCode(ASMText &out, bool lValue) const
{
    if (lValue && getType() != CLASS_TYPE)
    {
        errormsg("cannot use function call as l-value unless type is struct or union");
        return false;
    }

    assert(function != NULL);

    // Get the type of the prototype of the function to be called.
    // We will get NULL if the function is undeclared.
    const TypeDesc *calledTD = getCalledPrototypeTypeDesc();

    const CallConvention callConv = (calledTD ? calledTD->getCallConvention() : DEFAULT_CMOC_CALL_CONV);

    ssize_t firstByteArgIndex = -1, firstWordArgIndex = -1;  // zero-based indexes
    if (callConv == GCC6809_CALL_CONV && calledTD != NULL)
        calledTD->getGCCCallConventionInfo(firstByteArgIndex, firstWordArgIndex);

    const IdentifierExpr *ie = dynamic_cast<IdentifierExpr *>(function);

    // If standard call (i.e., not made through pointer), then get name of callee.
    //
    string functionId = isDereferenceOfFunctionId(*function);
    if (functionId.empty())
        functionId = (isCallThroughPointer() ? "" : ie->getId());

    /*cout << "- FunctionCallExpr::emitCode: ie=" << ie << ", functionId=" << functionId
            << ", funcPtrVarDecl=" << funcPtrVarDecl << endl;*/

    writeLineNoComment(out, "function call"
                            + (functionId.empty() ? " through pointer" : ": " + functionId + "()"));

    TranslationUnit &tu = TranslationUnit::instance();

    uint16_t numBytesPushed = 0;

    if (!emitArgumentPushCode(out, functionId, callConv, numBytesPushed))
        return false;


    // If return value is struct/union, pass address of allocated local struct as hidden parameter.
    if (passesHiddenParam())
    {
        assert(returnValueDeclaration);
        out.ins("LEAX", returnValueDeclaration->getFrameDisplacementArg(0),
                                "address of struct/union to be returned by " + functionId + "()");
        if (    callConv == FIRST_PARAM_IN_REG_CALL_CONV
            || (callConv == GCC6809_CALL_CONV && firstWordArgIndex == -2))
        {
            out.emitComment("Pass hidden argument in X.");
        }
        else
        {
            out.ins("PSHS", "X", "hidden argument");
            numBytesPushed += 2;
        }
    }

    if (callConv == GCC6809_CALL_CONV)
    {
        // Under the GCC6809 convention, some parameters may be passed in B or X.
        // Here, we load those registers from immediate values if possible,
        // or from the temporary stack locations where emitArgumentPushCode()
        // has stored the parameter value.
        //
        if (firstByteArgIndex >= 0)
        {
            if (tempDeclarationForFirstByteParam != NULL)
                out.ins("LDB", tempDeclarationForFirstByteParam->getFrameDisplacementArg(0), "__gcccall: restore first byte-sized argument");
            else if (firstByteParamImmedValue == 0)
                out.ins("CLRB", "", "__gcccall: first byte-sized argument");
            else
                out.ins("LDB", "#" + wordToString(firstByteParamImmedValue), "__gcccall: first byte-sized argument");
        }
        if (firstWordArgIndex >= 0)
        {
            if (tempDeclarationForFirstWordParam != NULL)
                out.ins("LDX", tempDeclarationForFirstWordParam->getFrameDisplacementArg(0), "__gcccall: restore first word-sized argument");
            else
            {
                out.ins("LDX", "#" + wordToString(firstWordParamImmedValue), "__gcccall: first word-sized argument");
                out.optimizeXParameterLoad();  // check if this LDX makes previous instructions useless
            }
        }
    }

    // Call the function.  If the function to call is designated simply
    // by an identifier, and that identifier is not a variable,
    // then find the corresponding assembly label, and call it directly.
    // Otherwise, compute the address of the function to call and
    // call it indirectly.

    if ((ie != NULL || !functionId.empty()) && funcPtrVarDecl == NULL)  // if standard call
    {
        assert(!functionId.empty());

        string functionLabel = tu.getFunctionLabel(functionId);
        if (functionLabel.empty())
        {
            functionLabel = FunctionDef::makeLabelFromFunctionId(functionId);  // guess the ID: useful with K&R code
            out.emitComment("Call to undeclared function " + functionId + "(): label guessed; import in case function provided by other module");
            out.emitImport(functionLabel);  // assume that the function may be provided by another translation unit
        }

        out.ins(tu.isRelocatabilitySupported() ? "LBSR" : "JSR", functionLabel);  // JSR takes 1 fewer cycle
    }
    else if (ie != NULL && funcPtrVarDecl != NULL)  // if called address is in a variable using no-asterisk notation, e.g., pf()
    {
        assert(functionId.empty());

        out.ins("JSR", "[" + funcPtrVarDecl->getFrameDisplacementArg() + "]",
                       "indirect call through variable `" + ie->getId() + "'");
    }
    else  // called address is (*pf)() or object.member().
    {
        assert(functionId.empty());

        string jsrArg = ",X";
        const UnaryOpExpr *unary = dynamic_cast<UnaryOpExpr *>(function);
        if (unary && unary->getOperator() == UnaryOpExpr::INDIRECTION)  // if (*pf)()
        {
            const Tree *unarySubExpr = unary->getSubExpr();
            const TypeDesc *td = unarySubExpr->getTypeDesc();
            assert(td->type == POINTER_TYPE);
            if (td->pointedTypeDesc->type != FUNCTION_TYPE)
            {
                // We have (*ppf)(void) where ppf is a pointer to a pointer to a function (or more pointer levels).
                // The type of ppf is then: int (**ppf)(void).
                //
                // If the call convention is __gcccall, we must preserve B and X,
                // which may get trashed by emitCode().
                //
                assert(td->pointedTypeDesc->type == POINTER_TYPE);

                if (callConv == GCC6809_CALL_CONV)
                    out.ins("PSHS", "Y,X,B", "allocate word, then preserve parameter registers re: __gcccall");

                if (!function->emitCode(out, true))  // gets an address in X
                    return false;

                if (callConv == GCC6809_CALL_CONV)
                {
                    out.ins("STX", "3,S", "store function pointer in word allocated on stack");
                    out.ins("PULS", "B,X", "restore parameter registers re: __gcccall");
                    jsrArg = "[,S++]";
                }
                else
                    jsrArg = "[,X]";
            }
            else
            {
                const VariableExpr *ve = unarySubExpr->asVariableExpr();
                if (ve)
                    jsrArg = "[" + ve->getFrameDisplacementArg(0) + "]";
                else
                {
                    if (callConv == GCC6809_CALL_CONV)
                        out.ins("PSHS", "Y,X,B", "allocate word, then preserve parameter registers re: __gcccall");

                    if (!function->emitCode(out, true))  // get function address in X
                        return false;

                    if (callConv == GCC6809_CALL_CONV)
                    {
                        out.ins("STX", "3,S", "store function pointer in word allocated on stack");
                        out.ins("PULS", "B,X", "restore parameter registers re: __gcccall");
                        jsrArg = "[,S++]";
                    }
                }
            }
        }
        else if (isCallToConstantAddress(jsrArg))
        {
        }
        else  // general case: namely, object.member()
        {
            if (callConv == GCC6809_CALL_CONV)
                out.ins("PSHS", "Y,X,B", "allocate word, then preserve parameter registers re: __gcccall");

            if (!function->emitCode(out, false))  // get function pointer in D
                return false;

            if (callConv == GCC6809_CALL_CONV)
            {
                out.ins("STD", "3,S", "store function pointer in word allocated on stack");
                out.ins("PULS", "B,X", "restore parameter registers re: __gcccall");
                jsrArg = "[,S++]";
            }
            else
                out.ins("TFR", "D,X");
        }
        out.ins("JSR", jsrArg);
    }

    // Pop the arguments off the stack:
    if (numBytesPushed > 0)
        out.ins("LEAS", wordToString(numBytesPushed) + ",S");

    // If an l-value was requested, we are returning a struct/union, so point X to it:
    if (lValue)
    {
        assert(getType() == CLASS_TYPE);
        out.ins("LEAX", returnValueDeclaration->getFrameDisplacementArg(0),
                                "address of struct/union returned by " + functionId + "()");
    }

    // If the return value has been received in X, move it to D as expected by default
    // by code generated by CMOC.
    //
    if (callConv == GCC6809_CALL_CONV && getType() != VOID_TYPE && getType() != BYTE_TYPE && getType() != CLASS_TYPE)
        out.ins("TFR", "X,D", "16-bit result received in X as per GCC6809 convention");

    return true;
}


bool
FunctionCallExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!function->iterate(f))
        return false;
    if (!arguments->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
