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

    enum Type { SINGLETON, ARRAY };

    // _id: Allowed to be empty, but only to call createFormalParameter().
    // _functionPointerLevel: Relevant for function pointers only.
    //                        Use 1 for T (*)(...), 2 for T (**)(...), etc.
    //
    Declarator(const std::string &_id,
               size_t _functionPointerLevel,
               const std::string &_srcFilename, int _lineno);

    // Calls delete on formalParamList, but that pointer will be null if
    // detachFormalParamList() was called.
    //
    ~Declarator();

    void setInitExpr(Tree *_initExpr);

    const Tree *getInitExpr() const;

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

    // This object becomes owner of the FormalParamList.
    // See also detachFormalParamList().
    //
    void setFormalParamList(FormalParamList *_formalParamList);

    const FormalParamList *getFormalParamList() const;

    // Ownership of the FormalParamList transferred to caller.
    // After this call, this Declarator does not have a FormalParamList anymore.
    //
    FormalParamList *detachFormalParamList();

    Declaration *declareVariable(const TypeDesc *varType, bool isStatic, bool isExtern);

    const std::string &getId() const;

    bool isFunctionPrototype() const;

    bool isFunctionPointer() const;

    bool isArrayOfFunctionPointers() const;

    size_t getFunctionPointerLevel() const;

    bool isArray() const { return type == ARRAY; }  // may be multi-dimensional

    bool getNumDimensions(size_t &numDimensions) const;

    // Returns the number of elements (over all the dimensions), not the number of bytes.
    // Returns size_t(-1) if the number is invalid.
    //
    // errorLocation: Tree on which to call errormsg() or warnmsg(), if needed, so that
    //                the right line number appears in the message. Allowed to be null.
    //
    size_t getTotalNumArrayElements(const Tree *errorLocation = NULL) const;

    // v: Must come from new. This Declarator takes ownership of the vector object and will delete it in its destructor.
    //
    void setPointerLevel(TypeQualifierBitFieldVector *v) { typeQualifierBitFieldVector = v; }

    size_t getPointerLevel() const { return typeQualifierBitFieldVector ? typeQualifierBitFieldVector->size() : 0; }

    // Takes ownership of the FormalParamList, which must come from new,
    // but the ownership MUST be transferred to another object before this
    // Declarator is destroyed.
    //
    void setAsFunctionPointer(FormalParamList *params);

    // Takes ownership of the FormalParamList, which must come from new,
    // but the ownership MUST be transferred to another object before this
    // Declarator is destroyed.
    // _subscripts: If not null, addArraySizeExpr() called with each element,
    //              and makes this declarator an array.
    //              MUST not be null and MUST come from new. This constructor destroys it.
    //
    void setAsArrayOfFunctionPointers(FormalParamList *params, TreeSequence *_subscripts);

    const TypeDesc *processPointerLevel(const TypeDesc *td) const;

    // Upon success, returns an object allocated with 'new'.
    // Upon failure, returns NULL.
    //
    FormalParameter *createFormalParameter(DeclarationSpecifierList &dsl) const;

    const std::string &getSourceFilename() const { return srcFilename; }

    int getLineNo() const { return lineno; }

    // Only call this version for arrays.
    // Upon success, returns the size of each dimension of the array.
    // Does not return any dimension for a non-array declaration.
    //
    // Displays an error message upon failure.
    //
    // arrayDimensions: NOT cleared before elements are appended, if any.
    // allowUnknownFirstDimension: if true, this unknown 1st dimension is assumed to be 1.
    //
    // errorLocation: Tree on which to call errormsg() or warnmsg(), if needed, so that
    //                the right line number appears in the message. Allowed to be null.
    //
    static bool computeArrayDimensions(std::vector<uint16_t> &arrayDimensions,
                                       bool allowUnknownFirstDimension,
                                       const std::vector<Tree *> &arraySizeExprList,
                                       const std::string &id,
                                       const Tree *initExpr,
                                       const Tree *errorLocation);

    // May also be called for non-arrays (does nothing and succeeds).
    bool computeArrayDimensions(std::vector<uint16_t> &arrayDimensions,
                                bool allowUnknownFirstDimension,
                                const Tree *errorLocation) const
    {
        if (type != ARRAY)
            return true;
        return computeArrayDimensions(arrayDimensions, allowUnknownFirstDimension,
                                      arraySizeExprList, id, initExpr, errorLocation);
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

    // Issues an error if the bit field is invalid.
    // typeDesc is allowed to be null.
    //
    void checkBitField(const TypeDesc *typeDesc) const;

    // Calls setFormalParamList() on this object after building a new FormalParamList
    // from the given parameters.
    // Must only be called on a declarator for a K&R-defined function.
    // paramNameList: The function's parameter names in the order they appear in the parenthesis.
    //                This order will be used by the new FormalParamList.
    // krFormalParamList: The type declarations of the K&R-defined function.
    //                    The order is not significant.
    //                    Must not be null.
    //                    If this list contains a declaration for a parameter that
    //                    is not named in paramNameList, an error message is issued.
    //                    If this list does not contain a declaration for a parameter
    //                    that is named in paramNameList, that parameter is assumed
    //                    to be of type 'int'.
    //                    Operator delete gets called on krFormalParamList.
    //
    void processKAndRFunctionParameters(const std::vector<std::string> &paramNameList,
                                        FormalParamList *krFormalParamList);

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
    size_t functionPointerLevel;
    int bitFieldWidth;  // if >= 0, bit field width in bits, otherwise, BitFieldCode value
};


#endif  /* _H_Declarator */
