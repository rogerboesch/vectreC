/*  $Id: Declaration.h,v 1.29 2019/08/17 19:52:21 sarrazip Exp $

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

#ifndef _H_Declaration
#define _H_Declaration

#include "Tree.h"

class IdentifierExpr;


class Declaration : public Tree
{
public:

    // _arrayDimensions: empty means not an array.
    //
    Declaration(const std::string &id,
                const TypeDesc *td,
                const std::vector<uint16_t> &_arrayDimensions,
                bool isStatic, bool isExtern);

    // Builds a Declaration object partially. The work is finished by DeclarationFinisher.
    // To be used during parsing, when it is too soon to completely initialize a Declaration.
    // varTypeDesc: Type of the variable, as opposed to the type of the declarator.
    // Example: with "int a[3]", varTypeDesc is 'int'; the array part is in the declarator.
    //
    Declaration(const std::string &id,
                const TypeDesc *varTypeDesc,
                const std::vector<Tree *> &arraySizeExprList,
                bool isStatic, bool isExtern);

    virtual ~Declaration();

    void setInitExpr(Tree *initExpr);

    std::string getVariableId() const;

    // Example: int v[], without an initializer, is an incomplete type.
    //
    bool isCompleteType() const;

    bool getVariableSizeInBytes(uint16_t &sizeInBytes, bool skipFirstDimensionIfArray = false) const;

    const std::vector<uint16_t> &getArrayDimensions() const;

    // Once a function's stack frame has been set up:
    //   0,U points to the saved stack frame pointer;
    //   2,U points to the return address;
    // so negative offsets on U are local variables
    // and offsets of 4 or more are function parameters.
    // Note that 0,U can refer to a local variable if it is of an empty struct.
    //
    enum { FIRST_FUNC_PARAM_FRAME_DISPLACEMENT = 2 + 2 };

    void setFrameDisplacement(int16_t disp);
    int16_t getFrameDisplacement(int16_t offset = 0) const;
    std::string getFrameDisplacementArg(int16_t offset = 0) const;
    bool hasFunctionParameterFrameDisplacement() const;
    bool hasLocalVariableFrameDisplacement() const;
    Tree *getInitExpr();

    void setGlobal(bool g);
    bool isGlobal() const;
    bool isArray() const;
    void setReadOnly(bool ro);  // only used to place global variable in separate section
    bool isReadOnly() const;

    // Returns true only if this object declares an array that has
    // an initializer that only contains integer values, and no
    // string literals.
    //
    bool isArrayWithOnlyNumericalLiteralInitValues() const;

    // Returns true only if this object declares a struct or union that has
    // an initializer that only contains integer values, and no
    // string literals.
    //
    bool isStructWithOnlyNumericalLiteralInitValues() const;

    bool hasOnlyNumericalLiteralInitValues() const;

    // Sets the assembly language label to be used to represent
    // the starting address of this variable (useful only with global declarations).
    //
    void setLabel(const std::string &_label);

    void setLabelFromVariableId();

    std::string getLabel() const;

    // Emits FCB or FDB directives, but only if isArrayWithIntegerInitValues()
    // is true. Fails otherwise and writes nothing.
    // Returns true on success, false on failure.
    //
    CodeStatus emitStaticArrayInitializer(ASMText &out);

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    virtual void replaceChild(Tree *existingChild, Tree *newChild)
    {
        if (deleteAndAssign(initializationExpr, existingChild, newChild))
            return;
        assert(!"child not found");
    }

    CodeStatus emitStaticValues(ASMText &out, Tree *arrayElementInitializer, const TypeDesc *requiredTypeDesc);

    virtual bool isLValue() const { return false; }

    // Creates a Declaration and puts it in the current scope.
    // The TypeDesc of the Declaration will 'typeDesc' unless it is null,
    // then it will be that of 'parentExpression'.
    // The caller is responsible for deleting the returned object.
    //
    static Declaration *declareHiddenVariableInCurrentScope(const Tree &parentExpression,
                                                            const TypeDesc *typeDesc = NULL);

public:

    std::string variableId;
    int16_t frameDisplacement;   // displacement from stack frame
    std::vector<uint16_t> arrayDimensions;   // empty means non-array; { a, b, c } meants T[a][b][c]
    Tree *initializationExpr;    // owns the pointed object
    std::string label;           // useful only with global declarations
    bool global;                 // true iff declaration is global
    bool readOnly;               // if true, can be put in ROM
    bool isStatic;               // if true, the 'static' keyword was used on this declaration
    bool isExtern;
    bool needsFinish;    // true means init to be completed by DeclarationFinisher after parsing done
    std::vector<Tree *> arraySizeExprList;  // used by DeclarationFinisher; Tree objects owned by this Declaration

private:
    // Forbidden:
    Declaration(const Declaration &);
    Declaration &operator = (const Declaration &);

private:

    void init(const std::string &id, uint16_t _numArrayElements = 0);
    static void checkInitExpr(Tree *initializationExpr, const TypeDesc *varTypeDesc, const std::string &variableId, const std::vector<uint16_t> &arrayDimensions, size_t dimIndex);
    static void checkArrayInitializer(Tree *initializationExpr, const TypeDesc *varTypeDesc, const std::string &variableId, const std::vector<uint16_t> &arrayDimensions, size_t dimIndex);
    static void checkClassInitializer(Tree *initializationExpr, const TypeDesc *varTypeDesc, const std::string &variableId);
    static bool isRealOrLongInitWithNumber(const TypeDesc *varTypeDesc, const Tree &initializationExpr);
    static bool isTreeSequenceWithOnlyNumericalLiterals(const TreeSequence *seq);
    CodeStatus emitSequenceInitCode(ASMText &out, const Tree *initializer, const TypeDesc *requiredTypeDesc, int16_t arraySizeInBytes, uint16_t& writingOffset) const;
    static bool emitArrayAddress(ASMText &out, const IdentifierExpr &ie, const TypeDesc &requiredTypeDesc);

};


#endif  /* _H_Declaration */
