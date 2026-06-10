/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2023 Pierre Sarrazin <http://sarrazip.com/>

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

#include "UnaryOpExpr.h"

#include "TranslationUnit.h"
#include "VariableExpr.h"
#include "WordConstantExpr.h"
#include "VariableExpr.h"
#include "ObjectMemberExpr.h"
#include "BinaryOpExpr.h"
#include "IdentifierExpr.h"
#include "CastExpr.h"
#include "Declaration.h"
#include "StringLiteralExpr.h"

#include <assert.h>

using namespace std;


const char *
UnaryOpExpr::getOperatorName(Op op)
{
    switch (op)
    {
    case IDENTITY: return "identity";
    case NEG: return "arithmetic negation";
    case POSTINC: return "post-increment";
    case POSTDEC: return "post-decrement";
    case PREINC: return "pre-increment";
    case PREDEC: return "pre-decrement";
    case ADDRESS_OF: return "address-of";
    case INDIRECTION: return "indirection";
    case BOOLEAN_NEG: return "boolean negation";
    case BITWISE_NOT: return "bitwise not";
    case SIZE_OF: return "sizeof";
    default: return "<UNKNOWN UNARY OPERATOR>";
    }
}

UnaryOpExpr::UnaryOpExpr(Op op, Tree *e)
  : Tree(),
    oper(op),
    dereferencingVoidAllowed(false),
    subExpr(e),
    sizeofArgTypeDesc(NULL),  // will be determined by checkSemantics()
    resultDeclaration(NULL)
{
    assert(subExpr != NULL);
}


UnaryOpExpr::UnaryOpExpr(const TypeDesc *_typeDesc)
  : Tree(),
    oper(SIZE_OF),
    dereferencingVoidAllowed(false),
    subExpr(NULL),
    sizeofArgTypeDesc(_typeDesc),
    resultDeclaration(NULL)
{
}


/*virtual*/
UnaryOpExpr::~UnaryOpExpr()
{
    delete resultDeclaration;
    delete subExpr;
}


UnaryOpExpr::Op UnaryOpExpr::getOperator() const
{
    return oper;
}


const Tree *
UnaryOpExpr::getSubExpr() const
{
    return subExpr;
}


Tree *
UnaryOpExpr::getSubExpr()
{
    return subExpr;
}


void
UnaryOpExpr::checkSemantics(Functor & /*f*/)
{
    bool requireLValueSubExpr = false;
    bool declareTemporary = false;

    switch (oper)
    {
        case ADDRESS_OF:
            assert(subExpr != NULL);
            requireLValueSubExpr = true;
            break;

        case INDIRECTION:
            assert(subExpr != NULL);
            if (subExpr->getType() == POINTER_TYPE || subExpr->getType() == ARRAY_TYPE)
            {
                if (subExpr->getTypeDesc()->pointedTypeDesc->type == VOID_TYPE && !dereferencingVoidAllowed)
                    errormsg("indirection of a pointer to void");
            }
            else if (subExpr->getType() == BYTE_TYPE || subExpr->getType() == WORD_TYPE || subExpr->getType() == VOID_TYPE)
                ;  // already warned about this in ExpressionTypeSetter::processUnaryOp
            else
                errormsg("indirection of a non-pointer");
            break;

        case SIZE_OF:
            if (!sizeofArgTypeDesc)  // should have been set by ctor, or by ExpressionTypeSetter calling setSizeofArgTypeDesc()
                errormsg("sizeof argument is invalid");
            break;

        case PREINC:
        case PREDEC:
        case POSTINC:
        case POSTDEC:
        {
            requireLValueSubExpr = true;
            const char *prefix = (oper == PREINC || oper == POSTINC ? "in" : "de");
            if (subExpr->getType() == ARRAY_TYPE)
                errormsg("cannot %screment array name", prefix);
            else if (subExpr->getTypeDesc()->isConstAtFirstLevel())
                warnmsg("%scrementing expression of type `%s' is not const-correct",
                        prefix, subExpr->getTypeDesc()->toString().c_str());

            if (subExpr->getTypeDesc()->isPtrToFunction())
                warnmsg("%scrementing a function pointer", prefix);

            if ((oper == POSTINC || oper == POSTDEC) && getTypeDesc()->isRealOrLong())
                declareTemporary = true;
            break;
        }

        case NEG:
            if (getTypeDesc()->isRealOrLong())
                declareTemporary = true;
            if (subExpr->getType() == ARRAY_TYPE)
                errormsg("cannot use minus operator on array name");
            break;

        case BITWISE_NOT:
            if (subExpr->getType() == ARRAY_TYPE)
                errormsg("cannot do bitwise negation on array name");
            else if (getTypeDesc()->isLong())
                declareTemporary = true;
            break;

        default:
            ;
    }

    if (requireLValueSubExpr && !subExpr->isLValue() && subExpr->getType() != VOID_TYPE)
    {
        errormsg("l-value required as operand of %s", getOperatorName(oper));
            // Error message not issued if subExpr is void because it has
            // already been done by ExpressionTypeSetter::processUnaryOp().
    }

    if (declareTemporary)
    {
        // The result of the expression must be stored in a temporary, because it
        // cannot be left in D or B.
        resultDeclaration = Declaration::declareHiddenVariableInCurrentScope(*this);
    }
}


// Determines if 'tree', which must be the argument of an indirection operator,
// is of the form var++, where 'var' is a pointer to a byte or word.
// Returns the VariableExpr if true, NULL otherwise.
//
const VariableExpr *
UnaryOpExpr::isPostIncOfPtrToSmallType(const Tree &tree)
{
    const UnaryOpExpr *unary = dynamic_cast<const UnaryOpExpr *>(&tree);
    if (!unary)
        return NULL;
    if (unary->getOperator() != POSTINC)
        return NULL;
    const VariableExpr *ve = unary->getSubExpr()->asVariableExpr();
    if (!ve)
        return NULL;
    if (ve->getType() != POINTER_TYPE)
        return NULL;
    const TypeDesc *pointedTD = ve->getTypeDesc()->getPointedTypeDesc();
    int16_t sizeInBytes = TranslationUnit::instance().getTypeSize(*pointedTD);
    return sizeInBytes <= 2 ? ve : NULL;
}


static const BinaryOpExpr *
isArrayRef(const Tree *expr)
{
    const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(expr);
    if (bin == NULL)
        return NULL;
    if (bin->getOperator() == BinaryOpExpr::ARRAY_REF)
        return bin;
    return NULL;
}


void
UnaryOpExpr::setSizeofArgTypeDesc()
{
    assert(oper == SIZE_OF);

    if (subExpr != NULL && sizeofArgTypeDesc == NULL)  // if sizeof unary_expr instead of sizeof(type)
    {
        sizeofArgTypeDesc = subExpr->getTypeDesc();
        assert(sizeofArgTypeDesc != NULL);
    }

    assert(sizeofArgTypeDesc != NULL);
}


void
UnaryOpExpr::checkForSizeOfUnknownStruct()
{
    if (oper != SIZE_OF || subExpr != NULL)
        return;

    assert(sizeofArgTypeDesc);  // should have been set by setSizeofArgTypeDesc()

    if (sizeofArgTypeDesc->type != CLASS_TYPE)
        return;
    if (TranslationUnit::instance().getClassDef(sizeofArgTypeDesc->className) != NULL)
        return;

    errormsg("cannot take sizeof unknown struct or union '%s'", sizeofArgTypeDesc->className.c_str());
}


bool
UnaryOpExpr::getSizeOfValue(uint16_t &size) const
{
    size = 0;  // ensure defined value
    assert(sizeofArgTypeDesc != NULL);  // should have been set by a ctor or by checkSemantics()

    if (subExpr == NULL)
    {
        if (sizeofArgTypeDesc->type == CLASS_TYPE && TranslationUnit::instance().getClassDef(sizeofArgTypeDesc->className) == NULL)
            return false;  // error msg already emitted by checkForSizeOfUnknownStruct()

        size = TranslationUnit::instance().getTypeSize(*sizeofArgTypeDesc);
        return true;
    }

    if (const VariableExpr *ve = subExpr->asVariableExpr())
    {
        const Declaration *decl = ve->getDeclaration();
        if (! decl->isCompleteType())
        {
            errormsg("invalid application of `sizeof' to incomplete type `%s'", decl->getTypeDesc()->toString().c_str());
            return false;
        }
        if (!decl->getVariableSizeInBytes(size))
        {
            errormsg("invalid dimensions for array`%s'", ve->getId().c_str());
            return false;
        }
        return true;
    }

    if (const ObjectMemberExpr *obj = dynamic_cast<const ObjectMemberExpr *>(subExpr))
    {
        const ClassDef *cl = obj->getClass();
        if (cl == NULL)
        {
            errormsg("taking size of undefined struct %s", obj->getClassName().c_str());
            return false;
        }
        const ClassDef::ClassMember *member = cl->getDataMember(obj->getMemberName());
        if (member == NULL)
        {
            errormsg("taking size of undefined member %s oF %s", obj->getMemberName().c_str(), obj->getClassName().c_str());
            return false;
        }
        size = member->getSizeInBytes();
        return true;
    }

    if (const BinaryOpExpr *bin = isArrayRef(subExpr))
    {
        // We have sizeof(a[0]) or sizeof(a[0][0]), etc.
        // (The actual indices do not matter.)
        // Count the number of [] in a[0][0]...[0] until we get to 'a'.
        //
        size_t numDimsInSizeof = 0;
        const VariableExpr *ve = NULL;
        const ObjectMemberExpr *ome = NULL;
        const Tree *left = NULL;
        do
        {
            ++numDimsInSizeof;
            left = bin->getLeft();
            assert(left != NULL);
            ve = left->asVariableExpr();
            ome = dynamic_cast<const ObjectMemberExpr *>(left);
            if (ve != NULL || ome != NULL)
                break;
        } while ((bin = isArrayRef(left)) != NULL);

        if (ve != NULL)  // if reached a variable
        {
            // Get the dimensions of the array.
            // Example: if the array is int [5][3][4], we want { 5, 3, 4 }.
            //
            const Declaration *decl = ve->getDeclaration();
            assert(decl != NULL);
            const std::vector<uint16_t> &arrayDims = decl->getArrayDimensions();
            if (numDimsInSizeof > arrayDims.size())
            {
                errormsg("sizeof() argument uses more indices (%u) than the array accepts (%u)",
                        unsigned(numDimsInSizeof), unsigned(arrayDims.size()));
                return false;
            }

            // The number of array elements to be counted by sizeof() is the product
            // of the array dimensions except those reprensented by the indices used
            // in the sizeof() argument.
            // Example: If 'a' is a 4-dimension array, then for sizeof(a[0]) we compute
            // the product of the *last* 3 dimensions.
            // If the number of indices is equal to the number of array dimensions,
            // we take the size of the final array element.
            //
            size = ve->getFinalArrayElementTypeSize();  // initialize product with size in bytes of final array element
            for (size_t index = numDimsInSizeof; index < arrayDims.size(); ++index)
            {
                uint16_t dim = arrayDims[index]; //*it;
                if (size > 0xFFFF / dim)  // test if size * dim is too large, without overflowing in the test itself
                {
                    errormsg("sizeof() value not representable in 16 bits");
                    return false;
                }
                size *= dim;
            }
            return true;
        }

        if (ome != NULL)  // if reached a struct member access
        {
            const ClassDef::ClassMember *member = ome->getClassMember();
            if (!member)
                return true;  // error message issued

            vector<uint16_t> dims = member->getArrayDimensions();

            vector<uint16_t> dimsDueToType;
            ome->getTypeDesc()->appendDimensions(dimsDueToType);

            size = ome->getFinalArrayElementTypeSize();

            size_t index = 0;
            for ( ; index < dimsDueToType.size(); ++index)
                size *= dimsDueToType[index];

            for (index = numDimsInSizeof; index < dims.size(); ++index)
            {
                uint16_t dim = dims[index];
                if (size > 0xFFFF / dim)  // test if size * dim is too large, without overflowing in the test itself
                {
                    errormsg("sizeof() value not representable in 16 bits");
                    return false;
                }
                size *= dim;
            }
            return true;
        }
    }

    // sizeof("literal") is the size between the quotes, plus 1 for the terminating '\0'.
    if (const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(subExpr))
    {
        size_t decodedLength = sle->getDecodedLength();
        if (decodedLength + 1 > 0xFFFF)
        {
            errormsg("string literal used in sizeof() is too large (%u bytes)", decodedLength + 1);
            return false;
        }
        size = uint16_t(decodedLength) + 1;
        return true;
    }

    if (sizeofArgTypeDesc)
    {
        if (sizeofArgTypeDesc->type == ARRAY_TYPE)
        {
            vector<uint16_t> arrayDimensions;
            sizeofArgTypeDesc->appendDimensions(arrayDimensions);
            int16_t finalElemSizeInBytes = TranslationUnit::instance().getTypeSize(*sizeofArgTypeDesc->getFinalArrayType());
            int16_t s = computeDimensionsProduct(arrayDimensions, 0, finalElemSizeInBytes);
            if (s < 0)
            {
                errormsg("unsupported sizeof (on type `%s')", sizeofArgTypeDesc->toString().c_str());
                return true;
            }
            size = uint16_t(s);
            return true;
        }

        // Not an array type.
        if (sizeofArgTypeDesc->type == CLASS_TYPE && TranslationUnit::instance().getClassDef(sizeofArgTypeDesc->className) == NULL)
            return false;  // error msg already emitted by checkForSizeOfUnknownStruct()

        size = TranslationUnit::instance().getTypeSize(*sizeofArgTypeDesc);
        return true;
    }

    errormsg("unexpected type %s as argument of sizeof()", subExpr->getTypeDesc()->toString().c_str());
    return false;
}


/*virtual*/
CodeStatus
UnaryOpExpr::emitCode(ASMText &out, bool lValue) const
{
    if (!isYieldedValueNeeded())  // issue a line comment if this unary operation is a statement
        writeLineNoComment(out, getOperatorName(oper));

    // Check if tree is a constant expression. Emit a single instruction if it is.
    //
    uint16_t value;
    if (evaluateConstantExpr(value))
    {
        if (getType() == BYTE_TYPE)
            value &= 0xFF;
        out.ins(getLoadInstruction(getType()), "#" + wordToString(value, true),
                        "constant expression: " + wordToString(value, false) + " decimal, "
                        + (isSigned() ? "" : "un") + "signed");
        return true;
    }

    const char *variant = subExpr->isLong() ? "DWord" : (subExpr->isSingle() ? "Single" : "Double");

    switch (oper)
    {
        case IDENTITY:
            if (lValue)
            {
                if (! subExpr->isRealOrLong())
                    return false;
                return subExpr->emitCode(out, true);  // put address of number in X
            }
            if (!subExpr->emitCode(out, false))
                return false;
            break;

        case NEG:
        {
            if (lValue)
            {
                if (! subExpr->isRealOrLong())
                    return false;
                if (!subExpr->emitCode(out, true))  // get address of operand
                    return false;
                out.ins("TFR", "X,D", "operand");
                assert(resultDeclaration);
                out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0), "address of result of operator");
                callUtility(out, "copy" + string(variant), "preserves X");
                callUtility(out, "negate" + string(variant), "preserves X");
                return true;
            }

            // Special case: -wordVariable. Do D=0, D-=wordVariable.
            //
            if (getType() != BYTE_TYPE)
            {
                const VariableExpr *ve = subExpr->asVariableExpr();
                if (ve != NULL)
                {
                    out.ins("CLRA", "", "Negation of variable " + ve->getId());
                    out.ins("CLRB");
                    out.ins("SUBD", ve->getFrameDisplacementArg());
                    return true;
                }
            }

            if (!subExpr->emitCode(out, false))
                return false;
            if (getType() == BYTE_TYPE)
                out.ins("NEGB");
            else
            {
                out.ins("COMA");
                out.ins("COMB");
                out.ins("ADDD", "#1");
            }
            break;
        }

        case BITWISE_NOT:
            if (lValue)
            {
                if (! subExpr->isLong())
                    return false;
                if (!subExpr->emitCode(out, true))  // get address of operand
                    return false;
                out.ins("TFR", "X,D", "operand");
                assert(resultDeclaration);
                out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0), "address of result of operator");
                callUtility(out, "copy" + string(variant), "preserves X");
                out.ins("COM", ",X", "negate 32 bits at X");
                out.ins("COM", "1,X");
                out.ins("COM", "2,X");
                out.ins("COM", "3,X");
                return true;
            }
            if (!subExpr->emitCode(out, false))
                return false;
            if (getType() != BYTE_TYPE)
                out.ins("COMA");
            out.ins("COMB");
            break;

        case POSTINC:
        case PREINC:
        case POSTDEC:
        case PREDEC:
        {
            bool isInc = (oper == POSTINC || oper == PREINC);
            bool isPre = (oper == PREINC || oper == PREDEC);
            if (getTypeDesc()->isRealOrLong())
            {
                // Make X point to number to be modified.
                if (!subExpr->emitCode(out, true))
                    return false;

                if (!isPre)
                {
                    assert(resultDeclaration);
                    out.ins("PSHS", "X", "preserve address of number to inc/dec");
                    out.ins("TFR", "X,D");
                    out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0), "temporary that receives init value of inc/dec");
                    callUtility(out, "copy" + string(variant));  // preserves X
                    out.ins("PULS", "X", "point to number to inc/dec");
                }
                callUtility(out, string(isInc ? "increment" : "decrement") + variant, "inc/dec number at X");  // preserves X
                if (!isPre)
                {
                    out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0),
                                    "result of inc/dec is preserved original number");
                }
            }
            else
            {
                // If the expression to change is a variable, refer to it directly.
                // Otherwise, make X refer to the l-value.
                //
                const VariableExpr *ve = subExpr->asVariableExpr();
                string incrementedLocation, loadComment;
                if (ve != NULL)
                {
                    incrementedLocation = ve->getFrameDisplacementArg();
                    loadComment = "variable `" + ve->getId() + "', declared at " + subExpr->getLineNo();
                }
                else
                {
                    if (!subExpr->emitCode(out, true))
                        return false;
                    incrementedLocation = ",X";
                }

                if (getType() == BYTE_TYPE)
                {
                    const char *instr = (isInc ? "INC" : "DEC");
                    if (isPre)
                        out.ins(instr, incrementedLocation);
                    if (isYieldedValueNeeded())
                        out.ins("LDB", incrementedLocation);
                    if (!isPre)
                        out.ins(instr, incrementedLocation);
                }
                else
                {
                    int16_t increment = (int16_t) (subExpr->getType() == POINTER_TYPE ? subExpr->getPointedTypeSize() : 1);
                    if (increment == 0)
                    {
                        if (isYieldedValueNeeded())
                            out.ins("LDD", incrementedLocation, loadComment);
                        break;
                    }

                    if (!isInc)
                        increment = - increment;

                    // Use LEAX offset,X instead of ADDD if the increment is small enough
                    // for LEAX to be shorter than ADDD, and if the final value does not need
                    // to be left in D.
                    // LEAX is only used if the incrementedLocation refers to a C variable,
                    // because then it does not involve X (it is "__,U" or "__,PCR").
                    //
                    bool useX = (ve != NULL && !isYieldedValueNeeded() && increment >= -16 && increment <= 15);
                    out.ins(useX ? "LDX" : "LDD", incrementedLocation, loadComment);
                    string incStr = intToString(increment, false);
                    if (useX)
                        out.ins("LEAX", incStr + ",X");
                    else
                        out.ins("ADDD", "#" + incStr);
                    out.ins(useX ? "STX" : "STD", incrementedLocation);
                    if (!isPre && isYieldedValueNeeded())
                        out.ins("SUBD", "#" + incStr, "post increment yields initial value");
                }
            }
            break;
        }

        case ADDRESS_OF:
        {
            if (lValue)
                return false;  // should have been rejected before code emission

            // Special case #1: if subExpr is an array, we cannot ask for its l-value,
            // because it has none (i.e., cannot do this: word a[5]; a = 0;).
            // So in such a case, we ask subExpr->emitCode() to put the array address in D,
            // so no TFR X,D is needed.
            //
            bool subIsLValue = (subExpr->getType() != ARRAY_TYPE);
            if (!subExpr->emitCode(out, subIsLValue))
            {
                assert(false);
                return false;
            }
            if (subIsLValue)
            {
                out.ins("TFR", "X,D", "address-of operator");
                out.optimizeTFRXD();  // remove this TFR is previous instruction is LDX
            }
            break;
        }

        case INDIRECTION:
            {
                // Indirection of struct pointer as r-value not supported.
                if (!lValue && subExpr->getType() == POINTER_TYPE && subExpr->getTypeDesc()->getPointedTypeDesc()->type == CLASS_TYPE)
                {
                    errormsg("indirection of pointer to struct as an r-value not supported: use '->' operator instead");
                    return true;  // continuing is harmless and allows compiler to report further errors if any
                }

                const WordConstantExpr *wce = dynamic_cast<WordConstantExpr *>(subExpr);
                const CastExpr *castExpr = dynamic_cast<CastExpr *>(subExpr);
                const WordConstantExpr *castWCE = (castExpr != NULL
                                    ? dynamic_cast<const WordConstantExpr *>(castExpr->getSubExpr())
                                    : NULL);
                const VariableExpr *ve = NULL;
                bool checkNullPtr = TranslationUnit::instance().isNullPointerCheckingEnabled();

                if (wce != NULL || castWCE != NULL)  // if address is a constant
                {
                    uint16_t wc = (wce != NULL ? wce->getWordValue() : castWCE->getWordValue());

                    // Load directly from memory, without using X.
                    if (lValue)
                        out.ins("LDX", "#" + wordToString(wc, true), "decimal " + wordToString(wc));
                    else
                        out.ins(getLoadInstruction(getType()), wordToString(wc, true), "decimal " + wordToString(wc));
                }
                else if ((ve = isPostIncOfPtrToSmallType(*subExpr)) != NULL)
                {
                    out.ins("LDX", ve->getFrameDisplacementArg(), "get pointer " + ve->getId());

                    if (checkNullPtr)
                        callUtility(out, "check_null_ptr_x");

                    if (!lValue)
                    {
                        out.ins(getLoadInstruction(getType()), string(",X+") + (getType() == BYTE_TYPE ? "" : "+"), "indirection with post-increment");
                        out.ins("STX", ve->getFrameDisplacementArg(), "store incremented pointer " + ve->getId());
                    }
                    else
                    {
                        string leaxArg = string(getType() == BYTE_TYPE ? "1" : "2") + ",X";
                        out.ins("LEAX", leaxArg, "++");
                        out.ins("STX", ve->getFrameDisplacementArg(), "store incremented pointer " + ve->getId());
                        out.ins("LEAX", "-" + leaxArg, "post-increment yields original pointer");
                    }
                }
                else
                {
                    bool indirectionDone = false;
                    bool pointerLoadedInX = true;
                    string addressRef = ",X";
                    ve = subExpr->asVariableExpr();

                    if (subExpr->getTypeDesc()->isPtrToArray())
                    {
                        if (!subExpr->emitCode(out, false))  // get pointer value in D
                            return false;
                        if (lValue)
                        {
                            subExpr->errormsg("unsupported indirection");
                            return true;
                        }
                        // r-value.
                        indirectionDone = true;
                        pointerLoadedInX = false;
                    }
                    else if (ve != NULL)
                    {
                        string comment = "get address for indirection of variable " + ve->getId();
                        if (subExpr->getType() == ARRAY_TYPE)
                            out.ins("LEAX", ve->getFrameDisplacementArg(), comment);
                        else if (lValue || checkNullPtr)
                            out.ins("LDX", ve->getFrameDisplacementArg(), comment);
                        else  // emitting r-value, so result expected in D or B, not in X
                        {
                            assert(!lValue);
                            addressRef = "[" + ve->getFrameDisplacementArg() + "]";
                            pointerLoadedInX = false;
                        }
                    }
                    else
                    {
                        if (!subExpr->emitCode(out, false))
                            return false;
                        if (subExpr->getType() == BYTE_TYPE)
                            out.ins("CLRA", "", "pointer is actually byte; need to cast");
                        out.ins("TFR", "D,X");
                    }

                    if (pointerLoadedInX && checkNullPtr)
                        callUtility(out, "check_null_ptr_x");

                    if (!lValue && !indirectionDone)  // if emitting r-value
                        out.ins(getLoadInstruction(getType()), addressRef, "indirection");
                }
            }
            break;

        case BOOLEAN_NEG:
            assert(getType() == BYTE_TYPE);
            if (lValue)
                return false;
            
            // Check if the subexpression is another BOOLEAN_NEG. If it is, optimize for that.
            if (const UnaryOpExpr *subUE = dynamic_cast<const UnaryOpExpr *>(subExpr))
            {
                if (subUE->getOperator() == BOOLEAN_NEG)
                {
                    if (!subUE->emitCode(out, false))
                        return false;
                    out.ins("EORB", "#1", "double boolean negation");
                    return true;
                }
            }

            if (subExpr->getTypeDesc()->isRealOrLong())
            {
                if (!subExpr->emitCode(out, true))  // point to real/long with X
                    return false;
                callUtility(out, "is" + string(variant) + "Zero");
            }
            else
            {
                if (!subExpr->emitCode(out, false))
                    return false;
                if (subExpr->getType() == BYTE_TYPE)
                    out.ins("TSTB", "", "boolean negation: determine Z flag");
                else
                    out.emitCMPDImmediate(0, "boolean negation: determine Z flag");
            }
            callUtility(out, "tfrZtoB");  // 3-byte LBSR instead of the 6-byte TFR+ANDB+LSRB+LSRB sequence
            break;

        case SIZE_OF:
        {
            if (lValue)
                return false;

            uint16_t size = 0;
            if (!getSizeOfValue(size))
                return true;
            if (size == 0)
            {
                out.ins("CLRA", "", "sizeof");
                out.ins("CLRB", "", "sizeof");
            }
            else
                out.ins("LDD", "#" + wordToString(size), "sizeof");
            break;
        }

        default:
            assert(!"invalid operator");
            return false;
    }
    return true;
}


// Does not necessarily yield the resulting value in B or D.
// Useful to ForStmt.
//
CodeStatus
UnaryOpExpr::emitSimplerIfIncrement(ASMText &out)
{
    if (oper != POSTINC && oper != POSTDEC && oper != PREINC && oper != PREDEC)
        return emitCode(out, false);

    assert(subExpr != NULL);
    const VariableExpr *ve = subExpr->asVariableExpr();
    if (ve == NULL)
        return emitCode(out, false);

    switch (getType())
    {
    case BYTE_TYPE:
    {
        string instr = (oper == POSTINC || oper == PREINC ? "INC" : "DEC");
        out.ins(instr, ve->getFrameDisplacementArg());
        return true;
    }
    case WORD_TYPE:
    case POINTER_TYPE:
    {
        uint16_t inc = (getType() == POINTER_TYPE ? getPointedTypeSize() : 1);
        if (inc != 0)
        {
            out.ins("LDD", ve->getFrameDisplacementArg());
            string instr = (oper == POSTINC || oper == PREINC ? "ADDD" : "SUBD");
            out.ins(instr, "#" + wordToString(inc));
            out.ins("STD", ve->getFrameDisplacementArg());
        }
        return true;
    }
    default:
        return emitCode(out, false);
    }
}


bool
UnaryOpExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (subExpr != NULL && !subExpr->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}


void
UnaryOpExpr::replaceChild(Tree *existingChild, Tree *newChild)
{
    if (deleteAndAssign(subExpr, existingChild, newChild))
        return;
    assert(!"child not found");
}
