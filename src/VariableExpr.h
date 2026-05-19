/*  $Id: VariableExpr.h,v 1.9 2016/09/15 03:34:58 sarrazip Exp $

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

#ifndef _H_VariableExpr
#define _H_VariableExpr

#include "Tree.h"

class Declaration;
class FuncAddrExpr;


class VariableExpr : public Tree
{
public:

    VariableExpr(const std::string &id);

    virtual ~VariableExpr();

    std::string getId() const;

    std::string getFrameDisplacementArg(int16_t offset = 0) const;

    void setDeclaration(Declaration *decl);

    const Declaration *getDeclaration() const;

    void markAsFuncAddrExpr() { _isFuncAddrExpr = true; }

    bool isFuncAddrExpr() const { return _isFuncAddrExpr; }

    virtual void checkSemantics(Functor &f);

    virtual CodeStatus emitCode(ASMText &out, bool lValue) const;

    virtual bool iterate(Functor &f);

    virtual bool isLValue() const { return true; }

private:
    // Forbidden:
    VariableExpr(const VariableExpr &);
    VariableExpr &operator = (const VariableExpr &);

private:

    std::string id;
    Declaration *declaration;  // does not own the object
    bool _isFuncAddrExpr;  // when true, 'id' is name of function whose address is taken

};


#endif  /* _H_VariableExpr */
