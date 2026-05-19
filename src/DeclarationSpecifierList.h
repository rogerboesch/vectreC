/*  $Id: DeclarationSpecifierList.h,v 1.13 2019/06/22 03:35:44 sarrazip Exp $

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

#ifndef _H_DeclarationSpecifierList
#define _H_DeclarationSpecifierList

#include "util.h"


class DeclarationSpecifierList
{
public:

    enum Specifier
    {
        TYPEDEF_SPEC,
        INTERRUPT_SPEC,
        FUNC_RECEIVES_FIRST_PARAM_IN_REG_SPEC,
        ASSEMBLY_ONLY_SPEC,
        EXTERN_SPEC,
        STATIC_SPEC,
        NO_RETURN_INSTRUCTION,
        CONST_QUALIFIER,
        VOLATILE_QUALIFIER,
    };

    DeclarationSpecifierList();

    ~DeclarationSpecifierList();

    void add(const TypeSpecifier &ts);

    void add(Specifier specifier);

    const TypeDesc *getTypeDesc() const;

    bool isInterruptServiceFunction() const;

    bool isFunctionReceivingFirstParamInReg() const;

    bool isAssemblyOnly() const;

    bool hasNoReturnInstruction() const;

    bool isTypeDefinition() const;

    bool isExternDeclaration() const;

    bool isStaticDeclaration() const;

    bool isConstant() const;

    const std::string &getEnumTypeName() const;

    bool hasEnumeratorList() const;

    std::vector<Enumerator *> *detachEnumeratorList();

    bool isModifierLegalOnVariable() const;

private:

    DeclarationSpecifierList(const DeclarationSpecifierList &);
    DeclarationSpecifierList &operator = (const DeclarationSpecifierList &);

private:

    const TypeDesc *typeDesc;  // not owned by this class
    bool isTypeDef;
    bool isISR;
    bool receivesFirstParamInReg;
    bool asmOnly;
    bool noReturnInstruction;  // when true, no RTS/RTI emitted at end of asm-only function
    bool isExtern;
    bool isStatic;
    bool isConst;
    bool isVolatile;
    std::string enumTypeName;  // empty if type is not a named enum
    std::vector<Enumerator *> *enumeratorList;

};


#endif  /* _H_DeclarationSpecifierList */
