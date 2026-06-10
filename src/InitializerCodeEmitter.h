/*  $Id: InitializerCodeEmitter.h,v 1.2 2025/08/03 03:52:09 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2022 Pierre Sarrazin <http://sarrazip.com/>

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

#ifndef _H_InitializerCodeEmitter
#define _H_InitializerCodeEmitter

#include "ClassDef.h"

class Declaration;
class ASMText;


class InitializerCodeEmitter
{
public:
    // The declaration's initializer must be a TreeSequence.
    //
    InitializerCodeEmitter(const Declaration &_declaration);

    ~InitializerCodeEmitter();

    CodeStatus emitCode(ASMText &out);

private:
    InitializerCodeEmitter(const InitializerCodeEmitter &);
    InitializerCodeEmitter &operator = (const InitializerCodeEmitter &);

    // Recursive function (can call the other emitCode*() methods, which can call this one).
    // Upon success, returns the number of bytes written by the emitted code, i.e.,
    // the size of the type that gets initialized.
    // Upon failure, returns uint16_t(-1).
    //
    // typeDesc: Type of the field to initialize.
    //           Determines the number of bytes to be written, including any trailing padding null bytes.
    // initializer: Values to be used to initialize the field.
    //              Allowed to be shorter than the number of values needed.
    //              The remaining parts are padded with null bytes.
    // atDeclarationLevel: Must be true for the top-call (this method is recursive).
    //                     When true, if the Declaration is an array, its arrayDimensions member
    //                     will be used as the size of the arry.
    //                     When false, an array is only expected to be a member of a struct,
    //                     and emitCodeForArray() is called on such an array.
    //                     The dimensions of that array are determined by emitCodeForArray()
    //                     from the struct member's declarator and from that member's TypeDesc.
    // Returns the number of bytes written by the emitted code, or uint16_t(-1) upon error.
    //
    uint16_t emitCode(ASMText &out,
                        const TypeDesc *typeDesc,
                        const Tree *initializer,
                        bool atDeclarationLevel);
    uint16_t emitCodeForStruct(ASMText &out,
                                const TypeDesc *structTypeDesc,
                                const TreeSequence *structInitializer);
    uint16_t emitCodeForStructMember(ASMText &out,
                                        const TypeDesc *memberTypeDesc,
                                        const Tree *memberInitializer,
                                        const ClassDef::ClassMember &member);
    uint16_t emitCodeForArray(ASMText &out,
                                const TypeDesc *arrayTypeDesc,
                                const Tree *arrayInitializer,
                                const std::vector<uint16_t> &arrayDimensions,
                                size_t dimIndex);
    uint16_t emitCodeForSimpleType(ASMText &out,
                                const TypeDesc *structTypeDesc,
                                const Tree *structInitializer);
    void emitNullBytes(ASMText &out, size_t numBytes);

    const Declaration &declaration;
    uint16_t writingOffset;  // relative to start of variable defined by the Declaration object
    uint16_t recursionLevel;  // level of recursion by calls to emitCode*() methods (useful for debugging)
};


#endif  /* _H_InitializerCodeEmitter */
