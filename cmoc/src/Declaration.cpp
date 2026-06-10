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

#include "Declaration.h"

#include "TranslationUnit.h"
#include "WordConstantExpr.h"
#include "StringLiteralExpr.h"
#include "ClassDef.h"
#include "TreeSequence.h"
#include "ExpressionTypeSetter.h"
#include "VariableExpr.h"
#include "ScopeCreator.h"
#include "SemanticsChecker.h"
#include "DWordConstantExpr.h"
#include "RealConstantExpr.h"
#include "FunctionCallExpr.h"
#include "IdentifierExpr.h"
#include "BinaryOpExpr.h"
#include "CastExpr.h"
#include "InitializerCodeEmitter.h"

#include <assert.h>

using namespace std;


Declaration::Declaration(const string &id, const TypeDesc *td,
                         const vector<uint16_t> &_arrayDimensions,
                         bool _isStatic, bool _isExtern)
:   Tree(td),
    variableId(id),
    frameDisplacement(FIRST_FUNC_PARAM_FRAME_DISPLACEMENT - 1),  // means "unallocated"
    arrayDimensions(_arrayDimensions),
    initializationExpr(NULL),
    label(),
    global(false),
    readOnly(false),
    hasStaticKeyword(_isStatic),
    isExtern(_isExtern),
    needsFinish(false),
    dynamicInitializerForced(false),
    arraySizeExprList()
{
    if (isExtern)
        setLabelFromVariableId();
}


Declaration::Declaration(const string &id, const TypeDesc *varTypeDesc,
                         const std::vector<Tree *> &_arraySizeExprList,
                         bool _isStatic, bool _isExtern)
:   Tree(varTypeDesc),
    variableId(id),
    frameDisplacement(FIRST_FUNC_PARAM_FRAME_DISPLACEMENT - 1),  // means "unallocated"
    arrayDimensions(),
    initializationExpr(NULL),
    label(),
    global(false),
    readOnly(false),
    hasStaticKeyword(_isStatic),
    isExtern(_isExtern),
    needsFinish(true),
    dynamicInitializerForced(false),
    arraySizeExprList(_arraySizeExprList)
{
    if (isExtern)
        setLabelFromVariableId();
}


/*virtual*/
Declaration::~Declaration()
{
    deleteVectorElements(arraySizeExprList);
    delete initializationExpr;
}


void
Declaration::setInitExpr(Tree *initExpr)
{
    initializationExpr = initExpr;
}


string
Declaration::getVariableId() const
{
    return variableId;
}


bool
Declaration::isCompleteType() const
{
    if (getType() != ARRAY_TYPE)
        return true;
    if (std::find(arraySizeExprList.begin(), arraySizeExprList.end(), (Tree *) 0) == arraySizeExprList.end())
        return true;  // no empty [] found
    return initializationExpr != NULL;  // [] found but initializer specifies size
}


// skipFirstDimensionIfArray: If this declaration is an array, do not include the first dimension
//                            in the product of all dimensions. This is useful when the caller
//                            wants the size of the type pointed to.
//
bool
Declaration::getVariableSizeInBytes(uint16_t &sizeInBytes, bool skipFirstDimensionIfArray) const
{
    if (needsFinish)
        return false;  // DeclarationFinisher failed

    sizeInBytes = 0;

    if (getType() == ARRAY_TYPE)
    {
        if (arrayDimensions.size() == 0)
            return false;
        uint16_t totalNumElements = 1;
        for (vector<uint16_t>::const_iterator it = arrayDimensions.begin();
                                             it != arrayDimensions.end(); ++it)
        {
            if (skipFirstDimensionIfArray)
            {
                skipFirstDimensionIfArray = false;
                continue;
            }
            uint16_t dim = *it;
            if (dim == 0)
            {
                totalNumElements = 0;
                break;
            }
            totalNumElements *= dim;
        }
        sizeInBytes = uint16_t(getFinalArrayElementTypeSize()) * totalNumElements;
    }
    else
        sizeInBytes = getTypeSize();

    return sizeInBytes <= 32767;
}


const std::vector<uint16_t> &
Declaration::getArrayDimensions() const
{
    return arrayDimensions;
}


void
Declaration::setFrameDisplacement(int16_t disp)
{
    frameDisplacement = disp;

    if (!hasLocalVariableFrameDisplacement() && !hasFunctionParameterFrameDisplacement())
        errormsg("invalid frame displacement %d for variable %s", disp, variableId.c_str());
}


int16_t
Declaration::getFrameDisplacement(int16_t offset) const
{
    return frameDisplacement + offset;
}


string
Declaration::getFrameDisplacementArg(int16_t offset) const
{
    if (global || isExtern || isLocalStatic())
    {
        // "label,Y" under OS-9 (Y is initialized after program_start;
        // see TranslationUnit::emitAssembler()).
        // "label,PCR" under other platforms.
        //
        assert(!label.empty());
        return label + "+" + wordToString(offset) + TranslationUnit::instance().getDataIndexRegister(true, readOnly);
    }

    if (!hasLocalVariableFrameDisplacement() && !hasFunctionParameterFrameDisplacement())
        errormsg("unallocated variable: %s", variableId.c_str());

    return intToString(getFrameDisplacement(offset)) + ",U";
}


bool
Declaration::hasFunctionParameterFrameDisplacement() const
{
    return frameDisplacement >= FIRST_FUNC_PARAM_FRAME_DISPLACEMENT;
}


bool
Declaration::hasLocalVariableFrameDisplacement() const
{
    return frameDisplacement <= 0;
}


const Tree *
Declaration::getInitExpr() const
{
    return initializationExpr;
}


Tree *
Declaration::getInitExpr()
{
    return initializationExpr;
}


void
Declaration::setGlobal(bool g)
{
    global = g;
}


bool
Declaration::isGlobal() const
{
    return global;
}


bool
Declaration::isStatic() const
{
    return hasStaticKeyword;
}


bool
Declaration::isExternal() const
{
    return isExtern;
}


bool
Declaration::isArray() const
{
    return arrayDimensions.size() > 0;
}


void
Declaration::setReadOnly(bool ro)
{
    readOnly = ro;
}


bool
Declaration::isReadOnly() const
{
    return readOnly;
}


bool
Declaration::hasOnlyNumericalLiteralInitValues() const
{
    if (!initializationExpr)
        return false;
    if (initializationExpr->isNumericalLiteral())
        return true;
    if (initializationExpr->isArrayWithOnlyNumericalLiteralInitValues(*getTypeDesc()))
        return true;
    return initializationExpr->isStructWithOnlyNumericalLiteralInitValues(*getTypeDesc());
}


void
Declaration::setLabel(const string &_label)
{
    assert(_label.length() > 0);
    label = _label;
}


void
Declaration::setLabelFromVariableId()
{
    setLabel("_" + variableId);
}


void
Declaration::setLocalStaticLabel(const string &functionId)
{
    setLabel(".local.static." + functionId + "." + variableId);
}


string
Declaration::getLabel() const
{
    return label;
}


bool
Declaration::emitGlobalVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const
{
    uint16_t size = 0;
    if (!getVariableSizeInBytes(size))
        return false;  // failure

    if (isReadOnly() != readOnlySection)
        return true;  // nothing to do

    bool canBeStaticInit = (initializationExpr && initializationExpr->isStaticallyInitializable(*getTypeDesc()));
    if (canBeStaticInit != withStaticInitializer)
        return true;  // nothing to do

    if (withStaticInitializer)
    {
        if (initializationExpr && initializationExpr->definesOnlyAMatrixOfCharsAndHasInitializer(*getTypeDesc()))  // e.g., char s[3][10] = { "foo", "bar", "baz" };
            emitStaticArrayOfFixedSizeStringLiterals(out);
        else if (initializationExpr && initializationExpr->isArrayWithOnlyNumericalLiteralInitValues(*getTypeDesc()))
            emitStaticArrayInitializer(out);
        else
        {
            out.emitLabel(getLabel(), getVariableId() + ": " + getTypeDesc()->toString() + ": " + getLineNo());
            emitStaticValues(out, initializationExpr, getTypeDesc());
        }
    }
    else  // !withStaticInitializer
    {
        // We do not emit an FCB or FDB because these globals are initialized
        // at run-time by INITGL (or in the case of a local static, at the first execution
        // of the containing function), so that they are re-initialized every time
        // the program is run.
        // This re-initialization does not happen for constant global integer arrays,
        // for space saving purposes.
        //
        if (isLocalStatic() && isDynamicInitializerForced())  // see also Declaration::emitCode()
        {
            out.emitLabel(getLabel() + ".initialized");
            out.ins("RMB", "1");
        }
        out.emitLabel(getLabel());
        string comment = getVariableId() + ": " + getTypeDesc()->toString() + ": " + getLineNo();
        if (size == 0)
            out.emitComment("* Variable has size zero: " + comment);
        else
            out.ins("RMB", wordToString(size), comment);  // init to false because in BSS section
    }

    if (isGlobalStatic())  // emit an extra label to help avoid warnings on multiple defs of static symbols
        out.emitLabel(".global.static.variable." + getVariableId());

    return true;  // success
}


CodeStatus
Declaration::emitStaticArrayOfFixedSizeStringLiterals(ASMText &out) const
{
    assert(arrayDimensions.size() > 0);

    if (label.empty())
        return false;

    out.emitLabel(label, getVariableId() + ": " + getTypeDesc()->toString() + " (array of string literals)");

    const TreeSequence *topSeq = dynamic_cast<const TreeSequence *>(initializationExpr);
    const uint16_t stringLiteralMaxSize = arrayDimensions[arrayDimensions.size() - 1];
    return emitTreeSequenceOfFixedSizeStringLiterals(out, topSeq, stringLiteralMaxSize);
}


// Emits FCC and FDB directives that layout the string literals in 'seq'
// in a character matrix whose rows have a size specified by 'stringLiteralMaxSize'.
//
CodeStatus
Declaration::emitTreeSequenceOfFixedSizeStringLiterals(ASMText &out, const TreeSequence *seq, uint16_t stringLiteralMaxSize)
{
    // If initializationExpr is an array of literals, then pad and concatenate the literals
    // into a single array of characters.
    // Otherwise, initializationExpr is an array of arrays, so recurse.

    assert(seq);
    assert(seq->size() > 0);
    const Tree *firstElement = *seq->begin();
    assert(firstElement);
    if (dynamic_cast<const StringLiteralExpr *>(firstElement))
    {
        stringstream acc;
        for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
        {
            const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(*it);
            assert(sle);
            const string value = sle->getValue();
            acc.write(value.data(), std::min(value.length(), size_t(stringLiteralMaxSize)));  // support null characters
            for (size_t len = value.length(); len < stringLiteralMaxSize; ++len)
                acc << '\0';
        }
        const string accString = acc.str();
        assert(accString.length() == seq->size() * stringLiteralMaxSize);

        // Emit accString without adding a terminating null byte, since accString already contains one,
        // since the last accumulated value is a string literal.
        //
        StringLiteralExpr::emitStringLiteralDefinition(out, accString, false);
    }
    else
    {
        // Array of arrays, so recurse.
        for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
        {
            const TreeSequence *innerSeq = dynamic_cast<const TreeSequence *>(*it);
            assert(innerSeq);
            if (!emitTreeSequenceOfFixedSizeStringLiterals(out, innerSeq, stringLiteralMaxSize))
                return false;
        }
    }

    return true;
}


// Also supports string literal initializer.
//
CodeStatus
Declaration::emitStaticArrayInitializer(ASMText &out) const
{
    assert(getType() == ARRAY_TYPE);

    if (const StringLiteralExpr *sle = dynamic_cast<StringLiteralExpr *>(initializationExpr))
    {
        assert(getTypeDesc()->getPointedType() == BYTE_TYPE);
        assert(arrayDimensions.size() == 1);

        out.emitLabel(label, getVariableId() + ": " + getTypeDesc()->toString());
        sle->emitStringLiteralDefinition(out);

        // If literal too short, emit null bytes for remaining array elements.
        //
        size_t numEmittedBytes = sle->getDecodedLength() + 1;  // including terminating NUL
        size_t numArrayBytes = arrayDimensions[0];
        if (numEmittedBytes < numArrayBytes)
            out.ins("ZMB", wordToString(numArrayBytes - numEmittedBytes));

        return true;
    }

    const TreeSequence *seq = dynamic_cast<TreeSequence *>(initializationExpr);
    if (seq == NULL)
        return false;

    if (label.empty())
        return false;

    out.emitLabel(label, getVariableId() + ": " + getTypeDesc()->toString());

    return emitStaticValues(out, seq, getTypeDesc());
}


// Emits FCB and FDB directives.
// The initializer value will be in the representation specified by 'requiredTypeDesc'.
// Supports longs and reals.
//
CodeStatus
Declaration::emitStaticValues(ASMText &out, const Tree *initializer, const TypeDesc *requiredTypeDesc) const
{
    // Try to evaluate initializer as a constant (short int) expression.
    //
    uint16_t initValue = 0;
    if (initializer->evaluateConstantExpr(initValue))
    {
        if (initializer->getTypeDesc()->isIntegral()
            && requiredTypeDesc->type != BYTE_TYPE
            && requiredTypeDesc->type != WORD_TYPE
            && requiredTypeDesc->type != POINTER_TYPE
            && !requiredTypeDesc->isRealOrLong())
        {
            errormsg("integer value cannot be used to initialize field of type %s", requiredTypeDesc->toString().c_str());
            return false;
        }

        // Warn if trying to fit a >8 bit value in a byte field.
        //
        if (requiredTypeDesc->type == BYTE_TYPE)
        {
            if (! requiredTypeDesc->isSigned && initValue > 0xFF)
            {
                warnmsg("value %u (0x%x) does not fit in an unsigned character (value will be truncated)", initValue, initValue);
                initValue &= 0xFF;
            }
            else if (requiredTypeDesc->isSigned && (int16_t(initValue) < -128 || int16_t(initValue) > 127))
            {
                warnmsg("value %u (0x%x) does not fit in a signed character (value will be truncated)", initValue, initValue);
                initValue &= 0xFF;
            }
        }

        string decimalInitValue = (initializer->isSigned() ? intToString(initValue) : wordToString(initValue));

        if (requiredTypeDesc->isReal())
        {
            double realValue = (initializer->isSigned() ? (double) (int16_t) initValue : (double) (int32_t) initValue);
            RealConstantExpr rce(realValue, "f");
            rce.emitRealConstantDefinition(out);
        }
        else if (requiredTypeDesc->isLong())
        {
            uint16_t highWord = (initializer->isSigned() && (initValue & 0x8000) ? 0xFFFF : 0x0000);
            out.ins("FDB", wordToString(highWord, true), "decimal " + decimalInitValue);
            out.ins("FDB", wordToString(initValue, true));
        }
        else
        {
            if (initializer->isSigned() && initializer->getType() == BYTE_TYPE
                && requiredTypeDesc->type == WORD_TYPE)
            {
                initValue = (uint16_t) (int8_t) initValue;
            }

            const char *directive = (requiredTypeDesc->type == BYTE_TYPE ? "FCB" : "FDB");
            out.ins(directive, wordToString(initValue, true), "decimal " + decimalInitValue);
        }
        return true;
    }

    if (const DWordConstantExpr *dwce = dynamic_cast<const DWordConstantExpr *>(initializer))
    {
        if (requiredTypeDesc->isReal())
        {
            RealConstantExpr rce(dwce->getRealValue(), requiredTypeDesc->isSingle() ? "f" : "");
            rce.emitRealConstantDefinition(out);
            return true;
        }

        if (requiredTypeDesc->isLong())
        {
            dwce->emitDWordConstantDefinition(out);
            return true;
        }

        if (requiredTypeDesc->type == WORD_TYPE || requiredTypeDesc->type == BYTE_TYPE)
        {
            uint16_t value = dwce->getDWordValue() & (requiredTypeDesc->type == WORD_TYPE ? 0xFFFF : 0x00FF);
            const char *directive = (requiredTypeDesc->type == BYTE_TYPE ? "FCB" : "FDB");
            out.ins(directive, wordToString(value, true), doubleToString(double(dwce->getDWordValue())));
            return true;
        }

        errormsg("value of type `%s' used to initialize `%s'",
                 initializer->getTypeDesc()->toString().c_str(),
                 requiredTypeDesc->toString().c_str());
        return true;
    }

    if (const RealConstantExpr *rce = dynamic_cast<const RealConstantExpr *>(initializer))
    {
        if (requiredTypeDesc->isSingle())
        {
            if (rce->isSingle())
                rce->emitRealConstantDefinition(out);
            else
            {
                RealConstantExpr single(rce->getRealValue(), "f");  // make float
                single.emitRealConstantDefinition(out);
            }
            return true;
        }

        if (requiredTypeDesc->isDouble())
        {
            if (rce->isDouble())
                rce->emitRealConstantDefinition(out);
            else
            {
                RealConstantExpr dbl(rce->getRealValue(), "");  // make double
                dbl.emitRealConstantDefinition(out);
            }
            return true;
        }

        if (requiredTypeDesc->isLong())
        {
            double real = rce->getRealValue();
            if (real < 0 && !requiredTypeDesc->isSigned)
                real = 0;
            DWordConstantExpr dwce(real, requiredTypeDesc->isSigned);
            dwce.emitDWordConstantDefinition(out);
            return true;
        }

        if (requiredTypeDesc->type == WORD_TYPE || requiredTypeDesc->type == BYTE_TYPE)
        {
            uint32_t dw;
            if (rce->getRealValue() < 0 && !requiredTypeDesc->isSigned)
                dw = 0;
            else
                dw = rce->getDWordValue();
            const char *directive = (requiredTypeDesc->type == BYTE_TYPE ? "FCB" : "FDB");
            out.ins(directive, wordToString(uint16_t(dw), true), doubleToString(rce->getRealValue()));
            return true;
        }

        errormsg("value of type `%s' used to initialize `%s'",
                 initializer->getTypeDesc()->toString().c_str(),
                 requiredTypeDesc->toString().c_str());
        return true;
    }

    // Check for constant cast to pointer or numerical type.
    if (const CastExpr *ce = dynamic_cast<const CastExpr *>(initializer))
    {
        if (ce->getSubExpr()->evaluateConstantExpr(initValue))
        {
            bool isCastSigned = ce->getTypeDesc()->isSigned;
            bool isSubExprSigned = ce->getSubExpr()->getTypeDesc()->isSigned;
            double value;
            if (isSubExprSigned)
                value = int16_t(initValue);
            else
                value = initValue;

            if (ce->getTypeDesc()->isLong())
            {
                DWordConstantExpr dwce(value, isCastSigned);
                dwce.copyLineNo(*ce);
                dwce.emitDWordConstantDefinition(out);
                return true;
            }
            if (ce->getTypeDesc()->isReal())
            {
                RealConstantExpr rce(value, "");
                rce.copyLineNo(*ce);
                rce.emitRealConstantDefinition(out);
                return true;
            }
            if (ce->getTypeDesc()->type == POINTER_TYPE || ce->getTypeDesc()->type == WORD_TYPE || ce->getTypeDesc()->type == BYTE_TYPE)
            {
                const char *dir = "FDB";
                if (ce->getTypeDesc()->type == BYTE_TYPE)
                {
                    dir = "FCB";
                    initValue &= 0x00FF;
                }
                out.ins(dir, wordToString(initValue, true), wordToString(initValue, false) + " decimal");
                return true;
            }
            initializer->errormsg("unsupported initializer expression (not casting to long/float/double)");
            return true;
        }
        if (ce->getTypeDesc()->isNumerical() && dynamic_cast<const DWordConstantExpr *>(ce->getSubExpr()))  // e.g., (int) 100000L
        {
            auto dwordSubExpr = dynamic_cast<const DWordConstantExpr *>(ce->getSubExpr());
            uint32_t value = dwordSubExpr->getDWordValue();
            switch (ce->getTypeDesc()->type)
            {
                case BYTE_TYPE:
                    value &= 0xFF;
                    out.ins("FCB", wordToString(value & 0xFF, true), "cast to " + ce->getTypeDesc()->toString());
                    return true;
                case WORD_TYPE:
                    out.ins("FDB", wordToString(value & 0xFFFF, true), "cast to " + ce->getTypeDesc()->toString());
                    return true;
                default:
                    if (ce->isLong())
                        return dwordSubExpr->emitDWordConstantDefinition(out);
                    ce->errormsg("unsupported initializer expression (cast from `%s' to `%s')",
                                ce->getSubExpr()->getTypeDesc()->toString().c_str(),
                                ce->getTypeDesc()->toString().c_str());
                    return true;
            }
        }
        initializer->errormsg("unsupported initializer expression (not a 16-bit constant)");
        return true;
    }

    // Check for { ... }.
    if (const TreeSequence *seq = dynamic_cast<const TreeSequence *>(initializer))
    {
        // 'seq' must contain one element for each member of the struct or array to initialize.
        //
        if (requiredTypeDesc->type == CLASS_TYPE)
        {
            const ClassDef *cl = TranslationUnit::instance().getClassDef(requiredTypeDesc->className);
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

                stringstream comment;
                comment << "Element " << size_t(it - seq->begin()) << " of compound initializer: member '"
                        << member->getName() << "' of type " << member->getTypeDesc()->toString();
                out.emitComment(comment.str());
                const TreeSequence *memberSeq = dynamic_cast<const TreeSequence *>(*it);
                bool isMatrix = (*it)->definesOnlyAMatrixOfCharsAndHasInitializer(*member->getTypeDesc());
                const std::vector<uint16_t> memberArrayDimensions = member->getArrayDimensions();
                const uint16_t stringLiteralMaxSize = member->isArray() ? memberArrayDimensions[memberArrayDimensions.size() - 1] : 0;
                const StringLiteralExpr *stringLiteralInitExpr = dynamic_cast<const StringLiteralExpr *>(*it);
                if (isMatrix)
                {
                    if (!emitTreeSequenceOfFixedSizeStringLiterals(out, memberSeq, stringLiteralMaxSize))
                        return false;
                }
                else if (member->getTypeDesc()->isArrayOfChar() && stringLiteralInitExpr)
                {
                    // Array of char initialized with string literal.
                    const string value = stringLiteralInitExpr->getValue();
                    stringstream acc;
                    acc.write(value.data(), std::min(value.length(), size_t(stringLiteralMaxSize)));  // support null characters
                    for (size_t len = value.length(); len < stringLiteralMaxSize; ++len)
                        acc << '\0';
                    const string accString = acc.str();
                    assert(accString.length() == stringLiteralMaxSize);
                    StringLiteralExpr::emitStringLiteralDefinition(out, accString, false);
                }
                else
                    if (!emitStaticValues(out, *it, member->getTypeDesc()))
                        return false;
            }

            // If seq too short, emit null bytes for remaining members.
            for ( ; memberIndex < cl->getNumDataMembers(); ++memberIndex)
            {
                const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
                assert(member);
                stringstream comment;
                comment << "Element " << memberIndex << " of compound initializer: member '"
                        << member->getName() << "' of type " << member->getTypeDesc()->toString();
                out.emitComment(comment.str());
                out.ins("ZMB", wordToString(member->getSizeInBytes()));
            }
            return true;
        }
        if (requiredTypeDesc->type == ARRAY_TYPE)
        {
            const TypeDesc *arrayElemTypeDesc = requiredTypeDesc->getPointedTypeDesc();
            for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
            {
                stringstream comment;
                comment << "Element " << size_t(it - seq->begin()) << " of array, of type " << arrayElemTypeDesc->toString();
                out.emitComment(comment.str());
                if (!emitStaticValues(out, *it, arrayElemTypeDesc))
                    return false;
            }

            uint16_t requiredNumArrayElements = seq->getRequiredNumArrayElements();
            if (seq->size() < requiredNumArrayElements)
            {
                const WordConstantExpr wce(0, "0");
                for (size_t i = seq->size(); i < requiredNumArrayElements; ++i)
                    if (!emitStaticValues(out, &wce, arrayElemTypeDesc))
                        return false;
            }

            return true;
        }

        errormsg("compound initializer must only be used to initialize a struct or array");
        return false;
    }

    // Emit an array address if initializer is an array name and
    // we do NOT support relocatability.
    //
    if (const IdentifierExpr *ie = dynamic_cast<const IdentifierExpr *>(initializer))
        return emitArrayAddress(out, *ie, *requiredTypeDesc);

    initializer->errormsg("invalid element (%s) in initializer for static-valued array '%s'",
                            initializer->getTypeDesc()->toString().c_str(),
                            getVariableId().c_str());
    return false;
}


// Emits an FDB or issues an error message.
//
bool
Declaration::emitArrayAddress(ASMText &out, const IdentifierExpr &ie, const TypeDesc &requiredTypeDesc)
{
    if (TranslationUnit::instance().isRelocatabilitySupported())
    {
        ie.errormsg("`%s' in initializer would make program non-relocatable (see --no-relocate)",
                    ie.getId().c_str());
        return false;
    }

    if (ie.isFuncAddrExpr())
    {
        const FunctionDef *fd = TranslationUnit::instance().getFunctionDef(ie.getId());
        if (!fd)
        {
            ie.errormsg("`%s' is not the name of a declared function");
            return false;
        }
        if (! fd->getBody() && ! fd->hasInternalLinkage())
            out.emitImport(fd->getLabel().c_str());
        out.ins("FDB", fd->getLabel(), "function address");
        return true;
    }

    if (requiredTypeDesc.type != POINTER_TYPE || !ie.getVariableExpr() || !ie.getVariableExpr()->getDeclaration())
    {
        ie.errormsg("identifier `%s' not expected as initializer for `%s'",
                    ie.getId().c_str(), requiredTypeDesc.toString().c_str());
        return false;
    }

    if (!ie.getTypeDesc()->isArray())
    {
        ie.errormsg("expected `%s' to be array name", ie.getId().c_str());
        return false;
    }

    out.ins("FDB", ie.getVariableExpr()->getDeclaration()->getLabel(), "array " + ie.getId());
    return true;
}


// Returns true iff tree if name[...][...][...]...
//
static bool
isMatrixElementReferenceOnArrayName(const Tree &tree)
{
    const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(&tree);
    if (!bin)
        return false;
    if (bin->getOperator() != BinaryOpExpr::ARRAY_REF)
        return false;
    if (bin->getLeft()->asVariableExpr())
        return true;  // left side is name
    return isMatrixElementReferenceOnArrayName(*bin->getLeft());  // recurse
}


// Returns true iff all expressions in name[...][...][...] are constant.
// Must only be called on trees on which isMatrixElementReferenceOnArrayName() returns true.
//
static bool
isConstantExprArrayRefChain(const Tree &tree)
{
    const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(&tree);
    assert(bin && bin->getOperator() == BinaryOpExpr::ARRAY_REF);

    // Check that bracket expression is constant.
    uint16_t value;
    if (!bin->getRight()->evaluateConstantExpr(value))
        return false;
    
    if (bin->getLeft()->asVariableExpr())
        return true;

    return isConstantExprArrayRefChain(*bin->getLeft());
}


static bool
isAddressOfVariable(const Tree &tree)
{
    const UnaryOpExpr *u = dynamic_cast<const UnaryOpExpr *>(&tree);
    if (!u || u->getOperator() != UnaryOpExpr::ADDRESS_OF)
        return u;
    const Tree *subExpr = u->getSubExpr();
    if (subExpr->asVariableExpr() != NULL)
        return true;
    
    if (isMatrixElementReferenceOnArrayName(*subExpr))
        return isConstantExprArrayRefChain(*subExpr);

    return false;
}


// Returns true if 'tree' only contains variables, constant expressions
// and arithmetic operators.
// numVariables and numConstantExpressions must be initialized.
//
static bool
countVariablesAndConstantExpressions(const Tree &tree, size_t &numVariables, size_t &numConstantExpressions)
{
    uint16_t value;
    if (tree.asVariableExpr())
        ++numVariables;
    else if (tree.evaluateConstantExpr(value))
        ++numConstantExpressions;
    else if (const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(&tree))
    {
        switch (bin->getOperator())
        {
            case BinaryOpExpr::ADD:
            case BinaryOpExpr::SUB:
            case BinaryOpExpr::MUL:
            case BinaryOpExpr::DIV:
            case BinaryOpExpr::MOD:
                assert(bin->getLeft() != NULL);
                assert(bin->getRight() != NULL);
                if (!countVariablesAndConstantExpressions(*bin->getLeft(), numVariables, numConstantExpressions))
                    return false;
                if (!countVariablesAndConstantExpressions(*bin->getRight(), numVariables, numConstantExpressions))
                    return false;
                break;
            default:
                return false;  // unexpected operator
        }
    }
    else
        return false;  // unexpected expression type
    return true;
}


static bool
isConstantInitializer(const Tree &initExpr)
{
    if (initExpr.isNumericalLiteral())  // includes longs, floats and doubles
        return true;
    if (initExpr.isNumericalLiteralCastToOtherType())
        return true;
    if (initExpr.isStringLiteralCastToPtrToAnyChar())
        return true;
    uint16_t value;
    if (initExpr.evaluateConstantExpr(value))
        return true;
    if (dynamic_cast<const StringLiteralExpr *>(&initExpr))
        return true;
    if (isAddressOfVariable(initExpr))
        return true;
    if (dynamic_cast<const IdentifierExpr *>(&initExpr))
    {
        if (initExpr.getTypeDesc()->isPtrToFunction())  // address of function by ID
            return true;
        if (initExpr.getType() == ARRAY_TYPE)  // name of an array
            return true;
    }
    const TreeSequence *seq = dynamic_cast<const TreeSequence *>(&initExpr);
    if (seq)  // if { ... }
    {
        for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
            if (!isConstantInitializer(**it))
                return false;
        return true;
    }
    if (const BinaryOpExpr *bin = dynamic_cast<const BinaryOpExpr *>(&initExpr))
    {
        size_t numVariables = 0, numConstantExpressions = 0;
        return countVariablesAndConstantExpressions(*bin, numVariables, numConstantExpressions)
                && numVariables == 1;
    }
    return false;
}


/*virtual*/
void
Declaration::checkSemantics(Functor &)
{
    // cout << "# Declaration::checkSemantics: this=" << this << ", " << getLineNo() << ": " << getVariableId() << ": "
    //      << getTypeDesc() << " '" << getTypeDesc()->toString() << "', extern=" << isExtern
    //      << ", initializationExpr of type " << (initializationExpr ? initializationExpr->getTypeDesc()->toString() : "<none>")
    //      << endl;

    // If this declaration is global, use a ScopeCreator to register it in the
    // global space and to set the declaration of each variable expression in
    // any initialization expression.
    // If this declaration is local, nothing is done here because the ScopeCreator
    // is created and used by FunctionDef::checkSemantics(), which will end up here.
    //
    if (global)
    {
        TranslationUnit &translationUnit = TranslationUnit::instance();
        ScopeCreator sc(translationUnit, &translationUnit.getGlobalScope());
        iterate(sc);

        if (initializationExpr != NULL)
        {
            // Also set the expression type in the initializer. For local declarations,
            // this is done when the semantics checker is called on the function definition.
            //
            ExpressionTypeSetter ets;
            initializationExpr->iterate(ets);

            // Check that the initializer is constant, i.e., does not require run-time code.
            //
            if (!isConstantInitializer(*initializationExpr))
                errormsg("initializer element is not constant");
        }
    }

    if (initializationExpr != NULL)
    {
        if (isExtern)
            warnmsg("`%s' initialized and declared `extern'", variableId.c_str());

        checkInitExpr(initializationExpr, getTypeDesc(), variableId, arrayDimensions, 0);
    }
}


void
Declaration::checkInitExpr(Tree *initializationExpr, const TypeDesc *varTypeDesc, const string &variableId,
                           const vector<uint16_t> &arrayDimensions, size_t dimIndex)
{
    assert(initializationExpr);
    assert(varTypeDesc);
    assert(!variableId.empty());

    BasicType varType = varTypeDesc->type;

    BasicType initExprType = initializationExpr->getType();
    const TypeDesc *initExprTD = initializationExpr->getTypeDesc();

    // cout << "# Declaration::checkInitExpr(" << typeid(*initializationExpr).name() << ", '" << varTypeDesc->toString() << "', " << variableId
    //      << ", {" << join(", ", arrayDimensions) << "}, " << dimIndex << "): initExprTD='" << initExprTD->toString()
    //      << "'; line " << initializationExpr->getLineNo() << "\n";

    uint16_t value = 0;
    bool initExprIsConstant = initializationExpr->evaluateConstantExpr(value);

    if (varType == WORD_TYPE && initExprType == BYTE_TYPE)
        ;
    else if (varType == BYTE_TYPE && initExprType == BYTE_TYPE)
        ;  // do not care about signedness
    else if (varType == WORD_TYPE && initExprType == WORD_TYPE)
        ; // do not care about signedness
    else if (varType == POINTER_TYPE && (initExprType == WORD_TYPE || initExprType == BYTE_TYPE))
    {
        if (! initExprIsConstant)
            initializationExpr->warnmsg("initializing pointer '%s' from integer expression", variableId.c_str());
        else if (value >= 0x8000 && initExprTD->isSigned)
            initializationExpr->warnmsg("initializing pointer '%s' from negative constant", variableId.c_str());
    }
    else if (varType == POINTER_TYPE && initExprType == ARRAY_TYPE)
    {
        const TypeDesc *declPointedTypeDesc = varTypeDesc->getPointedTypeDesc();
        const TypeDesc *initPointedTypeDesc = initExprTD->getPointedTypeDesc();

        if (*declPointedTypeDesc != *initPointedTypeDesc)
        {
            ConstCorrectnessCode code = isPointerInitConstCorrect(declPointedTypeDesc, initPointedTypeDesc);
            if (code == CONST_INCORRECT)
            {
                if (TranslationUnit::instance().warnOnConstIncorrect())
                    initializationExpr->warnmsg("initializing non-constant `%s' (%s) from `%s'",
                                                varTypeDesc->toString().c_str(),
                                                variableId.c_str(),
                                                initExprTD->toString().c_str());
            }
            else if (code == INCOMPAT_TYPES)
            {
                initializationExpr->warnmsg("initializing `%s' (%s) from incompatible `%s'",
                                            varTypeDesc->toString().c_str(),
                                            variableId.c_str(),
                                            initExprTD->toString().c_str());
            }
            else
                assert(code == CONST_CORRECT);
        }
    }
    else if (varType == ARRAY_TYPE)
        checkArrayInitializer(initializationExpr, varTypeDesc, variableId, arrayDimensions, dimIndex);
    else if (varType == CLASS_TYPE)
        checkClassInitializer(initializationExpr, varTypeDesc, variableId);
    else if (varTypeDesc->isIntegral() && initExprTD->isReal())
    {
        initializationExpr->warnmsg("initializing %s from %s expression",
                                    varTypeDesc->toString().c_str(),
                                    initExprTD->toString().c_str());
    }
    else if (*varTypeDesc != *initExprTD)
    {
        switch (FunctionCallExpr::paramAcceptsArg(*varTypeDesc, *initializationExpr))
        {
        case FunctionCallExpr::NO_PROBLEM:
            break;
        case FunctionCallExpr::WARN_CONST_INCORRECT:
            initializationExpr->warnmsg("using `%s' to initialize `%s' is not const-correct",
                                initExprTD->toString().c_str(),
                                varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_NON_PTR_ARRAY_FOR_PTR:
            initializationExpr->warnmsg("using non-pointer/array (%s) to initialize `%s`",
                                 initExprTD->toString().c_str(),
                                 varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_PASSING_CHAR_CONSTANT_FOR_PTR:
            initializationExpr->warnmsg("using character constant to initialize `%s'",
                                 initExprTD->toString().c_str(),
                                 varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_PASSING_NON_ZERO_CONSTANT_FOR_PTR:
            initializationExpr->warnmsg("using non-zero numeric constant to initialize `%s'",
                                 initExprTD->toString().c_str(),
                                 varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_ARGUMENT_TOO_LARGE:
            initializationExpr->warnmsg("initializer of type `%s' is too large for `%s`",
                              initExprTD->toString().c_str(),
                              varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_REAL_FOR_INTEGRAL:
            initializationExpr->warnmsg("using real type `%s' to initialize `%s`",
                              initExprTD->toString().c_str(),
                              varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_FUNC_PTR_FOR_PTR:
            initializationExpr->warnmsg("assigning function pointer `%s' to `%s`",
                              initExprTD->toString().c_str(),
                              varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_DIFFERENT_SIGNEDNESS:
            initializationExpr->warnmsg("using `%s' to initialize `%s' (different signedness)",
                                initExprTD->toString().c_str(),
                                varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARNING_VOID_POINTER:
            initializationExpr->warnmsg("using `%s' to initialize `%s' (implicit cast of void pointer)",
                                initExprTD->toString().c_str(),
                                varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::WARN_PTR_FOR_INTEGRAL:
            initializationExpr->warnmsg("using pointer type `%s' to initialize `%s`",
                              initExprTD->toString().c_str(),
                              varTypeDesc->toString().c_str());
            break;
        case FunctionCallExpr::ERROR_MSG:
            initializationExpr->errormsg("using `%s' to initialize `%s'",
                                initExprTD->toString().c_str(),
                                varTypeDesc->toString().c_str());
            break;
        }
    }
}


// This Declaration must be of ARRAY_TYPE.
// initializationExpr must be of type TreeSequence *.
// Checks the number of elements in that sequence. Issues a warning or error
// if it is not equal to the size of the array.
// Checks that the type of each element of initializationExpr is compatible
// with the array element type.
//
void
Declaration::checkArrayInitializer(Tree *initializationExpr, const TypeDesc *varTypeDesc, const string &variableId, const vector<uint16_t> &arrayDimensions, size_t dimIndex)
{
    assert(initializationExpr != NULL);
    assert(varTypeDesc);
    assert(varTypeDesc->type == ARRAY_TYPE);
    assert(!variableId.empty());

    TreeSequence *seq = dynamic_cast<TreeSequence *>(initializationExpr);
    if (seq != NULL)
    {
        // Check that initializationExpr is not too long. Warn if too short.
        //
        if (dimIndex >= arrayDimensions.size())
        {
            initializationExpr->errormsg("dimension index %u exceeds dimension array of size %u (variable `%s')",
                                            dimIndex, arrayDimensions.size(), variableId.c_str());
            return;
        }
        uint16_t numArrayElements = arrayDimensions[dimIndex];
        if (seq->size() > size_t(numArrayElements))
        {
            if (TranslationUnit::instance().warnTooManyElementsInInitializer())
                initializationExpr->warnmsg("too many elements (%u) in initializer for array of %hu element(s)",
                                            seq->size(), numArrayElements);
            return;
        }
        if (seq->size() < size_t(numArrayElements))
        {
            initializationExpr->warnmsg("only %u element(s) in initializer for array of %hu element(s)",
                                            seq->size(), numArrayElements);
        }

        // Tell the code generator how many array elements are supposed to be initialized.
        //
        seq->setRequiredNumArrayElements(numArrayElements);

        for (vector<Tree *>::const_iterator it = seq->begin(); it != seq->end(); ++it)
            checkInitExpr(*it, varTypeDesc->pointedTypeDesc, variableId, arrayDimensions, dimIndex + 1);
    }
    else  // initializer is not a sequence:
    {
        // Require that the variable to initialize be a byte array and that the initializer be one too.
        //
        if (varTypeDesc->getPointedType() != BYTE_TYPE
            || ! initializationExpr->getTypeDesc()->isArrayOfChar())
        {
            initializationExpr->errormsg("initializer for array `%s' is invalid", variableId.c_str());
        }
        else if (const StringLiteralExpr *sle = dynamic_cast<StringLiteralExpr *>(initializationExpr))
        {
            // Check that the string literal, including its terminating \0, fits in the array.
            //
            assert(dimIndex < arrayDimensions.size());
            size_t sleLen = sle->getDecodedLength() + 1;
            uint16_t numArrayElements = arrayDimensions[dimIndex];
            if (sleLen > numArrayElements)
            {
                initializationExpr->errormsg("too many characters (%u) in string literal initializer for array of %hu character(s)",
                                             sleLen, numArrayElements);
            }
        }
    }
}

// This Declaration must be of CLASS_TYPE.
// initializationExpr must be of type TreeSequence *.
// Checks that the number of values in the sequence is equal to the number
// of members in the class.
//
void
Declaration::checkClassInitializer(Tree *initializationExpr, const TypeDesc *varTypeDesc, const string &variableId)
{
    assert(initializationExpr != NULL);
    assert(varTypeDesc);
    assert(varTypeDesc->type == CLASS_TYPE);
    assert(!variableId.empty());

    const string &className = varTypeDesc->className;

    TreeSequence *seq = dynamic_cast<TreeSequence *>(initializationExpr);
    if (seq == NULL && ! TypeDesc::sameTypesModuloConst(*initializationExpr->getTypeDesc(), *varTypeDesc)
                    && ! isRealOrLongInitWithNumber(varTypeDesc, *initializationExpr))
    {
        initializationExpr->errormsg("initializer for struct %s is of type `%s': must be list, or struct of same type",
                                     className.c_str(), initializationExpr->getTypeDesc()->toString().c_str());
        return;
    }

    if (seq)
    {
        const ClassDef *cl = TranslationUnit::instance().getClassDef(className);
        assert(cl);

        if (! cl->isUnion())
        {
            if (seq->size() < cl->getNumDataMembers())
            {
                if (TranslationUnit::instance().warnOnMissingFieldInitializers())
                    for (size_t fieldIndex = seq->size(); fieldIndex < cl->getNumDataMembers(); ++fieldIndex)
                        seq->warnmsg("missing initializer for field `%s' of `%s'",
                                        cl->getDataMember(fieldIndex)->getName().c_str(),
                                        varTypeDesc->toString().c_str());
            }
            else if (seq->size() > cl->getNumDataMembers())
            {
                seq->warnmsg("excess elements in `%s' initializer", varTypeDesc->toString().c_str());
            }
        }

        size_t memberIndex = 0;
        for (std::vector<Tree *>::const_iterator initExprIt = seq->begin(); initExprIt != seq->end(); ++initExprIt, ++memberIndex)
        {
            const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
            if (member == NULL)
                break;  // tolerate having more expressions in 'seq' than data members

            const TypeDesc *memberTD = member->getTypeDesc();

            vector<uint16_t> dims = member->getArrayDimensions();
            memberTD->appendDimensions(dims);  // add dimensions due to type

            checkInitExpr(*initExprIt, memberTD, variableId, dims, 0);
        }
    }
}


Declaration *
Declaration::declareHiddenVariableInCurrentScope(const Tree &parentExpression,
                                                 const TypeDesc *typeDesc)
{
    TranslationUnit &tu = TranslationUnit::instance();

    // Use '$' as the name prefix, to avoid clashing with a C variable.
    Declaration *decl = new Declaration("$" + tu.generateLabel('V'),
                                        typeDesc ? typeDesc : parentExpression.getTypeDesc(),
                                        vector<uint16_t>(), false, false);
    decl->copyLineNo(parentExpression);
    Scope *scope = tu.getCurrentScope();
    assert(scope);
    if (!scope->declareVariable(decl))  // 'scope' keeps copy of Declaration pointer but does not take ownership
        assert(!"decl must have unique name");
    return decl;
}


bool
Declaration::isRealOrLongInitWithNumber(const TypeDesc *varTypeDesc, const Tree &initializationExpr)
{
    return (varTypeDesc->isLong() || varTypeDesc->isReal())
           && initializationExpr.getTypeDesc()->isNumerical();
}


CodeStatus
Declaration::emitInitializationExprCode(ASMText &out) const
{
    const TypeDesc *declTD = getTypeDesc();
    const TypeDesc *initExprTD = initializationExpr->getTypeDesc();
    const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(initializationExpr);

    uint16_t initExprConstValue = 0;
    bool isInitExprConstant = initializationExpr->evaluateConstantExpr(initExprConstValue);

    if (getType() == BYTE_TYPE && isInitExprConstant && initExprConstValue == 0)  // covers case where init expr is enum name (e.g., FALSE)
        out.ins("CLR", getFrameDisplacementArg(), "init of variable " + variableId);
    else if (dynamic_cast<const TreeSequence *>(initializationExpr))  // if init from { ... }
    {
        InitializerCodeEmitter emitter(*this);
        if (!emitter.emitCode(out))
            return false;
    }
    else if (getType() == ARRAY_TYPE && sle)  // if char[] init by string literal
    {
        // Emit a call to copyMem utility routine that reads from the string literal
        // and writes to the array address.
        //
        out.ins("LEAX", sle->getArg(), sle->getEscapedVersion());
        out.ins("PSHS", "X", "source array");
        out.ins("LEAX", getFrameDisplacementArg(), "destination byte array " + variableId);
        out.ins("LDD", "#" + wordToString(uint16_t(sle->getLiteral().length()) + 1), "length of string literal + terminating NUL");
        callUtility(out, "copyMem");
        out.ins("LEAS", "2,S");
    }
    else if (initializationExpr->getType() == CLASS_TYPE)  // if initialization from struct
    {
        if (!initializationExpr->emitCode(out, true))  // get address of expression in X
            return false;

        if (initExprTD->isRealOrLong())  // if expression is float or double or long
        {
            if (isReal())
            {
                out.ins("TFR", "X,D", "address of source number in D");
                out.ins("LEAX", getFrameDisplacementArg(), "address of variable to init: " + variableId);

                callUtility(out, "init" + string(declTD->isSingle() ? "Single" : "Double")
                                    + "From" + (initExprTD->isLong()
                                                    ? (initExprTD->isSigned ? "Signed" : "Unsigned") + string("DWord")
                                                    : (initExprTD->isSingle() ? "Single" : "Double")));
            }
            else if (isLong())
            {
                if (initExprTD->isLong())
                {
                    out.ins("TFR", "X,D", "address of source number");
                    out.ins("LEAX", getFrameDisplacementArg(), "variable " + variableId);
                    callUtility(out, "copyDWord");
                }
                else  // long from real
                {
                    assert(initExprTD->isReal());

                    out.ins("TFR", "X,D", "address of source number");
                    out.ins("LEAX", getFrameDisplacementArg(), "variable " + variableId);
                    callUtility(out, "init" + string(declTD->isSigned ? "Signed" : "Unsigned") + "DWordFrom" + (initExprTD->isSingle() ? "Single" : "Double"));
                }
            }
            else if (getTypeSize() <= 2)  // if variable is byte-sized or word-sized
            {
                if (initExprTD->isReal())
                {
                    out.ins("TFR", "X,D", "address of source number in D");
                    out.ins("LEAX", getFrameDisplacementArg(), "address of variable to init: " + variableId);
                    callUtility(out, "init" + string(declTD->isSigned ? "Signed" : "Unsigned")
                                            + (declTD->type == BYTE_TYPE ? "Byte" : "Word")
                                            + "From" + (initExprTD->isSingle() ? "Single" : "Double"));
                }
                else if (getTypeSize() == 2)  // init word from long
                {
                    out.ins("LDD", "2,X", "init word from low word of long");
                    out.ins("STD", getFrameDisplacementArg(), "variable " + variableId);
                }
                else if (getTypeSize() == 1)
                {
                    out.ins("LDB", "3,X", "init byte from low byte of long");
                    out.ins("STB", getFrameDisplacementArg(), "variable " + variableId);
                }
                else
                    assert(0);
            }
            else
                assert(0);
        }
        else if (getType() == CLASS_TYPE)  // init a struct from a struct
        {
            out.ins("PSHS", "X", "source struct");
            int16_t structSizeInBytes = TranslationUnit::instance().getTypeSize(*declTD);
            out.ins("LDD", "#" + wordToString(structSizeInBytes), "size of struct " + declTD->className);
            out.ins("LEAX", getFrameDisplacementArg(), "destination struct: " + variableId);
            callUtility(out, "copyMem", "initialize struct");
            out.ins("LEAS", "2,S");
        }
        else
        {
            assert(false);
        }
    }
    else
    {
        // Evaluate the initializer and get its value in D or B.
        //
        if (isInitExprConstant)
        {
            if (getType() == BYTE_TYPE)
            {
                initExprConstValue &= 0xFF;
                if (initExprConstValue == 0)
                    out.ins("CLRB");
                else
                    out.ins("LDB", "#" + wordToString(initExprConstValue, true), wordToString(initExprConstValue, false));
            }
            else if (initExprConstValue == 0)
            {
                out.ins("CLRA");
                out.ins("CLRB");
            }
            else
            {
                out.ins("LDD", "#" + wordToString(initExprConstValue, true), wordToString(initExprConstValue, false));
            }
        }
        else
        {
            if (!initializationExpr->emitCode(out, false))
                return false;

            // If initializing word from byte expression, make sure MSB is 0.
            if (getType() != BYTE_TYPE && initializationExpr->getType() == BYTE_TYPE)
                out.ins(initExprTD->getConvToWordIns());
        }

        if (declTD->isReal())
        {
            out.ins("LEAX", getFrameDisplacementArg(), "address of variable " + variableId);
            callUtility(out, "init" + string(declTD->isSingle() ? "Single" : "Double")
                                + "From" + (initExprTD->isSigned ? "Signed" : "Unsigned") + "Word");
        }
        else if (declTD->isLong())
        {
            if (initExprTD->type == BYTE_TYPE)
                out.ins(initExprTD->getConvToWordIns());

            out.ins("LEAX", getFrameDisplacementArg(), "address of variable " + variableId);
            callUtility(out, "initDWordFrom" + string(initExprTD->isSigned ? "Signed" : "Unsigned") + "Word");
        }
        else
        {
            string store = getStoreInstruction(getType());
            if (store.empty())
            {
                assert(!"unknown type");
                return false;
            }

            out.ins(store, getFrameDisplacementArg(), "variable " + variableId);
        }
    }
    return true;
}


/*virtual*/
CodeStatus
Declaration::emitCode(ASMText &out, bool /*lValue*/) const
{
    if (isExternal())
        out.emitImport(getLabel());

    if (initializationExpr == NULL)
        return true;  // nothing to do

    if (isLocalStatic() && !isDynamicInitializerForced())
        return true;  // nothing to do

    string initLabel, postInitLabel;
    if (isLocalStatic())
    {
        // This is a local static variable that whose initialization is being forced to be dynamic.
        // This is because it contains string literals, whose address is only known as run-time,
        // because the OS or the LOADM command may have loaded the program at an address that is
        // only determined at run-time.
        // In such a case, the initializer of the variable is run only the first time that execution
        // passes through that variable's declaration.
        // A global hidden boolean is used to remember if the variable has already been initialized.
        //
        initLabel = getLabel() + ".initialized";
        postInitLabel = TranslationUnit::instance().generateLabel('L');
        out.ins("TST", initLabel, "has `" + variableId + "' been initialized?");
        out.ins("BNE", postInitLabel, "if yes, skip init code");
    }

    writeLineNoComment(out, "init of variable " + variableId);

    emitInitializationExprCode(out);

    if (isLocalStatic())
    {
        out.ins("INC", initLabel, "mark `" + variableId + "' as initialized");
        out.emitLabel(postInitLabel);
    }

    return true;
}


bool
Declaration::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (initializationExpr != NULL && !initializationExpr->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
