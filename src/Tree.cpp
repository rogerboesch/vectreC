/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2026 Pierre Sarrazin <http://sarrazip.com/>

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

#include "Tree.h"

#include "TranslationUnit.h"
#include "ClassDef.h"
#include "WordConstantExpr.h"
#include "DWordConstantExpr.h"
#include "RealConstantExpr.h"
#include "StringLiteralExpr.h"
#include "VariableExpr.h"
#include "IdentifierExpr.h"
#include "Declaration.h"
#include "BinaryOpExpr.h"
#include "UnaryOpExpr.h"
#include "CastExpr.h"
#include "ObjectMemberExpr.h"
#include "TreeSequence.h"
#include "TypeManager.h"
#include "ConditionalExpr.h"

using namespace std;


extern int lineno;
extern string sourceFilename;


Tree::Tree()
  : scope(NULL),
    sourceFilename(::sourceFilename),
    lineno(::lineno),
    typeDesc(TranslationUnit::getTypeManager().getVoidType()),
    yieldedValueNeeded(true)
{
    assert(typeDesc);
}


Tree::Tree(const TypeDesc *td)
  : scope(NULL),
    sourceFilename(::sourceFilename),
    lineno(::lineno),
    typeDesc(td),
    yieldedValueNeeded(true)
{
    assert(typeDesc);
    assert(typeDesc->type != SIZELESS_TYPE);
}


/*virtual*/
Tree::~Tree()
{
}


void Tree::setScope(Scope *s)
{
    scope = s;
}


const Scope *Tree::getScope() const
{
    return scope;
}


Scope *Tree::getScope()
{
    return scope;
}


void Tree::pushScopeIfExists() const
{
    if (scope != NULL)
        TranslationUnit::instance().pushScope(scope);
}


void Tree::popScopeIfExists() const
{
    if (scope != NULL)
        TranslationUnit::instance().popScope();
}


/*virtual*/
void
Tree::checkSemantics(Functor & /*f*/)
{
}


/*virtual*/
CodeStatus
Tree::emitCode(ASMText & /*out*/, bool /*lValue*/) const
{
    return false;
}


void
Tree::setLineNo(const char *srcFilename, int no)
{
    sourceFilename = srcFilename;
    lineno = no;
}


void
Tree::setLineNo(const string &srcFilename, int no)
{
    sourceFilename = srcFilename;
    lineno = no;
}


string
Tree::getLineNo() const
{
    char temp[1024];
    snprintf(temp, sizeof(temp), "%s:%d", sourceFilename.c_str(), lineno);
    return temp;
}


void
Tree::copyLineNo(const Tree &tree)
{
    sourceFilename = tree.sourceFilename;
    lineno = tree.lineno;
}


// Write a comment that gives the source file name and line number.
//
void
Tree::writeLineNoComment(ASMText &out, const string &text) const
{
    stringstream comment;
    if (lineno >= 1)
        comment << "Line " << sourceFilename << ":" << lineno;
    if (lineno >= 1 && !text.empty())
        comment << ": ";
    if (!text.empty())
        comment << text;
    out.emitComment(comment.str());
}


BasicType
Tree::getType() const
{
    if (!typeDesc)
        return VOID_TYPE;
    assert(typeDesc->isValid());
    return typeDesc->type;
}


int16_t
Tree::getTypeSize() const
{
    return TranslationUnit::instance().getTypeSize(*typeDesc);
}


int16_t
Tree::getPointedTypeSize() const
{
    assert(typeDesc && typeDesc->isValid());
    assert(typeDesc->isPtrOrArray());

    if (typeDesc->type == ARRAY_TYPE)
    {
        // If this tree is an array variable, get the size from its Declaration.
        // This is necessary in the case of an array because Declaration::getVariableSizeInBytes()
        // knows the array dimensions.
        //
        const VariableExpr *ve = asVariableExpr();
        if (ve)
        {
            const Declaration *decl = ve->getDeclaration();
            assert(decl);
            uint16_t sizeInBytes = 0;
            if (!decl->getVariableSizeInBytes(sizeInBytes, true))  // true means skipFirstDimensionIfArray, because we want the *pointed* size
                assert(!"Declaration::getVariableSizeInBytes() failed");
            return sizeInBytes;
        }

        if (const ObjectMemberExpr *ome = dynamic_cast<const ObjectMemberExpr *>(this))
        {
            const ClassDef *classDef = ome->getClass();
            assert(classDef);
            const ClassDef::ClassMember *member = classDef->getDataMember(ome->getMemberName());
            assert(member);
            vector<uint16_t> dims = member->getArrayDimensions();

            // The following code is similar to Declaration::getVariableSizeInBytes():
            assert(dims.size() > 0);
            uint16_t totalNumElements = 1;
            for (vector<uint16_t>::const_iterator it = dims.begin() + 1; it != dims.end(); ++it)
            {
                uint16_t dim = *it;
                if (dim == 0)
                    return 0;
                totalNumElements *= dim;
            }
            uint16_t sizeInBytes = uint16_t(ome->getFinalArrayElementTypeSize()) * totalNumElements;
            return sizeInBytes;
        }
    }

    return TranslationUnit::instance().getTypeSize(*typeDesc->pointedTypeDesc);
}


const TypeDesc *
Tree::getFinalArrayElementType() const
{
    assert(typeDesc && typeDesc->isValid());
    assert(typeDesc->isPtrOrArray());

    const TypeDesc *td = NULL;

    if (typeDesc->type == POINTER_TYPE)
    {
        if (typeDesc->pointedTypeDesc->type != ARRAY_TYPE)
            return typeDesc->pointedTypeDesc;
        td = typeDesc->pointedTypeDesc;
    }
    else
        td = typeDesc;

    while (td->type == ARRAY_TYPE)
    {
        td = td->pointedTypeDesc;
        assert(td && td->isValid());
    }
    return td;
}


// Gets the size (in bytes) of the final type (non-array) of this tree's array type.
// Example: If this tree is of type T[][][], then returns the size of type T.
//
int16_t
Tree::getFinalArrayElementTypeSize() const
{
    return TranslationUnit::instance().getTypeSize(*getFinalArrayElementType());
}


// Returns true if this tree represents an unsigned expression
// or a positive (16-bit) constant that can be seen as unsigned.
//
bool
Tree::isUnsignedOrPositiveConst() const
{
    if (!isSigned())
        return true;
    uint16_t value = 0;
    if (!evaluateConstantExpr(value))
        return false;
    if (getType() == BYTE_TYPE)
        return (value & 0xFF) <= 0x7F;
    return value <= 0x7FFF;
}


const TypeDesc *
Tree::getTypeDesc() const
{
    assert(!typeDesc || typeDesc->isValid());
    return typeDesc;
}


const string &
Tree::getClassName() const
{
    assert(typeDesc && typeDesc->isValid());
    assert(typeDesc->type == CLASS_TYPE);
    return typeDesc->className;
}


void
Tree::setTypeDesc(const TypeDesc *td)
{
    assert(td && td->isValid());
    assert(td->type != SIZELESS_TYPE);
    typeDesc = td;
}


// Indirection: 'tree' must be an expression of type pointer or array.
//
void
Tree::setTypeToPointedType(const Tree &tree)
{
    assert(tree.typeDesc && tree.typeDesc->isValid());
    assert(tree.typeDesc->isPtrOrArray());

    typeDesc = tree.typeDesc->getPointedTypeDesc();  // TypeDesc is now shared between the two Trees
    assert(typeDesc && typeDesc->isValid());
}


// Set this tree's type to POINTER_TYPE and the pointed type
// to that of 'treeOfPointedType'.
// Example: If treeOfPointedType is CLASS_TYPE with class name "Foo",
//          then this tree will be of type (POINTER_TYPE, CLASS_TYPE, "Foo").
//
void
Tree::setPointerType(const Tree &treeOfPointedType)
{
    assert(treeOfPointedType.typeDesc->isValid());

    typeDesc = TranslationUnit::getTypeManager().getPointerTo(treeOfPointedType.getTypeDesc());

    assert(typeDesc->isValid());
}


bool
Tree::isExpressionAlwaysTrueOrFalse(bool boolToReturnIfZero) const
{
    uint16_t value = 0;
    if (evaluateConstantExpr(value))
        return value == 0 ? boolToReturnIfZero : !boolToReturnIfZero;
    if (auto dce = dynamic_cast<const DWordConstantExpr *>(this))
        return dce->getRealValue() == 0.0f ? boolToReturnIfZero : !boolToReturnIfZero;
    if (auto dce = dynamic_cast<const RealConstantExpr *>(this))
        return dce->getRealValue() == 0.0f ? boolToReturnIfZero : !boolToReturnIfZero;

    if (auto binOp = dynamic_cast<const BinaryOpExpr *>(this))
    {
        // If condition is unsignedExpr < 0, it is always false.
        // If condition is unsignedExpr >= 0, it is always true.
        if (binOp->getLeft()->isIntegral() && ! binOp->getLeft()->isSigned())
        {
            uint16_t rightValue = 0;
            if (binOp->getOperator() == BinaryOpExpr::INFERIOR
                    && binOp->getRight()->evaluateConstantExpr(rightValue)
                    && rightValue == 0)
                return boolToReturnIfZero;

            if (binOp->getOperator() == BinaryOpExpr::SUPERIOR_OR_EQUAL
                    && binOp->getRight()->evaluateConstantExpr(rightValue)
                    && rightValue == 0)
                return !boolToReturnIfZero;
        }
    }

    return false;
}


bool
Tree::isExpressionAlwaysTrue() const
{
    return isExpressionAlwaysTrueOrFalse(false);
}


bool
Tree::isExpressionAlwaysFalse() const
{
    return isExpressionAlwaysTrueOrFalse(true);
}


void
Tree::callUtility(ASMText &out, const string &utilitySubRoutine, const string &comment)
{
    TranslationUnit::instance().registerNeededUtility(utilitySubRoutine);
    out.ins("LBSR", utilitySubRoutine, comment);
}


void
Tree::errormsg(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    diagnoseVa(true, getLineNo(), fmt, ap);
    va_end(ap);
}


void
Tree::warnmsg(const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    diagnoseVa(false, getLineNo(), fmt, ap);
    va_end(ap);
}


void
Tree::diagnose(bool isError, const char *fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    diagnoseVa(isError, getLineNo(), fmt, ap);
    va_end(ap);
}


void
Tree::errormsg(const Tree *optionalTree, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    diagnoseVa(true, optionalTree ? optionalTree->getLineNo() : getSourceLineNo(), fmt, ap);
    va_end(ap);
}


void
Tree::warnmsg(const Tree *optionalTree, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    diagnoseVa(false, optionalTree ? optionalTree->getLineNo() : getSourceLineNo(), fmt, ap);
    va_end(ap);
}


const VariableExpr *
Tree::asVariableExpr() const
{
    if (const VariableExpr *ve = dynamic_cast<const VariableExpr *>(this))
        return ve;
    if (const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(this))
        return ie->getVariableExpr();
    return NULL;
}


bool
Tree::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!f.close(this))
        return false;
    return true;
}


bool
Tree::evaluateConstantExpr(uint16_t &result) const
{
    try
    {
        result = evaluateConstantExpr();
        return true;
    }
    catch (int)
    {
        result = 0;
        return false;
    }
}


uint16_t
Tree::evaluateConstantExpr() const
{
    if (const WordConstantExpr *wce = dynamic_cast<const WordConstantExpr *>(this))
        return wce->getWordValue();

    if (const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(this))
    {
        const Tree &leftExpr = *bin->getLeft(), &rightExpr = *bin->getRight();
        uint16_t left = leftExpr.evaluateConstantExpr();   // may throw
        if (leftExpr.getType() == BYTE_TYPE && leftExpr.isSigned() && left >= 0x0080)
            left |= 0xFF00;  // sign extend
        uint16_t right = rightExpr.evaluateConstantExpr();  // may throw
        if (rightExpr.getType() == BYTE_TYPE && rightExpr.isSigned() && right >= 0x0080)
            right |= 0xFF00;  // sign extend

        switch (bin->getOperator())
        {
        case BinaryOpExpr::ADD:
            return left + right;
        case BinaryOpExpr::SUB:
            return left - right;
        case BinaryOpExpr::MUL:
            if (leftExpr.isSigned() && rightExpr.isSigned())
                return int16_t(left) * int16_t(right);
            return left * right;
        case BinaryOpExpr::DIV:
            if (right == 0)
                throw -1;
            if (leftExpr.isSigned() && rightExpr.isSigned())
                return int16_t(left) / int16_t(right);
            return left / right;
        case BinaryOpExpr::MOD:
            if (right == 0)
                throw -1;
            if (leftExpr.isSigned() && rightExpr.isSigned())
                return int16_t(left) % int16_t(right);
            return left % right;
        case BinaryOpExpr::BITWISE_OR:
            return left | right;
        case BinaryOpExpr::BITWISE_XOR:
            return left ^ right;
        case BinaryOpExpr::BITWISE_AND:
            return left & right;
        case BinaryOpExpr::LEFT_SHIFT:
            return uint16_t(left) << right;
        case BinaryOpExpr::RIGHT_SHIFT:
            if (leftExpr.isSigned())
                return int16_t(left) >> right;
            return uint16_t(left) >> right;
        case BinaryOpExpr::EQUALITY:
            return left == right;
        case BinaryOpExpr::INEQUALITY:
            return left != right;
        case BinaryOpExpr::INFERIOR_OR_EQUAL:
            return left <= right;
        case BinaryOpExpr::INFERIOR:
            return left < right;
        case BinaryOpExpr::SUPERIOR_OR_EQUAL:
            return left >= right;
        case BinaryOpExpr::SUPERIOR:
            return left > right;
        case BinaryOpExpr::LOGICAL_AND:
            return left && right;
        case BinaryOpExpr::LOGICAL_OR:
            return left || right;
        default:
            throw -1;
        }
    }

    if (const UnaryOpExpr *un = dynamic_cast<const UnaryOpExpr *>(this))
    {
        uint16_t sub = 0;
        if (un->getOperator() != UnaryOpExpr::SIZE_OF)
            sub = un->getSubExpr()->evaluateConstantExpr();  // may throw

        switch (un->getOperator())
        {
        case UnaryOpExpr::IDENTITY:
            return sub;
        case UnaryOpExpr::NEG:
            return - sub;
        case UnaryOpExpr::BOOLEAN_NEG:
            return ! sub;
        case UnaryOpExpr::BITWISE_NOT:
            return ~ sub;
        case UnaryOpExpr::SIZE_OF:
        {
            uint16_t size = 0;
            if (un->getSizeOfValue(size))
                return size;
            throw -1;
        }
        default:
            throw -1;
        }
    }

    if (const CastExpr *castExpr = dynamic_cast<const CastExpr *>(this))
    {
        if (! castExpr->getTypeDesc()->isByteOrWord())
            throw -1;  // casting to long is not considered constant b/c result would not necessarily fit uint16_t

        uint16_t sub = castExpr->getSubExpr()->evaluateConstantExpr();  // may throw

        switch (castExpr->getType())
        {
        case BYTE_TYPE:
            return castExpr->isSigned() ? ((int16_t) (int8_t) sub) : (sub & 0xFF);
        default:
            return sub;
        }
    }

    if (const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(this))
    {
        if (!ie->isEnumeratorName())
            throw -1;
        uint16_t value;
        if (TranslationUnit::getTypeManager().getEnumeratorValue(ie->getId(), value))
            return value;
    }

    if (const ConditionalExpr *ce = dynamic_cast<const ConditionalExpr *>(this))
    {
        uint16_t condValue  = ce->getConditionExpression()->evaluateConstantExpr();  // may throw
        uint16_t trueValue  = ce->getTrueExpression()->evaluateConstantExpr();       // may throw
        uint16_t falseValue = ce->getFalseExpression()->evaluateConstantExpr();      // may throw
        return condValue ? trueValue : falseValue;
    }

    throw -1;
}


bool
Tree::isNumericalLiteral() const
{
    if (dynamic_cast<const DWordConstantExpr *>(this))
        return true;
    if (dynamic_cast<const RealConstantExpr *>(this))
        return true;

    uint16_t dummy;
    if (evaluateConstantExpr(dummy))
        return true;

    // If --no-relocatable has NOT been specified, no other cases are
    // considered to be numerical literals.
    //
    if (TranslationUnit::instance().isRelocatabilitySupported())
        return false;

    // Check for identifier that designates a global array name.
    //
    if (const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(this))
    {
        const Declaration *globalDeclaration = TranslationUnit::instance().getGlobalScope().getVariableDeclaration(ie->getId(), false);
        if (globalDeclaration)
            return globalDeclaration->getTypeDesc()->isArray();
        return ie->isFuncAddrExpr();
    }

    return false;
}


bool
Tree::isCastToMultiByteType() const
{
    if (const CastExpr *castExpr = dynamic_cast<const CastExpr *>(this))
    {
        return castExpr->getTypeSize() > 1;
    }
    return false;
}


bool
Tree::is8BitConstant(uint16_t *valuePtr) const
{
    if (isCastToMultiByteType())
        return false;

    uint16_t value = 0;
    if (! evaluateConstantExpr(value))
        return false;
    if (value <= 255)
    {
        if (valuePtr != NULL)
            *valuePtr = value;
        return true;
    }

    // 0xFFB0 is -80 when cast to int16_t, but if the tree is marked UNsigned,
    // it is normally because the user wrote 0xFFB0 or 65456 and not -80.
    //
    if ((int16_t) value >= -128 && (int16_t) value < 0 && isSigned())
    {
        if (valuePtr != NULL)
            *valuePtr = uint8_t(value);
        return true;
    }
    return false;
}


bool
Tree::fits8Bits() const
{
    return getType() == BYTE_TYPE || is8BitConstant(); 
}


bool
Tree::fits8BitsWithSignedness() const
{
    uint16_t value = 0;
    if (is8BitConstant(&value))
    {
        if (isSigned())
            return (int16_t) value >= -128 && (int16_t) value <= 127;  // e.g., 255 is constant that fits 8 bits, but does not fit signed char type
        return (int16_t) value >= 0;  // reject -128..-1 if unsigned type
    }
    return getType() == BYTE_TYPE;
}


// Calls delete on a tree pointer ('member') if it points to a tree to be replaced ('oldAddr'),
// then assigns 'newAddr' to that pointer ('member').  Does nothing otherwise.
//
bool
Tree::deleteAndAssign(Tree *&member, Tree *oldAddr, Tree *newAddr)
{
    if (member == oldAddr)
    {
        delete member;
        member = newAddr;
        return true;
    }
    return false;
}


bool
Tree::definesOnlyAMatrixOfCharsAndHasInitializer(const TypeDesc &varTypeDesc) const
{
    if (!varTypeDesc.isArray())
        return false;

    const TypeDesc *finalArrayTD = varTypeDesc.getFinalArrayType();
    if (finalArrayTD->type != BYTE_TYPE)
        return false;

    const TreeSequence *seq = dynamic_cast<const TreeSequence *>(this);
    if (seq == NULL)
        return false;  // not supposed to happen

    return seq->isTreeSequenceWithOnlyStringLiterals();
}


bool
Tree::isArrayWithOnlyNumericalLiteralInitValues(const TypeDesc &varTypeDesc) const
{
    if (!varTypeDesc.isArray())
        return false;

    if (dynamic_cast<const StringLiteralExpr *>(this) && varTypeDesc.isArrayOfChar())
        return true;  // passes for an array of byte integers

    const TreeSequence *seq = dynamic_cast<const TreeSequence *>(this);
    if (seq == NULL)
        return false;

    return seq->isTreeSequenceWithOnlyNumericalLiterals();
}


bool
Tree::isStructWithOnlyNumericalLiteralInitValues(const TypeDesc &varTypeDesc) const
{
    if (!varTypeDesc.isStruct())
        return false;

    const TreeSequence *seq = dynamic_cast<const TreeSequence *>(this);
    if (seq == NULL)
        return false;

    return seq->isTreeSequenceWithOnlyNumericalLiterals();
}


bool
Tree::isNumericalLiteralCastToOtherType() const
{
    const CastExpr *ce = dynamic_cast<const CastExpr *>(this);
    return ce && ce->getSubExpr()->isNumericalLiteral();
}


// Returns true if initExpr is (C *) "...", where C is a character type, signed or not.
//
bool
Tree::isStringLiteralCastToPtrToAnyChar() const
{
    const CastExpr *ce = dynamic_cast<const CastExpr *>(this);
    if (!ce)
        return false;
    auto sle = dynamic_cast<const StringLiteralExpr *>(ce->getSubExpr());
    if (!sle)
        return false;
    const TypeDesc *td = ce->getTypeDesc();
    if (td->type != POINTER_TYPE)
        return false;
    return td->getPointedTypeDesc()->type == BYTE_TYPE;
}


bool
Tree::isStaticallyInitializable(const TypeDesc &varTypeDesc, bool allowStringLiterals) const
{
    if (definesOnlyAMatrixOfCharsAndHasInitializer(varTypeDesc))
        return true;
    
    if (isArrayWithOnlyNumericalLiteralInitValues(varTypeDesc))
        return true;

    uint16_t initValue = 0;
    if (evaluateConstantExpr(initValue))
        return true;
    
    if (dynamic_cast<const DWordConstantExpr *>(this))
        return true;

    if (isNumericalLiteralCastToOtherType())
        return true;

    if (dynamic_cast<const RealConstantExpr *>(this))
        return true;

    // Check for array or struct initializers.
    if (const TreeSequence *seq = dynamic_cast<const TreeSequence *>(this))
    {
        if (varTypeDesc.type == CLASS_TYPE)  // if struct
        {
            const ClassDef *cl = TranslationUnit::instance().getClassDef(varTypeDesc.className);
            assert(cl && cl->getType() == CLASS_TYPE);

            size_t memberIndex = 0;
            for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it, ++memberIndex)
            {
                const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
                if (member == NULL)
                {
                    // Tolerate having more expressions in 'seq' than data members.
                    continue;
                }

                if (! (*it)->isStaticallyInitializable(*member->getTypeDesc(), allowStringLiterals))
                    return false;
            }
            return true;
        }
        if (varTypeDesc.type == ARRAY_TYPE)
        {
            const TypeDesc *arrayElemTypeDesc = varTypeDesc.getPointedTypeDesc();
            for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
            {
                if (! (*it)->isStaticallyInitializable(*arrayElemTypeDesc, allowStringLiterals))
                    return false;
            }
            return true;
        }
        return false;  // neither struct nor array
    }

    if (allowStringLiterals && (dynamic_cast<const StringLiteralExpr *>(this) || isStringLiteralCastToPtrToAnyChar()))
        return true;

    if (varTypeDesc.isConstant() && ! TranslationUnit::instance().isRelocatabilitySupported())
        return true;

    return false;  // other types of trees
}


void
Tree::setYieldedValueNeeded(bool needed)
{
    yieldedValueNeeded = needed;
}


bool
Tree::isYieldedValueNeeded() const
{
    return yieldedValueNeeded;
}
