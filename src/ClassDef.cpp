/*  $Id: ClassDef.cpp,v 1.21 2019/06/22 03:35:43 sarrazip Exp $

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

#include "ClassDef.h"

#include "FunctionDef.h"
#include "TranslationUnit.h"
#include "DeclarationSpecifierList.h"

using namespace std;


ClassDef::ClassDef()
  : Tree(),
    name(),
    dataMembers(),
    Union(false)
{
}


/*virtual*/
ClassDef::~ClassDef()
{
    clearMembers();
}


void
ClassDef::setName(const string &_name)
{
    name = _name;
}


string
ClassDef::getName() const
{
    return name;
}


void
ClassDef::setUnion(bool u)
{
    Union = u;
}


bool
ClassDef::isUnion() const
{
    return Union;
}


void
ClassDef::addDataMember(ClassMember *m)
{
    if (m != NULL)
        dataMembers.push_back(m);
}


int16_t
ClassDef::getSizeInBytes() const
{
    int16_t aggregateSize = 0;
    for (vector<ClassMember *>::const_iterator it = dataMembers.begin();
                                              it != dataMembers.end(); it++)
    {
        int16_t memberSize = (*it)->getSizeInBytes();
        if (Union)
            aggregateSize = max(aggregateSize, memberSize);
        else
            aggregateSize += memberSize;
    }
    return aggregateSize;
}


const ClassDef::ClassMember *
ClassDef::getDataMember(const string &memberName) const
{
    for (vector<ClassMember *>::const_iterator it = dataMembers.begin();
                                              it != dataMembers.end(); it++)
        if ((*it)->getName() == memberName)
            return *it;
    return NULL;
}


int16_t
ClassDef::getDataMemberOffset(const string &memberName,
                              const ClassMember *&member) const
{
    int16_t offset = 0;
    member = NULL;
    for (vector<ClassMember *>::const_iterator it = dataMembers.begin();
                                              it != dataMembers.end(); it++)
    {
        if ((*it)->getName() == memberName)
        {
            member = *it;
            return Union ? 0 : offset;
        }
        offset += (*it)->getSizeInBytes();
    }
    return -1;
}


void
ClassDef::clearMembers()
{
    for (std::vector<ClassMember *>::iterator it = dataMembers.begin(); it != dataMembers.end(); ++it)
        delete *it;
    dataMembers.clear();
}


// See DeclarationSequence::processDeclarator() for a similar treatment of declarators.
//
// Destroys 'dsl', which must come from 'new'.
//
std::vector<ClassDef::ClassMember *> *
ClassDef::createClassMembers(DeclarationSpecifierList *dsl,
                             std::vector<Declarator *> *memberDeclarators)
{
    assert(dsl);
    assert(memberDeclarators);

    // Return a tree sequence of ClassMembers defined by struct_declarator_list.
    std::vector<ClassMember *> *members = new std::vector<ClassMember *>();
    for (std::vector<Declarator *>::iterator it = memberDeclarators->begin(); it != memberDeclarators->end(); ++it)
    {
        Declarator *declarator = *it;
        assert(declarator);

        // Check bit field widths and types.
        declarator->checkBitField(*dsl->getTypeDesc());

        // Apply asterisks specified in the Declarator.
        // For example: if the declaration is of type char **, then dsl->getTypeDesc()
        // is type "char", and declarator->pointerLevel == 2.
        // After the call to processPointerLevel(), td will be "char **".
        //
        // The 'const' keywords that may appear in the member type declaration
        // are processed by DeclarationSpecifierList::getTypeDesc() and/or by
        // Declarator::processPointerLevel().
        //
        const TypeDesc *td = declarator->processPointerLevel(dsl->getTypeDesc());
        if (declarator->isFunctionPointer() || declarator->isArrayOfFunctionPointers())
            td = TranslationUnit::getTypeManager().getFunctionPointerType(td,
                                                                          *declarator->getFormalParamList(),
                                                                          dsl->isInterruptServiceFunction(),
                                                                          dsl->isFunctionReceivingFirstParamInReg());

        ClassMember *member = new ClassDef::ClassMember(td, declarator);  // Declarator now owned by 'member'
        members->push_back(member);
    }

    delete dsl;
    delete memberDeclarators;  // destroy the vector<Declarator *>, but not the Declarators
    return members;
}


///////////////////////////////////////////////////////////////////////////////


ClassDef::ClassMember::ClassMember(const TypeDesc *_tp, Declarator *_di)
  : Tree(_tp), declarator(_di)
{
    assert(declarator != NULL);
    if (isArray())
    {
        // If this member is 'char a[4]' for example, then _tp is "char".
        // We adjust the member type to char array.
        //
        size_t numDims = 1;
        if (!_di->getNumDimensions(numDims))
        {
            assert(!"failed to compute number of array dimensions");
            numDims = 1;  // try to survive
        }

        // Note: Getting numDims == 0 here is normal in a case like "typedef A int[5]; A n;"
        // where the declarator of array 'n' (_di) has no dimensions and the array dimensions
        // are specified by the type of 'A' (_tp).
        // Passing 0 to getArrayOf() is fine because it will just return the type of 'A',
        // i.e., int[5] in this example.
        // In a case like "int m[6]", numDims will be 1 and getTypeDesc() will be 'int'.

        setTypeDesc(TranslationUnit::getTypeManager().getArrayOf(getTypeDesc(), numDims));
    }
}


ClassDef::ClassMember::~ClassMember()
{
    assert(declarator != NULL);
    delete declarator;
}


std::string
ClassDef::ClassMember::getName() const
{
    assert(declarator != NULL);
    return declarator->getId();
}


// Returns 1 for a non-array class member.
//
int16_t
ClassDef::ClassMember::getNumArrayElements() const
{
    assert(declarator != NULL);

    uint16_t numElements = 1;
    if (declarator->isArray())
        numElements = declarator->getNumArrayElements();
    size_t numElementsInType = getTypeDesc()->getNumArrayElements();
    return int16_t(numElements > 0 ? numElements * numElementsInType : 1);
}


int16_t
ClassDef::ClassMember::getSizeInBytes() const
{
    assert(declarator != NULL);

    // If array, then get the final array type (e.g., the "int" in "int[2][3]").
    const TypeDesc *td = getTypeDesc();
    while (td->type == ARRAY_TYPE)
    {
        td = td->pointedTypeDesc;
        assert(td);
    }

    return TranslationUnit::instance().getTypeSize(*td) * getNumArrayElements();
}


std::vector<uint16_t>
ClassDef::ClassMember::getArrayDimensions() const
{
    assert(declarator != NULL);

    vector<uint16_t> arrayDimensions;
    if (!declarator->computeArrayDimensions(arrayDimensions, false, this))
        assert(false);
    return arrayDimensions;
}


bool
ClassDef::ClassMember::isArray() const
{
    assert(declarator != NULL);
    return getTypeDesc()->isArray() || declarator->isArray();
}


const Declarator &
ClassDef::ClassMember::getDeclarator() const
{
    return *declarator;
}
