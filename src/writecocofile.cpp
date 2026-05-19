/*  $Id: writecocofile.cpp,v 1.20 2018/04/20 00:09:58 sarrazip Exp $

    writecocofile.cpp - A tool to write native files to a CoCo DECB disk image.
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <iterator>

using namespace std;


static const char *PROGRAM = "writecocofile";

static bool verbose = false;
static bool convertASCIIBasicNewlines = false;
static bool printToStdOut = false;


// Case-insensitive memcmp().
//
static int compareWithoutCase(const char *a, const char *b, size_t numChars)
{
    for (size_t i = 0; i < numChars; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return tolower(a[i]) < tolower(b[i]) ? -1 : +1;
    return 0;
}


class CoCoDisk
{
public:
    enum Error
    {
        SUCCESS = 0,
        INVALID_IMAGE_FILE_LENGTH,
        INVALID_IMAGE_FILE,
        INVALID_FILENAME,
        INVALID_EXTENSION,
        FAILED_TO_FIND_SIZE,
        OUT_OF_SPACE,
        NO_FREE_DIR_ENTRY,
    };

    enum
    {
        BYTES_PER_SECTOR = 256,
        SECTORS_PER_GRANULE = 9,
        GRANULES_PER_TRACK = 2,
        SECTORS_PER_TRACK = SECTORS_PER_GRANULE * GRANULES_PER_TRACK,
        BYTES_PER_GRANULE = BYTES_PER_SECTOR * SECTORS_PER_GRANULE,
        BYTES_PER_TRACK = BYTES_PER_SECTOR * SECTORS_PER_TRACK,
        DIR_TRACK = 17,
        DIR_FAT_SECTOR = 2,
        DIR_LIST_FIRST_SECTOR = 3,
        BYTES_PER_DIR_ENTRY = 32,
        FREE_FAT_ENTRY = 0xFF,
        NUM_DIR_LIST_SECTORS = SECTORS_PER_TRACK - (DIR_LIST_FIRST_SECTOR - 1),
        NUM_DIR_LIST_ENTRIES = NUM_DIR_LIST_SECTORS * (BYTES_PER_SECTOR / BYTES_PER_DIR_ENTRY),
        MAX_GRANULES = 68
    };

    // Disk image file format.
    enum ImageFormat
    {
        RAW_FORMAT,
        JVC_FORMAT
    };

    // CoCo file formats.
    enum Format
    {
        BINARY = 0,
        ASCII = 0xFF
    };

    static const char *getFormatName(Format fmt)
    {
        switch (fmt)
        {
            case BINARY: return "binary";
            case ASCII: return "ascii";
        }
        return "???";
    }

    enum Type
    {
        BASIC_PROGRAM = 0,
        BASIC_DATA = 1,
        MACHINE_CODE = 2,
        ASCII_TEXT = 3
    };

    static const char *getFileTypeName(Type type)
    {
        switch (type)
        {
            case BASIC_PROGRAM: return "basic";
            case BASIC_DATA: return "data";
            case MACHINE_CODE: return "machine";
            case ASCII_TEXT: return "text";
        }
        return "???";
    }

    // Does not keep a reference to the istream.
    // May throw CoCoDisk::Error.
    //
    CoCoDisk(istream &file);

    ~CoCoDisk();

    const char *getImageFormatName() const;

    size_t getContentOffset() const;

    unsigned char *getFAT();

    /*  Reads the contents of 'file' and adds it to this disk's
        file system under the given file name and extension.
        Returns 0 for success, or a negative error code otherwise.
    */
    CoCoDisk::Error addFile(istream &file,
                            const string &filename, const string &extension,
                            Type type, Format format);

    unsigned char *getDirEntryPtr(const string &filename, const string &extension);

    int killFile(const string &filename, const string &extension);

    int commit(ostream &file) const;

    int extractFile(const char *filenameToExtract, const string &cocoFilename, const string &cocoFileExt);

    static void printErrorMessage(CoCoDisk::Error e);

    static bool isEntryFree(const unsigned char *entryPtr)
    {
        if (entryPtr == NULL)
            return false;
        return entryPtr[0] == 0xFF || entryPtr[0] == 0x00;
    }

    // Case-sensitive.
    static bool doesEntryHaveNameAndExt(const unsigned char *entryPtr, const char *nameAndExt)
    {
        if (entryPtr == NULL || nameAndExt == NULL)
            return false;
        return compareWithoutCase((const char *) entryPtr, nameAndExt, 11) == 0;
    }

    static int parseEntry(const unsigned char *entryPtr,
                                string &name,
                                string &ext,
                                Type &type,
                                Format &format,
                                unsigned char &firstGranuleNo,
                                size_t &bytesInLastSector)
    {
        assert(entryPtr != NULL);
        const char *e = reinterpret_cast<const char *>(entryPtr);
        name.assign(e, 0, 8);
        ext.assign(e, 8, 3);

        type = static_cast<Type>(entryPtr[11]);
        if (type != BASIC_PROGRAM && type != BASIC_DATA && type != MACHINE_CODE && type != ASCII_TEXT)
            return -1;

        format = static_cast<Format>(entryPtr[12]);
        if (format != ASCII && format != BINARY)
            return -2;

        firstGranuleNo = entryPtr[13];
        if (firstGranuleNo >= (unsigned char) MAX_GRANULES)
            return -3;

        bytesInLastSector = size_t(entryPtr[14]) << 8 | entryPtr[15];
        if (bytesInLastSector > BYTES_PER_SECTOR)
            return -4;

        return 0;
    }


    // Returns number of sectors in last granule of file.
    // Returns size_t(-1) if the FAT is corrupt.
    //
    size_t getGranuleList(const unsigned char *entryPtr, vector<size_t> &granules)
    {
        granules.clear();
        if (entryPtr == NULL)
            return 0;

        unsigned char granuleNo = entryPtr[13];
        assert(granuleNo < 0xC0);
        const unsigned char *fat = getFAT();
        assert(fat != NULL);
        do
        {
            granules.push_back(granuleNo);
            granuleNo = fat[granuleNo];
            if (granuleNo > 0xC9)
                return size_t(-1);
        } while (granuleNo < 0xC0);

        return granuleNo & 0x0F;
    }


    class DirIterator
    {
    public:

        DirIterator(CoCoDisk &_dsk)
          : dsk(_dsk),
            entryPtr(dsk.getFirstDirEntry())
        {
            assert(entryPtr != NULL);
        }

        bool atEnd() const
        {
            return currentEntryIndex() == NUM_DIR_LIST_ENTRIES;
        }

        int currentEntryIndex() const
        {
            return size_t(entryPtr - dsk.getFirstDirEntry()) / BYTES_PER_DIR_ENTRY;
        }

        unsigned char *currentEntry() const
        {
            return entryPtr;
        }

        void next()
        {
            entryPtr += BYTES_PER_DIR_ENTRY;
        }

    private:

        CoCoDisk &dsk;
        unsigned char *entryPtr;

    };

    static const char *getErrorMessage(Error error);

private:
    ImageFormat imageFormat;
    size_t numTracks;
    unsigned char *contents;

private:
    unsigned char *getFirstDirEntry();
    unsigned char *getGranule(unsigned char granuleNo);

    template <class OutputIterator>
    int allocateGranules(size_t n, OutputIterator it);

    struct ErrorMessage
    {
        Error error;
        const char *message;
    };
    static ErrorMessage errorMessages[];

    // Forbidden:
    CoCoDisk(const CoCoDisk &);
    CoCoDisk &operator = (const CoCoDisk &);
};


CoCoDisk::CoCoDisk(istream &file)
  : imageFormat(RAW_FORMAT),
    numTracks(0),
    contents(NULL)
{
    file.seekg(0, ios::end);
    size_t size = file.tellg();
    numTracks = size / BYTES_PER_TRACK;

    if (numTracks * BYTES_PER_TRACK == size)
        ;
    else if (4 + numTracks * BYTES_PER_TRACK == size)
        imageFormat = JVC_FORMAT;  // this format has 4-byte header
    else
        throw INVALID_IMAGE_FILE_LENGTH;

    if (numTracks != 35)
        throw INVALID_IMAGE_FILE_LENGTH;
    contents = new unsigned char[size];

    file.seekg(getContentOffset(), ios::beg);
    file.read((char *) contents, numTracks * BYTES_PER_TRACK);
    if (!file)
        throw INVALID_IMAGE_FILE;
}


CoCoDisk::~CoCoDisk()
{
    delete [] contents;
}


const char *
CoCoDisk::getImageFormatName() const
{
    switch (imageFormat)
    {
    case RAW_FORMAT: return "raw";
    case JVC_FORMAT: return "JVC";
    default: return "???";
    }
}


size_t
CoCoDisk::getContentOffset() const
{
    return imageFormat == JVC_FORMAT ? 4 : 0;
}


CoCoDisk::ErrorMessage CoCoDisk::errorMessages[] =
{
    { INVALID_IMAGE_FILE_LENGTH, "invalid disk image file length" },
    { INVALID_IMAGE_FILE, "invalid disk image file" },
    { INVALID_FILENAME, "invalid filename" },
    { INVALID_EXTENSION, "invalid extension" },
    { FAILED_TO_FIND_SIZE, "failed to find size of file" },
    { OUT_OF_SPACE, "out of free space on the disk image" },
    { NO_FREE_DIR_ENTRY, "out of free directory entries on the disk image" },
};


const char *
CoCoDisk::getErrorMessage(Error error)
{
    for (size_t i = 0; i < sizeof(errorMessages) / sizeof(errorMessages[0]); ++i)
        if (errorMessages[i].error == error)
            return errorMessages[i].message;
    return "unexpected error code";
}


CoCoDisk::Error
CoCoDisk::addFile(istream &file,
                        const string &filename, const string &extension,
                        Type type, Format format)
{
    if (filename.length() < 1 || filename.length() > 8)
        return INVALID_FILENAME;
    if (extension.length() < 1 || extension.length() > 3)
        return INVALID_EXTENSION;

    /*  Measure file to be added to disk.
    */
    file.seekg(0, ios::end);
    size_t sizeInBytes = file.tellg();
    file.seekg(0, ios::beg);

    if (ssize_t(sizeInBytes) < 0)
    {
        //cout << "Failed to find size of file\n";
        return FAILED_TO_FIND_SIZE;
    }

    if (verbose)
        cout << "File to add has " << sizeInBytes << " bytes\n";

    size_t sizeInGranules =
                (sizeInBytes + BYTES_PER_GRANULE - 1) / BYTES_PER_GRANULE;
    if (sizeInGranules == 0)
        sizeInGranules = 1;

    /*  See if enough free granules can be allocated:
    */
    vector<unsigned char> fileGranules;
    if (allocateGranules(sizeInGranules,
            back_insert_iterator< vector<unsigned char> >(fileGranules)) != 0)
        return OUT_OF_SPACE;

    assert(fileGranules.size() == sizeInGranules);

    /*  Try to allocate a directory entry:
    */
    unsigned char *entryPtr = NULL;
    DirIterator iter(*this);
    for ( ; !iter.atEnd(); iter.next())
    {
        entryPtr = iter.currentEntry();
        if (isEntryFree(entryPtr))
            break;
    }
    if (iter.atEnd())
        return NO_FREE_DIR_ENTRY;


    /*  Prepare the directory entry:
    */
    size_t bytesInLastSector;
    if (sizeInBytes > 0 && sizeInBytes % 256 == 0)
        bytesInLastSector = 256;
    else
        bytesInLastSector = sizeInBytes % 256;

    string paddedFilename = string(filename + "        ", 0, 8);
    string paddedExtension = string(extension + "   ", 0, 3);
    string dirEntry = paddedFilename + paddedExtension
                        + char(type) + char(format)
                        + char(fileGranules[0])
                        + char(bytesInLastSector / 256)
                        + char(bytesInLastSector % 256);

    assert(dirEntry.length() == 16);

    size_t bytesInLastGranule = sizeInBytes % BYTES_PER_GRANULE;
    if (bytesInLastGranule == 0 && sizeInBytes > 0)
        bytesInLastGranule = BYTES_PER_GRANULE;
    size_t sectorsInLastGranule =
                (bytesInLastGranule + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;
    assert(sectorsInLastGranule <= SECTORS_PER_GRANULE);


    /*  Write the directory entry:
    */
    memcpy(entryPtr, dirEntry.data(), dirEntry.length());
    memset(entryPtr + dirEntry.length(), '\0', 16);


    /*  Write to the FAT:
    */
    unsigned char *fat = getFAT();
    for (size_t i = 0; i < sizeInGranules - 1; i++)
        fat[fileGranules[i]] = fileGranules[i + 1];
    fat[fileGranules[sizeInGranules - 1]] = 0xC0 + sectorsInLastGranule;


    /*  Write the file contents:
    */
    bool convertLF = (format == ASCII);
    char granule[BYTES_PER_GRANULE];
    for (size_t i = 0; file.read(granule, BYTES_PER_GRANULE), file.gcount() > 0; i++)
    {
        size_t bytesRead = file.gcount();
        assert(bytesRead > 0);
        assert(bytesRead <= BYTES_PER_GRANULE);

        if (convertLF)
        {
            for (size_t j = file.gcount(); j-- > 0; )
                if (granule[j] == '\n')
                    granule[j] = '\r';
        }

        assert(i < sizeInGranules);
        unsigned char granuleNo = fileGranules[i];
        if (verbose)
            cout << "Writing to granule " << (int) granuleNo << "\n";
        unsigned char *dest = getGranule(granuleNo);
        memcpy(dest, granule, bytesRead);
    }

    return SUCCESS;
}


unsigned char *
CoCoDisk::getFAT()
{
    return contents
                + DIR_TRACK * BYTES_PER_TRACK
                + (DIR_FAT_SECTOR - 1) * BYTES_PER_SECTOR;
}


unsigned char *
CoCoDisk::getFirstDirEntry()
{
    return contents
                + DIR_TRACK * BYTES_PER_TRACK
                + (DIR_LIST_FIRST_SECTOR - 1) * BYTES_PER_SECTOR;
}


unsigned char *
CoCoDisk::getGranule(unsigned char granuleNo)
{
    if (granuleNo < DIR_TRACK * 2)
        return contents + granuleNo * BYTES_PER_GRANULE;
    return contents + BYTES_PER_TRACK + granuleNo * BYTES_PER_GRANULE;
}


template <class OutputIterator>
int
CoCoDisk::allocateGranules(size_t n, OutputIterator it)
{
    if (n < 1)
        return -1;
    const unsigned char *fat = getFAT();
    unsigned char granuleNo = 0;
    unsigned char granulesPerDisk = (numTracks - 1) * GRANULES_PER_TRACK;
    for (size_t i = 0; i < n; i++)
    {
        while (granuleNo < granulesPerDisk && fat[granuleNo] != FREE_FAT_ENTRY)
            granuleNo++;
        if (granuleNo == granulesPerDisk)  // if no free entry in FAT
            return -1;
        *it++ = granuleNo++;
    }
    return 0;
}


unsigned char *
CoCoDisk::getDirEntryPtr(const string &filename, const string &extension)
{
    string paddedFilename = string(filename + "        ", 0, 8);
    string paddedExtension = string(extension + "   ", 0, 3);
    string completeFilename = paddedFilename + paddedExtension;
    assert(completeFilename.length() == 11);
    
    unsigned char *entryPtr = NULL;
    DirIterator iter(*this);
    for ( ; !iter.atEnd(); iter.next())
    {
        entryPtr = iter.currentEntry();
        if (entryPtr[0] == 0xFF)
            return NULL;  // end of dir: fail
        if (doesEntryHaveNameAndExt(entryPtr, completeFilename.data()))
            return entryPtr;  // found entry
    }
    return NULL;
}


int
CoCoDisk::killFile(const string &filename, const string &extension)
{
    unsigned char *entryPtr = getDirEntryPtr(filename, extension);
    if (!entryPtr)
        return -1;

    unsigned char granuleNo = entryPtr[13];
    assert(granuleNo < 0xC0);
    unsigned char *fat = getFAT();
    assert(fat != NULL);
    for (;;)
    {
        if (verbose)
            cout << "Freeing granule " << (int) granuleNo << "\n";
        unsigned char g = fat[granuleNo];
        fat[granuleNo] = FREE_FAT_ENTRY;
        if (g >= 0xC0 && g <= 0xC9)
            break;
        granuleNo = g;
    }

    entryPtr[0] = '\0';

    return 0;
}


int
CoCoDisk::commit(ostream &file) const
{
    file.seekp(ostream::pos_type(getContentOffset()));
    file.write((char *) contents, numTracks * BYTES_PER_TRACK);
    return !file ? -1 : 0;
}


// filenameToExtract: If NULL, write contents to stdout.
//
int
CoCoDisk::extractFile(const char *filenameToExtract, const string &cocoFilename, const string &cocoFileExt)
{
    const unsigned char *entryPtr = getDirEntryPtr(cocoFilename, cocoFileExt);
    if (!entryPtr)
        return -1;

    string name, ext;
    Type type;
    Format format;
    unsigned char firstGranuleNo;
    size_t bytesInLastSector;
    int err = CoCoDisk::parseEntry(entryPtr, name, ext, type, format, firstGranuleNo, bytesInLastSector);
    if (err != 0)
    {
        cout << PROGRAM << ": ERROR: entry for CoCo file "
                << cocoFilename << "." << cocoFileExt
                << " is invalid (error #" << err << ")\n";
        return -100 + err;
    }

    unsigned char granuleNo = entryPtr[13];
    assert(granuleNo < 0xC0);
    const unsigned char *fat = getFAT();
    assert(fat != NULL);
    string fileContents;
    for (;;)
    {
        if (verbose)
            cout << "Reading granule " << (int) granuleNo << "\n";
        unsigned char g = fat[granuleNo];
        if (g >= 0xC0 && g <= 0xC9)  // if last granule
        {
            size_t sectorsLastGranule = g & 0x0F;
            if (verbose)
                cout << sectorsLastGranule << " sector(s) in last granule\n"
                     << bytesInLastSector << " byte(s) in last sector\n";

            if (sectorsLastGranule > 0)
            {
                const unsigned char *lastGranule = getGranule(granuleNo);
                fileContents.append((const char *) lastGranule, (sectorsLastGranule - 1) * BYTES_PER_SECTOR + bytesInLastSector);
            }
            break;
        }

        const unsigned char *currentGranule = getGranule(granuleNo);
        fileContents.append((const char *) currentGranule, SECTORS_PER_GRANULE * BYTES_PER_SECTOR);
        granuleNo = g;
    }

    if (verbose)
        cout << "Total file contents: " << fileContents.length() << " byte(s)\n";

    ofstream nativeFile;
    ostream *pDestFile = &cout;
    if (filenameToExtract != NULL)
    {
        nativeFile.open(filenameToExtract, ios::binary);
        if (!nativeFile)
        {
            cout << PROGRAM << ": ERROR: failed to create native file " << filenameToExtract << "\n";
            return EXIT_FAILURE;
        }
        pDestFile = &nativeFile;
    }

    pDestFile->write(fileContents.c_str(), fileContents.length());
    if (!pDestFile->good())
        return -2;

    return 0;
}


void
CoCoDisk::printErrorMessage(CoCoDisk::Error e)
{
    cout << PROGRAM << ": ERROR: ";
    switch (e)
    {
        case CoCoDisk::INVALID_IMAGE_FILE_LENGTH:
            cout << "length of disk image file is unexpected: format may not be supported";
            break;
        case CoCoDisk::INVALID_IMAGE_FILE:
            cout << "failed to read contents of disk image file";
            break;
        default:
            cout << "unexpected error: report this bug to the author";
    }
    cout << endl;
}


static string
uc(const string &s)
{
    string result;
    for (size_t i = 0; i < s.length(); ++i)
        result += toupper(s[i]);
    return result;
}


// Returns name and extension of basename of 'filename', converted to uppercase.
// The extension is returned without the dot.
// Example: /tmp/foo.bin returns "FOO" and "BIN".
//
static void
parseFilename(const string &filename, string &nameWithoutExt, string &ext)
{
    string::size_type lastSlash = filename.rfind('/');
    string::size_type basenamePos = (lastSlash == string::npos ? 0 : lastSlash + 1);

    nameWithoutExt.assign(filename, basenamePos, string::npos);

    string::size_type lastDot = nameWithoutExt.rfind('.');
    if (lastDot == string::npos)  // if no extension
    {
        ext.clear();
    }
    else
    {
        ext.assign(nameWithoutExt, lastDot + 1, string::npos);
        nameWithoutExt.erase(lastDot);  // remove extension
    }

    // CoCo filenames should be in upper-case.
    nameWithoutExt = uc(nameWithoutExt);
    ext = uc(ext);
}


static void
displayVersionNo()
{
    cout << PROGRAM << " (" << PACKAGE << " " << VERSION << ")\n";
}


static void
displayHelp()
{
    cout << "\n";
    displayVersionNo();
    cout << "\n";
    cout << "Copyright (C) 2003-2015 Pierre Sarrazin <http://sarrazip.com/>\n";
    cout <<
"This program is free software; you may redistribute it under the terms of\n"
"the GNU General Public License.  This program has absolutely no warranty.\n";

    cout << "\n"
        << PROGRAM << " IMAGE.DSK [NATIVE_FILE]\n"
        "\n"
        "Writes (or reads) a native file to (or from) a Color Computer Disk Basic\n"
        "diskette image. This image must have 35 tracks of 18 sectors of 256 bytes,\n"
        "for a total size of 161280 bytes.\n"
        "\n"
        "--help|-h        Display this help page and exit.\n"
        "--version|-v     Display this program's version number and exit.\n"
        "--verbose        Print more details of what is happening.\n"
        "--format=F       Specify the format of the file to write ('binary' or 'ascii').\n"
        "-b or --binary   Short for --format=binary.\n"
        "-a or --ascii    Short for --format=ascii.\n"
        "--newlines|-n    Like --ascii, but converts newlines from native to CoCo and\n"
        "                 prepends an empty line (useful to transfer ASCII Basic programs.)\n"
        "--dir|-d         List the contents of the disk's directory.\n"
        "--kill|-k        Kill the designated file.\n"
        "--read|-r        Read instead of writing. Refuses to overwrite existing native file.\n"
        "--stdout|-s      Like --read, but print file contents on standard out.\n"
        "\n";
}


static int
readFile(const char *dskFilename, const char *filenameToRead)
{
    if (!printToStdOut)
    {
        struct stat statbuf;
        if (stat(filenameToRead, &statbuf) == 0)
        {
            cout << PROGRAM << ": ERROR: native file " << filenameToRead << " already exists\n";
            return EXIT_FAILURE;
        }
    }

    string cocoFilename, cocoFileExt;
    parseFilename(filenameToRead, cocoFilename, cocoFileExt);

    fstream dskFile(dskFilename, ios::in | ios::out);
    if (!dskFile)
    {
        int e = errno;
        cout << PROGRAM << ": ERROR: failed to open disk image file "
             << dskFilename << ": " << strerror(e) << endl;
        return EXIT_FAILURE;
    }

    try
    {
        CoCoDisk theCoCoDisk(dskFile);
        int errorCode = theCoCoDisk.extractFile(printToStdOut ? NULL : filenameToRead, cocoFilename, cocoFileExt);
        switch (errorCode)
        {
        case -1:
            cout << PROGRAM << ": ERROR: file " << filenameToRead << " not found on disk image " << dskFilename << "\n";
            return EXIT_FAILURE;
        case -2:
            cout << PROGRAM << ": ERROR: failed to write to native file " << filenameToRead << "\n";
            return EXIT_FAILURE;
        }

        if (!printToStdOut)
            cout << "Wrote native file " << filenameToRead << "\n";
    }
    catch (CoCoDisk::Error &e)
    {
        CoCoDisk::printErrorMessage(e);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// format ignored if killOnly true.
static int
killAndWriteFile(const char *dskFilename, const char *filenameToAdd,
                CoCoDisk::Format format, bool killOnly)
{
    string cocoFilename, cocoFileExt;
    parseFilename(filenameToAdd, cocoFilename, cocoFileExt);

    CoCoDisk::Type type = CoCoDisk::BASIC_DATA;
    if (cocoFileExt == "BAS")
        type = CoCoDisk::BASIC_PROGRAM;
    else if (cocoFileExt == "BIN")
        type = CoCoDisk::MACHINE_CODE;
    else if (cocoFileExt == "ASM")
        type = CoCoDisk::ASCII_TEXT;

    if (verbose && !killOnly)
        cout << "Writing native file " << filenameToAdd
            << " to CoCo file " << cocoFilename << "." << cocoFileExt
            << " as file type " << CoCoDisk::getFileTypeName(type)
            << " with format " << CoCoDisk::getFormatName(format)
            << "\n";

    try
    {
        fstream dskFile(dskFilename, ios::in | ios::out);
        if (!dskFile)
        {
            int e = errno;
            cout << PROGRAM << ": ERROR: failed to open disk image file "
                 << dskFilename << ": " << strerror(e) << endl;
            return EXIT_FAILURE;
        }

        ifstream fileToAdd;
        stringstream asciiBasicMemoryFile;
        istream *pFileToAdd = &fileToAdd;

        if (!killOnly)
        {
            fileToAdd.open(filenameToAdd, ios::in | (convertASCIIBasicNewlines ? ios_base::openmode(0) : ios::binary));
            if (!fileToAdd)
            {
                cout << PROGRAM << ": ERROR: native file " << filenameToAdd << " not found\n";
                return EXIT_FAILURE;
            }
            if (convertASCIIBasicNewlines)
            {
                asciiBasicMemoryFile << '\r';  // first line of DECB ASCII Basic file must be empty
                for (size_t lineNo = 1; ; ++lineNo)
                {
                    char line[512];
                    fileToAdd.getline(line, sizeof(line));
                    if (fileToAdd.eof())
                        break;
                    if (fileToAdd.fail())
                    {
                        cout << PROGRAM << ": ERROR: line " << lineNo << " of native file " << filenameToAdd << " is too long\n";
                        return EXIT_FAILURE;
                    }
                    asciiBasicMemoryFile << line << '\r';
                }
                pFileToAdd = &asciiBasicMemoryFile;  // pass converted text to addFile() instead of native file
            }
        }

        CoCoDisk theCoCoDisk(dskFile);
        if (theCoCoDisk.killFile(cocoFilename, cocoFileExt) != 0 && killOnly)
            cout << PROGRAM << ": killFile: " << cocoFilename << "." << cocoFileExt << " not found\n";

        if (!killOnly)
        {
            CoCoDisk::Error error = theCoCoDisk.addFile(*pFileToAdd, cocoFilename, cocoFileExt, type, format);
            if (error != CoCoDisk::SUCCESS)
            {
                cout << PROGRAM << ": ERROR: failed to add file: " << CoCoDisk::getErrorMessage(error) << endl;
                return EXIT_FAILURE;
            }
        }

        if (theCoCoDisk.commit(dskFile) != 0)
        {
            cout << PROGRAM << ": ERROR: commit failed\n";
            return EXIT_FAILURE;
        }
    }
    catch (CoCoDisk::Error &e)
    {
        CoCoDisk::printErrorMessage(e);
        return EXIT_FAILURE;
    }

    if (verbose)
        cout << "Success." << endl;
    return EXIT_SUCCESS;
}


enum GranState { FREE, ALLOCATED, LOST };


int listDirectory(const char *dskFilename)
{
    try
    {
        // Try to open the disk image file.
        //
        fstream dskFile(dskFilename, ios::in);
        if (!dskFile)
        {
            int e = errno;
            cout << PROGRAM << ": failed to open image file " << dskFilename
                    << ": " << strerror(e) << endl;
            return EXIT_FAILURE;
        }
        CoCoDisk theCoCoDisk(dskFile);


        // List the FAT.
        //
        vector<GranState> granStates;
        granStates.resize(CoCoDisk::MAX_GRANULES, LOST);

        cout << "File Allocation Table (" << CoCoDisk::MAX_GRANULES << " entries):\n";
        cout << "     ";
        for (int i = 0; i < 16; ++i)
            cout << setw(3) << i << " ";
        cout << '\n';
        cout << "     ";
        for (int i = 0; i < 16; ++i)
            cout << "--- ";
        cout << '\n';

        const unsigned char *fat = theCoCoDisk.getFAT();
        for (size_t g = 0; g < CoCoDisk::MAX_GRANULES; ++g)
        {
            if (g % 16 == 0)
                cout << setw(2) << g << ":  ";

            unsigned entry = unsigned(fat[g]);
            if (entry == 0xFF)   // if free granule
            {
                cout << "___";
                granStates[g] = FREE;
            }
            else if (entry < 0xC0)  // if pointer to next file granule
                cout << setw(3) << entry;
            else  // last granule of file:
                cout << setw(2) << (entry & ~0xC0) << '*';

            if (g % 16 < 15)
                cout << " ";
            else
                cout << '\n';
        }
        cout << '\n';
        cout << '\n';


        // Iterate through the directory entries.
        //
        bool corrupt = false;
        size_t numGransUsedByDir = 0;
        CoCoDisk::DirIterator iter(theCoCoDisk);

        for (size_t entryIndex = 0; !iter.atEnd(); iter.next(), ++entryIndex)
        {
            const unsigned char *entryPtr = iter.currentEntry();
            if (entryPtr[0] == 0xFF)
                break;  // end of dir
            if (CoCoDisk::isEntryFree(entryPtr))
                continue;
            string name, ext;
            CoCoDisk::Type type = CoCoDisk::BASIC_PROGRAM;
            CoCoDisk::Format format = CoCoDisk::BINARY;
            unsigned char firstGranuleNo = 0;
            size_t bytesInLastSector = 0;
            int err = CoCoDisk::parseEntry(entryPtr, name, ext, type, format, firstGranuleNo, bytesInLastSector);
            if (err != 0)
            {
                cout << PROGRAM << ": warning: entry #" << entryIndex << " of image file " << dskFilename
                        << " is invalid (error #" << err << ")\n";
                //return EXIT_FAILURE;
            }

            vector<size_t> granules;
            size_t numSectorsLastGranule = theCoCoDisk.getGranuleList(entryPtr, granules);
            size_t fileLen = 0;
            if (numSectorsLastGranule == size_t(-1))
                corrupt = true;
            else if (granules.size() == 0)
                fileLen = 0;
            else
            {
                fileLen = (granules.size() - 1) * CoCoDisk::BYTES_PER_GRANULE
                          + bytesInLastSector;
                if (numSectorsLastGranule > 0)
                    fileLen += (numSectorsLastGranule - 1) * CoCoDisk::BYTES_PER_SECTOR;
            }

            cout << setw(3) << entryIndex << ".  "
                        << name << "." << ext
                        << "  " << setw(7) << left << CoCoDisk::getFileTypeName(type)
                        << "  " << setw(6) << left << CoCoDisk::getFormatName(format)
                        << "  " << setw(6) << right << fileLen
                        << "  " << setw(3) << right << bytesInLastSector
                        << "  " << setw(2) << right << granules.size();

            // List granules.
            if (numSectorsLastGranule == size_t(-1))  // if corruption
            {
                cout << " CORRUPT";
                corrupt = true;
            }

            cout << "  {";
            for (vector<size_t>::const_iterator g = granules.begin(); g != granules.end(); ++g)
            {
                cout << " " << setw(2) << *g;
                if (*g >= CoCoDisk::MAX_GRANULES)
                    corrupt = true;
                granStates[*g] = ALLOCATED;
            }
            cout << " }";

            cout << "\n";

            numGransUsedByDir += granules.size();
        }

        size_t numFreeGranules = 0;
        bool foundLostGrans = false;
        for (size_t g = 0; g < CoCoDisk::MAX_GRANULES; ++g)
        {
            if (granStates[g] == LOST)
            {
                if (g != 66 && g != 67)  // don't warn about granules that may be a DOS-command loader
                {
                    if (!foundLostGrans)
                        cout << "\n*** WARNING: LOST GRANULES:";
                    cout << ' ' << g;
                    foundLostGrans = true;
                }
            }
            else if (granStates[g] == FREE)
                ++numFreeGranules;
        }
        if (foundLostGrans)
            cout << '\n';

        size_t numBytesUsedByDir = numGransUsedByDir * CoCoDisk::BYTES_PER_GRANULE;
        size_t numFreeBytes = numFreeGranules * CoCoDisk::BYTES_PER_GRANULE;

        cout << "\n";
        cout << setw(6) << numFreeGranules << " granule(s) free ("
                        << setprecision(3) << 100.0 * numFreeGranules / CoCoDisk::MAX_GRANULES << "%)\n";
        cout << setw(6) << numFreeBytes << " byte(s) free\n";
        cout << setw(6) << numGransUsedByDir << " granule(s) used by directory entries\n";
        cout << setw(6) << numBytesUsedByDir << " byte(s) used by directory entries\n";

        if (corrupt)
        {
            cout << "\n*** WARNING: DISK IS CORRUPT.\n\n";
            return EXIT_FAILURE;
        }
    }
    catch (CoCoDisk::Error &e)
    {
        cout << PROGRAM << ": error #" << int(e)
             << " (" << CoCoDisk::getErrorMessage(e) << ")" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


int
main(int argc, char *argv[])
{
    enum Mode { WRITE_FILE, KILL_FILE, LIST_DIRECTORY, READ_FILE } mode = WRITE_FILE;

    CoCoDisk::Format format = CoCoDisk::BINARY;


    /*  Process command-line arguments.
    */
    int optind = 1;
    for (optind = 1; optind < argc; ++optind)
    {
        string curopt = argv[optind];

        if (curopt == "--version" || curopt == "-v")
        {
            displayVersionNo();
            return EXIT_SUCCESS;
        }
        if (curopt == "--help" || curopt == "-h")
        {
            displayHelp();
            return EXIT_SUCCESS;
        }
        if (curopt == "--verbose")
        {
            verbose = true;
            continue;
        }
        if (curopt.compare(0, 9, "--format=") == 0)
        {
            string arg(curopt, 9, string::npos);
            if (arg == "b" || arg == "bin" || arg == "binary")
                format = CoCoDisk::BINARY;
            else if (arg == "a" || arg == "asc" || arg == "ascii")
                format = CoCoDisk::ASCII;
            else
            {
                cout << PROGRAM << ": "
                        << "invalid argument for --format switch:"
                        << " " << arg << "\n\n";
                displayHelp();
                return EXIT_FAILURE;
            }
            continue;
        }
        if (curopt == "--binary" || curopt == "-b")
        {
            format = CoCoDisk::BINARY;
            continue;
        }
        if (curopt == "--ascii" || curopt == "-a")
        {
            format = CoCoDisk::ASCII;
            continue;
        }
        if (curopt == "--newlines" || curopt == "-n")
        {
            format = CoCoDisk::ASCII;
            convertASCIIBasicNewlines = true;
            continue;
        }
        if (curopt == "--dir" || curopt == "-d")
        {
            mode = LIST_DIRECTORY;
            continue;
        }
        if (curopt == "--kill" || curopt == "-k")
        {
            mode = KILL_FILE;
            continue;
        }
        if (curopt == "--read" || curopt == "-r")
        {
            mode = READ_FILE;
            continue;
        }
        if (curopt == "--stdout" || curopt == "-s")
        {
            mode = READ_FILE;
            printToStdOut = true;
            continue;
        }


        if (curopt.empty() || curopt[0] == '-')
        {
            cout << PROGRAM << ": Invalid option: " << curopt << "\n";
            displayHelp();
            return EXIT_FAILURE;
        }

        break;  // end of options; optind now designated 1st non-option argument
    }

    if (optind >= argc)
    {
        displayHelp();
        return EXIT_FAILURE;
    }


    /*  Execute user request.
    */
    const char *dskFilename = argv[optind++];

    switch (mode)
    {
        case WRITE_FILE:
        case KILL_FILE:
            if (optind + 1 != argc)  // if missing filename or extra argument
            {
                cout << PROGRAM << ": error: " << (optind == argc ? "missing filename" : "extra argument(s)") << "\n";
                displayHelp();
                return EXIT_FAILURE;
            }
            return killAndWriteFile(dskFilename, argv[optind], format, mode == KILL_FILE);

        case LIST_DIRECTORY:
            if (optind != argc)  // if extra argument
            {
                cout << PROGRAM << ": error: " << "extra argument(s)" << "\n";
                displayHelp();
                return EXIT_FAILURE;
            }
            return listDirectory(dskFilename);

        case READ_FILE:
            if (optind + 1 != argc)  // if missing filename or extra argument
            {
                cout << PROGRAM << ": error: " << (optind == argc ? "missing filename" : "extra argument(s)") << "\n";
                displayHelp();
                return EXIT_FAILURE;
            }
            return readFile(dskFilename, argv[optind]);
    }

    return EXIT_SUCCESS;
}
