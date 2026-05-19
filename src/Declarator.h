/*  $Id: Declarator.h,v 1.20 2018/05/23 03:34:12 sarrazip Exp $

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

#ifndef _H_Declarator
#define _H_Declarator

#include <string>

#include "util.h"

class Tree;
class Declaration;
class FormalParameter;
class DeclarationSpecifierList;


class Declarator
{
public:

    enum Type { SINGLETON, ARRAY, FUNCPTR };

    // _id: Allow to be empyt, but only to call createFormalParameter().
    // _subscripts: If not null, addArraySizeExpr() called with each element,
    //              and makes this declarator an array.
    //              If not null, MUST come from new. This constructor destroys it.
    //
    Declarator(const std::string &_id,
               const std::string &_srcFilename, int _lineno);

    // Does NOT call delete on formalParamList.
    //
    ~Declarator();

    void setInitExpr(Tree *_initExpr);

    void checkForFunctionReturningArray() const;

    // Adds a dimension to this declarator.
    // (Can be called more than once.)
    // _arraySizeExpr may be null: it means no size specified, as in v[].
    // However, only the first dimension can be null, as in v[][5][7].
    // Sets 'type' to ARRAY, even if _arraySizeExpr is null.
    //
    // NOTE: The Tree becomes owned by this object. It will be destroyed by ~Declarator().
    //
    void addArraySizeExpr(Tree *_arraySizeExpr);

    void setFormalParamList(FormalParamList *_formalParamList);

    const FormalParamList *getFormalParamList() const;

    // Ownership of the FormalParamList transferred to caller.
    // After this call, this Declarator does not have a FormalParamList anymore.
    //
    FormalParamList *detachFormalParamList();

    Declaration *declareVariable(const TypeDesc *varType, bool isStatic, bool isExtern);

    const std::string &getId() const;

    bool isFunctionPointer() const { return type == FUNCPTR; }

    bool isArrayOfFunctionPointers() const { return type == ARRAY && formalParamList != NULL; }

    bool isArray() const { return type == ARRAY; }  // may be multi-dimensional

    bool getNumDimensions(size_t &numDimensions) const;

    // Returns the number of elements, not the number of bytes.
    uint16_t getNumArrayElements() const;

    // v: Must come from new. This Declarator takes ownership of the vector object and will delete it in its destructor.
    //
    void setPointerLevel(TypeQualifierBitFieldVector *v) { typeQualifierBitFieldVector = v; }

    size_t getPointerLevel() const { return typeQualifierBitFieldVector ? typeQualifierBitFieldVector->size() : 0; }

    // Takes ownership of the FormalParamList, which must come from new,
    // but the ownership MUST be transferred to another object before this
    // Declarator is destroyed.
    //
    void setAsFunctionPointer(FormalParamList *params);

    void setAsArrayOfFunctionPointers(FormalParamList *params, TreeSequence *_subscripts);

    const TypeDesc *processPointerLevel(const TypeDesc *td) const;

    // Upon success, returns an object allocated with 'new'.
    // Upon failure, returns NULL.
    //
    FormalParameter *createFormalParameter(DeclarationSpecifierList &dsl) const;

    const std::string &getSourceFilename() const { return srcFilename; }

    int getLineNo() const { return lineno; }

    // Only call this version for arrays.
    static bool computeArrayDimensions(std::vector<uint16_t> &arrayDimensions,
                                       bool allowUnknownFirstDimension,
                                       const std::vector<Tree *> &arraySizeExprList,
                                       const std::string &id,
                                       const Tree *initExpr,
                                       const Tree *declarationTree);

    // May also be called for non-arrays (does nothing and succeeds).
    bool computeArrayDimensions(std::vector<uint16_t> &arrayDimensions,
                                bool allowUnknownFirstDimension,
                                const Tree *declarationTree) const
    {
        if (type != ARRAY)
            return true;
        return computeArrayDimensions(arrayDimensions, allowUnknownFirstDimension,
                                      arraySizeExprList, id, initExpr, declarationTree);
    }

    std::string toString() const;

    enum BitFieldCode
    {
        NOT_BIT_FIELD = -1,
        INVALID_WIDTH_EXPR = -2,  // not a constant expression
        NEGATIVE_WIDTH_EXPR = -3,
    };

    void setBitFieldWidth(Tree &bitFieldWidthExpr);

    int getBitFieldWidth() const { return bitFieldWidth; }

    void checkBitField(const TypeDesc &typeDesc) const;

private:

    // Forbidden:
    Declarator(const Declarator &);
    Declarator &operator = (const Declarator &);

private:
    std::string id;
    std::string srcFilename;
    int lineno;
    Tree *initExpr;
    std::vector<Tree *> arraySizeExprList;  // only useful if type == ARRAY is true; may still be null in that case (size then given by initExpr)
    FormalParamList *formalParamList;  // when not null, must come from new
    Type type;
    TypeQualifierBitFieldVector *typeQualifierBitFieldVector;  // defines pointer level; comes from new; owned by this Declarator; deleted by destructor
    int bitFieldWidth;  // if >= 0, bit field width in bits, otherwise, BitFieldCode value
};


#endif  /* _H_Declarator */
