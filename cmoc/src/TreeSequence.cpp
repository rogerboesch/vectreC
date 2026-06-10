/*  $Id: TreeSequence.cpp,v 1.20 2025/08/30 02:20:34 sarrazip Exp $

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

#include "TreeSequence.h"

#include "TranslationUnit.h"
#include "CastExpr.h"
#include "IdentifierExpr.h"
#include "StringLiteralExpr.h"
#include "CommaExpr.h"

using namespace std;


TreeSequence::TreeSequence()
:   Tree(),
    sequence(),
    requiredNumArrayElements(0)
{
}


/*virtual*/
TreeSequence::~TreeSequence()
{
    deleteVectorElements(sequence);
}


void
TreeSequence::addTree(Tree *tree)
{
    sequence.push_back(tree);
}


size_t
TreeSequence::size() const
{
    return sequence.size();
}


vector<Tree *>::const_iterator
TreeSequence::begin() const
{
    return sequence.begin();
}


vector<Tree *>::iterator
TreeSequence::begin()
{
    return sequence.begin();
}


vector<Tree *>::const_iterator
TreeSequence::end() const
{
    return sequence.end();
}


vector<Tree *>::iterator
TreeSequence::end()
{
    return sequence.end();
}


vector<Tree *>::const_reverse_iterator
TreeSequence::rbegin() const
{
    return sequence.rbegin();
}


vector<Tree *>::reverse_iterator
TreeSequence::rbegin()
{
    return sequence.rbegin();
}


vector<Tree *>::const_reverse_iterator
TreeSequence::rend() const
{
    return sequence.rend();
}


vector<Tree *>::reverse_iterator
TreeSequence::rend()
{
    return sequence.rend();
}


const Tree *
TreeSequence::getTree(size_t index) const
{
    if (index >= sequence.size())
        return NULL;
    return sequence[index];
}


Tree *
TreeSequence::getTree(size_t index)
{
    return const_cast<Tree *>(static_cast<const TreeSequence *>(this)->getTree(index));
}


void
TreeSequence::clear()
{
    sequence.clear();
}


/*virtual*/
CodeStatus
TreeSequence::emitCode(ASMText &out, bool lValue) const
{
    const bool isCommaExpr = !!dynamic_cast<const CommaExpr *>(this);

    pushScopeIfExists();

    bool success = true;
    for (vector<Tree *>::const_iterator it = sequence.begin(); it != sequence.end(); it++)
    {
        const Tree *tree = *it;

        // Do not emit anything if 'tree' is a constant or a variable name
        // cast to some type (e.g., (void) 0 or (void) n).
        //
        // or (void) variable.
        if (const CastExpr *castExpr = dynamic_cast<const CastExpr *>(tree))
        {
            const Tree *subExpr = castExpr->getSubExpr();
            if (dynamic_cast<const IdentifierExpr *>(subExpr))
                continue;
            uint16_t result = 0;
            if (subExpr->evaluateConstantExpr(result))
                continue;
        }

        // Emit the tree as an r-value, unless it is:
        // - a struct (including longs and reals), then it has to be emitted as an l-value;
        // - the last sub-expression of a comma expression, and the caller wants an l-value;
        //   this supports a statement like (a = 1, b = 2) = 3, which ends by putting 3 in b.
        //
        bool emitAsLValue = false;
        if (tree->getType() == CLASS_TYPE)
            emitAsLValue = true;
        else if (lValue && isCommaExpr && it + 1 == sequence.end())
            emitAsLValue = true;
        if (!tree->emitCode(out, emitAsLValue))
        {
            success = false;
            break;  // go pop the scope before returning
        }
    }

    popScopeIfExists();

    return success;
}


bool
TreeSequence::iterate(Functor &f)
{
    if (!f.open(this))
        return false;

    for (vector<Tree *>::iterator it = sequence.begin();
                                    it != sequence.end();
                                    it++)
        if (!(*it)->iterate(f))
            return false;

    if (!f.close(this))
        return false;
    return true;
}


void
TreeSequence::replaceChild(Tree *existingChild, Tree *newChild)
{
    for (vector<Tree *>::iterator it = sequence.begin(); it != sequence.end(); it++)
        if (deleteAndAssign(*it, existingChild, newChild))
            return;
    assert(!"child not found");
}


void
TreeSequence::detachChild(const Tree *existingChild)
{
    sequence.erase(std::remove(sequence.begin(), sequence.end(), existingChild), sequence.end());
}


string
TreeSequence::toString() const
{
    stringstream ss;
    for (vector<Tree *>::const_iterator it = begin(); it != end(); ++it)
    {
        if (it != begin())
            ss << ", ";
        ss << (*it)->getTypeDesc()->toString();
    }
    return ss.str();
}


// Recursive check.
// Returns false if no string literal is found at any level.
//
bool
TreeSequence::isTreeSequenceWithOnlyStringLiterals() const
{
    bool atLeastOne = false;

    for (vector<Tree *>::const_iterator it = begin(); it != end(); ++it)
    {
        const Tree *tree = *it;

        if (const TreeSequence *subSeq = dynamic_cast<const TreeSequence *>(tree))
        {
            if (!subSeq->isTreeSequenceWithOnlyStringLiterals())
                return false;
            atLeastOne = true;
            continue;
        }

        if (dynamic_cast<const StringLiteralExpr *>(tree) == NULL)
            return false;

        atLeastOne = true;
    }

    return atLeastOne;
}


bool
TreeSequence::isTreeSequenceWithOnlyNumericalLiterals() const
{
    for (vector<Tree *>::const_iterator it = begin(); it != end(); ++it)
    {
        const Tree *tree = *it;

        if (const TreeSequence *subSeq = dynamic_cast<const TreeSequence *>(tree))
        {
            if (!subSeq->isTreeSequenceWithOnlyNumericalLiterals())
                return false;
            continue;
        }

        if (!tree->isNumericalLiteral())
            return false;
    }

    return true;
}


void
TreeSequence::setRequiredNumArrayElements(uint16_t _requiredNumArrayElements)
{
    requiredNumArrayElements = _requiredNumArrayElements;
}


uint16_t
TreeSequence::getRequiredNumArrayElements() const
{
    return requiredNumArrayElements;
}

