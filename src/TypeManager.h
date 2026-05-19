/*  $Id: TypeManager.h,v 1.37 2020/04/05 02:57:22 sarrazip Exp $

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

#ifndef _H_TypeManager
#define _H_TypeManager

#include "util.h"

#include <vector>
#include <map>

class Declarator;
class Enumerator;
class Tree;
class FunctionDef;
class FormalParamList;


// Represents an enumeration (enum) that has a name.
//
struct NamedEnum
{
    std::string sourceLineNo;
    std::vector<std::string> members;

    NamedEnum(const std::string &_sourceLineNo = std::string())
        : sourceLineNo(_sourceLineNo), members() {}
};


// Sole owner of all instances of class TypeDesc.
//
class TypeManager
{
public:

    TypeManager();

    ~TypeManager();

    void createBasicTypes();

    void createInternalStructs(class Scope &globalScope, TargetPlatform targetPlatform);

    const TypeDesc *getVoidType() const;

    const TypeDesc *getIntType(BasicType byteOrWordType, bool isSigned) const;

    const TypeDesc *getIntType(const TypeDesc *baseTypeDesc, bool isSigned) const;

    const TypeDesc *getSizelessType(bool isSigned) const;  // for 'signed' and 'unsigned'

    const TypeDesc *getLongType(bool isSigned) const;

    const TypeDesc *getRealType(bool isDoublePrecision) const;

    const TypeDesc *getPointerTo(const TypeDesc *td) const;

    const TypeDesc *getPointerTo(const TypeDesc *td, const TypeQualifierBitFieldVector &typeQualifierBitFieldPerPointerLevel) const;

    const TypeDesc *getPointerToIntegral(BasicType byteOrWordType, bool isSigned) const;

    const TypeDesc *getConst(const TypeDesc *typeDesc) const;

    const TypeDesc *getArrayOfChar() const;

    const TypeDesc *getArrayOfConstChar() const;

    const TypeDesc *getPointerToVoid() const;

    const TypeDesc *getArrayOf(const TypeDesc *td, size_t numArrayDimensions) const;

    const TypeDesc *getSizedArrayOf(const TypeDesc *td, const std::vector<uint16_t> &arrayDimensions, size_t dimIndex) const;

    // isUnion: false for a struct.
    // createIfAbsent: If false, returns NULL if the className is not found.
    //
    const TypeDesc *getClassType(const std::string &className, bool isUnion, bool createIfAbsent) const;

    // Returns a type that describes the designated function definition.
    //
    // Does not keep a reference to 'fd'.
    //
    const TypeDesc *getFunctionPointerType(const FunctionDef &fd) const;

    // Returns a type that describes a function returning the given return type
    // and formal parameter types.
    //
    // Does not keep a reference to 'params'.
    //
    const TypeDesc *getFunctionPointerType(const TypeDesc *returnTypeDesc,
                                           const FormalParamList &params,
                                           bool isISR,
                                           bool receivesFirstParamInReg) const;

    const TypeDesc *getInterruptType(const TypeDesc *existingType) const;

    const TypeDesc *getFPIRType(const TypeDesc *existingType) const;

    const TypeDesc *getTypeWithoutCallingConventionFlags(const TypeDesc *existingType) const;

    bool addTypeDef(const TypeDesc *declSpecTypeDef, Declarator *declarator);

    const TypeDesc *getTypeDef(const char *id) const;

    void declareEnumerationList(const std::string &enumTypeName,
                                std::vector<Enumerator *> &enumerationList);

    bool declareEnumerator(Enumerator *enumerator /*, uint16_t defautValue, uint16_t &enumeratorValue*/ );

    bool isEnumeratorName(const std::string &id) const;

    const TypeDesc *getEnumeratorTypeDesc(const std::string &id) const;

    // Returns the numerical value of the requested enumerated name.
    // Example: Given "A" when 'enum { A = 1 << 3 }' is defined, 8 is returned.
    // If the identifier is not defined, false is returned.
    // If the expression is not constant, an error message is issued and false is returns.
    // 'value' is only defined when true is returned.
    //
    bool getEnumeratorValue(const std::string &id, uint16_t &value) const;

    bool isIdentiferMemberOfNamedEnum(const std::string &enumTypeName, const std::string &id) const;

    void dumpTypes(std::ostream &out) const;

    // In bytes. Returns 0 if floats are not supposed on the given platform.
    //
    static size_t getFloatingPointFormatSize(TargetPlatform platform, bool isDoublePrecision);

private:

    void createStructWithArrayOfBytes(Scope &globalScope, const char *structName, size_t numBytesInArray);
    void createStructWithPairOfWords(Scope &globalScope, const char *structName, bool isHighWordSigned);
    Enumerator *findEnumerator(const std::string &enumeratorName) const;
    const TypeDesc *findFunctionPointerType(const TypeDesc *returnTypeDesc,
                                            const FormalParamList &params,
                                            bool isISR,
                                            bool receivesFirstParamInReg) const;
    const TypeDesc *getSizedOneDimArrayOf(const TypeDesc *pointedTypeDesc, size_t numArrayElements) const;

private:

    typedef std::map<std::string, const TypeDesc *> TypeDefMap;  // key: typedef name; value: type defined in types[]
    typedef std::map<std::string, NamedEnum> EnumTypeNameMap;  // key: enum type name

    typedef std::pair<std::string, class Enumerator *> EnumeratorNamePair;
    typedef std::vector<EnumeratorNamePair> EnumeratorList;  // key: enum name
        // NOTE: Enumerators listed in declaration order so that they get processed in that order by the DeclarationFinisher.

    mutable std::vector<TypeDesc *> types;  // see the constructor for predefined types
    TypeDefMap typeDefs;
    EnumTypeNameMap enumTypeNames;
    EnumeratorList enumerators;  // owns the Enumerators, must delete them

};


// Represents a member of an enum, e.g., A in "enum { A };".
// In "enum {B = 42}", valueExpr is the tree that represents 42.
//
class Enumerator
{
public:
    std::string name;
    Tree *valueExpr;  // allowed to be null
    std::string sourceLineNo;  // where this enumerator was defined in the source code
    const Enumerator *previousEnumerator;  // if valueExpr null, use this chained list to deduce value

    Enumerator(const char *_name, Tree *_valueExpr, const std::string &_sourceLineNo);

    ~Enumerator();  // destroys valueExpr; does not destroy previousEnumerator

    void setPreviousEnumerator(const Enumerator *prev) { previousEnumerator = prev; }

private:
    Enumerator(const Enumerator &);
    Enumerator &operator = (const Enumerator &);
};


#endif  /* _H_TypeManager */
