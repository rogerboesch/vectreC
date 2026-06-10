/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2026 Pierre Sarrazin <http://sarrazip.com/>

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


int16_t
ClassDef::getDataMemberOffset(size_t memberIndex,
                              const ClassMember **member) const
{
    int16_t offset = 0;
    if (member)
        *member = NULL;
    for (vector<ClassMember *>::const_iterator it = dataMembers.begin();
                                              it != dataMembers.end(); it++)
    {
        if (memberIndex == 0)
        {
            if (member)
                *member = *it;
            return Union ? 0 : offset;
        }
        offset += (*it)->getSizeInBytes();
        --memberIndex;
    }
    return -1;
}


void
ClassDef::clearMembers()
{
    deleteVectorElements(dataMembers);
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
        declarator->checkBitField(dsl->getTypeDesc());

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
                                                                          dsl->getCallConvention());
        else if (dsl->getTypeDesc()->type == VOID_TYPE && declarator->getPointerLevel() == 0)
            ::errormsg("member of struct is void");

        ClassMember *member = new ClassDef::ClassMember(td, declarator);  // Declarator now owned by 'member'
        members->push_back(member);
    }

    delete dsl;
    delete memberDeclarators;  // destroy the vector<Declarator *>, but not the Declarators
    return members;
}


void
ClassDef::check()
{
    for (const ClassMember *member : dataMembers)
        if (member->isArray())
        {
            size_t numElements = member->getTotalNumArrayElements();
            if (numElements == size_t(-1))
                member->errormsg("invalid dimensions for member array `%s' in struct `%s'",
                                member->getDeclarator().getId().c_str(), getName().c_str());
            else if (numElements == 0
                        && TranslationUnit::instance().warnArraySizeZero())
                member->warnmsg("array member `%s' of struct `%s' has size zero",
                                    member->getName().c_str(), getName().c_str());
        }
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
    else if (getTypeDesc()->isPtrToFunction())
    {
        // If a call conv. has been pushed with push_calling_convention,
        // apply it to this struct member of type function pointer.
        //
        CallConvention defaultConv = TranslationUnit::instance().getCurrentDefaultCallConvention();
        if (defaultConv != DEFAULT_CMOC_CALL_CONV)
        {
            const TypeDesc *funcTypeDesc = getTypeDesc()->getFinalPointerType(); // function type pointed to
            if (funcTypeDesc->getCallConvention() == DEFAULT_CMOC_CALL_CONV)  // if no explicit call conv. keyword on struct member
            {
                // Construct the same function pointer type, but with the current default call conv. on it.
                TypeManager &tm = TranslationUnit::instance().getTypeManager();
                const TypeDesc *newFuncTypeDesc = tm.getTypeWithCallConvention(funcTypeDesc, defaultConv);
                const TypeDesc *newFuncPtrTypeDesc = tm.getPointerTo(newFuncTypeDesc);
                setTypeDesc(newFuncPtrTypeDesc);
            }
        }
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


size_t
ClassDef::ClassMember::getTotalNumArrayElements() const
{
    assert(declarator != NULL);

    size_t numElements = 1;
    if (declarator->isArray())
    {
        numElements = declarator->getTotalNumArrayElements(this);
        if (numElements == size_t(-1))
            return size_t(-1);  // invalid size
    }
    size_t numElementsInType = getTypeDesc()->getTotalNumArrayElements();
    if (numElementsInType > 0x7FFF || numElements * numElementsInType > 0x7FFF)
        return size_t(-1);  // invalid size
    return numElements * numElementsInType;
}


int16_t
ClassDef::ClassMember::getSizeInBytes() const
{
    assert(declarator != NULL);

    const TypeDesc *td = getTypeDesc()->getFinalArrayType();
    size_t numElements = getTotalNumArrayElements();
    if (numElements == size_t(-1))  // if invalid
        return 0;
    return TranslationUnit::instance().getTypeSize(*td) * numElements;
}


std::vector<uint16_t>
ClassDef::ClassMember::getArrayDimensions() const
{
    assert(declarator != NULL);

    vector<uint16_t> arrayDimensions;
    if (!declarator->computeArrayDimensions(arrayDimensions, false, this))
        errormsg("failed to compute array dimensions of struct member `%s'", getName().c_str());

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
