/*  $Id: Pragma.cpp,v 1.10 2018/09/15 20:00:49 sarrazip Exp $

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

#include "Pragma.h"

#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>

using namespace std;


Pragma::Pragma(const std::string &_directive)
:   directive(_directive)
{
}


bool
Pragma::isCodeOrg(uint16_t &address) const
{
    if (strncmp(directive.c_str(), "org", 3) != 0)
        return false;
    return parseOrg(address);
}


bool
Pragma::isCodeLimit(uint16_t &address) const
{
    if (strncmp(directive.c_str(), "limit", 5) != 0)
        return false;
    return parseOrg(address);
}


bool
Pragma::isDataOrg(uint16_t &address) const
{
    if (strncmp(directive.c_str(), "data", 4) != 0)
        return false;
    return parseOrg(address);
}


bool
Pragma::parseOrg(uint16_t &address) const
{
    size_t i = 0, len = directive.length();
    for ( ; i < len && !isspace(directive[i]); ++i)  // pass org, data or other keyword
        ;
    for ( ; i < len && isspace(directive[i]); ++i)  // pass following white space
        ;
    if (i == len)
        return false;
    int base = 10;
    if (directive[i] == '0' && tolower(directive[i + 1]) == 'x')
        base = 16, i += 2;
    char *endptr = NULL;
    errno = 0;
    const char *argStart = directive.c_str() + i;
    unsigned long n = strtoul(argStart, &endptr, base);
    if (errno != 0 || endptr == argStart || n > 0xFFFF)
        return false;
    address = uint16_t(n);
    return true;
}


// startIndex: MUST be initialized to the index in 'directive' where
//             the scanning must start.
// endIndex: Will receive the end of the found word.
// Examples: With directive == "foo waldo blargh" and startIndex at 3,
//           this function will return 4 and 9, i.e., "waldo".
//           With startIndex at 9, 10 and 16 are returned ("blargh").
//           With startIndex at 16, 16 and 16 are returned (no word found).
//
void
Pragma::getNextWord(size_t &startIndex, size_t &endIndex) const
{
    passSpaces(directive, startIndex);
    endIndex = startIndex;
    passNonSpaces(directive, endIndex);
}


bool
Pragma::isExecOnce() const
{
    return directive == "exec_once";
}


bool
Pragma::isStackSpace(uint16_t &numBytes) const
{
    if (strncmp(directive.c_str(), "stack_space", 11) != 0)
        return false;

    size_t argStart = 11, argEnd;
    getNextWord(argStart, argEnd);
    string arg(directive, argStart, argEnd);

    unsigned long n = strtoul(arg.c_str(), NULL, 10);
    if (n == 0 || n > 0xFFFF || errno == ERANGE)
        return false;

    numBytes = (uint16_t) n;
    return true;
}


string
Pragma::getDirective() const
{
    return directive;
}


// #pragma vx_title "My title"
//
bool
Pragma::isVxTitle(std::string &title) const
{
    if (strncmp(directive.c_str(), "vx_title", 8) != 0)
        return false;

    size_t i = 0, len = directive.length();
    for ( ; i < len && !isspace(directive[i]); ++i)  // pass vx_title
        ;
    for ( ; i < len && isspace(directive[i]); ++i)  // pass following white space
        ;
    if (i == len)
        return false;

    if ( i < len && directive[i] != '"' )
        return false;

    i++;

    title = "";
    for ( ; i < len && directive[i] != '"'; ++i ) // read in title
      title += directive[i]; 

    if (i == len)
      return false; 

    return true;
}


// #pragma vx_title_size h, w
//
bool
Pragma::isVxTitleSize(int8_t &height, int8_t &width) const
{
  const char* pragma = "vx_title_size";
  if ( strncmp(directive.c_str(), pragma, strlen(pragma)) != 0)
    return false;

  size_t i = strlen(pragma), len = directive.length();
  for ( ; i < len && isspace(directive[i]); ++i )
    ;
  if ( i == len )
    return false;

  std::string h = "";
  for ( ; i < len && directive[i] != ','; i++ )
    h += directive[i];

  if ( i == len )
    return false;

  i++;
  for ( ; i < len && isspace(directive[i]); i++ )
    ;
  if ( i == len )
    return false;

  std::string w = "";
  for ( ; i < len && !isspace(directive[i]); ++i )
    w += directive[i];

  height = (int8_t) atoi(h.c_str());  
  width = (int8_t) atoi(w.c_str());

  return true;
}


// #pragma vx_title_pos y, x
//
bool
Pragma::isVxTitlePos(int8_t &y, int8_t &x) const
{
  const char* pragma = "vx_title_pos";
  if ( strncmp(directive.c_str(), pragma, strlen(pragma)) != 0)
    return false;

  size_t i = strlen(pragma), len = directive.length();
  for ( ; i < len && isspace(directive[i]); ++i )
    ;
  if ( i == len )
    return false;

  std::string _y = "";
  for ( ; i < len && directive[i] != ','; i++ )
    _y += directive[i];

  if ( i == len )
    return false;

  i++;
  for ( ; i < len && isspace(directive[i]); i++ )
    ;
  if ( i == len )
    return false;

  std::string _x = "";
  for ( ; i < len && !isspace(directive[i]); ++i )
    _x += directive[i];

  y = (int8_t) atoi(_y.c_str());  
  x = (int8_t) atoi(_x.c_str());

  return true;
}


// #pragma vx_music label
//
bool
Pragma::isVxMusic(std::string &label) const
{
    if (strncmp(directive.c_str(), "vx_music", 8) != 0)
        return false;

    size_t i = 0, len = directive.length();
    for ( ; i < len && !isspace(directive[i]); ++i)  // pass vx_music
        ;
    for ( ; i < len && isspace(directive[i]); ++i)  // pass following white space
        ;
    if (i == len)
        return false;

    label = "";
    for ( ; i < len && !isspace(directive[i]); ++i ) // read in label
      label += directive[i]; 

    return true;
}


// #pragma vx_copyright "copyright"
//
bool
Pragma::isVxCopyright(std::string &copyright) const
{
    if (strncmp(directive.c_str(), "vx_copyright", 12) != 0)
        return false;

    size_t i = 0, len = directive.length();
    for ( ; i < len && !isspace(directive[i]); ++i)  // pass vx_copyright
        ;
    for ( ; i < len && isspace(directive[i]); ++i)  // pass following white space
        ;
    if (i == len)
        return false;

    if ( i < len && directive[i] != '"' )
        return false;

    i++;

    copyright = "";
    size_t l = 0;
    for ( ; l < 4 && i < len && directive[i] != '"'; ++i, l++ ) // read in copyright
      copyright += directive[i]; 

    if (i == len)
      return false; 

    for( ; l < 4; l++ ) {
      copyright += " ";
    }

    return true;
}
