/*  $Id: TypeManager.cpp,v 1.54 2020/04/05 02:57:21 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2016 Pierre Sarrazin <http://sarrazip.com/>

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

#include "TypeManager.h"

#include "util.h"
#include "Declarator.h"
#include "WordConstantExpr.h"
#include "ClassDef.h"
#include "Scope.h"
#include "Declarator.h"
#include "FunctionDef.h"
#include "IdentifierExpr.h"
#include "VariableExpr.h"
#include "Declaration.h"
#include "TranslationUnit.h"

using namespace std;


TypeManager::TypeManager()
:   types(),
    typeDefs(),
    enumTypeNames(),
    enumerators()
{
}


void
TypeManager::createBasicTypes()
{
    // The order is significant. See the other methods.
    types.push_back(new TypeDesc(VOID_TYPE, NULL, string(), false, false));
    types.push_back(new TypeDesc(BYTE_TYPE, NULL, string(), false, false));
    types.push_back(new TypeDesc(BYTE_TYPE, NULL, string(), true, false));
    types.push_back(new TypeDesc(WORD_TYPE, NULL, string(), false, false));
    types.push_back(new TypeDesc(WORD_TYPE, NULL, string(), true, false));
    types.push_back(new TypeDesc(SIZELESS_TYPE, NULL, string(), false, false));
    types.push_back(new TypeDesc(SIZELESS_TYPE, NULL, string(), true, false));
}


void
TypeManager::createInternalStructs(Scope &globalScope, TargetPlatform targetPlatform)
{
    // Internal structs that represent 'float', 'double', 'long' and 'unsigned long', e.g.,
    // struct _Float { unsigned char bytes[N]; };
    // struct _Long { int hi; unsigned lo; };
    //
    // Order matters: these calls add elements to types[] and their indices are used in
    // getFloatType(), getLongType().
    //
    createStructWithPairOfWords(globalScope, "_Long",   true);
    createStructWithPairOfWords(globalScope, "_ULong", false);
    createStructWithArrayOfBytes(globalScope, "_Float",  getFloatingPointFormatSize(targetPlatform, false));
    createStructWithArrayOfBytes(globalScope, "_Double", getFloatingPointFormatSize(targetPlatform, true));
}


void
TypeManager::createStructWithArrayOfBytes(Scope &globalScope, const char *structName, size_t numBytesInArray)
{
    types.push_back(new TypeDesc(CLASS_TYPE, NULL, structName, false, false));
    ClassDef *theStruct = new ClassDef();
    theStruct->setName(structName);  // use same name as TypeDesc
    const TypeDesc *memberTypeDesc = getIntType(BYTE_TYPE, false);
    Declarator *memberDeclarator = new Declarator("bytes", "<internal>", 0);  // no source filename and line no
    memberDeclarator->addArraySizeExpr(new WordConstantExpr(numBytesInArray, true, false));  // make the declarator an array
    assert(memberDeclarator->isArray());
    ClassDef::ClassMember *structMember = new ClassDef::ClassMember(memberTypeDesc, memberDeclarator);  // this appends 'unsigned char[]' to types[]
    theStruct->addDataMember(structMember);
    globalScope.declareClass(theStruct);
}


void
TypeManager::createStructWithPairOfWords(Scope &globalScope, const char *structName, bool isHighWordSigned)
{
    types.push_back(new TypeDesc(CLASS_TYPE, NULL, structName, isHighWordSigned, false));
    ClassDef *theStruct = new ClassDef();
    theStruct->setName(structName);  // use same name as TypeDesc

    const TypeDesc *highWordTypeDesc = getIntType(WORD_TYPE, isHighWordSigned);
    const TypeDesc *lowWordTypeDesc  = getIntType(WORD_TYPE, false);

    Declarator *highMemberDeclarator = new Declarator("hi", "<internal>", 0);  // no source filename and line no
    Declarator *lowMemberDeclarator  = new Declarator("lo", "<internal>", 0);

    ClassDef::ClassMember *highStructMember = new ClassDef::ClassMember(highWordTypeDesc, highMemberDeclarator);
    ClassDef::ClassMember *lowStructMember  = new ClassDef::ClassMember(lowWordTypeDesc,  lowMemberDeclarator);

    // We compile for a big endian platform, so the high word is declared first.
    theStruct->addDataMember(highStructMember);
    theStruct->addDataMember(lowStructMember);

    globalScope.declareClass(theStruct);
}


TypeManager::~TypeManager()
{
    for (vector<TypeDesc *>::iterator it = types.begin(); it != types.end(); ++it)
        delete *it;

    // Destroy the Enumerator objects.
    for (EnumeratorList::iterator it = enumerators.begin(); it != enumerators.end(); ++it)
        delete it->second;
}


const TypeDesc *
TypeManager::getVoidType() const
{
    return types[0];
}


const TypeDesc *
TypeManager::getIntType(BasicType byteOrWordType, bool isSigned) const
{
    if (byteOrWordType == BYTE_TYPE)
        return types[isSigned ? 2 : 1];
    if (byteOrWordType == WORD_TYPE)
        return types[isSigned ? 4 : 3];
    assert(false);
    return NULL;
}


const TypeDesc *
TypeManager::getIntType(const TypeDesc *baseTypeDesc, bool isSigned) const
{
    if (baseTypeDesc->type == BYTE_TYPE || baseTypeDesc->type == WORD_TYPE)
        return getIntType(baseTypeDesc->type, isSigned);
    return getLongType(isSigned);
}


const TypeDesc *
TypeManager::getLongType(bool isSigned) const
{
    return types[isSigned ? 7 : 8];
}


const TypeDesc *
TypeManager::getRealType(bool isDoublePrecision) const
{
    return types[isDoublePrecision ? 11 : 9];
}


const TypeDesc *
TypeManager::getSizelessType(bool isSigned) const
{
    return types[isSigned ? 6 : 5];
}


const TypeDesc *
TypeManager::getPointerTo(const TypeDesc *pointedTypeDesc) const
{
    if (pointedTypeDesc == NULL)
        return NULL;

    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        assert(td && td->isValid());
        if (td->type == POINTER_TYPE && td->pointedTypeDesc == pointedTypeDesc)
            return td;
    }

    types.push_back(new TypeDesc(POINTER_TYPE, pointedTypeDesc, string(), false, false));
    return types.back();
}


// typeQualifierBitFieldPerPointerLevel: List of bitfields containing CONST_BIT or not.
// The number of elements determines the pointer level.
// Example 1: getPointerTo('char', {0, 0, 0}) will return 'char ***'.
// Example 2: getPointerTo('int', {CONST_BIT}) will return 'int * const', i.e., a constant pointer to an int.
// Example 3: getPointerTo('const int', {0}) will return 'const int *', i.e., a non-constant pointer to a constant integer.
// Example 4: getPointerTo('int', {0, CONST_BIT}) will return 'int ** const'.
// Example 5: getPointerTo('int', {}) will return 'int'.
//
const TypeDesc *
TypeManager::getPointerTo(const TypeDesc *typeDesc, const TypeQualifierBitFieldVector &typeQualifierBitFieldPerPointerLevel) const
{
    for (size_t i = 0; i < typeQualifierBitFieldPerPointerLevel.size(); ++i)
    {
        typeDesc = getPointerTo(typeDesc);
        if (typeQualifierBitFieldPerPointerLevel[i] & CONST_BIT)
            typeDesc = getConst(typeDesc);
    }
    return typeDesc;
}


// Returns a type that is equivalent to 'typeDesc' but whose isConst field is true.
//
const TypeDesc *
TypeManager::getConst(const TypeDesc *typeDesc) const
{
    if (typeDesc == NULL)
        return NULL;

    if (typeDesc->isConst)
        return typeDesc;

    TypeDesc targetTypeDesc(*typeDesc);
    targetTypeDesc.isConst = true;
    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        assert(td && td->isValid());
        if (*td == targetTypeDesc)
            return td;  // found existing const version of 'typeDesc'
    }

    types.push_back(new TypeDesc(targetTypeDesc));
    return types.back();
}


const TypeDesc *
TypeManager::getPointerToIntegral(BasicType byteOrWordType, bool isSigned) const
{
    return getPointerTo(getIntType(byteOrWordType, isSigned));
}


const TypeDesc *
TypeManager::getArrayOfChar() const
{
    return getArrayOf(getIntType(BYTE_TYPE, true), 1);
}


const TypeDesc *
TypeManager::getArrayOfConstChar() const
{
    return getArrayOf(getConst(getIntType(BYTE_TYPE, true)), 1);
}


const TypeDesc *
TypeManager::getPointerToVoid() const
{
    return getPointerTo(getVoidType());
}


// Can be called for a non-array, by passing zero for numArrayDimensions.
// Then this function just returns pointedTypeDesc.
//
const TypeDesc *
TypeManager::getArrayOf(const TypeDesc *pointedTypeDesc, size_t numArrayDimensions) const
{
    if (pointedTypeDesc == NULL)
        return NULL;

    if (numArrayDimensions == 0)
        return pointedTypeDesc;

    if (numArrayDimensions == 1)
    {
        for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
        {
            const TypeDesc *td = *it;
            assert(td && td->isValid());
            if (td->type == ARRAY_TYPE && td->pointedTypeDesc == pointedTypeDesc && td->numArrayElements == uint16_t(-1))
                return td;
        }

        types.push_back(new TypeDesc(ARRAY_TYPE, pointedTypeDesc, string(), false, false));
        return types.back();
    }

    return getArrayOf(getArrayOf(pointedTypeDesc, numArrayDimensions - 1), 1);
}


// getSizedArrayOf("int", {2, 3, 4}, 2) gives int[2][3][4].
// It recursively calls getSizedArrayOf("int", {2, 3, 4}, 1) to request int[2][3].
// This calls getSizedArrayOf("int", {2, 3, 4}, 0) to get int[2].
// Then int[2][3] is created and returned.
// Then int[2][3][4] is created and returned.
//
const TypeDesc *
TypeManager::getSizedArrayOf(const TypeDesc *pointedTypeDesc, const std::vector<uint16_t> &arrayDimensions, size_t dimIndex) const
{
    //cerr << "# TypeManager::getSizedArrayOf([" << pointedTypeDesc->toString() << "], {" << join(", ", arrayDimensions) << "}, " << dimIndex << ")\n";
    assert(dimIndex < arrayDimensions.size());

    if (pointedTypeDesc == NULL)
        return NULL;

    if (dimIndex == 0)
        return getSizedOneDimArrayOf(pointedTypeDesc, arrayDimensions[0]);

    const TypeDesc *subTypeDesc = getSizedArrayOf(pointedTypeDesc, arrayDimensions, dimIndex - 1);
    return getSizedOneDimArrayOf(subTypeDesc, arrayDimensions[dimIndex]);
}


const TypeDesc *
TypeManager::getSizedOneDimArrayOf(const TypeDesc *pointedTypeDesc, size_t numArrayElements) const
{
    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        assert(td && td->isValid());
        if (td->type == ARRAY_TYPE && td->pointedTypeDesc == pointedTypeDesc && td->numArrayElements == numArrayElements)
            return td;
    }

    types.push_back(new TypeDesc(ARRAY_TYPE, pointedTypeDesc, string(), false, false, numArrayElements));
    return types.back();
}

const TypeDesc *
TypeManager::getClassType(const std::string &className, bool isUnion, bool createIfAbsent) const
{
    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        assert(td && td->isValid());
        if (td->type == CLASS_TYPE && td->isUnion == isUnion && td->className == className)
            return td;
    }

    if (createIfAbsent)
    {
        if (getClassType(className, !isUnion, false) != NULL)
            errormsg("referring to %s as a %s, but it is a %s",
                        className.c_str(), isUnion ? "union" : "struct", !isUnion ? "union" : "struct");

        types.push_back(new TypeDesc(CLASS_TYPE, NULL, className, false, isUnion));
        return types.back();
    }

    return NULL;
}


const TypeDesc *
TypeManager::getFunctionPointerType(const FunctionDef &fd) const
{
    assert(fd.getFormalParamList());
    return getFunctionPointerType(fd.getTypeDesc(), *fd.getFormalParamList(), fd.isInterruptServiceRoutine(), fd.isFunctionReceivingFirstParamInReg());
}


// Looks up types[] to find an existing function pointer type with the given
// return type and formal parameter types.
//
const TypeDesc *
TypeManager::findFunctionPointerType(const TypeDesc *returnTypeDesc,
                                     const FormalParamList &params,
                                     bool isISR,
                                     bool receivesFirstParamInReg) const
{
    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        assert(td && td->isValid());
        if (td->type != POINTER_TYPE)
            continue;
        const TypeDesc *funcTD = td->pointedTypeDesc;
        if (funcTD->type != FUNCTION_TYPE)
            continue;
        if (funcTD->isISR != isISR || funcTD->receivesFirstParamInReg != receivesFirstParamInReg || funcTD->ellipsis != params.endsWithEllipsis())
            continue;
        if (*funcTD->returnTypeDesc != *returnTypeDesc)
            continue;
        size_t numParams = (params.hasSingleVoidParam() ? 0 : params.size());
        if (funcTD->formalParamTypeDescList.size() != numParams)
            continue;
        bool allParamsMatch = true;
        vector<Tree *>::const_iterator paramTreeIt = params.begin();
        for (vector<const TypeDesc *>::const_iterator typeDescIt = funcTD->formalParamTypeDescList.begin();
                                                     typeDescIt != funcTD->formalParamTypeDescList.end(); ++typeDescIt, ++paramTreeIt)
            if (*(*paramTreeIt)->getTypeDesc() != **typeDescIt)
            {
                allParamsMatch = false;
                break;
            }
        if (!allParamsMatch)
            continue;

        //cout << "# TypeManager::findFunctionPointerType: return " << td->toString() << "\n";
        return td;
    }
    return NULL;
}


const TypeDesc *
TypeManager::getFunctionPointerType(const TypeDesc *returnTypeDesc, const FormalParamList &params, bool isISR, bool receivesFirstParamInReg) const
{
    //cout << "# TypeManager::getFunctionPointerType({" << returnTypeDesc->toString() << "}, ell=" << params.endsWithEllipsis() << ")\n";

    const TypeDesc *fixedReturnTypeDesc = getTypeWithoutCallingConventionFlags(returnTypeDesc);

    const TypeDesc *preexistingTD = findFunctionPointerType(fixedReturnTypeDesc, params, isISR, receivesFirstParamInReg);
    if (preexistingTD)
        return preexistingTD;

    TypeDesc *funcTD = new TypeDesc(fixedReturnTypeDesc, isISR, params.endsWithEllipsis(), receivesFirstParamInReg);
    assert(funcTD->type == FUNCTION_TYPE);
    types.push_back(funcTD);

    // Add the argument types to funcTD, unless the list of args is just (void).
    //
    if (!params.hasSingleVoidParam())
        for (vector<Tree *>::const_iterator it = params.begin(); it != params.end(); ++it)
             funcTD->addFormalParamTypeDesc((*it)->getTypeDesc());

    //cout << "# TypeManager::getFunctionPointerType:   funcTD={" << funcTD->toString() << "}\n";
    return getPointerTo(funcTD);
}


const TypeDesc *
TypeManager::getInterruptType(const TypeDesc *existingType) const
{
    if (existingType->isISR)
        return existingType;

    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        if (((- TypeDesc::compare(*existingType, *td)) & 2) && td->isISR)
            return td;
    }

    TypeDesc *newTD = new TypeDesc(*existingType);
    newTD->isISR = true;
    types.push_back(newTD);
    return newTD;
}


const TypeDesc *
TypeManager::getFPIRType(const TypeDesc *existingType) const
{
    if (existingType->receivesFirstParamInReg)
        return existingType;

    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        if (((- TypeDesc::compare(*existingType, *td)) & 4) && td->receivesFirstParamInReg)
            return td;
    }

    TypeDesc *newTD = new TypeDesc(*existingType);
    newTD->receivesFirstParamInReg = true;
    types.push_back(newTD);
    return newTD;
}


const TypeDesc *
TypeManager::getTypeWithoutCallingConventionFlags(const TypeDesc *existingType) const
{
    if (! existingType->isISR && ! existingType->receivesFirstParamInReg)
        return existingType;

    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        const TypeDesc *td = *it;
        // If td has no calling convention flags and differs from existingType only by such flags, then td is it.
        if (!td->isISR && !td->receivesFirstParamInReg && ((- TypeDesc::compare(*existingType, *td)) & (2 | 4)))
            return td;
    }

    TypeDesc *newTD = new TypeDesc(*existingType);
    newTD->isISR = false;
    newTD->receivesFirstParamInReg = false;
    types.push_back(newTD);
    return newTD;
}


// Ends by calling delete on 'declarator'.
//
// Note: In the cast of a function pointer, 'declSpecTypeDef' is only the return type.
//       For example, with typedef int (*f)(), declSpecTypeDef will represent int,
//       and declarator->isFunctionPointer() will be true.
//
bool
TypeManager::addTypeDef(const TypeDesc *declSpecTypeDef, Declarator *declarator)
{
    assert(declSpecTypeDef);
    assert(declarator);

    bool success = false;

    TypeDefMap::iterator it;
    const string &id = declarator->getId();
    if (id.empty())
        errormsg("empty typename");
    else if ((it = typeDefs.find(id)) != typeDefs.end())  // if type name already used:
        errormsg("cannot redefine typedef `%s'", id.c_str());
    else if (declSpecTypeDef->isInterruptServiceRoutine() && ! declarator->isFunctionPointer())
        errormsg("modifier `interrupt' cannot be used on typedef");
    else
    {
        if (! declarator->isFunctionPointer() && ! declarator->isArrayOfFunctionPointers() && declarator->getFormalParamList() != NULL)
        {
            errormsg("invalid function typedef");
            // Continue despite the error and register the new typedef name,
            // which avoids a syntax error when the name likely gets used in
            // the C code that follows.
        }

        // Now, check for asterisks, i.e., pointers.
        // Example: With "typedef int **PP;", declSpecTypeDef represents "int"
        // and declarator represents (2, "PP"), where 2 is the pointer level.
        // The next call assigns "int **" to specificTypeDesc.
        //
        const TypeDesc *specificTypeDesc = declarator->processPointerLevel(declSpecTypeDef);
        assert(specificTypeDesc);

        if (declarator->isFunctionPointer() || declarator->isArrayOfFunctionPointers())
            specificTypeDesc = getFunctionPointerType(specificTypeDesc,
                                                      *declarator->getFormalParamList(),
                                                      specificTypeDesc->isInterruptServiceRoutine(),
                                                      specificTypeDesc->receivesFirstParamInReg);

        // Now, check for array dimensions, e.g., a[5][7].
        vector<uint16_t> arrayDimensions;
        if (declarator->computeArrayDimensions(arrayDimensions, false, NULL))  // arrayDimensions will be empty if non-array
        {
            if (arrayDimensions.size() > 0)
                specificTypeDesc = getSizedArrayOf(specificTypeDesc, arrayDimensions, arrayDimensions.size() - 1);

            typeDefs[id] = specificTypeDesc;
            success = true;
        }
    }

    delete declarator;

    return success;
}


const TypeDesc *
TypeManager::getTypeDef(const char *id) const
{
    TypeDefMap::const_iterator it = typeDefs.find(id);
    if (it != typeDefs.end())
        return it->second;
    return NULL;
}


// If an Enumerator in enumerationList is a duplicate, it gets destroyed
// and removed from enumerationList.
// Otherwise, calls declareEnumerator() for each Enumerator.
// Also registers the enum type name with the list of its enumerated names.
//
// Does not keep a reference to enumerationList.
//
void
TypeManager::declareEnumerationList(const std::string &enumTypeName,
                                    std::vector<Enumerator *> &enumerationList)
{
    if (!enumTypeName.empty())
    {
        EnumTypeNameMap::const_iterator it = enumTypeNames.find(enumTypeName);
        if (it != enumTypeNames.end())
            errormsg("enum `%s' already defined at %s", enumTypeName.c_str(), it->second.sourceLineNo.c_str());
        else
        {
            // Register the named enum with the source line number where it is defined
            // and with the names of its members.
            //
            enumTypeNames[enumTypeName] = NamedEnum(getSourceLineNo());
            NamedEnum &namedEnum = enumTypeNames[enumTypeName];
            for (std::vector<Enumerator *>::const_iterator jt = enumerationList.begin();
                                                          jt != enumerationList.end(); ++jt)
                namedEnum.members.push_back((*jt)->name);
        }
    }

    const Enumerator *prevEnumerator = NULL;

    for (vector<Enumerator *>::iterator it = enumerationList.begin();
                                       it != enumerationList.end(); /* iterator moves in for() body */ )
    {
        Enumerator *enumerator = *it;
        if (!declareEnumerator(enumerator))
        {
            delete enumerator;
            it = enumerationList.erase(it);
            continue;
        }

        enumerator->setPreviousEnumerator(prevEnumerator);  // tie each enumerator to its predecessor(s)
        prevEnumerator = enumerator;
        ++it;
    }
}


Enumerator *
TypeManager::findEnumerator(const string &enumeratorName) const
{
    for (size_t i = 0; i < enumerators.size(); ++i)
        if (enumerators[i].first == enumeratorName)
            return enumerators[i].second;
    return NULL;
}


// enumerator: Upon success, this pointer is stored in the 'enumerators' map.
// Returns true for success, false for failure (an error message is issued and
// 'enumerator' is not destroyed).
//
bool
TypeManager::declareEnumerator(Enumerator *enumerator)
{
    //cout << "# TypeManager::declareEnumerator: enumerator " << enumerator << ": " << enumerator->name << endl;
    assert(enumerator);

    Enumerator *existingEnumerator = findEnumerator(enumerator->name);
    if (existingEnumerator)
    {
        errormsg("enumerated name `%s' already defined at %s",
                 enumerator->name.c_str(), existingEnumerator->sourceLineNo.c_str());
        return false;
    }

    enumerators.push_back(make_pair(enumerator->name, enumerator));
    return true;
}

bool
TypeManager::isEnumeratorName(const string &id) const
{
    return findEnumerator(id) != NULL;
}


const TypeDesc *
TypeManager::getEnumeratorTypeDesc(const std::string &id) const
{
    const Enumerator *enumerator = findEnumerator(id);
    if (!enumerator)
        return NULL;  // not an enumerated name

    while (enumerator && enumerator->valueExpr == NULL)
        enumerator = enumerator->previousEnumerator;
    if (!enumerator)
        return getIntType(WORD_TYPE, true);  // signed int as default

    return enumerator->valueExpr->getTypeDesc();
}


bool
TypeManager::getEnumeratorValue(const std::string &id, uint16_t &value) const
{
    const Enumerator *enumerator = findEnumerator(id);
    if (!enumerator)
        return false;  // not an enumerated name

    uint16_t increment = 0;
    while (enumerator && enumerator->valueExpr == NULL)
    {
        // Use value of preceding name, if any, plus one.
        enumerator = enumerator->previousEnumerator;
        ++increment;
    }

    if (!enumerator)  // if no initializer expression found before reaching start of enum{}
    {
        value = increment - 1;  // value depends on rank of 'id' in enum{}
        return true;
    }

    if (!enumerator->valueExpr->evaluateConstantExpr(value))
    {
        enumerator->valueExpr->errormsg("expression for enumerated name `%s' must be constant", id.c_str());
        value = 0;  // return zero as fallback value; no code should be emitted anyway
        return true;
    }

    value += increment;
    return true;
}


bool
TypeManager::isIdentiferMemberOfNamedEnum(const std::string &enumTypeName, const std::string &id) const
{
    EnumTypeNameMap::const_iterator it = enumTypeNames.find(enumTypeName);
    if (it == enumTypeNames.end())
        return false;  // unknown enum
    const NamedEnum &namedEnum = it->second;
    return find(namedEnum.members.begin(), namedEnum.members.end(), id) != namedEnum.members.end();
}


void
TypeManager::dumpTypes(std::ostream &out) const
{
    for (vector<TypeDesc *>::const_iterator it = types.begin(); it != types.end(); ++it)
        out << **it << "\n";
}


size_t
TypeManager::getFloatingPointFormatSize(TargetPlatform platform, bool isDoublePrecision)
{
    switch (platform)
    {
    case COCO_BASIC:
        return 5;
    case OS9:
        return isDoublePrecision ? 8 : 4;
    default:
        return 0;
    }
}


Enumerator::Enumerator(const char *_name, Tree *_valueExpr, const std::string &_sourceLineNo)
:   name(_name),
    valueExpr(_valueExpr),
    sourceLineNo(_sourceLineNo),
    previousEnumerator(NULL)
{
}


Enumerator::~Enumerator()
{
    delete valueExpr;
}
