/*  $Id: TypeDesc.h,v 1.31 2019/08/17 19:52:21 sarrazip Exp $

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

#ifndef _H_TypeDesc
#define _H_TypeDesc

#include <assert.h>
#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>


enum BasicType
{
    VOID_TYPE,
    BYTE_TYPE,
    WORD_TYPE,
    POINTER_TYPE,
    ARRAY_TYPE,
    CLASS_TYPE,
    FUNCTION_TYPE,
    SIZELESS_TYPE,  // for 'signed' and 'unsigned'
};


const char *getBasicTypeName(BasicType bt);


std::ostream &operator << (std::ostream &out, BasicType bt);


// Instances must be allocated only by TypeManager.
//
class TypeDesc
{
public:
    BasicType type;
    const TypeDesc *pointedTypeDesc;  // must come from TypeManager or be null;
                                      // relevant when type == POINTER_TYPE or type == ARRAY_TYPE
private:
    // Revelant only when type == FUNCTION_TYPE:
    const TypeDesc *returnTypeDesc;
    std::vector<const TypeDesc *> formalParamTypeDescList;  // may be empty
    bool isISR;  // function type uses the 'interrupt' keyword
    bool ellipsis;  // variadic function, i.e., arguments end with '...'
    bool receivesFirstParamInReg; // function that expects its first argument in a register, instead of on stack
    bool isConst;

public:
    std::string className;      // non empty if type == CLASS_TYPE
    uint16_t numArrayElements;  // relevant when type == ARRAY_TYPE; uint16_t(-1) means undetermined number of elements
    bool isSigned;
    bool isUnion;               // false means struct (only applies when type == CLASS_TYPE)


    bool isValid() const;

    std::string toString() const;

    bool isArray() const;

    bool isPtrOrArray() const;

    bool isPtrToFunction() const;

    bool isByteOrWord() const;

    bool isIntegral() const;

    bool isNumerical() const;

    bool isLong() const;

    bool isReal() const;

    bool isSingle() const;

    bool isDouble() const;

    bool isRealOrLong() const;

    bool isStruct() const;

    bool isInterruptServiceRoutine() const;

    bool isFunctionReceivingFirstParamInReg() const;

    bool isTypeWithCallingConventionFlags() const;

    bool isTypeWithoutCallingConventionFlags() const;

    // Returns true if this type has the const keyword at the first level (e.g., const int)
    // or if it is an array of elements whose type isConstant() (e.g., const int a[]).
    // Note that this method returns false for 'const int *', because the pointer is writable,
    // i.e., the const keyword is not at the first level.
    //
    bool isConstant() const;

    // Determines if a variable of this type is suitable for the rodata section, for ROM.
    // This is different from isConstant(), which checks for "C constness".
    //
    bool canGoInReadOnlySection(bool isRelocatabilitySupported) const;

    // Returns a TypeDesc that represents the pointed type.
    // Returns NULL if this type is not a pointer or array.
    //
    const TypeDesc *getPointedTypeDesc() const;

    BasicType getPointedType() const;

    size_t getPointerLevel() const;

    // Returns NULL if this type is not a FUNCTION_TYPE.
    //
    const TypeDesc *getReturnTypeDesc() const;

    const std::vector<const TypeDesc *> &getFormalParamTypeDescList() const;

    bool endsWithEllipsis() const;

    void appendDimensions(std::vector<uint16_t> &arrayDimensions) const;

    // Returns a number of elements, not a number of bytes.
    //
    size_t getNumArrayElements() const;

    // Returns true iff this type and 'td' are both pointers or arrays
    // and the pointed type is the same.
    //
    bool pointsToSameType(const TypeDesc &td) const;

    // Returns 0 if a and b are exactly the same type.
    // Returns -1 if they differ.
    // Returns a negate bit field if they differ only by the isISR or receivesFirstParamInReg fields.
    // In the bit field:
    //   2 means differs by isISR;
    //   4 means differs by receivesFirstParamInReg.
    //
    static int compare(const TypeDesc &a, const TypeDesc &b);

    static bool sameTypesModuloConst(const TypeDesc &a, const TypeDesc &b);

    static bool samePointerOrArrayTypesModuloSignedness(const TypeDesc &a, const TypeDesc &b);

    static bool sameTypesModuloConstAtPtrLevel(const TypeDesc &a, const TypeDesc &b);

private:

    TypeDesc(const TypeDesc &);

    friend std::ostream &operator << (std::ostream &out, const TypeDesc &td);
    friend bool operator == (const TypeDesc &a, const TypeDesc &b);

    static void printFunctionSignature(std::ostream &out, const TypeDesc &funcTD,
                                       bool pointer, bool isPointerConst, bool arrayOfPointers);

    friend class TypeManager;  // the only class that can create TypeDesc instances

    TypeDesc(BasicType _basicType,
             const TypeDesc *_pointedTypeDesc,
             const std::string &_className,
             bool _isSigned,
             bool _isUnion,
             uint16_t _numArrayElements = uint16_t(-1));

    TypeDesc(const TypeDesc *_returnTypeDesc, bool _isISR, bool _endsWithEllipsis, bool _receivesFirstParamInReg);

    void addFormalParamTypeDesc(const TypeDesc *_formalParamTypeDesc);

    bool canMemberGoInReadOnlySection(const TypeDesc *memberTypeDesc, bool isRelocatabilitySupported) const;

    // Forbidden:
    TypeDesc &operator = (const TypeDesc &);

};


class Enumerator;


class TypeSpecifier
{
public:
    const TypeDesc *typeDesc;
    std::string enumTypeName;  // empty for anonymous enum, and for non-enum type
    std::vector<Enumerator *> *enumeratorList;  // null unless enum type

    TypeSpecifier(const TypeDesc *_typeDesc, const std::string &_enumTypeName, std::vector<Enumerator *> *_enumeratorList)
    :   typeDesc(_typeDesc), enumTypeName(_enumTypeName), enumeratorList(_enumeratorList) {}

private:
    TypeSpecifier(const TypeSpecifier &);
    TypeSpecifier &operator = (const TypeSpecifier &);
};


std::ostream &operator << (std::ostream &out, const TypeDesc &td);


bool operator == (const TypeDesc &a, const TypeDesc &b);


inline bool
operator != (const TypeDesc &a, const TypeDesc &b)
{
    return !(a == b);
}


std::ostream &
operator << (std::ostream &out, const TypeDesc &td);


int16_t getTypeSize(BasicType t);


#endif  /* _H_TypeDesc */
