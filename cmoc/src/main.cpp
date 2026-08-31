/*  CMOC - A C-like cross-compiler
    Copyright (C) 2003-2025 Pierre Sarrazin <http://sarrazip.com/>

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

#include "TranslationUnit.h"
#include "FunctionDef.h"
#include "Parameters.h"

#include <iomanip>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__MINGW32__)
// MinGW has no <sys/wait.h>; system() returns the child's exit code directly.
#define WIFEXITED(x)   ((unsigned) (x) < 259)
#define WEXITSTATUS(x) ((x) & 0xff)
#else
#include <sys/wait.h>
#endif

using namespace std;


// Quote a filename or argument for a shell command line. cmoc.exe is a native
// Windows binary whose system()/popen() run through cmd.exe, which does not
// honor single-quote quoting; double quotes are used there. sh-based platforms
// (macOS, Linux) keep single quotes.
static string
shq(const string &s)
{
#if defined(__MINGW32__)
    return "\"" + s + "\"";
#else
    return "'" + s + "'";
#endif
}


extern int numErrors;


const char *fatalErrorPrefix = ": fatal error: ";


static bool removeFile(const string &path);


// If the line has the form "Symbol: SYMBOLNAME (OBJECTFILENAME) = HEXADDR",
// then: SYMBOLNAME is put in symbolName,
//       OBJECTFILENAME is put in objectFilename,
//       the value of HEXADDR is put in in symbolValue,
//       and true is returned.
// HEXADDR may be more than 4 digits. This is why symbolValue is a uint32_t.
// Returns false if the line does not have the expected form.
//
static bool
parseLinkerMapSymbolLine(const string &line,
                         string &symbolName,
                         string &objectFilename,
                         uint32_t &symbolValue)
{
    if (line.find("Symbol: ") != 0)
        return false;

    size_t posAfterSymbolName = line.find(" (");
    if (posAfterSymbolName == string::npos)
        return false;

    size_t posAfterFilename = line.find(") = ");
    if (posAfterFilename == string::npos)
        return false;

    char *endptr = NULL;
    unsigned long a = strtoul(line.c_str() + posAfterFilename + 4, &endptr, 16);
    if (a > 0xFFFFFFFFul)
        return false;

    symbolName = line.substr(8, posAfterSymbolName - 8);
    objectFilename = line.substr(posAfterSymbolName + 2, posAfterFilename - (posAfterSymbolName + 2));
    symbolValue = uint32_t(a);
    return true;
}


static string
getDefaultOutputExtension(TargetPlatform p, const string &forcedLWLinkFormat)
{
    if (!forcedLWLinkFormat.empty())
        return "." + forcedLWLinkFormat;
    switch (p)
    {
    case COCO_BASIC:  return ".bin";
    case USIM:        return ".srec";
    case VECTREX:     return ".bin";
    case OS9:         return "";
    case DRAGON:      return ".bin";
    case VOID_TARGET: return ".srec";
    case THOMMO:      return ".bin";
    case THOMTO:      return ".bin";
    case FLEX:        return ".cmd";
    case INTERM_REP:  return ".json";
    }
    return "";
}


// Create a link script to tell lwlink in which order to write out the sections to the executable file.
// The first section is "start": it contains only the program_start routine.
// We want the start of the binary to be the entry point.
//
static bool
createLinkScript(const string &linkScriptFilename,
                 TargetPlatform targetPlatform,
                 uint16_t codeAddress,
                 uint16_t dataAddress)
{
    ofstream linkScript(linkScriptFilename.c_str(), ios::trunc);
    if (!linkScript)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "could not create link script " + linkScriptFilename + ": "
                                            << strerror(e) << endl;
        return false;
    }

    // N.B.: The initgl_end section must be the last code section listed, because it defines program_end.
    //       sbrk() and sbrkmax() rely on program_end.

    const char *constructorSections =
                  "section constructors_start\n"  // defines "constructors" labels
                  "section constructors\n"
                  "section constructors_end\n";   // RTS
    const char *destructorSections =
                  "section destructors_start\n"   // defines "denstructors" labels
                  "section destructors\n"
                  "section destructors_end\n";    // RTS
    const char *initGLSections =
                  "section initgl_start\n"  // defines INITGL label
                  "section initgl\n"        // global variable initialization code
                  "section initgl_end\n";   // RTS

    // Have the linker define a symbol for the ultimate base address and length of each section.
    // See the LWLINK manual (section 4.3).
    linkScript << "define basesympat s_%s\n"
                  "define lensympat l_%s\n";

    linkScript << "section start load " << hex << codeAddress << "\n"
                  "section code\n";  // main code section

    linkScript << constructorSections << destructorSections << initGLSections;

    // Put the read-only data next to the code.
    // Under OS-9, references to read-only data are made with ,PCR (see TranslationUnit's
    // getDataIndexRegister() and getLiteralIndexRegister()).
    // When targeting a CoCo cartridge, the read-only data goes in the ROM with the code.
    //
    linkScript << "section rodata\n";  // for OS-9, this must be last section of ,PCR segment

    if (targetPlatform == OS9)
    {
        // On OS-9, the end of the program is right after the code and read-only globals.
        // Those are typically in high memory, while the rwdata and bss sections are typically
        // in lower memory, in the data segment.
        //
        linkScript << "section program_end\n";
    }

    // The writable data either follows, or it is positioned in a separate segment.
    // The latter is the case for OS-9 (where the writable data is in a segment designated by Y).
    // It is also the case if --data was passed.
    //
    linkScript << "section rwdata";

    if (targetPlatform == OS9)
        linkScript << " load 1";  // do not allocate data to offset 0, to distinguish (void *) 0 as invalid pointer
    else if (dataAddress != 0xFFFF)
        linkScript << " load " << dataAddress;

    linkScript << "\n";

    // Data area that does not receive initial values from the executable file.
    // It comes last to avoid a hole in the executable.
    //
    linkScript << "section bss,bss\n";

    if (targetPlatform != OS9)
    {
        // Emit a section that defines the program_end label.
        // This must be the last section in the script.
        //
        linkScript << "section program_end\n";
    }

    linkScript << "entry program_start\n" << dec;

    linkScript.close();
    if (!linkScript)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "could not close link script " + linkScriptFilename + ": "
                                            << strerror(e) << endl;
        return false;
    }

    return true;
}


// If a limit address must be enforced, looks for 'program_end' in the link map file.
// Checks for and reports multiply defined symbols.
//
static int
checkLinkingMap(uint16_t limitAddress,
                const string &mapFilename,
                const string &outputFilename,
                bool verbose)
{
    ifstream mapFile(mapFilename.c_str());
    if (!mapFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to open linker map file " << mapFilename << endl;
        return EXIT_FAILURE;
    }

    typedef multimap<string, string> SymbolMap;  // key: symbol name; value(s): object filenames

    SymbolMap externalLinkageSymbolMap;
    set<string> internalLinkageSymbolMap;
    bool programEndFound = false;
    int status = EXIT_SUCCESS;

    string line, symbolName, objectFilename;
    while (getline(mapFile, line))
    {
        // If the line defines a symbol, remember the name and its object filename,
        // in case multiple definitions are seen.
        // Also check for program_end vs. limitAddress.
        //
        uint32_t symbolValue = 0;
        if (parseLinkerMapSymbolLine(line, symbolName, objectFilename, symbolValue))
        {
            if (limitAddress != 0xFFFF && symbolName == "program_end")
            {
                programEndFound = true;
                if (symbolValue > limitAddress)
                {
                    cout << PACKAGE << fatalErrorPrefix << "code limit set at $" << hex << limitAddress
                                    << " but program_end exceeds it at $" << symbolValue << dec << endl;
                    status = EXIT_FAILURE;
                }
            }

            // Ignore symbols that come from CMOC's standard library (.decb_o, etc.)
            //
            if (endsWith(objectFilename, "_o"))
                continue;

            // We only care about underscore-led names because the map will mention any symbol
            // that appears in the linked modules, even if they were not exported from their module.
            //
            // Global static variables use a prefix does not start with an underscore, as of CMOC 0.1.83.
            // Those variables have internal linkage, so they are not put in externalLinkageSymbolMap.
            //
            bool b = false;
            if (symbolName[0] == '_')
                externalLinkageSymbolMap.insert(make_pair(symbolName, objectFilename));
            else if ((b = startsWith(symbolName, ".global.static.variable.")) || startsWith(symbolName, ".static.function."))
            {
                string key = "_" + string(symbolName, b ? 24 : 17);  // keep ID that follows long prefix, prepend underscore
                internalLinkageSymbolMap.insert(key);
            }
        }
    }

    if (limitAddress != 0xFFFF && !programEndFound)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to find value of program_end symbol in " << mapFilename << endl;
        status = EXIT_FAILURE;
    }

    // Check for multiple definitions among the symbols that have external linkage.
    //
    SymbolMap::const_iterator firstItSameName = externalLinkageSymbolMap.end();
    vector<string> objectFilenames;
    auto issueDiagnosticIfRepetition = [&]()  // local function
    {
        if (firstItSameName != externalLinkageSymbolMap.end() && objectFilenames.size() > 0)
        {
            const string &sym = firstItSameName->first;
            if (internalLinkageSymbolMap.find(sym) == internalLinkageSymbolMap.end())  // ignore static symbols
            {
                status = EXIT_FAILURE;
                cout << PACKAGE << ": error: multiple definitions of symbol " << sym << " in modules ";
                for (vector<string>::const_iterator m = objectFilenames.begin(); m != objectFilenames.end(); ++m)
                {
                    if (m != objectFilenames.begin())
                        cout << ", ";
                    cout << *m;
                }
                cout << endl;
            }
            objectFilenames.clear();
        }
    };

    for (SymbolMap::const_iterator it = externalLinkageSymbolMap.begin(); it != externalLinkageSymbolMap.end(); ++it)
    {
        const string &id = it->first;
        if (firstItSameName != externalLinkageSymbolMap.end() && firstItSameName->first == id)
        {
            const string &objectFilename = it->second;
            objectFilenames.push_back(firstItSameName->second);
            objectFilenames.push_back(objectFilename);
        }
        else
        {
            issueDiagnosticIfRepetition();
            firstItSameName = it;
        }
    }
    issueDiagnosticIfRepetition();

    if (status != EXIT_SUCCESS)
    {
        if (verbose)
            cout << PACKAGE << ": deleting output file " << outputFilename << endl;
        removeFile(outputFilename);
    };

    return status;
}


// Upon error, prints an error message to cout and calls exit(1).
// A warning may be issued to cout.
// Returns true iff no warning was issued.
//
static bool
removeFile(const string &path)
{
    // Belt and suspenders: refuse to remove a C file.
    if (endsWith(path, ".c") || endsWith(path, ".h"))
    {
        cout << PACKAGE << fatalErrorPrefix << "refusing to remove " << path << "\n";
        exit(1);
    }

    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0)
    {
        int e = errno;
        if (e == ENOENT)
            return true;  // file does not exist; nothing to do
        cout << PACKAGE << ": warning: failed to stat " << path << ": " << strerror(e) << "\n";
        return false;
    }

    if (unlink(path.c_str()) != 0)
    {
        int e = errno;
        cout << PACKAGE << ": warning: failed to remove " << path << ": " << strerror(e) << "\n";
        return false;
    }

    return true;
}


static void
removeIntermediateCompilationFiles(const vector<string> &intermediateCompilationFiles)
{
    for (vector<string>::const_iterator it = intermediateCompilationFiles.begin();
                                       it != intermediateCompilationFiles.end(); ++it)
        removeFile(*it);
}


static void
removeIntermediateLinkingFiles(const string &linkScriptFilename,
                               const string &mapFilename,
                               const vector<string> &intermediateObjectFiles)
{
    removeFile(linkScriptFilename);
    removeFile(mapFilename);

    for (vector<string>::const_iterator it = intermediateObjectFiles.begin();
                                       it != intermediateObjectFiles.end(); ++it)
        removeFile(*it);
}


static int
invokeLinker(const Parameters &params,
             const vector<string> &objectFilenames,
             const vector<string> &libraryFilenames,  // allowed to contain -l<name> elements
             const string &linkScriptFilename,
             const string &mapFilename,
             const string &outputFilename)
{
    assert(!outputFilename.empty());

    if (!createLinkScript(linkScriptFilename, params.targetPlatform, params.codeAddress, params.dataAddress))
        return EXIT_FAILURE;

    // targetKW is the name that appears in the '*' position in the libcmoc-crt-*.a filename.
    // lwlinkFormat is an argument to lwlink's --format option.
    //
    const char *targetKW = NULL;
    string lwlinkFormat;
    switch (params.targetPlatform)
    {
    case COCO_BASIC:  targetKW = "ecb";  lwlinkFormat = "decb"; break;
    case OS9:         targetKW = "os9";  lwlinkFormat = "os9";  break;
    case USIM:        targetKW = "usim"; lwlinkFormat = "srec"; break;
    case VECTREX:     targetKW = "vec";  lwlinkFormat = "raw";  break;
    case DRAGON:      targetKW = "dgn";  lwlinkFormat = "decb"; break;
    case VOID_TARGET: targetKW = "void"; lwlinkFormat = "srec"; break;
    case THOMMO:      targetKW = "thm";  lwlinkFormat = "decb"; break;
    case THOMTO:      targetKW = "tht";  lwlinkFormat = "decb"; break;
    case FLEX:        targetKW = "flex"; lwlinkFormat = "srec"; break;
    case INTERM_REP:  targetKW = "void"; lwlinkFormat = "NONE"; break;
    }
    if (!params.forcedLWLinkFormat.empty())
        lwlinkFormat = params.forcedLWLinkFormat;

    string lwlinkCmdLine = params.lwlinkPath
                           + " --format=" + lwlinkFormat
                           + " --output=" + shq(outputFilename)
                           + " --script=" + shq(linkScriptFilename)
                           + " --map=" + shq(mapFilename);
    for (vector<string>::const_iterator it = params.libDirs.begin(); it != params.libDirs.end(); ++it)
        lwlinkCmdLine += " -L" + shq(*it);

    lwlinkCmdLine += " -L" + params.cmocfloatlibdir;
    lwlinkCmdLine += " -lcmoc-crt-" + string(targetKW);
    if (params.useDefaultLibraries)
    {
        lwlinkCmdLine += " -lcmoc-std-" + string(targetKW);

        const char *floatKW = NULL;
        if ((params.targetPlatform == COCO_BASIC || params.targetPlatform == DRAGON) && params.floatingPointLibrary == FloatingPointLibrary::ECB_ROM)
            floatKW = targetKW;  // use a library that calls ECB or Dragon Basic float routines
        else if (params.floatingPointLibrary == FloatingPointLibrary::NATIVE_LIB)  // if --native-float explicitly given
            floatKW = "native";  // use a library that calls the Floatable library
        else if (params.floatingPointLibrary == FloatingPointLibrary::MC6839_LIB)  // if --mc6839 explicitly given
            floatKW = (params.targetPlatform == OS9 ? "mc6839_os9" : "mc6839");

        if (floatKW != NULL)
            lwlinkCmdLine += " " + params.cmocfloatlibdir + "/float-ctor." + floatKW + "_o -lcmoc-float-" + floatKW;
    }

    for (vector<string>::const_iterator it = objectFilenames.begin();
                                       it != objectFilenames.end(); ++it)
        lwlinkCmdLine += " " + shq(params.useIntDir(*it) + ".o");

    for (vector<string>::const_iterator it = libraryFilenames.begin();
                                       it != libraryFilenames.end(); ++it)
        lwlinkCmdLine += " " + shq(*it);

    if (params.verbose)
        cout << "Linker command: " << lwlinkCmdLine << endl;

    // Start the linker through a pipe and redirect the linker's stderr to that pipe.
    //
    FILE *lwlinkPipe = popen((lwlinkCmdLine + " 2>&1").c_str(), "r");
    if (!lwlinkPipe)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "failed to start linker: " << strerror(e) << endl;
        return EXIT_FAILURE;
    }

    PipeCloser closer("linker", lwlinkPipe);

    // Print every line from the linker. Tag non-warning messages as errors.
    // Filter out some unneeded warnings.
    //
    size_t numLinkerErrors = 0;
    char line[512];
    while (fgets(line, sizeof(line), lwlinkPipe))
    {
        if (strstr(line, "does not resolve any symbols") != NULL)
            continue;
        cout << "lwlink: ";
        if (strstr(line, "Warning:") == NULL)
        {
            cout << "error: ";
            ++numLinkerErrors;
        }
        cout << line;
    }

    int status = closer.close();

    if (params.verbose)
    {
        cout << "Exit code from linker command: " << WEXITSTATUS(status) << "\n";
        cout << "Number of error messages from linker: " << numLinkerErrors << "\n";
    }

    if (status != 0 || numErrors > 0)
        return EXIT_FAILURE;

    return checkLinkingMap(params.limitAddress, mapFilename, outputFilename, params.verbose);
}


class BinFormatBlock
{
public:
    bool isPreamble;
    uint16_t length;
    uint16_t address;
    vector<uint8_t> contents;

    BinFormatBlock() : isPreamble(false), length(0), address(0), contents() {}
    uint16_t end() const { return address + length; }
};


static bool
readBinFormatBlock(ifstream &decbBinFile,
                   BinFormatBlock &block)
{
    unsigned char tmp[5];
    decbBinFile.read((char *) tmp, 5);
    if (!decbBinFile || decbBinFile.gcount() != 5)
        return false;
    if (tmp[0] != 0 && tmp[0] != 0xFF)
        return false;
    block.isPreamble = (tmp[0] == 0);
    block.length  = uint16_t(tmp[1]) << 8 | tmp[2];  // interpret big endian
    block.address = uint16_t(tmp[3]) << 8 | tmp[4];  // ditto

    // Read the data that the block announces.
    if (block.length > 0)
    {
        block.contents.resize(block.length);
        decbBinFile.read((char *) block.contents.data(), block.length);
        if (!decbBinFile || decbBinFile.gcount() != block.length)
            return false;
    }

    return true;
}


static vector<uint8_t>
formSingleBlock(const vector<BinFormatBlock> &blocks, uint16_t &startAddr)
{
    if (blocks.size() == 0)
        return vector<uint8_t>();

    // Get the global start and end addresses.
    startAddr = 0xFFFF;
    uint32_t endAddr = 0;
    for (vector<BinFormatBlock>::const_iterator it = blocks.begin(); it != blocks.end(); ++it)
    {
        if (it->address < startAddr)
            startAddr = it->address;
        if (it->end() > endAddr)
            endAddr = it->end();
    }

    // Allocate a single block buffer.
    size_t singleBlockLength = endAddr - startAddr;
    if (singleBlockLength > 0xFFFF)
        return vector<uint8_t>();

    vector<uint8_t> singleBlock;
    singleBlock.resize(singleBlockLength, '\0');

    // Copy the input blocks into the single block.
    for (vector<BinFormatBlock>::const_iterator it = blocks.begin(); it != blocks.end(); ++it)
    {
        if (it->length > 0)
        {
            vector<uint8_t>::iterator destIt = singleBlock.begin() + (it->address - startAddr);
            singleBlock.erase(destIt, destIt + it->length);
            singleBlock.insert(destIt, it->contents.begin(), it->contents.end());
        }
    }

    return singleBlock;
}


static int
convertBinToDragonFormat(const string &executableFilename,
                         bool verbose)
{
    // Open the DECB .BIN file and read in its blocks.
    ifstream decbBinFile(executableFilename.c_str());
    if (!decbBinFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to open linker executable file "
                                            << executableFilename << endl;
        return EXIT_FAILURE;
    }

    vector<BinFormatBlock> blocks;
    BinFormatBlock block;
    bool success = false;
    while (readBinFormatBlock(decbBinFile, block))
    {
        if (!block.isPreamble)
        {
            success = true;
            break;
        }
        blocks.push_back(block);
    }
    if (!success)
    {
        cout << PACKAGE << fatalErrorPrefix << "executable file " << executableFilename
                                            << " is invalid" << endl;
        return EXIT_FAILURE;
    }

    // Here, block.address is the entry point.

    uint16_t startAddr = 0;
    vector<uint8_t> singleBlock = formSingleBlock(blocks, startAddr);

    if (verbose)
        cout << "Read " << blocks.size() << " block(s) from " << executableFilename << ". "
             << "Entry point is $" << hex << block.address << dec << ". "
             << "Single block is " << singleBlock.size() << " byte(s) long.\n"
             << "Overwriting " << executableFilename << " in DragonDOS binary format." << endl;

    // Overwrite the .bin file with the single block, in the DragonDOS format.
    // Source: http://dragon32.info/info/binformt.html

    ofstream dragonFile(executableFilename.c_str(), ios_base::out | ios_base::binary | ios_base::trunc);
    if (!dragonFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to overwrite executable file " << executableFilename << endl;
        return EXIT_FAILURE;
    }

    dragonFile << uint8_t(0x55) << uint8_t(2)
               << uint8_t(startAddr >> 8) << uint8_t(startAddr & 0xFF)  // load address, in big endian
               << uint8_t(singleBlock.size() >> 8) << uint8_t(singleBlock.size() & 0xFF)  // length, in big endian
               << uint8_t(block.address >> 8) << uint8_t(block.address & 0xFF)  // entry point, in big endian
               << uint8_t(0xAA);
    if (dragonFile)
        dragonFile.write((char *) singleBlock.data(), singleBlock.size());
    if (!dragonFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to write to executable file " << executableFilename << endl;
        return EXIT_FAILURE;
    }

    dragonFile.close();
    if (!dragonFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to close executable file " << executableFilename << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// Sets errno to ERANGE upon failure.
// Fails with ERANGE if the decimal digits are followed by non decimal digits,
// which means that a hex string like 70C8 will be rejected.
//
static uint16_t
decimalToUInt16(const string &s)
{
    if (s.empty() || !isdigit(s[0]))
    {
        errno = ERANGE;
        return 0;
    }
    errno = 0;
    char *end = NULL;
    unsigned long n = strtoul(s.c_str(), &end, 10);
    if (n > 0xFFFF || errno == ERANGE || *end != '\0')
    {
        errno = ERANGE;
        return 0;
    }
    return uint16_t(n);
}



// Changes 'params' according to the command-line options specified by argc and argv[1..].
// If the return value is -1, then 'argi' receives the index of the first non-option argument in argv[1..].
//
// Returns:
//   0 to ask the caller to quit with EXIT_SUCCESS;
//   1 to ask the caller to quit with EXIT_FAILURE;
//  -1 to ask the caller to continue.
//
static int
interpretCommandLineOptions(Parameters &params, int argc, char *argv[], int &argi)
{
    size_t numPlatformsSpecified = 0;

    for (argi = 1; argi < argc; ++argi)
    {
        const string &curopt = argv[argi];

        if (curopt == "--version" || curopt == "-v")
        {
            params.displayVersionNo();
            return 0;
        }
        if (curopt == "--help" || curopt == "-h")
        {
            params.displayHelp();
            return 0;
        }
        if (curopt == "--preproc" || curopt == "-E")
        {
            params.preprocOnly = true;
            continue;
        }
        if (curopt == "--compile" || curopt == "-c")
        {
            params.compileOnly = true;
            continue;
        }
        if (curopt == "-S")
        {
            params.genAsmOnly = true;
            continue;
        }
        if (curopt == "--deps" || startsWith(curopt, "--deps="))
        {
            params.generatePrerequisitesFile = true;
            if (curopt.length() > 6)
                params.prerequisiteFilename.assign(curopt, 7, string::npos);
            continue;
        }
        if (curopt == "--deps-only" || startsWith(curopt, "--deps-only="))
        {
            params.generatePrerequisitesFile = true;
            params.generatePrerequisitesFileOnly = true;
            if (curopt.length() > 11)
                params.prerequisiteFilename.assign(curopt, 12, string::npos);
            params.compileOnly = true;
            continue;
        }

        // -MT, -MD, -MP and -MF help support the gcc3 dependency style supported by GNU Automake (as of 1.16).
        // Option --host=coco must be passed to ./configure to have Autoconf and Automake use cmoc.
        //
        if (curopt == "-MT")
        {
            if (argi + 1 >= argc)  // if no arg follows
            {
                cout << PACKAGE << ": " << curopt << " expects an argument\n";
                params.displayHelp();
                return 1;
            }
            ++argi;
            params.prerequisiteRuleTarget = argv[argi];
            continue;
        }
        if (curopt == "-MD")
        {
            params.generatePrerequisitesFile = true;
            continue;
        }
        if (curopt == "-MP")
        {
            continue;
        }
        if (curopt == "-MF")
        {
            if (argi + 1 >= argc)  // if no arg follows
            {
                cout << PACKAGE << ": " << curopt << " expects an argument\n";
                params.displayHelp();
                return 1;
            }
            ++argi;
            params.prerequisiteFilename = argv[argi];
            continue;
        }

        if (curopt == "--verbose" || curopt == "-V")
        {
            params.verbose = true;
            continue;
        }
        if (curopt.compare(0, 6, "--org=") == 0)
        {
            string address(curopt, 6, string::npos);
            unsigned long n = strtoul(address.c_str(), NULL, 16);
            if (n > 0xFFFF || errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid code address: " << address << " (address must be in hex)\n";
                params.displayHelp();
                return 1;
            }
            params.codeAddress = (uint16_t) n;
            params.codeAddressSetBySwitch = true;
            continue;
        }
        if (curopt.compare(0, 8, "--limit=") == 0)
        {
            string address(curopt, 8, string::npos);
            unsigned long n = strtoul(address.c_str(), NULL, 16);
            if (n > 0xFFFF || errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid limit address: " << address << " (address must be in hex)\n";
                params.displayHelp();
                return 1;
            }
            params.limitAddress = (uint16_t) n;
            params.limitAddressSetBySwitch = true;
            continue;
        }
        if (curopt.compare(0, 7, "--data=") == 0)
        {
            string address(curopt, 7, string::npos);
            unsigned long n = strtoul(address.c_str(), NULL, 16);
            if (n > 0xFFFF || errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid data address: " << address << " (address must be in hex)\n";
                params.displayHelp();
                return 1;
            }
            params.dataAddress = (uint16_t) n;
            params.dataAddressSetBySwitch = true;
            continue;
        }
        if (curopt == "--coco-basic" || curopt == "--coco" || curopt == "--coco3" || curopt == "--decb")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = COCO_BASIC;
            continue;
        }
        if (curopt == "--usim")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = USIM;
            continue;
        }
        if (curopt == "--thommo")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = THOMMO;
            params.codeAddress = 0x2800;
            continue;
        }
        if (curopt == "--thomto")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = THOMTO;
            params.codeAddress = 0xa000;
            continue;
        }
        if (curopt == "--dos")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = COCO_BASIC;
            params.assumeTrack34 = true;
            params.codeAddress = 0x2600;
            params.codeAddressSetBySwitch = true;
            continue;
        }
        if (curopt == "--os9")
        {
            ++numPlatformsSpecified;

            // As per LWTOOLS manual, subsection 4.4.1 ("OS9 Modules") of
            // section 4.4 ("Format Specific Linking Notes").
            params.codeAddress = 0x000D;

            params.targetPlatform = OS9;
            continue;
        }
        if (curopt == "--flex")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = FLEX;
            params.codeAddress = 0x0000;
            continue;
        }
        if (curopt == "--dragon")
        {
            ++numPlatformsSpecified;
            params.targetPlatform = DRAGON;
            continue;
        }
        if (curopt == "--vectrex")
        {
            ++numPlatformsSpecified;
            params.codeAddress = 0x0000;
            params.targetPlatform = VECTREX;
            continue;
        }
        if (curopt == "--void-target")
        {
            ++numPlatformsSpecified;
            params.codeAddress = 0x0000;
            params.targetPlatform = VOID_TARGET;
            continue;
        }
        if (curopt == "--ir")
        {
            ++numPlatformsSpecified;
            params.codeAddress = 0x0000;
            params.targetPlatform = INTERM_REP;
            params.compileOnly = true;  // linking not possible
            params.genAsmOnly = true;  // this skips invoking the assembler
            continue;
        }
        if (curopt == "--srec")
        {
            params.forcedLWLinkFormat = "srec";
            continue;
        }
        if (curopt == "--raw")
        {
            params.forcedLWLinkFormat = "raw";
            continue;
        }
        if (curopt.compare(0, 6, "--cpp=") == 0)
        {
            params.cppExecutablePath.assign(curopt, 6, string::npos);
            continue;
        }
        if (curopt.compare(0, 8, "--lwasm=") == 0)
        {
            params.lwasmPath.assign(curopt, 8, string::npos);
            continue;
        }
        if (curopt.compare(0, 15, "--lwasm-pragma=") == 0)
        {
            string arg(curopt, 15, string::npos);
            for (size_t j = 0; j < arg.length(); ++j)
                if (!isalnum(arg[j]) && arg[j] != ',')
                {
                    cout << PACKAGE
                         << ": Invalid argument for --lwasm-pragma: must be comma-separated list of lwasm pragmas.\n";
                    params.displayHelp();
                    return 1;
                }
            params.lwasmPragma = arg;
            continue;
        }
        if (curopt.compare(0, 9, "--lwlink=") == 0)
        {
            params.lwlinkPath.assign(curopt, 9, string::npos);
            continue;
        }
        if (curopt == "--frankasm")
        {
            cout << PACKAGE << ": --frankasm: obsolete option (must use lwasm).\n";
            params.displayHelp();
            return 1;
        }
        if (strncmp(curopt.c_str(), "-I", 2) == 0)
        {
            string dir(curopt, 2, string::npos);
            if (!dir.empty())
                params.includeDirList.push_back(dir);
            else if (argi + 1 < argc)  // if argument follows -I:
            {
                ++argi;
                params.includeDirList.push_back(argv[argi]);
            }
            else
            {
                cout << PACKAGE << ": Option -I not followed by directory.\n";
                return 1;
            }

            continue;
        }
        if (strncmp(curopt.c_str(), "-D", 2) == 0)
        {
            string arg(curopt, 2, string::npos);
            if (!arg.empty())
                params.defines.push_back(arg);
            else if (argi + 1 < argc)  // if argument follows -D:
            {
                ++argi;
                params.defines.push_back(argv[argi]);
            }
            else
            {
                cout << PACKAGE << ": Option -D not followed by macro definition.\n";
                return 1;
            }

            continue;
        }
        if (curopt == "-nostdinc")
        {
            params.searchDefaultIncludeDirs = false;
            continue;
        }
        if (startsWith(curopt, "-L"))
        {
            string arg(curopt, 2, string::npos);
            if (!arg.empty())
                params.libDirs.push_back(arg);
            else if (argi + 1 < argc)  // if argument follows -L:
            {
                ++argi;
                params.libDirs.push_back(argv[argi]);
            }
            else
            {
                cout << PACKAGE << ": Option -L not followed by directory.\n";
                return 1;
            }

            continue;
        }
        if (curopt == "-nodefaultlibs")
        {
            params.useDefaultLibraries = false;
            continue;
        }
        if (curopt == "--native-float")
        {
            params.floatingPointLibrary = FloatingPointLibrary::NATIVE_LIB;
            continue;
        }
        if (curopt == "--mc6839")
        {
            params.floatingPointLibrary = FloatingPointLibrary::MC6839_LIB;
            continue;
        }
        if (curopt == "--no-relocate")
        {
            params.relocatabilitySupported = false;
            continue;
        }
        if (curopt == "-funsigned-char")
        {
            params.isCharSignedByDefault = false;
            continue;
        }
        if (curopt == "-fsigned-char")
        {
            params.isCharSignedByDefault = true;
            continue;
        }
        if (curopt == "-Werror")
        {
            params.treatWarningsAsErrors = true;
            continue;
        }
        if (curopt == "-w")
        {
            params.inhibitAllWarningMessages = true;
            continue;
        }
        if (curopt == "--check-null")
        {
            params.nullPointerCheckingEnabled = true;
            continue;
        }
        if (curopt == "--check-stack")
        {
            params.stackOverflowCheckingEnabled = true;
            continue;
        }
        if (strncmp(curopt.c_str(), "--initial-s=", 12) == 0)
        {
            string arg(curopt, 12, string::npos);
            unsigned long n = strtoul(arg.c_str(), NULL, 16);
            if (n > 0xFFFF || errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid argument for --initial-s: " << arg << " (address must be in hex)\n";
                params.displayHelp();
                return 1;
            }
            params.initialStackRegValue = int32_t(n);
            continue;
        }
        if (strncmp(curopt.c_str(), "--stack-space=", 14) == 0)
        {
            string arg(curopt, 14, string::npos);
            uint16_t n = decimalToUInt16(arg);
            if (errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid argument for --stack-space: " << arg << " (address must be in decimal)\n";
                params.displayHelp();
                return 1;
            }
            params.stackSpace = n;
            params.stackSpaceSpecifiedByCommandLine = true;
            continue;
        }
        if (strncmp(curopt.c_str(), "--add-os9-stack-space=", 22) == 0)
        {
            string arg(curopt, 22, string::npos);
            uint16_t n = decimalToUInt16(arg);
            if (errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid argument for --add-os9-stack-space: " << arg << " (address must be in decimal)\n";
                params.displayHelp();
                return 1;
            }
            params.extraStackSpace = n;
            continue;
        }
        if (strncmp(curopt.c_str(), "--function-stack=", 17) == 0)
        {
            string arg(curopt, 17, string::npos);
            uint16_t n = decimalToUInt16(arg);
            if (errno == ERANGE)
            {
                cout << PACKAGE << ": Invalid argument for --function-stack: " << arg << " (address must be in decimal)\n";
                params.displayHelp();
                return 1;
            }
            params.functionStackSpace = n;
            continue;
        }
        if (curopt == "--allow-undef-func")
        {
            params.callToUndefinedFunctionAllowed = true;
            continue;
        }
        if (curopt == "-Wsign-compare")
        {
            params.warnSignCompare = true;
            continue;
        }
        if (curopt == "-Wno-const")
        {
            params.isConstIncorrectWarningEnabled = false;
            continue;
        }
        if (curopt == "-Wgives-byte")
        {
            params.isBinaryOpGivingByteWarningEnabled = true;
            continue;
        }
        if (curopt == "-Wlocal-var-hiding")
        {
            params.isLocalVariableHidingAnotherWarningEnabled = true;
            continue;
        }
        if (curopt == "-Wno-printf")
        {
            params.isNonLiteralPrintfFormatWarningEnabled = false;
            continue;
        }
        if (curopt == "-Wno-uncalled-static")
        {
            params.isUncalledStaticFunctionWarningEnabled = false;
            continue;
        }
        if (curopt == "-Wno-missing-field-initializers")
        {
            params.isMissingFieldInitializersWarningEnabled = false;
            continue;
        }
        if (curopt == "-Wno-inline-asm-array-indexes")
        {
            params.inlineAsmArrayIndexesWarningEnabled = false;
            continue;
        }
        if (curopt == "-Wshadow")
        {
            params.isLocalVariableHidingAnotherWarningEnabled = true;
            continue;
        }
        if (curopt == "-Wfor-condition-sizes")
        {
            params.forConditionComparesDifferentSizesWarningEnabled = true;
            continue;
        }
        if (curopt == "-Wpass-const-for-func-pointer")  // not documented b/c may be annoying
        {
            params.warnPassingConstForFuncPtr = true;
            continue;
        }
        if (curopt == "-Wno-unknown-first-dim")
        {
            params.warnArrayWithUnknownFirstDimension = false;
            continue;
        }
        if (curopt == "-Wno-array-size-zero")
        {
            params.warnArraySizeZero = false;
            continue;
        }
        if (curopt == "-Wno-too-many-elements")
        {
            params.warnTooManyElementsInInitializer = false;
            continue;
        }
        if (curopt == "-Wno-shift-always-zero")
        {
            params.warnShiftAlwaysZero = false;
            continue;
        }
        if (curopt == "-Wno-label-on-declaration")
        {
            params.warnLabelOnDeclaration = false;
            continue;
        }
        if (curopt == "-Wno-assign-in-condition")
        {
            params.warnAssignmentInCondition = false;
            continue;
        }
        if (curopt == "-Waccept-word-sized-return-type-mismatch")
        {
            params.onlyWarningOnWordSizedReturnTypeMismatch = true;
            continue;
        }
        if (strncmp(curopt.c_str(), "-O", 2) == 0)
        {
            string level(curopt, 2, string::npos);
            if (level == "y")
            {
                params.yRegAllowed = true;
                params.optimizationLevel = 2;
            }
            else if (level.length() != 1 || level[0] < '0' || level[0] > '2')
            {
                cout << PACKAGE << ": Invalid optimization option: " << curopt << "\n";
                params.displayHelp();
                return 1;
            }
            else
                params.optimizationLevel = size_t(level[0] - '0');
            continue;
        }
        if (curopt == "-fomit-frame-pointer")
        {
            params.framePointerOption = FramePointerOption::OMIT_A_LOT;
            continue;
        }
        if (curopt == "-fno-omit-frame-pointer")
        {
            params.framePointerOption = FramePointerOption::KEEP_ALL;
            continue;
        }
        if (curopt == "--no-peephole")
        {
            params.optimizationLevel = 0;
            continue;
        }
        if (curopt == "--wfo")
        {
            params.wholeFunctionOptimization = true;
            continue;
        }
        if (curopt == "--intermediate" || curopt == "-i")
        {
            params.intermediateFilesKept = true;
            continue;
        }
        if (startsWith(curopt, "--intdir"))
        {
            if (curopt.length() > 8)
            {
                if (curopt[8] == '=')
                    params.intermediateDir = string(curopt, 9);
                else
                    return params.declareInvalidOption(curopt);
            }
            else if (argi + 1 < argc)  // if argument follows
            {
                ++argi;
                params.intermediateDir = argv[argi];
            }
            else
            {
                cout << PACKAGE << ": Option --intdir not followed by directory.\n";
                return 1;
            }

            continue;
        }
        if (curopt == "-x")
        {
            if (argi + 1 < argc)  // if next arg, use it as language name
            {
                const char *lang = argv[argi + 1];
                if (strcmp(lang, "c") == 0)
                    params.assumeCFileByDefault = true;
                else if (strcmp(lang, "none") == 0)
                    params.assumeCFileByDefault = false;
                else
                {
                    cout << PACKAGE << fatalErrorPrefix << "-x followed by unsupported language name" << endl;
                    return 1;
                }
                ++argi;
            }
            else
            {
                cout << PACKAGE << fatalErrorPrefix << "-x not followed by supported language name" << endl;
                return 1;
            }
            continue;
        }
        if (strncmp(curopt.c_str(), "-o", 2) == 0)
        {
            string arg(curopt, 2, string::npos);
            if (!arg.empty())
                params.outputFilename = arg;
            else if (argi + 1 < argc)  // if argument follows -o:
            {
                ++argi;
                params.outputFilename = argv[argi];
            }
            else
            {
                cout << PACKAGE << ": Option -o not followed by filename.\n";
                return 1;
            }
            continue;
        }
        if (strncmp(curopt.c_str(), "--switch=", 9) == 0)
        {
            string arg(curopt, 9, string::npos);
            if (arg == "ifelse")
            {
                params.forceJumpMode = true;
                params.forcedJumpMode = SwitchStmt::IF_ELSE;
            }
            else if (arg == "jump")
            {
                params.forceJumpMode = true;
                params.forcedJumpMode = SwitchStmt::JUMP_TABLE;
            }
            else
            {
                cout << PACKAGE << ": Invalid argument for option --switch (must be ifelse or jump).\n";
                return 1;
            }
            continue;
        }

        if (curopt.empty() || curopt[0] == '-')
            return params.declareInvalidOption(curopt);

        break;  // end of options; argi now designates 1st non-option argument
    }

    if (numPlatformsSpecified > 1)
    {
        cout << PACKAGE << ": More than one platform option specified\n";
        return 1;
    }

    return -1;
}


int
main(int argc, char *argv[])
{
    Parameters params;

    /*  Allow an environment variable to override the system #include directory.
    */
    {
        const char *p = getenv("CMOCPKGDATADIR"), *l = getenv("CMOCFLOATLIBDIR");
        params.pkgdatadir = (p != NULL ? p : PKGDATADIR);
        params.cmocfloatlibdir = (l != NULL ? l : params.pkgdatadir + "/lib");
    }

    /*  Allow an environment variable to specify the C preprocessor to invoke.
        The redistributable Windows package uses this to point cmoc at its
        bundled cpp.exe. An explicit --cpp= option (parsed below) overrides it.
    */
    {
        const char *cpp = getenv("CMOC_CPP");
        if (cpp != NULL && cpp[0] != '\0')
            params.cppExecutablePath = cpp;
    }

    int argi = 1;
    int code = interpretCommandLineOptions(params, argc, argv, argi);
    if (code != -1)
        return code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;

    const char *targetPlatformName = getTargetPlatformName(params.targetPlatform);
    const char *targetPreprocId = getTargetPlatformPreprocId(params.targetPlatform);

    // Allow defining a prototype and calling the function,
    // but letting another module or library define that function.
    params.callToUndefinedFunctionAllowed = true;

    // Do not try to link if dumping the preprocessor output or only generating asm.
    if (params.preprocOnly)
        params.compileOnly = true;

    if (params.genAsmOnly)
        params.compileOnly = true;


    // Forbid --org and --data when targetting some platforms.
    if ((params.targetPlatform == OS9 || params.targetPlatform == VECTREX)
        && (params.codeAddressSetBySwitch || params.dataAddressSetBySwitch))
    {
        cout << PACKAGE << ": --org and --data are not permitted when targetting "
             << targetPlatformName << "\n";
        return EXIT_FAILURE;
    }

    // Under Vectrex, relocatability is not possible anyway.
    if (params.targetPlatform == VECTREX)
        params.relocatabilitySupported = false;

    string executableFilename;
    if (!params.outputFilename.empty() && !params.compileOnly)
        executableFilename = params.outputFilename;


    #if 0
    // When not targeting an MS Basic environment, use the native float library.
    if (params.targetPlatform != COCO_BASIC && params.targetPlatform != DRAGON)
        params.useNativeFloatLibrary = true;
    #endif


    // Add default include dir at the end of any user-specified dirs.
    if (params.searchDefaultIncludeDirs)
        params.includeDirList.push_back(params.pkgdatadir + "/include");


    assert(argi <= argc);

    if (argi == argc)
    {
        cout << PACKAGE << fatalErrorPrefix << "no input files\n";
        return EXIT_FAILURE;
    }

    if (params.targetPlatform == OS9)
    {
        if (!params.forcedLWLinkFormat.empty())
        {
            cout << PACKAGE << fatalErrorPrefix << "generating an OS-9 executable in '"
                            << params.forcedLWLinkFormat << "' format is not supported\n";
            return EXIT_FAILURE;
        }
        if (params.stackOverflowCheckingEnabled)
        {
            cout << PACKAGE << fatalErrorPrefix << "--check-stack not usable with OS-9\n";
            return EXIT_FAILURE;
        }
        if (! params.relocatabilitySupported)
        {
            cout << PACKAGE << fatalErrorPrefix << "--no-relocate not compatible with OS-9\n";
            return EXIT_FAILURE;
        }
        if (params.initialStackRegValue >= 0 && params.initialStackRegValue <= 0xFFFF)
        {
            cout << PACKAGE << fatalErrorPrefix << "--initial-s not compatible with OS-9\n";
            return EXIT_FAILURE;
        }
    }

    if (params.forceJumpMode)
        SwitchStmt::forceJumpMode(params.forcedJumpMode);

    {
        uint16_t fss = 0;  // by default, no stack checking upon function entry
        if (params.functionStackSpace != uint32_t(-1))  // if specified by --function-space
        {
            if (params.targetPlatform != OS9)
            {
                cout << PACKAGE << fatalErrorPrefix << "--function-stack only supported with OS-9\n";
                return EXIT_FAILURE;
            }
            fss = uint16_t(params.functionStackSpace);
        }
        else if (params.targetPlatform == OS9)
            fss = 64;
        FunctionDef::setFunctionStackSpace(fss);
    }

    vector<string> objectFilenames, libraryFilenames, intermediateCompilationFiles, intermediateObjectFiles;
    string programName;
    string asmFilename;

    int status = EXIT_SUCCESS;

    // Scan all non-option arguments.
    //
    for ( ; argi < argc; ++argi)
    {
        /*  Check that the source file exists.
        */
        const string inputFilename = argv[argi];

        if (inputFilename.empty())
        {
            cout << PACKAGE << fatalErrorPrefix << "empty filename" << endl;
            status = EXIT_FAILURE;
            break;
        }

        if (inputFilename[0] != '-')
        {
            struct stat statbuf;
            if (stat(inputFilename.c_str(), &statbuf) != 0)
            {
                int err = errno;
                cout << PACKAGE << fatalErrorPrefix << inputFilename << ": " << strerror(err) << endl;
                status = EXIT_FAILURE;
                break;
            }

            if (S_ISDIR(statbuf.st_mode))
            {
                cout << PACKAGE << fatalErrorPrefix << inputFilename << ": " << strerror(EISDIR) << endl;
                status = EXIT_FAILURE;
                break;
            }
        }


        // Extract the module name and input file extension.
        //
        string moduleName = getBasename(inputFilename);
        const string extension = removeExtension(moduleName);
        const bool isCFile = (inputFilename[0] != '-' && params.isCFileExtension(extension));

        // The first module name is the program name.
        if (programName.empty())
            programName = moduleName;

        if (moduleName.find('\"') != string::npos || moduleName.find('\\') != string::npos)
        {
            // This check helps ensure valid OS-9 module name.
            cout << PACKAGE << fatalErrorPrefix << "illegal program file name: " << inputFilename << endl;
            status = EXIT_FAILURE;
            break;
        }


        // Determine this module's output filename (if compilation/assembly required).
        //
        string compilationOutputFilename;
        if (isCFile || extension == ".s" || extension == ".asm")
        {
            const char *compilationOutputExt = (params.targetPlatform == INTERM_REP ? ".json" : ".o");
            if (params.compileOnly)
            {
                if (!params.outputFilename.empty())
                    compilationOutputFilename = params.outputFilename;
                else
                    compilationOutputFilename = params.useIntDir(moduleName + compilationOutputExt);
            }
            else
            {
                // In compile-and-link mode, outputFilename (if any) is the executable filename,
                // so it cannot be used for compilationOutputFilename.
                compilationOutputFilename = params.useIntDir(moduleName + compilationOutputExt);
            }
        }


        // Process the input filename depending on its extension.
        // C files are compiled.
        // Object files are passed to the linker.
        //
        if (isCFile)
        {
            asmFilename = params.useIntDir(moduleName + ".s");

            int s = params.compileCFile(inputFilename,
                                        moduleName,
                                        asmFilename,
                                        compilationOutputFilename,
                                        targetPlatformName,
                                        targetPreprocId);

            objectFilenames.push_back(moduleName);
            if (!params.genAsmOnly)
                intermediateCompilationFiles.push_back(asmFilename);
            if (!params.intermediateFilesKept)
                intermediateCompilationFiles.push_back(replaceExtension(asmFilename, ".lst"));
            if (!params.compileOnly)
                intermediateObjectFiles.push_back(compilationOutputFilename);

            if (s != EXIT_SUCCESS)
            {
                status = s;
                break;
            }
        }
        else if (extension == ".s" || extension == ".asm")
        {
            string lstFilename = params.useIntDir(moduleName + ".lst");
            int s = params.invokeAssembler(inputFilename, compilationOutputFilename, lstFilename, targetPreprocId);

            if (s != EXIT_SUCCESS)
            {
                status = s;
                break;
            }

            objectFilenames.push_back(moduleName);
            if (!params.intermediateFilesKept)
                intermediateCompilationFiles.push_back(replaceExtension(asmFilename, ".lst"));
            if (!params.compileOnly)
                intermediateObjectFiles.push_back(compilationOutputFilename);
        }
        else if (extension == ".o")
        {
            string objFilename = inputFilename;
            removeExtension(objFilename);
            objectFilenames.push_back(objFilename);
        }
        else if (extension == ".a")
        {
            libraryFilenames.push_back(inputFilename);
        }
        else if (startsWith(inputFilename, "-L"))
        {
            if (inputFilename.length() == 2)  // if -L alone
            {
                if (argi + 1 < argc)  // if next arg, use it as lib dir
                {
                    params.libDirs.push_back(argv[argi + 1]);
                    ++argi;
                }
                else
                {
                    cout << PACKAGE << fatalErrorPrefix << "-L not followed by library directory" << endl;
                    status = EXIT_FAILURE;
                    break;
                }
            }
            else  // -L followed by dir
                params.libDirs.push_back(inputFilename.substr(2));
        }
        else if (startsWith(inputFilename, "-l"))
        {
            libraryFilenames.push_back(inputFilename);
        }
        else
        {
            cout << PACKAGE << fatalErrorPrefix << "invalid input filename " << inputFilename << endl;
            status = EXIT_FAILURE;
            break;
        }

        // If -o not used, then use 1st module name to form output filename.
        if (executableFilename.empty())
            executableFilename = moduleName + getDefaultOutputExtension(params.targetPlatform, params.forcedLWLinkFormat);

    }   // for

    if (!params.intermediateFilesKept)
        removeIntermediateCompilationFiles(intermediateCompilationFiles);

    if (status != EXIT_SUCCESS)
        return status;

    if (params.compileOnly)
        return EXIT_SUCCESS;


    // Link all modules together.
    //
    if (params.compileOnly)
        return EXIT_SUCCESS;
    string linkScriptFilename = params.useIntDir(replaceExtension(executableFilename, ".link"));
    string mapFilename = params.useIntDir(replaceExtension(executableFilename, ".map"));
    status = invokeLinker(params,
                          objectFilenames, libraryFilenames,
                          linkScriptFilename, mapFilename,
                          executableFilename);
    if (!params.intermediateFilesKept)
        removeIntermediateLinkingFiles(linkScriptFilename, mapFilename, intermediateObjectFiles);

    if (status == EXIT_SUCCESS && params.targetPlatform == DRAGON && params.forcedLWLinkFormat.empty())
        status = convertBinToDragonFormat(executableFilename, params.verbose);

    if (status == EXIT_SUCCESS)
        cout << "Success! " << executableFilename << " was successfully built." << endl;

    return status;
}
