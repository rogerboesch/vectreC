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

#include "CastExpr.h"

#include "TranslationUnit.h"
#include "VariableExpr.h"
#include "Declaration.h"
#include "WordConstantExpr.h"
#include "VariableExpr.h"
#include "IdentifierExpr.h"
#include "BinaryOpExpr.h"

#include <assert.h>

using namespace std;


// Keeps a copy of pointer 'e'.
//
CastExpr::CastExpr(const TypeDesc *td, Tree *e)
  : Tree(td),
    subExpr(e),
    resultDeclaration(NULL)
{
    assert(subExpr != NULL);
}


/*virtual*/
CastExpr::~CastExpr()
{
    delete resultDeclaration;
    delete subExpr;
}


const Tree *
CastExpr::getSubExpr() const
{
    assert(subExpr != NULL);
    return subExpr;
}


Tree *
CastExpr::getSubExpr()
{
    assert(subExpr != NULL);
    return subExpr;
}


/*virtual*/
void
CastExpr::checkSemantics(Functor &)
{
    if (isRealOrLong())
    {
        // The result of the expression must be stored in a temporary, because it
        // cannot be left in D or B.
        resultDeclaration = Declaration::declareHiddenVariableInCurrentScope(*this);
    }
}


bool
CastExpr::isZeroCastToVoidPointer(const Tree &tree)
{
    if (tree.getType() != POINTER_TYPE)
        return false;
    if (tree.getTypeDesc()->getPointedType() != VOID_TYPE)
        return false;
    const CastExpr *ce = dynamic_cast<const CastExpr *>(&tree);
    if (!ce)
        return false;
    const Tree *subExpr = ce->getSubExpr();
    const WordConstantExpr *wce = dynamic_cast<const WordConstantExpr *>(subExpr);
    if (!wce)
        return false;
    return wce->getWordValue() == 0;
}


// Emits code for this case: (short) (longValue >> 16). Result left in D.
// Returns true iff this function emitted code.
// Returns false if this tree does not fit the targeted case.
//
bool
CastExpr::emitCodeIfShiftingLongRight16Bits(ASMText &out, bool lValue) const
{
    if (lValue)
        return false;  // optimization not applied
    if (getType() != WORD_TYPE)
        return false;
    if (! subExpr->isLong())
        return false;
    auto binOp = dynamic_cast<const BinaryOpExpr *>(subExpr);
    if (! binOp || binOp->getOperator() != BinaryOpExpr::RIGHT_SHIFT)
        return false;
    uint16_t numBitsToShift = 0;
    if (! binOp->getRight()->evaluateConstantExpr(numBitsToShift))
        return false;
    if (numBitsToShift != 16)
        return false;

    out.emitComment("Shifting a long right by 16 bits into a word. Emitting expression to be shifted.");
    if (!binOp->getLeft()->emitCode(out, true))  // l-value, so that X will points to resulting long
        return false;

    out.ins("LDD", ",X", "shift long right by 16 bits by keeping high word");
    return true;  // optimization applied
}


/*virtual*/
CodeStatus
CastExpr::emitCode(ASMText &out, bool lValue) const
{
    assert(getType() != CLASS_TYPE || isNumerical());
    assert(subExpr != NULL);

    const TypeDesc *castTD = getTypeDesc();
    const TypeDesc *subTD = subExpr->getTypeDesc();

    // If casting to long from real, or to real from long.
    if ((isLong() && subExpr->isReal()) || (isReal() && subExpr->isLong()))
    {
        if (!subExpr->emitCode(out, true))  // get address of source number in X
            return false;
        assert(resultDeclaration);
        out.ins("TFR", "X,D", "cast to " + castTD->toString() + ": address of source number in D");
        out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0), "destination of cast");
        if (isLong())
            callUtility(out, "init" + string(isSigned() ? "Signed" : "Unsigned") + "DWordFrom" + (subTD->isSingle() ? "Single" : "Double"));
        else
            callUtility(out, "init" + string(isSingle() ? "Single" : "Double") + "From" + (subTD->isSigned ? "Signed" : "Unsigned") + "DWord");
        out.emitComment("Address of `" + castTD->toString() + "' is in X.");
        return true;
    }

    // If case like (short) (longValue >> 16).
    if (emitCodeIfShiftingLongRight16Bits(out, lValue))
        return true;

    // If casting to integral from real or long.
    if (isIntegral() && subExpr->isRealOrLong())
    {
        if (!subExpr->emitCode(out, true))  // get address of real/long in X
            return false;

        if (isLong() && subExpr->isLong())
            return true;  // done

        out.ins("TFR", "X,D", "cast to `" + castTD->toString() + "': address of source number in D");

        // Allocate stack space for the result.
        int16_t resultTypeSize = TranslationUnit::instance().getTypeSize(*castTD);
        assert(resultTypeSize == 1 || resultTypeSize == 2);
        out.ins("LEAS", "-" + wordToString(resultTypeSize) + ",S", "result of cast");
        out.ins("LEAX", ",S");

        // Call a utility routine of the form initSomethingFromSomething.
        assert(!castTD->isLong());
        if (subTD->isLong())
            callUtility(out, "init" + string(castTD->type == BYTE_TYPE ? "Byte" : "Word") + "FromDWord");
        else
            callUtility(out, "init" + string(castTD->isSigned ? "Signed" : "Unsigned")
                                    + (castTD->type == BYTE_TYPE ? "Byte" : "Word")
                                    + "From" + (subTD->isSingle() ? "Single" : "Double"));

        // Pop result into register.
        out.ins("PULS", resultTypeSize == 1 ? "B" : "A,B", "result of cast");
        return true;
    }

    // If casting to pointer from long.
    if (castTD->type == POINTER_TYPE && subExpr->isLong())
    {
        if (!subExpr->emitCode(out, true))  // get address of real/long in X
            return false;
        out.ins("LDD", "2,X", "cast `" + subTD->toString() + "' to `" + castTD->toString() + "'");
        return true;
    }

    // If casting to real or long from integral.
    if (isRealOrLong() && subExpr->isIntegral())
    {
        if (!subExpr->emitCode(out, false))  // load integral in B or D
            return false;
        if (subExpr->getType() == BYTE_TYPE)
            out.ins(subExpr->getConvToWordIns(), "", "promote to word");
        assert(resultDeclaration);
        out.ins("LEAX", resultDeclaration->getFrameDisplacementArg(0), "destination of cast");
        if (isLong())
            callUtility(out, "initDWordFrom" + string(subExpr->isLong() ? "" : (subExpr->isSigned() ? "Signed" : "Unsigned")) + "Word");
        else
            callUtility(out, "init" + string(isSingle() ? "Single" : "Double")
                                + "From" + (subExpr->isSigned() ? "Signed" : "Unsigned") + (subExpr->isLong() ? "DWord" : "Word"));
        return true;
    }

    if (!subExpr->emitCode(out, lValue))
        return false;

    return emitCastCode(out, castTD, subTD, *this);
}


CodeStatus
CastExpr::emitCastCode(ASMText &out, const TypeDesc *castTD, const TypeDesc *subTD, const Tree &tree)
{
    if (castTD->type == VOID_TYPE || subTD == castTD)
        return true;

    if (subTD->type == BYTE_TYPE && castTD->type != BYTE_TYPE)
    {
        // We are casting to a 2-byte type.
        assert(TranslationUnit::instance().getTypeSize(*castTD) == 2);

        const char *extendIns = subTD->getConvToWordIns();  // as in C
        out.ins(extendIns, "", "cast from byte");
        return true;
    }

    if (castTD->type == BYTE_TYPE)
    {
        out.emitComment("Cast from `" + subTD->toString() + "' to byte: result already in B");
        return true;
    }

    int16_t subSize = subTD->isArray() ? 2 : TranslationUnit::instance().getTypeSize(*subTD);

    if (TranslationUnit::instance().getTypeSize(*castTD) != subSize)
    {
        tree.errormsg("casting from `%s' to `%s' is not supported", subTD->toString().c_str(), castTD->toString().c_str());
        return true;
    }

    out.emitComment("Emitted no code to cast `" + subTD->toString() + "' to `" + castTD->toString() + "'");
    return true;
}


bool
CastExpr::iterate(Functor &f)
{
    if (!f.open(this))
        return false;
    if (!subExpr->iterate(f))
        return false;
    if (!f.close(this))
        return false;
    return true;
}
