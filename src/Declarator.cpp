/*  $Id: Declarator.cpp,v 1.38 2020/04/04 17:41:44 sarrazip Exp $

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

#include "Declarator.h"

#include "Declaration.h"
#include "TreeSequence.h"
#include "StringLiteralExpr.h"
#include "ExpressionTypeSetter.h"
#include "TranslationUnit.h"
#include "FormalParameter.h"
#include "FormalParamList.h"
#include "DeclarationSpecifierList.h"
#include "TreeSequence.h"

using namespace std;


Declarator::Declarator(const std::string &_id,
                       const std::string &_srcFilename, int _lineno)
  : id(_id),
    srcFilename(_srcFilename),
    lineno(_lineno),
    initExpr(NULL),
    arraySizeExprList(),
    formalParamList(NULL),
    type(SINGLETON),
    typeQualifierBitFieldVector(NULL),
    bitFieldWidth(NOT_BIT_FIELD)
{
}


Declarator::~Declarator()
{
    delete typeQualifierBitFieldVector;
    delete formalParamList;
    delete initExpr;

    for (vector<Tree *>::iterator it = arraySizeExprList.begin(); it != arraySizeExprList.end(); ++it)
        delete *it;
}


void
Declarator::setInitExpr(Tree *_initExpr)
{
    assert(!initExpr);
    initExpr = _initExpr;
}


void
Declarator::checkForFunctionReturningArray() const
{
    if (type == FUNCPTR)
        errormsg("`%s' declared as function returning an array", id.c_str());
}

void
Declarator::addArraySizeExpr(Tree *_arraySizeExpr)
{
    arraySizeExprList.push_back(_arraySizeExpr);
    type = ARRAY;
}


const FormalParamList *
Declarator::getFormalParamList() const
{
    return formalParamList;
}


FormalParamList *
Declarator::detachFormalParamList()
{
    FormalParamList *ret = formalParamList;
    formalParamList = NULL;
    return ret;
}


void
Declarator::setFormalParamList(FormalParamList *_formalParamList)
{
    //cout << "# Declarator::setFormalParamList: id='" << id << "', type=" << type << "\n";
    assert(type == SINGLETON || type == FUNCPTR);
    assert(!formalParamList || type == FUNCPTR);

    formalParamList = _formalParamList;

    //cout << "# Declarator::setFormalParamList:   (" << (formalParamList ? formalParamList->toString() : "<none>") << ")" << endl;
}


// Function to be called during parsing.
//
// Returns a Declaration object allocated by 'new'.
// This Declarator does not own the Declaration object:
// the destruction is the responsibility of the caller.
//
// The ownership of initExpr is transferred to this Declaration object.
// This Declarator object loses its initialization expression.
//
// Upon error, sends an error message and returns NULL.
//
Declaration *
Declarator::declareVariable(const TypeDesc *varType, bool isStatic, bool isExtern)
{
    if (id.empty())
    {
        errormsgEx(::getSourceLineNo(), "empty declarator name");
        return NULL;
    }

    // Create a Declaration object but only initialize it partially.
    // In particular, the size of an array might be specified by enum names,
    // but during parsing, enums have not been fully processed yet.
    // That will be done later by the DeclarationFinisher, which will finish
    // initializing the Declaration object.
    // 
    Declaration *decl = new Declaration(id, varType, arraySizeExprList, isStatic, isExtern);

    arraySizeExprList.clear();  // ownership of the expressions transferred to decl

    decl->setInitExpr(initExpr);  // ownership of initExpr transferred to decl
    initExpr = NULL;  // so that ~Declarator() does not destroy this expression

    decl->setLineNo(srcFilename, lineno);
    return decl;
}


bool
Declarator::getNumDimensions(size_t &numDimensions) const
{
    numDimensions = 0;

    if (type != ARRAY)
        return true;  // success

    if (arraySizeExprList.size() == 0)
    {
        errormsg("array %s: no dimensions", id.c_str());
        return false;
    }

    numDimensions = arraySizeExprList.size();
    return true;
}


// Upon success, returns the size of each dimension of the array.
// Does not return any dimension for a non-array declaration.
//
// Displays an error message upon failure.
//
// arrayDimensions: NOT cleared before elements are added, if any.
// allowUnknownFirstDimension: if true, this unknown 1st dimension is assumed to be 1.
//
// declarationTree: Tree on which to call errormsg() or warnmsg(), so that
//                  the right line number appears in the message. Allowed to be null.
//
bool
Declarator::computeArrayDimensions(vector<uint16_t> &arrayDimensions,
                                   bool allowUnknownFirstDimension,
                                   const vector<Tree *> &arraySizeExprList,
                                   const string &id,
                                   const Tree *initExpr,
                                   const Tree *declarationTree)
{
    if (arraySizeExprList.size() == 0)
    {
        Tree::errormsg(declarationTree, "array %s: no dimensions", id.c_str());
        return false;
    }

    // Check that only the first dimension is unspecified, if any.
    //
    for (vector<Tree *>::const_iterator it = arraySizeExprList.begin(); it != arraySizeExprList.end(); ++it)
        if (*it == NULL && it != arraySizeExprList.begin())
        {
            Tree::errormsg(declarationTree, "array %s: dimension other than first one is unspecified", id.c_str());
            return false;
        }

    if (arraySizeExprList.front() == NULL)  // if no size in [] for 1st dimension, use # of elements in init list
    {
        if (initExpr != NULL)
        {
            size_t len = 1;
            if (const TreeSequence *ts = dynamic_cast<const TreeSequence *>(initExpr))
                len = ts->size();
            else if (const StringLiteralExpr *sle = dynamic_cast<const StringLiteralExpr *>(initExpr))
                len = sle->getLiteral().length() + 1;  // include terminating '\0'
            else
            {
                // Error to be issued by Declaration::checkArrayInitializer(), called by SemanticsChecker
            }
            if (len > 0xFFFF)
            {
                initExpr->errormsg("array initializer too long");
                len = 0xFFFF;
            }
            arrayDimensions.push_back(uint16_t(len));
        }
        else if (!allowUnknownFirstDimension)
        {
            Tree::warnmsg(declarationTree, "array `%s' assumed to have one element", id.c_str());
            arrayDimensions.push_back(1);
        }
    }

    for (vector<Tree *>::const_iterator it = arraySizeExprList.begin(); it != arraySizeExprList.end(); ++it)
    {
        Tree *arraySizeExpr = *it;
        if (arraySizeExpr == NULL)
        {
            if (initExpr == NULL)
                arrayDimensions.push_back(1);
            continue;
        }

        try
        {
            // Set the expression type for evaluateConstantExpr().
            // The reason why 'arraySizeExpr' is not typed can be because this method
            // is being called during a typedef creation, which happens during parsing,
            // which is done before the "main" ExpressionTypeSetter phase happens.
            //
            ExpressionTypeSetter ets;
            arraySizeExpr->iterate(ets);

            if (arraySizeExpr->getTypeDesc()->isPtrOrArray())
            {
                arraySizeExpr->errormsg("pointer or array expression used for size of array `%s'", id.c_str());
                return false;
            }
            uint16_t value;
            if (!arraySizeExpr->evaluateConstantExpr(value))
            {
                size_t dim = it - arraySizeExprList.begin() + 1;
                arraySizeExpr->errormsg("invalid size expression for dimension %u of array `%s'", dim, id.c_str());
                return false;
            }
            arrayDimensions.push_back(value);
        }
        catch (...)
        {
            arraySizeExpr->errormsg("unexpected exception caught in Declarator::computeArrayDimensions() for `%s'", id.c_str());
            return false;
        }
    }

    return true;
}


const string &
Declarator::getId() const
{
    return id;
}


uint16_t
Declarator::getNumArrayElements() const
{
    vector<uint16_t> arrayDimensions;
    if (!computeArrayDimensions(arrayDimensions, false, NULL))  // arrayDimensions will be empty if non-array
        return 0;
    if (arrayDimensions.size() == 0)
        return 0;

    uint16_t product = 1;
    for (vector<uint16_t>::const_iterator it = arrayDimensions.begin(); it != arrayDimensions.end(); ++it)
        product *= *it;
    return product;
}


void
Declarator::setAsFunctionPointer(FormalParamList *params)
{
    assert(params);

    type = FUNCPTR;
    setFormalParamList(params);
}


void
Declarator::setAsArrayOfFunctionPointers(FormalParamList *params, TreeSequence *_subscripts)
{
    assert(params);
    assert(_subscripts);

    type = FUNCPTR;
    setFormalParamList(params);

    for (vector<Tree *>::iterator it = _subscripts->begin(); it != _subscripts->end(); ++it)
        addArraySizeExpr(*it);  // Tree ownership transfered to 'this' Declarator

    _subscripts->clear();  // so that following 'delete' does not destroy the Trees, which are now owner by 'this'

    delete _subscripts;  // does not destroy any Trees
}


// Applies the pointer level to 'td' and returns the resulting type.
//
const TypeDesc *
Declarator::processPointerLevel(const TypeDesc *td) const
{
    assert(td);
    TypeManager &tm = TranslationUnit::getTypeManager();
    if (typeQualifierBitFieldVector != NULL)
        td = tm.getPointerTo(td, *typeQualifierBitFieldVector);
    assert(td);
    return td;
}


FormalParameter *
Declarator::createFormalParameter(DeclarationSpecifierList &dsl) const
{
    if (dsl.hasEnumeratorList())
    {
        errormsg("enum with enumerated names is not supported in a function's formal parameter");
        delete dsl.detachEnumeratorList();  // won't be needed
    }

    const TypeDesc *td = processPointerLevel(dsl.getTypeDesc());

    //cout << "# Declarator::createFormalParameter: lineno=" << lineno << ", id='" << id << "', type=" << type
    //     << ", td={" << td->toString() << "}, isFP=" << isFunctionPointer() << ", isAofFP=" << isArrayOfFunctionPointers() << "\n";

    assert(initExpr == NULL);  // no initialization expression for formal parameter
    TypeManager &tm = TranslationUnit::getTypeManager();

    if (isFunctionPointer() || isArrayOfFunctionPointers())
    {
        td = tm.getFunctionPointerType(td, *formalParamList, dsl.isInterruptServiceFunction(), dsl.isFunctionReceivingFirstParamInReg());
                // Last line does not transfer ownership of formalParamList, so still owned by 'this'.

        //cout << "# Declarator::createFormalParameter:   getFunctionPointerType -> {" << td->toString() << "}\n";
    }

    vector<uint16_t> arrayDimensions;  // empty means not an array
    if (isArray())
    {
        if (!computeArrayDimensions(arrayDimensions, true, NULL))  // arrayDimensions will be empty if Declarator is not array
            return NULL;
        if (dsl.getTypeDesc()->isArray())  // if specifier is typedef of array
        {
            td->appendDimensions(arrayDimensions);

            // Make td point to the what td is an array (of arrays) of.
            while (td->isArray())
                td = td->getPointedTypeDesc();
        }
        if (arrayDimensions.size() > 1)
            td = tm.getArrayOf(td, arrayDimensions.size() - 1);
        td = tm.getPointerTo(td);
    }
    else if (dsl.getTypeDesc()->isArray())
    {
        assert(dsl.getTypeDesc()->isValid());
        td->appendDimensions(arrayDimensions);
        td = tm.getPointerTo(dsl.getTypeDesc()->pointedTypeDesc);
        assert(td->isValid());
    }

    //cout << "# Declarator::createFormalParameter:   new FormalParameter({" << td->toString() << "}, " << id << ", " << vectorToString(arrayDimensions) << ")\n";

    return new FormalParameter(td, id, arrayDimensions, dsl.getEnumTypeName());
}


static const char *typeNames[] = { "SINGLETON", "ARRAY", "FUNCPTR" };


string
Declarator::toString() const
{
    stringstream ss;
    ss << "Declarator(id '" << id << "' of type " << typeNames[type] << " at " << srcFilename << ":" << lineno;
    if (initExpr)
       ss << ", with init expr";
    if (type == ARRAY)
        ss << ", array with " << arraySizeExprList.size() << " size expression(s)";
    else if (type == FUNCPTR)
        ss << ", function pointer";
    if (typeQualifierBitFieldVector != NULL)
    {
        ss << ",";
        for (size_t i = 0; i < typeQualifierBitFieldVector->size(); ++i)
        {
            ss << " *";
            TypeQualifierBitField field = (*typeQualifierBitFieldVector)[i];
            if (field & CONST_BIT)
                ss << " const";
            if (field & VOLATILE_BIT)
                ss << " volatile";
        }
    }
    if (formalParamList)
        ss << ", with formal param list";
    ss << ")";
    return ss.str();
}


void
Declarator::setBitFieldWidth(Tree &bitFieldWidthExpr)
{
    ExpressionTypeSetter ets;
    bitFieldWidthExpr.iterate(ets);

    uint16_t result;
    if (! bitFieldWidthExpr.evaluateConstantExpr(result))
    {
        bitFieldWidth = INVALID_WIDTH_EXPR;
        return;
    }
    if (bitFieldWidthExpr.isSigned() && int16_t(result) < 0)
    {
        bitFieldWidth = NEGATIVE_WIDTH_EXPR;
        return;
    }
    bitFieldWidth = result;
}


void
Declarator::checkBitField(const TypeDesc &typeDesc) const
{
    if (bitFieldWidth == NOT_BIT_FIELD)
        return;
    if (bitFieldWidth == INVALID_WIDTH_EXPR)
    {
        ::errormsgEx(srcFilename, lineno, "invalid width in bit-field `%s'", getId().c_str());
        return;
    }
    if (bitFieldWidth == NEGATIVE_WIDTH_EXPR)
    {
        ::errormsgEx(srcFilename, lineno, "negative width in bit-field `%s'", getId().c_str());
        return;
    }
    if (bitFieldWidth == 0)
    {
        ::errormsgEx(srcFilename, lineno, "zero width for bit-field `%s'", getId().c_str());
        return;
    }
    if (   (typeDesc.type == BYTE_TYPE && bitFieldWidth >  8)
        || (typeDesc.type == WORD_TYPE && bitFieldWidth > 16)
        || (typeDesc.isLong()          && bitFieldWidth > 32))
    {
        ::errormsgEx(srcFilename, lineno, "width of `%s' exceeds its type (`%s')", id.c_str(), typeDesc.toString().c_str());
        return;
    }
    if (!typeDesc.isIntegral())
    {
        ::errormsgEx(srcFilename, lineno, "bit-field `%s' has invalid type (`%s')", id.c_str(), typeDesc.toString().c_str());
        return;
    }
}
