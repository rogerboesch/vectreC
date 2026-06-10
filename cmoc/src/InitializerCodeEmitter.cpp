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

#include "InitializerCodeEmitter.h"

#include "TranslationUnit.h"
#include "Declaration.h"
#include "TreeSequence.h"
#include "StringLiteralExpr.h"
#include "VariableExpr.h"
#include "WordConstantExpr.h"

#include <iostream>

using namespace std;


// Use 2nd definition to get debugging traces.
#define TRACE(expr) ((void) 0)
//#define TRACE(expr) expr


// Debugging tool.
//
class RecursionLevelUpdater
{
public:
    RecursionLevelUpdater(uint16_t &_levelToUpdate) : levelToUpdate(_levelToUpdate) { ++levelToUpdate; }
    ~RecursionLevelUpdater() { --levelToUpdate; }
private:
    uint16_t &levelToUpdate;
};


InitializerCodeEmitter::InitializerCodeEmitter(const Declaration &_declaration)
:   declaration(_declaration),
    writingOffset(0),
    recursionLevel(0)
{
}


InitializerCodeEmitter::~InitializerCodeEmitter()
{
}


CodeStatus
InitializerCodeEmitter::emitCode(ASMText &out)
{
    TRACE(cout << "# InitializerCodeEmitter::emitCode: variable " << declaration.getVariableId()
                << " at " << declaration.getLineNo() << endl);
    recursionLevel = 0;
    return emitCode(out, declaration.getTypeDesc(), declaration.getInitExpr(), true) != uint16_t(-1);
}


uint16_t
InitializerCodeEmitter::emitCode(ASMText &out,
                                const TypeDesc *typeDesc,
                                const Tree *initializer,
                                bool atDeclarationLevel)
{
    RecursionLevelUpdater u(recursionLevel);

    assert(typeDesc != NULL);
    assert(initializer != NULL);
    
    TRACE(cout << "# InitializerCodeEmitter::emitCode: START: atDeclarationLevel=" << atDeclarationLevel << endl);
    TRACE(cout << "#   typeDesc: " << *typeDesc << endl);
    uint16_t written = 0;
    if (typeDesc->isStruct())
        written = emitCodeForStruct(out, typeDesc, dynamic_cast<const TreeSequence *>(initializer));
    else if (typeDesc->isArray())
    {
        if (!atDeclarationLevel)
        {
            declaration.errormsg("unexpected array initializer");
            written = uint16_t(-1);
        }
        else
        {
            TRACE(cout << "#   atDeclarationLevel: arrayDimensions (with TD dims): " << vectorToString(declaration.arrayDimensions) << endl);
            written = emitCodeForArray(out, typeDesc, initializer, declaration.arrayDimensions, 0);
            TRACE(cout << "#   atDeclarationLevel: wrote " << written << " byte(s)\n");
        }
    }
    else if (typeDesc->isNumerical() || typeDesc->type == POINTER_TYPE)
        written = emitCodeForSimpleType(out, typeDesc, initializer);
    else
    {
        declaration.errormsg("unsupported initializer");
        written = uint16_t(-1);
    }

    TRACE(cout << "# InitializerCodeEmitter::emitCode: END: written=" << written << endl);
    return written;
}


// Advances writingOffset.
//
uint16_t
InitializerCodeEmitter::emitCodeForSimpleType(ASMText &out,
                                            const TypeDesc *basicTypeDesc,
                                            const Tree *basicTypeInitializer)
{
    RecursionLevelUpdater u(recursionLevel);

    const int16_t typeSizeInBytes = TranslationUnit::instance().getTypeSize(*basicTypeDesc);
    TRACE(cout << "# InitializerCodeEmitter::emitCodeForSimpleType: START\n");
    TRACE(cout << "#   basic type: typeSizeInBytes: " << typeSizeInBytes << endl);
    bool success = true;

    assert(basicTypeDesc->isNumerical() || basicTypeDesc->type == POINTER_TYPE);

    auto strLitInitializer = dynamic_cast<const StringLiteralExpr *>(basicTypeInitializer);

    if (basicTypeDesc->isPtrToChar() && strLitInitializer != NULL)  // if char * from "..."
    {
        out.ins("LEAX", strLitInitializer->getArg(), strLitInitializer->getEscapedVersion());
        out.ins("STX", declaration.getFrameDisplacementArg(writingOffset),
                                        "offset in variable " + declaration.getVariableId());
    }
    else if (basicTypeDesc->isRealOrLong() && basicTypeInitializer->isRealOrLong())
    {
        if (!basicTypeInitializer->emitCode(out, true))  // get address of number in X
            return false;
        out.ins("TFR", "X,D", "address of source number");
        out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset),
                        "offset in variable " + declaration.getVariableId());
        const char *variant = basicTypeInitializer->isLong() ? "DWord"
                                    : (basicTypeInitializer->isSingle() ? "Single" : "Double");
        if (   (basicTypeDesc->isLong() && basicTypeInitializer->isLong())
            || (basicTypeDesc->isReal() && basicTypeInitializer->isReal()))  // if same type
        {
            declaration.callUtility(out, "copy" + string(variant), "preserves X");
        }
        else  // real/dword mix:
        {
            declaration.callUtility(out, basicTypeDesc->isReal()
                                            ? "initSingleFrom" + string(basicTypeInitializer->isSigned() ? "Signed" : "Unsigned") + "DWord"
                                            : "init" + string(basicTypeDesc->isSigned ? "Signed" : "Unsigned") + "DWordFromSingle");
        }
    }
    else if (basicTypeDesc->isRealOrLong() && basicTypeInitializer->isIntegral())  // real/long from short
    {
        assert(TranslationUnit::instance().getTypeSize(*basicTypeInitializer->getTypeDesc()) <= 2);
        if (!basicTypeInitializer->emitCode(out, false))  // get init value in D or B
            return false;
        if (basicTypeInitializer->getType() == BYTE_TYPE)  // extend to D if init value is byte
            out.ins(basicTypeInitializer->getConvToWordIns());
        out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset),
                                        "offset in variable " + declaration.getVariableId());
        string utility = string("init")
                            + (basicTypeDesc->isLong()
                                        ? "DWord"
                                        : (basicTypeDesc->isSingle() ? "Single" : "Double"))
                            + "From"
                            + (basicTypeInitializer->isSigned() ? "Signed" : "Unsigned") + "Word";
        declaration.callUtility(out, utility, "preserves X");
    }
    else if (basicTypeDesc->isByteOrWordOrPointer() && basicTypeInitializer->isLong())  // short from long
    {
        assert(typeSizeInBytes <= 2);
        if (!basicTypeInitializer->emitCode(out, true))  // get address of dword in X
            return false;
        bool varIsByte = (basicTypeDesc->type == BYTE_TYPE);
        out.ins(varIsByte ? "LDB" : "LDD", varIsByte ? "3,X" : "2,X", "take low part of long");
        out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset),
                                        "offset in variable " + declaration.getVariableId());
        out.ins(varIsByte ? "STB" : "STD", ",X");
    }
    else if (basicTypeDesc->isByteOrWordOrPointer() && basicTypeInitializer->isReal())  // short from real
    {
        assert(typeSizeInBytes <= 2);
        if (!basicTypeInitializer->emitCode(out, true))  // get address of real in X
            return false;
        out.ins("TFR", "X,D", "address of source number in D");
        out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset),
                                        "offset in variable " + declaration.getVariableId());
        string utility = "init"
                            + string(basicTypeDesc->isSigned ? "Signed" : "Unsigned")
                            + (basicTypeDesc->type == BYTE_TYPE ? "Byte" : "Word")
                            + "FromSingle";
        declaration.callUtility(out, utility);
    }
    else  // 1- or 2-byte cases:
    {
        assert(typeSizeInBytes <= 2);
        const char *storeIns = NULL;
        const VariableExpr *ve = basicTypeInitializer->asVariableExpr();
        const WordConstantExpr *wce = dynamic_cast<const WordConstantExpr *>(basicTypeInitializer);
        if (ve != NULL && basicTypeInitializer->getType() == ARRAY_TYPE)  // if array name assigned to pointer var
        {
            out.ins("LEAX", ve->getFrameDisplacementArg(), "address of array " + ve->getId());
            storeIns = "STX";
        }
        else if (wce != NULL && basicTypeDesc->type == BYTE_TYPE)  // if initializing byte from WordConstantExpr
        {
            wce->emitCodeToLoadByte(out);
            storeIns = "STB";
        }
        else if (basicTypeInitializer->emitCode(out, false))
            storeIns = (basicTypeDesc->type == BYTE_TYPE ? "STB" : "STD");

        if (storeIns)
        {
            if (basicTypeInitializer->getType() == BYTE_TYPE && basicTypeDesc->type != BYTE_TYPE)
                out.ins(basicTypeInitializer->getConvToWordIns(), "",
                                            "initializer of type byte, but field is 16 bits");

            out.ins(storeIns, declaration.getFrameDisplacementArg(writingOffset),
                                            "offset in variable " + declaration.getVariableId());
        }
        else
        {
            assert(!"emitCodeForSimpleType: code this case");
            success = false;
        }
    }

    writingOffset += typeSizeInBytes;
    TRACE(cout << "# InitializerCodeEmitter::emitCodeForSimpleType: END\n");
    return success ? typeSizeInBytes : uint16_t(-1);
}


uint16_t
InitializerCodeEmitter::emitCodeForStruct(ASMText &out,
                                            const TypeDesc *structTypeDesc,
                                            const TreeSequence *structInitializer)
{
    RecursionLevelUpdater u(recursionLevel);

    TRACE(cout << "# InitializerCodeEmitter::emitCodeForStruct: START\n");
    size_t memberIndex = 0;
    const ClassDef *cl = TranslationUnit::instance().getClassDef(structTypeDesc->className);
    assert(cl && cl->getType() == CLASS_TYPE);
    TRACE(cout << "#   cl: " << cl->getName() << endl);
    TRACE(cout << "#   members: " << cl->getNumDataMembers() << endl);
    TRACE(cout << "#   structInitializer: " << structInitializer->size() << endl);

    uint16_t origWritingOffset = writingOffset;
    uint16_t maxWritingOffset = writingOffset;

    uint16_t written = 0;
    for (auto it = structInitializer->begin(); it != structInitializer->end(); ++it, ++memberIndex)
    {
        const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
        if (member == NULL)
            break;  // too many values in initializer: ignore the extra ones
        uint16_t w = emitCodeForStructMember(out, member->getTypeDesc(), *it, *member);
        TRACE(cout << "# InitializerCodeEmitter::emitCodeForStruct: Got " << w
                    << " byte(s) written for member " << member->getName() << "\n");
        if (written != uint16_t(-1) && w != uint16_t(-1))
        {
            if (cl->isUnion())
                written = std::max(written, w);  // remember length of longest union member
            else
                written += w;
        }
        else
            written = uint16_t(-1);

        if (cl->isUnion())
        {
            maxWritingOffset = std::max(maxWritingOffset, writingOffset);
            writingOffset = origWritingOffset;  // for next union member
        }
    }

    if (cl->isUnion())
        writingOffset = maxWritingOffset;

    // If 'structInitializer' is too short for the struct, emit null bytes for the remaining fields.
    //
    if (written != uint16_t(-1) && memberIndex < cl->getNumDataMembers())
    {
        if (cl->isUnion())
        {
            uint16_t maxMemberSize = 0;
            for ( ; memberIndex < cl->getNumDataMembers(); ++memberIndex)
            {
                const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
                assert(member);
                maxMemberSize = std::max(maxMemberSize, uint16_t(member->getSizeInBytes()));
            }
            if (written < maxMemberSize)  // if one of remaining members longer than previous ones
            {
                uint16_t numNullBytes = maxMemberSize - written;
                emitNullBytes(out, numNullBytes);
                written += numNullBytes;
            }
        }
        else
        {
            for ( ; memberIndex < cl->getNumDataMembers(); ++memberIndex)
            {
                const ClassDef::ClassMember *member = cl->getDataMember(memberIndex);
                assert(member);
                uint16_t memberSize = uint16_t(member->getSizeInBytes());
                TRACE(cout << "# InitializerCodeEmitter::emitCodeForStruct: Emitting null bytes for member "
                            << member->getName() << " of size " << memberSize << "\n");
                emitNullBytes(out, memberSize);
                written += memberSize;
            }
        }
    }

    TRACE(cout << "# InitializerCodeEmitter::emitCodeForStruct: END: written=" << written << "\n");
    return written;
}


uint16_t
InitializerCodeEmitter::emitCodeForStructMember(ASMText &out,
                                                const TypeDesc *memberTypeDesc,
                                                const Tree *memberInitializer,
                                                const ClassDef::ClassMember &member)
{
    RecursionLevelUpdater u(recursionLevel);

    TRACE(cout << "# InitializerCodeEmitter::emitCodeForStructMember: START\n");
    TRACE(cout << "#   member: " << member.getName() << endl);
    TRACE(cout << "#   memberTypeDesc: " << *memberTypeDesc << endl);
    TRACE(cout << "#   memberInitializer: " << typeid(*memberInitializer).name() << endl);
    TRACE(const Declarator &memberDeclarator = member.getDeclarator());
    TRACE(cout << "#   memberDeclarator: " << memberDeclarator.toString() << endl);

    vector<uint16_t> arrayDimensions;  // from declarator and from TypeDesc
    arrayDimensions = member.getArrayDimensions();
    TRACE(cout << "#   member.getArrayDimensions(): " << vectorToString(arrayDimensions) << endl);

    memberTypeDesc->appendDimensions(arrayDimensions);
    TRACE(cout << "#   arrayDimensions (with TD dims): " << vectorToString(arrayDimensions) << endl);

    uint16_t written = 0;
    if (!member.isArray())
    {
        written = emitCode(out, memberTypeDesc, memberInitializer, false);
    }
    else
    {
        // We have a struct member that is an array, of one or more dimensions,
        // listed in arrayDimensions.
        // Emit code for the zeroth dimension of this array.
        //
        written = emitCodeForArray(out, memberTypeDesc, memberInitializer, arrayDimensions, 0);
    }
    TRACE(cout << "# InitializerCodeEmitter::emitCodeForStructMember: END: written=" << written << "\n");
    return written;
}


// arrayInitializer: A TreeSequence or a StringLiteralExpr.
//
uint16_t
InitializerCodeEmitter::emitCodeForArray(ASMText &out,
                                        const TypeDesc *arrayTypeDesc,
                                        const Tree *arrayInitializer,
                                        const vector<uint16_t> &arrayDimensions,
                                        size_t dimIndex)
{
    RecursionLevelUpdater u(recursionLevel);

    assert(arrayInitializer);
    TRACE(cout << "# InitializerCodeEmitter::emitCodeForArray: START\n");
    TRACE(cout << "#   arrayTypeDesc: " << *arrayTypeDesc << endl);
    TRACE(cout << "#   arrayInitializer: " << typeid(*arrayInitializer).name() << endl);
    TRACE(cout << "#   arrayDimensions: " << vectorToString(arrayDimensions) << endl);
    TRACE(cout << "#   dimIndex: " << dimIndex << endl);
    assert(dimIndex < arrayDimensions.size());
    uint16_t numElementsToFill = arrayDimensions[dimIndex];
    TRACE(cout << "#   numElementsToFill: " << numElementsToFill << endl);
    uint16_t finalArrayTypeSizeInBytes = (uint16_t) TranslationUnit::instance().getTypeSize(*arrayTypeDesc->getFinalArrayType());
    TRACE(cout << "#   final array TD: " << *arrayTypeDesc->getFinalArrayType() << ", of size " << finalArrayTypeSizeInBytes << endl);

    // Each element of the array must take exactly 'arrayElemSizeInBytes' memory locations.
    uint16_t dimProduct = 0;
    bool success = product(dimProduct, arrayDimensions.begin() + (dimIndex + 1), arrayDimensions.end());
    if (!success)
    {
        TRACE(cout << "#   OVERFLOW\n");
        return uint16_t(-1);
    }
    uint16_t arrayElemSizeInBytes = finalArrayTypeSizeInBytes * dimProduct;
    TRACE(cout << "#   arrayElemSizeInBytes: " << arrayElemSizeInBytes << endl);

    const size_t needFilledForArray = numElementsToFill * arrayElemSizeInBytes;
    assert(needFilledForArray < 0xFFFF);  // fits uint16_t, differs from uint16_t(-1) error code
    TRACE(cout << "#   needFilledForArray: " << needFilledForArray << " byte(s)\n");

    uint16_t written = 0;
    if (auto arrayInitSeq = dynamic_cast<const TreeSequence *>(arrayInitializer))
    {
        TRACE(cout << "#   arrayInitializer is TreeSequence of " << arrayInitSeq->size() << " element(s)\n");
        for (const Tree *arrayElementInitializer : *arrayInitSeq)
        {
            uint16_t w = 0;
            if (dimIndex + 1 == arrayDimensions.size())  // if array final type
            {
                TRACE(cout << "#     calling emitCode() on array element of type " << *arrayTypeDesc->pointedTypeDesc << "\n");
                w = emitCode(out,
                            arrayTypeDesc->pointedTypeDesc,
                            arrayElementInitializer,
                            false);
            }
            else  // sub-array:
            {
                TRACE(cout << "#     calling emitCodeForArray() on array element of type " << *arrayTypeDesc->pointedTypeDesc << "\n");
                w = emitCodeForArray(out,
                                    arrayTypeDesc->pointedTypeDesc,
                                    arrayElementInitializer,
                                    arrayDimensions,
                                    dimIndex + 1);
            }
            TRACE(cout << "#     array element filled " << w << " byte(s)\n");
            TRACE(cout << "#     space to fill: " << arrayElemSizeInBytes << endl);
            if (written != uint16_t(-1) && w != uint16_t(-1))
            {
                if (w < arrayElemSizeInBytes)
                {
                    TRACE(cout << "#     need to fill " << arrayElemSizeInBytes - w << " null byte(s) for this array element\n");
                    emitNullBytes(out, arrayElemSizeInBytes - w);
                    written += arrayElemSizeInBytes - w;
                }
                written += w;
            }
            else
                written = uint16_t(-1);
        }
        TRACE(cout << "#   wrote " << written << " byte(s) from TreeSequence\n");
        if (written < needFilledForArray)
        {
            TRACE(cout << "#     need to fill " << needFilledForArray - written << " null byte(s) for this array\n");
            emitNullBytes(out, needFilledForArray - written);
            written = uint16_t(needFilledForArray);
        }
    }
    else  // init char array from string literal:
    {
        assert(arrayTypeDesc->isArrayOfChar());
        auto sle = dynamic_cast<const StringLiteralExpr *>(arrayInitializer);
        assert(sle);
        TRACE(cout << "#   sle: " << sle->getEscapedVersion() << endl);
        assert(arrayElemSizeInBytes == 1);

        out.ins("LDD", "#" + wordToString(numElementsToFill), "size of char array");
        out.ins("PSHS", "B,A", "size argument of strncpy()");
        out.ins("LEAX", sle->getArg(), sle->getEscapedVersion());
        out.ins("PSHS", "X", "source argument of strncpy()");
        out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset), "array to initialize");
        out.ins("PSHS", "X", "destination argument of strncpy()");
        declaration.callUtility(out, "_strncpy", "copy without overflowing array");
        out.ins("LEAS", "6,S");

        written += numElementsToFill;
        writingOffset += numElementsToFill;
        TRACE(cout << "#   wrote " << numElementsToFill << " byte(s) from sle\n");
    }

    TRACE(cout << "# InitializerCodeEmitter::emitCodeForArray: END\n");
    return written;
}


// numBytes: must not be zero.
// Advances writingOffset by numBytes.
//
void
InitializerCodeEmitter::emitNullBytes(ASMText &out, size_t numBytes)
{
    RecursionLevelUpdater u(recursionLevel);

    TRACE(cout << "# InitializerCodeEmitter::emitNullBytes(" << numBytes << ")\n");
    assert(numBytes > 0);
    // TODO: Emit CLR instructions when shorter than following routine.
    out.ins("LEAX", declaration.getFrameDisplacementArg(writingOffset),
                    "offset in variable " + declaration.getVariableId() + " of byte(s) to reset");
    out.ins(numBytes <= 255 ? "LDB" : "LDD",
            "#" + wordToString(uint16_t(numBytes)),
            "number of bytes to reset");
    declaration.callUtility(out, numBytes <= 255 ? "resetBBytes" : "resetDBytes");
    writingOffset += numBytes;
}
