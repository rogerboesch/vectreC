/*  $Id: FunctionCallExpr.cpp,v 1.84 2020/05/07 01:04:08 sarrazip Exp $

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
    returnValueDeclaration(NULL)
{
    assert(function != NULL);
    assert(arguments != NULL);
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
        return argTD.isNumerical() || argTD.isPtrOrArray() ? NO_PROBLEM : ERROR_MSG;
    case CLASS_TYPE:
        if (paramTD.isNumerical())
        {
            if (paramTD.isReal() && argTD.isPtrOrArray())  // e.g., float <- float *
                return ERROR_MSG;
            if (paramTD.isIntegral() && argTD.isReal())  // e.g., long <- float
                return WARN_REAL_FOR_INTEGRAL;
            return argTD.isNumerical() || argTD.isPtrOrArray() ? NO_PROBLEM : ERROR_MSG;
        }
        // The parameter is a user struct.
        return argTD.isStruct() && paramTD.className == argTD.className ? NO_PROBLEM : ERROR_MSG;
    case POINTER_TYPE:
    case ARRAY_TYPE:
        if (argTD.isNumerical())
        {
            uint16_t result = 0;
            if (!argTree.evaluateConstantExpr(result))
                return WARN_NON_PTR_ARRAY_FOR_PTR;
            if (result != 0)
                return WARN_PASSING_CONSTANT_FOR_PTR;
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
        return paramTD == argTD ? NO_PROBLEM : ERROR_MSG;
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
    virtual bool hasNextParam() const { return it != formalParamList.end(); }
    virtual void nextParam() { assert(it != formalParamList.end()); ++it; }
    virtual bool isAcceptableNumberOfArguments(size_t numArguments) const
        { return formalParamList.isAcceptableNumberOfArguments(numArguments); }
    virtual bool endsWithEllipsis() const { return formalParamList.endsWithEllipsis(); }
    virtual size_t size() const { return formalParamList.size(); }
    virtual const TypeDesc *getCurrentParamTypeDesc() const
    {
        assert(it != formalParamList.end());
        assert(*it);
        return (*it)->getTypeDesc();
    }
    virtual const FormalParameter *getCurrentParamAsFormalParameter() const
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
    virtual bool hasNextParam() const { return it != formalParamTypes.end(); }
    virtual void nextParam() { assert(it != formalParamTypes.end()); ++it; }
    virtual bool isAcceptableNumberOfArguments(size_t numArguments) const
    {
        if (ellipsis)
            return numArguments >= size();
        return numArguments == size();
    }
    virtual bool endsWithEllipsis() const { return ellipsis; }
    virtual size_t size() const { return formalParamTypes.size(); }
    virtual const TypeDesc *getCurrentParamTypeDesc() const
    {
        assert(it != formalParamTypes.end());
        assert(*it);
        return *it;
    }
    virtual const FormalParameter *getCurrentParamAsFormalParameter() const { return NULL; }
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
        errormsg("call %s passes %u argument(s) but function expects %s%u",
                 (functionId.empty() ? "through function pointer" : ("to " + functionId + "()").c_str()), args.size(),
                 contraption.endsWithEllipsis() ? "at least " : "",
                 contraption.size());
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
            case WARN_PASSING_CONSTANT_FOR_PTR:
                if (TranslationUnit::instance().isWarningOnPassingConstForFuncPtr())  // if -Wpass-const-for-func-pointer
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
                    argTree->errormsg("parameter %u of %s must be a member of enum %s",
                                      fpIndex, temp.c_str(), enumTypeName->c_str());
                else
                {
                    // Get the enumerator list of the named enum.
                    string id = ie->getId();
                    if (! TranslationUnit::getTypeManager().isIdentiferMemberOfNamedEnum(*enumTypeName, id))
                        argTree->errormsg("`%s' used as parameter %u of %s but is not a member of enum %s",
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


// Called by the ExpressionTypeSetter.
//
bool
FunctionCallExpr::checkAndSetTypes()
{
    assert(arguments != NULL);

    if (isCallThroughPointer())
    {
        assert(function);
        const TypeDesc *funcTD = function->getTypeDesc();
        if (funcTD->type == POINTER_TYPE && funcTD->pointedTypeDesc->type == FUNCTION_TYPE)
            funcTD = funcTD->pointedTypeDesc;
        else if (funcTD->type != FUNCTION_TYPE)
        {
            function->errormsg("function pointer call through expression of invalid type (`%s')", function->getTypeDesc()->toString().c_str());
            return false;
        }

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
        return false;  // undeclared function: let FunctionChecker handle this

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
            warnmsg("first argument of sprintf() should be pointer or array instead of `%s'",
                    arg->getTypeDesc()->toString().c_str());
            return;
        }
        if (dynamic_cast<const StringLiteralExpr *>(arg))
        {
            warnmsg("first argument of sprintf() is a string literal", arg->getTypeDesc()->toString().c_str());
            return;
        }
        ++a;
        if (a == args.end())  // no 2nd argument: already reported
            return;
    }

    const StringLiteralExpr *formatArg = dynamic_cast<const StringLiteralExpr *>(*a);
    if (!formatArg)
    {
        warnmsg("format argument of %s() is not a string literal", functionId.c_str());
        return;  // cannot check format if not a string literal
    }

    ++a;  // advance to first post-format argument

    const string formatStr = formatArg->getLiteral();
    size_t formatLen = formatStr.length();
    for (size_t i = 0; i < formatLen; ++i)
        if (formatStr[i] == '%')
        {
            ++i;
            if (i < formatLen && formatStr[i] == '%')
                continue;

            // Look for end of placeholder.
            while (i < formatLen && !isalpha(formatStr[i]))
                ++i;

            if (i == formatLen)
            {
                formatArg->warnmsg("no letter follows last %% in %s() format string", functionId.c_str());
                break;
            }

            if (a == args.end())
            {
                formatArg->warnmsg("not enough arguments to %s() to match its format string", functionId.c_str());
                break;
            }

            // Scan the letters of the placeholder.
            bool haveLongModifier = false;
            while (i < formatLen && isalpha(formatStr[i]))
            {
                if (formatStr[i] == 'l')
                    haveLongModifier = true;
                ++i;
            }
            --i;  // go back to the last letter

            const TypeDesc *argTD = (*a)->getTypeDesc();

            if (formatStr[i] == 'f' && ! argTD->isReal())
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder is %%f",
                              unsigned(a - args.begin()) + 1, functionId.c_str(), argTD->toString().c_str());
            else if (argTD->isReal() && formatStr[i] != 'f')
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder is not %%f",
                              unsigned(a - args.begin()) + 1, functionId.c_str(), argTD->toString().c_str());

            if (haveLongModifier && ! argTD->isLong())
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder has the `l' modifier",
                              unsigned(a - args.begin()) + 1, functionId.c_str(), argTD->toString().c_str());
            else if (argTD->isLong() && ! haveLongModifier)
                (*a)->warnmsg("argument %u of %s() is of type `%s' but the placeholder does not have the `l' modifier",
                              unsigned(a - args.begin()) + 1, functionId.c_str(), argTD->toString().c_str());

            ++a;  // point to next argument
        }

    if (a != args.end())
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
}


bool
FunctionCallExpr::passesHiddenParam() const
{
    return getType() == CLASS_TYPE;
}


bool
FunctionCallExpr::isFunctionReceivingFirstParamInReg() const
{
    const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(getIdentifier());
    if (fd)
        return fd->isFunctionReceivingFirstParamInReg();
    assert(function);
    assert(function->getTypeDesc());
    return function->getTypeDesc()->isFunctionReceivingFirstParamInReg();
}


// Emits an instruction and increments numBytesPushed depending on whether the
// argument must be passed in a register (D) or pushed in the stack.
// isArgInRegX: Only applies if passInReg is true.
// pshsArg: Not used if passInReg is true.
//
bool
FunctionCallExpr::emitPushSingleArg(ASMText &out, bool passInReg, bool isArgInRegX, uint16_t &numBytesPushed,
                                    const char *pshsArg, const string &pshsComment) const
{
    if (!passInReg)
    {
        out.ins("PSHS", pshsArg, pshsComment);
        numBytesPushed += 2;
        return true;
    }

    if (isArgInRegX)
        out.ins("TFR", "X,D", "function receives argument 1 in D");

    return true;
}


// If the called function receives its first parameter in a register, the emitted code
// leaves the value of that parameter in D or B.
//
// numBytesPushed: Must be initialized to zero. Gets incremented by the number of bytes
//                 pushed onto the system stack by the code emitted by this method.
// functionId: If not empty, appears in the comments. Not used for anything else.
//
bool
FunctionCallExpr::emitArgumentPushCode(ASMText &out,
                                       const string &functionId,
                                       uint16_t &numBytesPushed) const
{
    /*  Push the arguments in reverse order on the stack.
        Promote byte expressions to word.
    */

    const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(getIdentifier());
    const FormalParamList *formalParams = (fd ? fd->getFormalParamList() : NULL);  // may be null
    const bool calledFunctionReceivesFirstVisibleParamInReg = (isFunctionReceivingFirstParamInReg() && !passesHiddenParam());
    size_t index = arguments->size();
    for (vector<Tree *>::reverse_iterator it = arguments->rbegin();
                                         it != arguments->rend(); it++, index--)
    {
        const Tree *expr = *it;
        string comment = "argument " + wordToString(uint16_t(index))
                         + (functionId.empty() ? "" : " of " + functionId + "()")
                         + ": " + expr->getTypeDesc()->toString();

        // Determine which formal parameter this argument corresponds to, if any.
        // (A function taking an ellipsis may receive more arguments that it has declared parameters.)

        assert(index > 0);
        size_t fpIndex = index - 1;
        const FormalParameter *param = NULL;
        if (formalParams && fpIndex < formalParams->size())
        {
            param = dynamic_cast<const FormalParameter *>(*(formalParams->begin() + fpIndex));
            assert(param);
        }

        const bool passInReg = (calledFunctionReceivesFirstVisibleParamInReg && fpIndex == 0);

        // Emit code depending on the argument type.

        const VariableExpr *ve = expr->asVariableExpr();
        const UnaryOpExpr *unary = dynamic_cast<const UnaryOpExpr *>(expr);

        if (const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(expr))
        {
            out.ins("LEAX", sle->getArg(), sle->getEscapedVersion());
            emitPushSingleArg(out, passInReg, true, numBytesPushed, "X", comment);
        }
        else if (ve && ve->getType() == ARRAY_TYPE)  // if argument is an array
        {
            out.ins("LEAX", ve->getFrameDisplacementArg(), "address of array " + ve->getId());
            emitPushSingleArg(out, passInReg, true, numBytesPushed, "X", comment);
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
            emitPushSingleArg(out, passInReg, true, numBytesPushed, "X", comment);
        }
        else if (expr->getType() == CLASS_TYPE)  // if passing a struct by value
        {
            assert(!passInReg);

            // Emit the struct expression as an l-value, i.e., compute its address in X.
            if (!expr->emitCode(out, true))
                return false;

            if (param && param->isIntegral() && !param->isLong() && expr->isRealOrLong())  // if passing real/long to short integral
            {
                // Emit code to convert the number at X into the integral expected by the function.

                // Pass the address of the argument in D.
                out.ins("TFR", "X,D");

                // Push enough bytes in the stack to contain the integral argument.
                int16_t paramSize = param->getTypeSize();
                int16_t passedSize = (paramSize == 1 ? 2 : paramSize);
                out.ins("LEAS", "-" + wordToString(passedSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", paramSize == 1 ? "1,S" : ",S");

                callUtility(out, "init" + string(expr->isLong() ? "" : (param->isSigned() ? "Signed" : "Unsigned"))
                                        + (param->getType() == BYTE_TYPE ? "Byte" : "Word")
                                        + "From" + (expr->isLong() ? "DWord" : (expr->isSingle() ? "Single" : "Double")),
                                 "convert argument to l-value at X");

                numBytesPushed += uint16_t(passedSize);

                if (paramSize == 1)
                {
                    out.ins("LDB", "1,S", "LSB of argument");
                    out.ins(param->isSigned() ? "SEX" : "CLRA", "", "promoting byte argument to word");
                    out.ins("STA", ",S", "MSB of argument");
                }
            }
            else if ((param && param->isLong() && expr->isReal())  // if passing real to long
                  || (param && param->isReal() && expr->isLong()))  // or passing long to real
            {
                // Emit code to convert the argument at X into the type expected by the function.

                // Pass the address of the argument in D.
                out.ins("TFR", "X,D");

                // Push enough bytes in the stack to contain the long argument.
                int16_t passedSize = param->getTypeSize();
                out.ins("LEAS", "-" + wordToString(passedSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", ",S");

                // Pass the signedness flag in the carry flag.
                if ((param->isLong() && param->isSigned()) || (expr->isLong() && expr->isSigned()))
                    out.ins("ORCC", "#$01", "C=1 means signed");
                else
                    out.ins("ANDCC", "#$FE", "C=0 means unsigned");

                if (param->isLong())
                    callUtility(out, "initDWordFrom" + string(expr->isSingle() ? "Single" : "Double"),
                                     "convert real argument to long at X");
                else
                    callUtility(out, "init" + string(param->isSingle() ? "Single" : "Double") + "FromDWord",
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
                    if (structSizeInBytes != 4 && structSizeInBytes != 5)
                        out.ins("LDD", "#" + wordToString(structSizeInBytes), "size of " + expr->getTypeDesc()->toString());
                    out.ins("LEAS", "-" + wordToString(structSizeInBytes) + ",S",
                                    "pass " + expr->getTypeDesc()->toString() + " by value");
                    const char *u;
                    switch (structSizeInBytes)
                    {
                    case 4:  u = "push4ByteStruct"; break;
                    case 5:  u = "push5ByteStruct"; break;
                    default: u = "pushStruct";
                    }
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
        else
        {
            if (!expr->emitCode(out, false))
                return false;
            if (expr->getType() == BYTE_TYPE)
                out.ins(expr->getTypeDesc()->isSigned ? "SEX" : "CLRA", "", "promoting byte argument to word");

            if (param && param->isRealOrLong())  // if passing basic type to real/long
            {
                assert(!passInReg);

                int16_t paramSize = param->getTypeSize();
                out.ins("LEAS", "-" + wordToString(paramSize) + ",S", "slot for argument " + wordToString(uint16_t(index)));

                // Pass the address of the argument slot to be filled to the utility routine.
                out.ins("LEAX", ",S");

                callUtility(out, "init" + string(param->isLong() ? "DWord" : (param->isSingle() ? "Single" : "Double"))
                                 + "From" + (expr->isLong() ? "" : (expr->isSigned() ? "Signed" : "Unsigned")) + "Word");

                numBytesPushed += paramSize;
            }
            else
            {
                emitPushSingleArg(out, passInReg, false, numBytesPushed, "B,A", comment);
            }
        }
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

    if (!emitArgumentPushCode(out, functionId, numBytesPushed))
        return false;


    // If return value is struct/union, pass address of allocated local struct as hidden parameter.
    if (passesHiddenParam())
    {
        assert(returnValueDeclaration);
        out.ins("LEAX", returnValueDeclaration->getFrameDisplacementArg(0),
                                "address of struct/union to be returned by " + functionId + "()");
        if (isFunctionReceivingFirstParamInReg())
            out.ins("TFR", "X,D", "pass hidden arg in register");
        else
        {
            out.ins("PSHS", "X", "hidden argument");
            numBytesPushed += 2;
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
            return false;  // error expected to have been reported by FunctionChecker
        out.ins("LBSR", functionLabel);
    }
    else if (ie != NULL && funcPtrVarDecl != NULL)  // if called address is in a variable, e.g., pf()
    {
        assert(functionId.empty());

        // Prepare a temporary VariableExpr with the function pointer variable declaration,
        // and have it emit code that loads that function pointer in D.
        //
        VariableExpr ve(ie->getId());
        ve.setTypeDesc(TranslationUnit::getTypeManager().getIntType(WORD_TYPE, false));
        ve.setDeclaration(funcPtrVarDecl);
        if (!ve.emitCode(out, false))
            return false;
        out.ins("TFR", "D,X");
        out.ins("JSR", ",X");
    }
    else  // called address is (*pf)() or object.member().
    {
        assert(functionId.empty());

        string jsrArg = ",X";
        const UnaryOpExpr *unary = dynamic_cast<UnaryOpExpr *>(function);
        if (unary && unary->getOperator() == UnaryOpExpr::INDIRECTION)  // if (*pf)()
        {
            const VariableExpr *ve = unary->getSubExpr()->asVariableExpr();
            if (ve)
                jsrArg = "[" + ve->getFrameDisplacementArg(0) + "]";
            else if (!function->emitCode(out, true))  // get function address in X
                return false;
        }
        else  // object.member()
        {
            if (!function->emitCode(out, false))
                return false;
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
