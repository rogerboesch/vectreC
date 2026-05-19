/*  $Id: Pragma.h,v 1.10 2018/09/15 20:00:49 sarrazip Exp $

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

#ifndef _H_PRAGMA
#define _H_PRAGMA

#include "Tree.h"


class Pragma : public Tree
{
public:

    Pragma(const std::string &_directive);

    // If this pragma is "org N", then stores N in 'address'
    // and returns true. Returns false otherwise.
    //
    bool isCodeOrg(uint16_t &address) const;

    // If this pragma is "limit N", then stores N in 'address'
    // and returns true. Returns false otherwise.
    //
    bool isCodeLimit(uint16_t &address) const;

    // If this pragma is "data N", then stores N in 'address'
    // and returns true. Returns false otherwise.
    //
    bool isDataOrg(uint16_t &address) const;

    // If #pragma exec_once.
    //
    bool isExecOnce() const;

    // If #pragma stack_space N.
    // numBytes receives N, but only when this method returns true.
    //
    bool isStackSpace(uint16_t &numBytes) const;

    std::string getDirective() const;

    // Vectrex directives:

    // If this pragma is 'vx_title "text"', then use the text as the ROM title.
    //
    bool isVxTitle(std::string &title) const;

    // This pragma defines the size of the title.
    //
    bool isVxTitleSize(int8_t &width, int8_t &height) const;

    // This pragma defines the position of the title.
    //
    bool isVxTitlePos(int8_t &x, int8_t &y) const;

    // This pragma defines the copyright text, can only be 4 chars.
    //
    bool isVxCopyright(std::string &copyright) const;

    // If this pragma is 'vx_music label', the label points to the startup music.
    //
    bool isVxMusic(std::string &label) const;

    virtual bool isLValue() const { return false; }

private:

    void getNextWord(size_t &startIndex, size_t &endIndex) const;
    bool parseOrg(uint16_t &newOriginAddress) const;

private:

    std::string directive;

};



#endif /* _H_PRAGMA */
