/*  $Id: DeclarationSpecifierList.cpp,v 1.15 2019/06/22 03:35:44 sarrazip Exp $

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

#include "DeclarationSpecifierList.h"

#include "TranslationUnit.h"

using namespace std;


DeclarationSpecifierList::DeclarationSpecifierList()
  : typeDesc(NULL),
    isTypeDef(false),
    isISR(false),
    receivesFirstParamInReg(false),
    asmOnly(false),
    noReturnInstruction(false),
    isExtern(false),
    isStatic(false),
    isConst(false),
    isVolatile(false),
    enumTypeName(),
    enumeratorList(NULL)
{
}

DeclarationSpecifierList::~DeclarationSpecifierList()
{
    // detachEnumeratorList() must have been called.
    if (enumeratorList)
    {
        warnmsg("suspicious use of enum");
        delete enumeratorList;
    }
}


// Does not keep a reference to 'tsToAdd', but keeps a pointer to 'tsToAdd.typeDesc'.
//
void
DeclarationSpecifierList::add(const TypeSpecifier &tsToAdd)
{
    if (!typeDesc)
    {
        typeDesc = tsToAdd.typeDesc;

        // See similar case in add(Specifier).
        if (isISR && !typeDesc->isInterruptServiceRoutine())
            typeDesc = TranslationUnit::getTypeManager().getInterruptType(typeDesc);
        if (receivesFirstParamInReg && !typeDesc->isFunctionReceivingFirstParamInReg())
            typeDesc = TranslationUnit::getTypeManager().getFPIRType(typeDesc);

        enumTypeName = tsToAdd.enumTypeName;
        assert(!enumeratorList);
        enumeratorList = tsToAdd.enumeratorList;

        if (enumeratorList)
            TranslationUnit::getTypeManager().declareEnumerationList(tsToAdd.enumTypeName, *enumeratorList);

        return;
    }

    if (tsToAdd.typeDesc->type == SIZELESS_TYPE)  // if type in 'tsToAdd' is just 'signed' or 'unsigned', without a size
    {
        // Apply the signedness of that keyword to 'typeDesc', if the latter is an integral type.
        if (!typeDesc->isIntegral())
        {
            errormsg("signed and unsigned modifiers can only be applied to integral type");
            return;
        }
        if (enumeratorList)
        {
            errormsg("signed and unsigned modifiers cannot be applied to an enum");
            return;
        }
        typeDesc = TranslationUnit::getTypeManager().getIntType(typeDesc, tsToAdd.typeDesc->isSigned);
        return;
    }

    if (typeDesc != tsToAdd.typeDesc)
        errormsg("combining type specifiers is not supported");
}


void
DeclarationSpecifierList::add(Specifier specifier)
{
    switch (specifier)
    {
    case TYPEDEF_SPEC:
        isTypeDef = true;
        break;
    case INTERRUPT_SPEC:
        isISR = true;

        // If we already know the type, convert it to an interrupt type.
        // This case here is needed when the program says "interrupt int".
        // When the program says "int interrupt", then 'typeDesc' is null here
        // and add(const TypeSpecifier &) handles that case.
        //
        if (typeDesc)
            typeDesc = TranslationUnit::getTypeManager().getInterruptType(typeDesc);
        break;
    case FUNC_RECEIVES_FIRST_PARAM_IN_REG_SPEC:
        receivesFirstParamInReg = true;

        if (typeDesc)
            typeDesc = TranslationUnit::getTypeManager().getFPIRType(typeDesc);
        break;
    case ASSEMBLY_ONLY_SPEC:
        asmOnly = true;
        break;
    case NO_RETURN_INSTRUCTION:
        noReturnInstruction = true;
        break;
    case EXTERN_SPEC:
        isExtern = true;
        break;
    case STATIC_SPEC:
        isStatic = true;
        break;
    case CONST_QUALIFIER:
        isConst = true;
        break;
    case VOLATILE_QUALIFIER:
        isVolatile = true;
        break;
    default:
        assert(!"specifier not handled");
    }
}


const TypeDesc *
DeclarationSpecifierList::getTypeDesc() const
{
    TypeManager &tm = TranslationUnit::getTypeManager();

    const TypeDesc *resultTD = NULL;
    if (!typeDesc)  // if no type_specifier given
        resultTD = tm.getIntType(WORD_TYPE, true);  // signed int is default type
    else if (typeDesc->type == SIZELESS_TYPE)  // if type described only with 'signed' or 'unsigned', it is an int
        resultTD =tm.getIntType(WORD_TYPE, typeDesc->isSigned);
    else
        resultTD = typeDesc;

    if (isConstant())
        resultTD = tm.getConst(resultTD);

    return resultTD;
}


bool
DeclarationSpecifierList::isInterruptServiceFunction() const
{
    return isISR;
}


bool
DeclarationSpecifierList::isFunctionReceivingFirstParamInReg() const
{
    return receivesFirstParamInReg;
}


bool
DeclarationSpecifierList::isAssemblyOnly() const
{
    return asmOnly;
}


bool
DeclarationSpecifierList::hasNoReturnInstruction() const
{
    return noReturnInstruction;
}


bool
DeclarationSpecifierList::isTypeDefinition() const
{
    return isTypeDef;
}


bool
DeclarationSpecifierList::isExternDeclaration() const
{
    return isExtern;
}


bool
DeclarationSpecifierList::isStaticDeclaration() const
{
    return isStatic;
}


bool
DeclarationSpecifierList::isConstant() const
{
    return isConst;
}


const string &
DeclarationSpecifierList::getEnumTypeName() const
{
    return enumTypeName;
}


bool
DeclarationSpecifierList::hasEnumeratorList() const
{
    return enumeratorList != NULL;
}


vector<Enumerator *> *
DeclarationSpecifierList::detachEnumeratorList()
{
    vector<Enumerator *> *ret = enumeratorList;
    enumeratorList = NULL;
    return ret;
}


bool
DeclarationSpecifierList::isModifierLegalOnVariable() const
{
    return !isISR && !receivesFirstParamInReg && !asmOnly && !noReturnInstruction;
}
