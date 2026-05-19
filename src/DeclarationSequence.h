/*  $Id: DeclarationSequence.h,v 1.8 2020/06/06 04:41:43 sarrazip Exp $

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

#ifndef _H_DeclarationSequence
#define _H_DeclarationSequence

#include "TreeSequence.h"
#include "Declarator.h"
#include "Declaration.h"
#include "FunctionDef.h"
#include "DeclarationSpecifierList.h"


class DeclarationSequence : public TreeSequence
{
public:

    // Keeps a pointer to _enumeratorList, if any.
    // This is used by checkSemantics() to detect function-local enum definitions,
    // which are not supported.
    //
    DeclarationSequence(const TypeDesc *_typeDesc, std::vector<Enumerator *> *_enumeratorList = NULL);

    virtual ~DeclarationSequence();

    // Adds a Declaration object to this sequence.
    // This Declaration object is built from this sequence's TypeDesc
    // and from 'declarator'.
    //
    // Finishes by calling delete on 'declarator'.
    //
    void processDeclarator(Declarator *declarator, const DeclarationSpecifierList &dsl);

    virtual void checkSemantics(Functor &f);

    void removeEnumeratorList() { enumeratorList = NULL; }

    // May be null.
    //
    const std::vector<Enumerator *> *getEnumeratorList() const { return enumeratorList; }
    std::vector<Enumerator *> *getEnumeratorList() { return enumeratorList; }

private:

    DeclarationSequence(const DeclarationSequence &);
    DeclarationSequence &operator = (const DeclarationSequence &);

private:

    std::vector<Enumerator *> *enumeratorList;  // owns the vector<>, but not the Enumerator objects

};


#endif  /* _H_DeclarationSequence */
