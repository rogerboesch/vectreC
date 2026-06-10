/*  $Id: TreeSequence.h,v 1.17 2025/08/30 02:20:34 sarrazip Exp $

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

#ifndef _H_TreeSequence
#define _H_TreeSequence

#include "Tree.h"


class TreeSequence : public Tree
{
public:

    TreeSequence();

    // Calls delete on each Tree pointer passed to addTree() that is still
    // part of this sequence.
    //
    virtual ~TreeSequence();

    // tree: Allowed to be null. Otherwise, must come from new.
    //
    void addTree(Tree *tree);

    size_t size() const;
    std::vector<Tree *>::const_iterator begin() const;
    std::vector<Tree *>::iterator begin();
    std::vector<Tree *>::const_iterator end() const;
    std::vector<Tree *>::iterator end();
    std::vector<Tree *>::const_reverse_iterator rbegin() const;
    std::vector<Tree *>::reverse_iterator rbegin();
    std::vector<Tree *>::const_reverse_iterator rend() const;
    std::vector<Tree *>::reverse_iterator rend();

    // index: Zero-based index in this sequence.
    // Returns NULL if 'index' is too large.
    //
    const Tree *getTree(size_t index) const;
    Tree *getTree(size_t index);

    // Does NOT call delete on the Tree pointers that may be contained in this sequence.
    //
    void clear();

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const override;

    virtual bool iterate(Functor &f) override;

    // Searches for 'existingChild' in this sequence, calls delete on it,
    // then puts 'newChild' in its place in this sequence.
    // existingChild MUST be in this sequence.
    //
    virtual void replaceChild(Tree *existingChild, Tree *newChild) override;

    // Removes the given pointer from the list of Tree pointers of this sequence.
    // Does not destroy *existingChild.
    // Does nothing if existingChild is not found in this sequence.
    //
    void detachChild(const Tree *existingChild);

    virtual bool isLValue() const override { return false; }

    std::string toString() const;

    bool isTreeSequenceWithOnlyStringLiterals() const;

    bool isTreeSequenceWithOnlyNumericalLiterals() const;

    void setRequiredNumArrayElements(uint16_t requiredNumArrayElements);

    uint16_t getRequiredNumArrayElements() const;

private:
    std::vector<Tree *> sequence;  // owns the Tree objects
    uint16_t requiredNumArrayElements;
};


#endif  /* _H_TreeSequence */
