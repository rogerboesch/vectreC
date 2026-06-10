/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2023 Pierre Sarrazin <http://sarrazip.com/>

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

#ifndef _H_util
#define _H_util

#include "TypeDesc.h"

#include <typeinfo>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdarg.h>
#include <memory>

#ifdef _MSC_VER  /* Hacks to help compile under Visual C++. */
#define PACKAGE "cmoc"
#pragma warning(disable:4996)  /* Tolerate so-called "unsafe" functions like _snprintf(). */
#pragma warning(disable:4822)  /* "local class member function does not have a body" */
#define snprintf _snprintf
#define popen _popen
#define pclose _pclose
#define strcasecmp _stricmp
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define WIFEXITED(x) true
#define WEXITSTATUS(x) (x)
#else
#include <strings.h>  /* for strcasecmp(), as per POSIX.1 */
#include <unistd.h>
#endif

class Tree;
class FormalParamList;
class TreeSequence;


// Supported platforms for the resulting executable.
//
enum TargetPlatform
{
    COCO_BASIC,     // Extended Color Basic on the Color Computer
    OS9,            // OS-9 or NitrOS-9
    USIM,           // USim 6809 simulator
    VECTREX,        // Vectrex video game console
    DRAGON,         // Dragon 32/64
    VOID_TARGET,    // target with no I/O system known to this compiler
    THOMMO,         // Thomson MO
    THOMTO,         // Thomson TO
    FLEX,           // FLEX by Technical Systems Consultants
    INTERM_REP,     // Intermediate representation (CMOC-specific)
};


// Returns the displayable name for the given platform.
//
const char *getTargetPlatformName(TargetPlatform targetPlatform);


// Returns the identifier that is used in the IFDEF directives in stdlib/*.asm.
//
const char *getTargetPlatformPreprocId(TargetPlatform targetPlatform);


enum class FloatingPointLibrary : uint8_t
{
    ECB_ROM,  // Extended Color Basic ROM routines (or Dragon ROM)
    NATIVE_LIB,  // yet-to-be-released library that would ship with CMOC
    MC6839_LIB,  // third party library
};


enum class FramePointerOption : uint8_t
{
    DEFAULT,  // omit frame pointer is simple cases, e.g., no locals and no params
    OMIT_A_LOT,  // use low-level optimizer to strip frame pointer when possible
    KEEP_ALL,  // except pure asm functions
};


// Names PC to CC have the values that are expected by the PSHS instruction.
//
enum Register
{
    PC = 0x80, U = 0x40, Y = 0x20, X  = 0x10,
    DP = 0x08, B = 0x04, A = 0x02, CC = 0x01,
    D = A | B,
    S = 0x100,
    NO_REGISTER = 0xFFFF
};


// Type qualifier bits.
//
enum
{
    CONST_BIT = 1,
    VOLATILE_BIT = 2,
};


// Integer that can combine CONST_BIT and VOLATILE_BIT.
//
typedef uint8_t TypeQualifierBitField;


typedef std::vector<TypeQualifierBitField> TypeQualifierBitFieldVector;


enum ConstCorrectnessCode { CONST_CORRECT, CONST_INCORRECT, INCOMPAT_TYPES };


// Returns 0 if the pointer initialization is const-correct (and the types are compatible).
// Returns -1 if not const-correct (and the types are compatible).
// Returns -2 if the types are NOT compatible.
//
ConstCorrectnessCode isPointerInitConstCorrect(const TypeDesc *declPointedTypeDesc, const TypeDesc *initPointedTypeDesc);


// Returns a number of bytes, or -1 upon overflow (i.e., beyond 32767).
// dims: Dimensions of an array. Must have at least one element.
// dimIndex: Product starts at dims[dimIndex] and finishes at the end of dims. Must be <= dims.size().
// finalArrayElementTypeSize: In bytes.
//
int16_t computeDimensionsProduct(const std::vector<uint16_t> &dims,
                                 size_t dimIndex,
                                 int16_t finalArrayElementTypeSize);


// name: Must be in upper-case. Does not have to end with '\0'.
// Example: getRegisterFromName("DP,...") returns DP.
// Returns REGISTER if no register name is recognized.
//
Register getRegisterFromName(const char *name);


// Returns the upper-case name of the given register, or NULL if 'reg'
// is NO_REGISTER or an invalid numerical value.
//
const char *getRegisterName(Register reg);


typedef std::vector<std::string> StringVector;


// Graph that maps a string to a list of strings.
//
typedef std::map<std::string, StringVector> StringGraph;


// Appends 'x' to 'v' only if 'x' is not already present.
// Takes linear time.
//
template <typename T>
inline void
pushBackUnique(std::vector<T> &v, const T &x)
{
    if (std::find(v.begin(), v.end(), x) == v.end())
        v.push_back(x);
}


template <typename T>
inline void
appendVector(std::vector<T>& dest, const std::vector<T>& src)
{
    std::copy(src.begin(), src.end(), std::inserter(dest, dest.end()));
}


// Returns an iterator on the first element of 'v' whose key is equal to 'key'.
// Returns v.end() if no such element found.
//
template <typename Key, typename Value>
typename std::vector< std::pair<Key, Value> >::iterator
findInVectorOfPairsByKey(std::vector< std::pair<Key, Value> > &v, const Key &key)
{
    for (typename std::vector< std::pair<Key, Value> >::iterator it = v.begin(); it != v.end(); ++it)
        if (it->first == key)
            return it;
    return v.end();
}


const char *getLoadInstruction(BasicType t);

const char *getAddInstruction(BasicType t);

const char *getSubInstruction(BasicType t);

const char *getAddOrSubInstruction(BasicType t, bool isAdd);

const char *getStoreInstruction(BasicType t);

// Inserts 'element' in 'v' in sorted order. Takes logarithmic time.
//
template <typename T>
bool addUnique(std::vector<T> &v, const T &element)
{
    typename std::vector<T>::iterator it = std::lower_bound(v.begin(), v.end(), element);
    if (it != v.end() && *it == element)
        return false;  // already present
    v.insert(it, element);
    return true;
}

// Determines if 'element' is in *sorted* vector 'v'. Takes logarithmic time.
//
template <typename T>
bool isPresent(std::vector<T> &v, const T &element)
{
    typename std::vector<T>::iterator it = std::lower_bound(v.begin(), v.end(), element);
    return it != v.end() && *it == element;
}


struct FilenameAndLineNo
{
    std::string filename;
    int lineno;

    FilenameAndLineNo(const std::string &fn, int ln)
      : filename(fn), lineno(ln) {}
};


struct BreakableLabels
{
    std::string breakLabel;
    std::string continueLabel;  // only allowed to be empty in the case of a switch() statement

    BreakableLabels()
      : breakLabel(), continueLabel() {}
};


// In hex, returned string starts with dollar sign.
//
std::string dwordToString(uint32_t dw, bool hex = false);


// In hex, returned string starts with dollar sign.
//
std::string wordToString(uint16_t w, bool hex = false);


// In hex, returned string starts with dollar sign.
//
std::string intToString(int16_t n, bool hex = false);
std::string int8ToString(int8_t n, bool hex = false);


std::string doubleToString(double d);


void stringToLower(std::string &s);


// Determines if 'c' is a character that can be the first character of a C identifier.
//
inline bool isCIdentifierStartingChar(char c)
{
    return isalpha(c) || c == '_';
}


// Determines if 'c' is a character that can be in a C identifier.
//
inline bool isCIdentifierChar(char c)
{
    return isCIdentifierStartingChar(c) || isdigit(c);
}


// Determines if 'c' is a character that can appear in an assembly language identifier.
//
inline bool isAssemblyIdentifierChar(char c)
{
    return isCIdentifierChar(c) || c == '@';
}


// Advances 'index' until s[index] is not a space character or 'index' is the length of 's'.
// Does nothing if 'index' is already at or beyond the length of 's'.
//
inline void passSpaces(const std::string &s, size_t &index)
{
    for (size_t len = s.length(); index < len && isspace(s[index]); ++index)
        ;
}


// Advances 'index' until s[index] is a space character or 'index' is the length of 's'.
// Does nothing if 'index' is already at or beyond the length of 's'.
//
inline void passNonSpaces(const std::string &s, size_t &index)
{
    for (size_t len = s.length(); index < len && !isspace(s[index]); ++index)
        ;
}


bool isRegisterName(const std::string &s);


bool isPowerOf2(uint16_t n);


bool startsWith(const std::string &s, const char *prefix);

bool endsWith(const std::string &s, const char *suffix);

// Returns the extension (which starts with a dot) of the basename of the given path,
// or an empty string if no dot is found.
//
std::string getExtension(const std::string &s);

// Returns the removed extension of the basename of the given path, or an empty string
// if no dot is found.
//
std::string removeExtension(std::string &s);

// Replaces the extension in the basename of path 's' by newExt,
// if an extension is found. Otherwise, appends newExt to s.
// newExt: Must start with dot, if a dot is wanted in the new extension.
//
std::string replaceExtension(const std::string &s, const char *newExt);

// Replaces the part of 's' that precedes the last directory separator with 'newDir'.
// If no directory separator is found, returns newDir + "/" + s.
//
std::string replaceDir(const std::string &s, const std::string &newDir);

std::string getBasename(const std::string &filename);


// Multiples [begin, end) and if no overflow occurs, stores the result
// in 'result' and returns true.
// Returns false if an overflow occurs.
// If [begin, end) is empty, 'result' becomes 1 and true is returned.
//
template <typename ForwardIterator>
inline bool
product(uint16_t& result, ForwardIterator begin, ForwardIterator end)
{
    result = 0;  // ensure defined value
    uint32_t longResult = 1;
    for ( ; begin != end; ++begin)
    {
        longResult *= *begin;
        if (longResult > 0xFFFF)
            return false;  // overflow
    }
    result = uint16_t(longResult);
    return true;
}


// Like Perl's join() function.
// Type T must be usable in an ostream.
//
template <typename T>
inline std::string
join(const std::string &delimiter, const std::vector<T> &vec)
{
    std::stringstream ss;
    for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin())
            ss << delimiter;
        ss << *it;
    }
    return ss.str();
}


template <typename T>
class VectorToString
{
public:
    VectorToString(const std::vector<T> &_vec, const char *_delimiter, const char *_opening, const char *_closing)
    : vec(_vec), delimiter(_delimiter), opening(_opening), closing(_closing) {}

    const std::vector<T> &vec;
    const char *delimiter;
    const char *opening;
    const char *closing;
};


// Manipulator to print vectors: e.g., with vector<T> v, do cout << vectorToString(v);
//
template <typename T>
inline VectorToString<T>
vectorToString(const std::vector<T> &_vec, const char *_delimiter = ", ", const char *_opening = "{", const char *_closing = "}")
{
    return VectorToString<T>(_vec, _delimiter, _opening, _closing);
}


// Calls delete on each pointer in an std::vector<T *>.
//
template <typename T>
void deleteVectorElements(std::vector<T *> &v)
{
    for (T *p : v)
        delete p;
}


template <typename T>
std::ostream &operator << (std::ostream &out, const VectorToString<T> &vts)
{
    out << vts.opening;
    for (typename std::vector<T>::const_iterator it = vts.vec.begin(); it != vts.vec.end(); ++it)
    {
        if (it != vts.vec.begin())
            out << vts.delimiter;
        out << *it;
    }
    out << vts.closing;
    return out;
}


// Array that makes no dynamic allocations and imposes a maximum compile-time capacity.
//
template <typename T, size_t capacity>
class StaticArray
{
public:
    StaticArray()
    :   array(),
        arraySize(0)
    {
    }

    // Returns false if there is no room left.
    //
    bool push_back(const T &x)
    {
        if (arraySize >= capacity)
            return false;
        array[arraySize++] = x;
        return true;
    }

    size_t size() const
    {
        return arraySize;
    }

    // index must lower than size().
    //
    const T &operator[](size_t index) const
    {
        if (index >= arraySize)
            throw -1;
        return array[index];
    }

private:
    T array[capacity];
    size_t arraySize;
};


// Returns a string of the form "foo.cpp:42".
//
std::string getSourceLineNo();

void errormsg(const char *fmt, ...);
void errormsgEx(const std::string &explicitLineNo, const char *fmt, ...);
void errormsgEx(const std::string &sourceFilename, int lineno, const char *fmt, ...);

// diagType: "error" or "warning".
void diagnoseVa(bool isError, const std::string &explicitLineNo, const char *fmt, va_list ap);

void warnmsg(const char *fmt, ...);


#endif  /* _H_util */
