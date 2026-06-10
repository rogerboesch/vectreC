/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2025 Pierre Sarrazin <http://sarrazip.com/>

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

#ifndef _H_ClassDef
#define _H_ClassDef

#include "Tree.h"
#include "Declarator.h"

#include "util.h"

class FunctionDef;


class ClassDef : public Tree
{
public:

    class ClassMember : public Tree
    {
    public:
        // _di: Must come from new. Will be destroyed by this class's destructor.
        ClassMember(const TypeDesc *_tp, Declarator *_di);

        // Calls delete on Declarator given to constructor.
        ~ClassMember();

        std::string getName() const;

        // Computes and returns the total number of array elements, i.e., the product of all dimensions.
        // Example: For m[10][5], 50 will be returned.
        // Returns 1 for a non-array class member.
        // Returns size_t(-1) if the array size is invalid.
        //
        size_t getTotalNumArrayElements() const;

        // Returns the size in bytes occupied by this member.
        int16_t getSizeInBytes() const;

        // Returns the dimensions due to the declarator, e.g., 3 in 'int m[3]'.
        // If the type of this member is itself an array (e.g., typedef int A[4]; A m[3];)
        // then the caller may want to call getTypeDesc()->appendDimensions() to get the 4,
        // in this example.
        //
        std::vector<uint16_t> getArrayDimensions() const;

        bool isArray() const;

        const Declarator &getDeclarator() const;

        virtual bool isLValue() const override { return false; }

    private:
        Declarator *declarator;  // owned by this ClassMember

        // Forbidden:
        ClassMember(const ClassMember &);
        ClassMember &operator = (const ClassMember &);
    };

    ClassDef();

    virtual ~ClassDef();

    void setName(const std::string &_name);

    std::string getName() const;

    void setUnion(bool u);

    bool isUnion() const;

    // The ClassMember must have been allocated with 'new'.
    // It becomes owned by this ClassDef and will be destructed by ~ClassDef().
    //
    void addDataMember(ClassMember *m);

    // Returns the size in bytes occupied by an instance of this class.
    //
    int16_t getSizeInBytes() const;

    size_t getNumDataMembers() const { return dataMembers.size(); }

    // memberIndex: Zero-based index of the member in the struct.
    //              The indexes correspond to the order of the members' declaration in the struct.
    //
    const ClassMember *getDataMember(size_t memberIndex) const
    {
        if (memberIndex >= dataMembers.size())
            return NULL;
        return dataMembers[memberIndex];
    }

    const ClassMember *getDataMember(const std::string &memberName) const;

    // Returns a number of bytes, or -1 if the name is not found.
    //
    int16_t getDataMemberOffset(const std::string &memberName,
                                const ClassMember *&member) const;

    // Returns a number of bytes, or -1 if the index is not found.
    // member: If not null, *member receives the address of the ClassMember found, if any.
    //
    int16_t getDataMemberOffset(size_t memberIndex,
                                const ClassMember **member) const;

    void clearMembers();

    virtual bool isLValue() const override { return false; }

    static std::vector<ClassMember *> *createClassMembers(DeclarationSpecifierList *dsl,
                                                          std::vector<Declarator *> *memberDeclarators);

    // Issue errors and warnings, if any, about the validity of each member of this class,
    // including array sizes.
    //
    void check();

private:

    // Forbidden:
    ClassDef(const ClassDef &);
    ClassDef &operator = (const ClassDef &);

private:

    std::string name;
    std::vector<ClassMember *> dataMembers;  // owns the pointed objects
    bool Union;  // false means struct

};


#endif  /* _H_ClassDef */
