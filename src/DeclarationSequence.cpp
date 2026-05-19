/*  $Id: DeclarationSequence.cpp,v 1.17 2019/06/22 03:35:44 sarrazip Exp $

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

#include "DeclarationSequence.h"
#include "TranslationUnit.h"
#include "SemanticsChecker.h"

using namespace std;


DeclarationSequence::DeclarationSequence(const TypeDesc *_typeDesc, std::vector<Enumerator *> *_enumeratorList)
:   TreeSequence(),
    enumeratorList(_enumeratorList)
{
    assert(_typeDesc);
    assert(_typeDesc->isValid());
    setTypeDesc(_typeDesc);
}


DeclarationSequence::~DeclarationSequence()
{
    delete enumeratorList;

    // We do not delete the Enumerators and the Trees b/c they are owned
    // by the TypeManager (see TypeManager::declareEnumerationList()).
}


void DeclarationSequence::processDeclarator(Declarator *declarator, const DeclarationSpecifierList &dsl)
{
    if (declarator == NULL)
        return;
    const TypeDesc *specificTypeDesc = getTypeDesc();

    // Apply asterisks specified in the Declarator.
    // For example: if the declaration is of type char **, then specificTypeDesc
    // currently is type "char", and declarator->getPointerLevel() == 2.
    // After the call to processPointerLevel(), specificTypeDesc will be "char **".
    //
    // If the declarator is a function pointer, specificTypeDesc will end up
    // representing the return type.
    //
    // If dsl says const, then we apply that before calling processPointerLevel().
    // Example: If the program declared "const int * const ptr;" then
    //          dsl.isConstant() is true to represent the 1st const,
    //          while declarator->typeQualifierBitFieldVector contains CONST_BIT to represent
    //          the asterisk and the 2nd const.
    //          specificTypeDesc starts as "int".
    //          We first call getConst() to go from "int" to "const int".
    //          Then we call processPointerLevel() to go to "const int * const".
    //
    if (dsl.isConstant())
        specificTypeDesc = TranslationUnit::getTypeManager().getConst(specificTypeDesc);
    specificTypeDesc = declarator->processPointerLevel(specificTypeDesc);

    /*cout << "# DeclarationSequence::processDeclarator: specificTypeDesc='" << *specificTypeDesc
         << "', declarator='" << declarator->getId() << "'"
         << (declarator->isFunctionPointer() ? " (func ptr)" : "")
         << (declarator->isArray() ? " (array)" : "")
         << ", at " << getLineNo() << "\n";*/

    if (!declarator->isFunctionPointer() && !declarator->isArray() && declarator->getFormalParamList() != NULL)  // if function prototype
    {
        FunctionDef *fd = new FunctionDef(dsl, *declarator);  // takes ownership of declarator's FormalParamList
        fd->setLineNo(declarator->getSourceFilename(), declarator->getLineNo());
        // Body of 'fd' is left null.
        addTree(fd);
    }
    else
    {
        if (dsl.isAssemblyOnly())
            errormsg("modifier `asm' cannot be used on declaration of variable `%s'", declarator->getId().c_str());
        if (dsl.hasNoReturnInstruction())
            errormsg("modifier `__norts__' cannot be used on declaration of variable `%s'", declarator->getId().c_str());

        const TypeDesc *td = NULL;
        if (declarator->isFunctionPointer() || declarator->isArrayOfFunctionPointers())
        {
            assert(declarator->getFormalParamList());
            td = TranslationUnit::getTypeManager().getFunctionPointerType(specificTypeDesc,
                                                                          *declarator->getFormalParamList(),
                                                                          dsl.isInterruptServiceFunction(),
                                                                          dsl.isFunctionReceivingFirstParamInReg());
        }
        else
        {
            if (dsl.isInterruptServiceFunction())
                errormsg("modifier `interrupt' used on declaration of variable `%s'", declarator->getId().c_str());
            td = specificTypeDesc;
        }

        //cout << "# DeclarationSequence::processDeclarator:   declaring '" << declarator->getId() << "' with type " << td->toString() << endl;

        Declaration *decl = declarator->declareVariable(td, dsl.isStaticDeclaration(), dsl.isExternDeclaration());

        if (decl)
            addTree(decl);
    }
    delete declarator;
}


void
DeclarationSequence::checkSemantics(Functor &f)
{
    if (!enumeratorList)
        return;

    const SemanticsChecker &sc = dynamic_cast<SemanticsChecker &>(f);
    if (sc.getCurrentFunctionDef() != NULL)
        errormsg("non-global enum not supported");
}
