/*  $Id: SemanticsChecker.h,v 1.4 2017/06/25 20:38:45 sarrazip Exp $

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

#ifndef _H_SemanticsChecker
#define _H_SemanticsChecker

#include "FunctionDef.h"


class SemanticsChecker : public Tree::Functor
{
public:

    SemanticsChecker();

    ~SemanticsChecker();

    virtual bool open(Tree *t);

    virtual bool close(Tree *t);

    void setCurrentFunctionDef(FunctionDef *fd);

    const FunctionDef *getCurrentFunctionDef() const;

private:
    // Forbidden:
    SemanticsChecker(const SemanticsChecker &);
    SemanticsChecker &operator = (const SemanticsChecker &);

private:

    FunctionDef *currentFunctionDef;

};


#endif  /* _H_SemanticsChecker */
