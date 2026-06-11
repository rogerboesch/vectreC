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

#include "Parameters.h"

#include "TranslationUnit.h"

#if defined(__MINGW32__)
// MinGW has no <sys/wait.h>; system() returns the child's exit code directly.
#define WIFEXITED(x)   ((unsigned) (x) < 259)
#define WEXITSTATUS(x) ((x) & 0xff)
#else
#include <sys/wait.h>  /* for WEXITSTATUS() and WIFEXITED() */
#endif


#ifndef PROGRAM  // Allow the compilation to define the program name as a macro.
static const char *PROGRAM = "cmoc";
#endif

using namespace std;


extern const char *fatalErrorPrefix;
extern int numErrors;
extern int numWarnings;


Parameters::Parameters()
:   codeAddress(0x2800),  // DECB Basic program starts at 0x2601 by default
    dataAddress(0xFFFF),
    codeAddressSetBySwitch(false),
    dataAddressSetBySwitch(false),
    initialStackRegValue(-1),  // none by default
    stackSpace(1024),
    extraStackSpace(0),
    functionStackSpace(uint32_t(-1)),
    pkgdatadir(),
    cmocfloatlibdir(),
    cppExecutablePath("cpp"),
    lwasmPath("lwasm"),
    lwasmPragma("forwardrefmax"),
    lwlinkPath("lwlink"),
    intermediateFilesKept(false),
    intermediateDir(),
    generatePrerequisitesFile(false),
    generatePrerequisitesFileOnly(false),
    prerequisiteFilename(),
    prerequisiteRuleTarget(),
    preprocOnly(false),
    genAsmOnly(false),
    compileOnly(false),
    verbose(false),
    treatWarningsAsErrors(false),
    inhibitAllWarningMessages(false),
    nullPointerCheckingEnabled(false),
    stackOverflowCheckingEnabled(false),
    targetPlatform(COCO_BASIC),
    assumeTrack34(false),
    forcedLWLinkFormat(),
    callToUndefinedFunctionAllowed(false),
    warnSignCompare(false),
    warnPassingConstForFuncPtr(false),
    isConstIncorrectWarningEnabled(true),
    isBinaryOpGivingByteWarningEnabled(false),
    isLocalVariableHidingAnotherWarningEnabled(false),
    isNonLiteralPrintfFormatWarningEnabled(true),
    isUncalledStaticFunctionWarningEnabled(true),
    isMissingFieldInitializersWarningEnabled(true),
    inlineAsmArrayIndexesWarningEnabled(true),
    forConditionComparesDifferentSizesWarningEnabled(false),
    warnArrayWithUnknownFirstDimension(true),
    warnArraySizeZero(true),
    warnTooManyElementsInInitializer(true),
    warnShiftAlwaysZero(true),
    warnLabelOnDeclaration(true),
    warnAssignmentInCondition(true),
    onlyWarningOnWordSizedReturnTypeMismatch(false),  // error by default
    wholeFunctionOptimization(false),
    forceJumpMode(false),
    forcedJumpMode(SwitchStmt::IF_ELSE),
    optimizationLevel(2),
    yRegAllowed(false),
    framePointerOption(FramePointerOption::DEFAULT),
    stackSpaceSpecifiedByCommandLine(false),
    limitAddress(0xFFFF),
    limitAddressSetBySwitch(false),
    assumeCFileByDefault(false),
    outputFilename(),
    libDirs(),
    useDefaultLibraries(true),
    floatingPointLibrary(FloatingPointLibrary::ECB_ROM),
    relocatabilitySupported(true),
    isCharSignedByDefault(true),
    includeDirList(),
    searchDefaultIncludeDirs(true),
    defines()
{
}


void
Parameters::displayVersionNo() const
{
    std::cout << PROGRAM << " (" << PACKAGE << " " << VERSION << ")\n";
}


void
Parameters::displayHelp() const
{
    std::cout << "\n";

    displayVersionNo();

    std::cout << "\n"
        "Copyright (C) 2003-2023 Pierre Sarrazin <http://sarrazip.com/>\n";
    std::cout <<
"This program is free software; you may redistribute it under the terms of\n"
"the GNU General Public License, either version 3 or later.\n"
"This program comes with absolutely no warranty.\n"
"\n";

    std::cout <<
        "--help|-h            Display this help page and exit.\n"
        "--version|-v         Display this program's version number and exit.\n"
        "--verbose|-V         Display more informationg about the compiling process.\n"
        "--preproc|-E         Copy preprocessor output to standard output, instead of compiling.\n"
        "-S                   Stop after generating an assembly language file.\n"
        "--compile|-c         Stop after generating an object file.\n"
        "-o FILE              Place the output in FILE (default: change C file extension to .bin).\n"
        "--deps[=F]           Create a .d file containing a makefile rule giving the dependencies\n"
        "                     of the compiled file. If a filename (F) is given, it is used instead\n"
        "                     of forming the dependency filename from the code output filename.\n"
        "--deps-only[=F]      Same, but do nothing else and stop.\n"
        "-O0|-O1|-O2          Optimization level (default is 2). Compilation is faster with -O0.\n"
        //"-Oy                  Allow the low-level optimizer to use the Y register. Implies -O2.\n"
        //"                     Has no effect when targeting OS-9 or in `__gcccall' functions.\n"
        "-funsigned-char      Assume that `char' is unsigned by default.\n"
        "-fsigned-char        Assume that `char' is signed by default. (This is the default.)\n"
        "-fomit-frame-pointer Do not keep the frame pointer in a register for functions that do not\n"
        "                     need one. Experimental. Use with caution.\n"
        "--org=X              Use X (in hex) as the first address at which to generate\n"
        "                     the code; default: " << hex << codeAddress << dec << ".\n"
        "--limit=X            Fail if program_end exceeds address X (in hex).\n"
        "--data=X             Use X (in hex) as the first address at which to generate the\n"
        "                     writable global variable space; by default that space follows\n"
        "                     the code.\n"
        "--coco               Compile for the CoCo Disk Basic environment (default).\n"
        "--decb               Synonym for --coco.\n"
        "--dos                Compile to a CoCo Disk Basic track 34 boot loader (implies --coco).\n"
        "                     Must be passed when compiling the .c file containing main().\n"
        "--dragon             Compile for the Dragon environment.\n"
        "--thommo             Compile for the Thomson MO.\n"
        "--thomto             Compile for the Thomson TO.\n"
        "--os9                Compile for OS-9.\n"
        "--flex               Compile for FLEX.\n"
        "--vectrex            Compile for the Vectrex video game console.\n"
        "--void-target        Compile for a target that has no I/O system known to CMOC.\n"
        "--usim               Compile for the USIM 6809 simulator.\n"
        "--srec               Executable in Motorola SREC format.\n"
        "--raw                Executable in raw format (passes --format=raw to lwlink)\n"
        "--cpp=X              Use X as the path to the C preprocessor (default: cpp).\n"
        "--lwasm=X            Use X as the path to the LWTOOLS assembler.\n"
        "--lwasm-pragma=X     Pass X to --pragma when invoking lwasm. Default: forwardrefmax\n"
        "                     X must be a comma-separated list of terms accepted by lwasm --pragma.\n"
        "--lwlink=X           Use X as the path to the LWTOOLS linker.\n"
        "-Idir                Add directory <dir> to the compiler's include directories\n"
        "                     (also applies to assembler).\n"
        "-Dxxx=yyy            Equivalent to #define xxx yyy\n"
        "-L dir               Add a directory to the library search path.\n"
        "-l name              Add a library to the linking phase. -lfoo expects `libfoo.a'.\n"
        "                     This option must be specified after the source/object files.\n"
        "-nodefaultlibs       Excludes CMOC-provided libraries from the linking phase.\n"
        "--no-relocate        Assume that the program will only be loaded at the addresses specified\n"
        "                     by --org and --data. Not compatible with OS-9. Default for Vectrex.\n"
        "--check-null         Insert run-time checks for null pointers. See the manual.\n"
        "--check-stack        Insert run-time checks for stack overflow. See the manual.\n"
        "                     Not usable under OS-9, where stack checking is automatic.\n"
        "--stack-space=N      Assume the stack may use as many as N bytes (in decimal).\n"
        "                     Affects --check-stack and sbrk().\n"
        "                     Must be used when compiling the main() function.\n"
        "                     Not usable under OS-9, where stack checking is automatic.\n"
        "                     Ignored when targeting Vectrex.\n"
        "--add-os9-stack-space=N\n"
        "                     (OS-9 only.) Allocate N more bytes (in decimal) to the stack space\n"
        "                     that OS-9 normally reserves.\n"
        "--function-stack=N   (OS-9 only.) Emit code at the start of each function to check that there\n"
        "                     is at least N bytes (in decimal) of free stack space in addition to\n"
        "                     local variables. 0 means no stack checking. Default is 64.\n"
        "--initial-s=X        Generate an immediate-mode LDS instruction as the first instruction\n"
        "                     of the executable. Use X (in hex) as the argument.\n"
        "                     Not compatible with --os9.\n"
        "-Wsign-compare       Warn when <, <=, >, >= used on operands of differing signedness.\n"
        "-Wno-const           Do not warn about const-incorrect code.\n"
        "-Wgives-byte         Warn about binary operations on bytes giving a byte.\n"
        "-Wlocal-var-hiding   Warn when a local variable hides another one.\n"
        "-Wshadow             Equivalent to -Wlocal-var-hiding.\n"
        "-Wno-printf          Do not warn when printf/sprintf format is not a string literal.\n"
        "-Wno-uncalled-static Do not warn when a static function is not called.\n"
        "-Wfor-condition-sizes\n"
        "                     Warn if a for() loop's condition compares values of different sizes.\n"
        "                     Ex.: unsigned n = 256; for (unsigned char i = 0; i < n; ++i) {...}\n"
        "-Wno-missing-field-initializers\n"
        "                     Do no warn when a struct is initialized with fewer values than\n"
        "                     there are fields.\n"
        "-Wno-inline-asm-array-indexes\n"
        "                     Do not warn when an inline assembly statement uses an array index in\n"
        "                     a C array variable reference. Such an index is taken as an element index,\n"
        "                     not a byte index. (An index into a non-array is a byte index.)\n"
        "-Wno-unknown-first-dim\n"
        "                     Do not warn about arrays whose first dimension has an unknown size,\n"
        "                     e.g., int a[] = { ... };\n"
        "-Wno-array-size-zero Do not warn when an array has size zero.\n"
        "-Wno-too-many-elements\n"
        "                     Do not warn about array initializers having more elements than the array\n"
        "                     they initialize, e.g., int a[] = { 1, 2, 3 };\n"
        "-Wno-shift-always-zero\n"
        "                     Do not warn about bit shifts that always gives a zero.\n"
        "-Wno-label-on-declaration\n"
        "                     Do not warn when a label is put on a declaration.\n"
        "-Wno-assign-in-condition\n"
        "                     Do not issue a warning when an assignment is used as a condition\n"
        "                     in an if(), while() or do-while() statement.\n"
        "-Waccept-word-sized-return-type-mismatch\n"
        "                     Issue a warning instead of an error when the argument of a return\n"
        "                     statement mismatches the return type of a function, and both types\n"
        "                     are words, pointers or arrays. (Helps tolerate K&R code.)\n"
        "-Werror              Treat warnings as errors.\n"
        "-w                   Inhibit all warning messages.\n"
        "--switch=MODE        Force all switch() statements to use MODE, where MODE is 'ifelse'\n"
        "                     for an if-else sequence or 'jump' for a jump table.\n"
        "--intermediate|-i    Keep intermediate compilation and linking files.\n"
        "--intdir=D           Put intermediate files in directory D.\n"
        "-x c                 Assume that files other than .s, .asm, .o and .a are C files.\n"
        "-x none              Only consider .c files as C files.\n"
        "\n"
        "Executable file formats:\n"
        "  CoCo BIN for CoCo Disk Basic, Dragon, Thomson MO/TO; raw binary for Vectrex;\n"
        "  SREC for FLEX, void-target and USIM; OS-9 for OS-9.\n"
        "  Can be overridden by --srec or --raw, except for OS-9.\n"
        "\n"
        "Preprocessor identifer _CMOC_VERSION_ is defined as: " << Parameters::getVersionInteger() << "\n"
        "\n";

    std::cout << "Compiler data directory: " << pkgdatadir << "\n\n";
    std::cout << "Compiler floating-point library directory: " << cmocfloatlibdir << "\n\n";

    std::cout << "For details, see the manual on the CMOC home page.\n\n";
}


// If VERSION is x.y.z, then returns x * 100000 + y * 1000 + z.
// Assumes that y <= 99 and z <= 999.
//
uint32_t
Parameters::getVersionInteger()
{
    char *endptr = NULL;
    unsigned long major = strtoul(VERSION, &endptr, 10);
    unsigned long minor = strtoul(endptr + 1, &endptr, 10);
    unsigned long micro = strtoul(endptr + 1, &endptr, 10);
    return uint32_t(major * 100000UL + minor * 1000UL + micro);
}


string
Parameters::useIntDir(const string &s) const
{
    if (intermediateDir.empty() || s.find('/') != string::npos)
        return s;
    return replaceDir(s, intermediateDir);
}


bool
Parameters::isCFileExtension(const std::string &extension) const
{
    if (assumeCFileByDefault)
        return extension != ".s" && extension != ".asm" && extension != ".o" && extension != ".a";
    return extension == ".c";
}


int
Parameters::declareInvalidOption(const string &opt) const
{
    cout << PACKAGE << ": Invalid option: " << opt << "\n";
    displayHelp();
    return 1;
}


// Checks if the line matches the pattern of an include marker as
// generated by the C preprocessor: ^\#\ [0-9]+\ \".*\"[ 0-9]*$
// i.e., a pound sign followed by a line number, followed by a
// double-quoted file path.
// If 'line' matches, this file path is stored in 'filename' and
// true is returned.
// Otherwise, false is returned.
//
static bool
parseIncludeMarker(const char *line, string &filename)
{
    if (!line)
        return false;
    if (*line++ != '#')
        return false;
    if (*line++ != ' ')
        return false;
    const char *p = line;
    while (isdigit(*p))  // pass line number
        ++p;
    if (p == line)
        return false;  // no line number
    if (*p++ != ' ')
        return false;
    if (*p++ != '\"')
        return false;
    const char *f = p;
    while (*p != '\"' && *p != '\0')  // reach end of quoted string
        ++p;
    if (*p != '\"')
        return false;
    filename.assign(f, size_t(p - f));
    return true;
}


// Quote a filename or argument for a shell command line. cmoc.exe is a native
// Windows binary, so its system()/popen() run the command through cmd.exe,
// which does not honor single-quote quoting; double quotes are used there.
// sh-based platforms (macOS, Linux) keep single quotes.
static string
shq(const string &s)
{
#if defined(__MINGW32__)
    return "\"" + s + "\"";
#else
    return "'" + s + "'";
#endif
}


string
Parameters::buildCppCommand(const string &inputFilename,
                            const char *targetPreprocId)
{
    stringstream cppCommand;
    // Note: cppExecutablePath is the command's first token, so it must NOT be
    // quoted here: popen() wraps the whole command as cmd.exe /c "...", and a
    // leading quoted token would create broken nested quotes. Install paths
    // with spaces are therefore unsupported for the preprocessor path.
    cppCommand << cppExecutablePath << " -xc++ -U__cplusplus";  // -xc++ makes sure cpp accepts C++-style comments
    for (list<string>::const_iterator it = includeDirList.begin(); it != includeDirList.end(); ++it)
        cppCommand << " -I" << shq(*it);
    cppCommand << " -D_CMOC_VERSION_=" << getVersionInteger();
    cppCommand << " -D" << targetPreprocId << "=1";
    if (!relocatabilitySupported)
        cppCommand << " -D_CMOC_NO_RELOCATE_=1";
    if (floatingPointLibrary == FloatingPointLibrary::NATIVE_LIB)
        cppCommand << " -D_CMOC_NATIVE_FLOAT_=1";
    if (floatingPointLibrary == FloatingPointLibrary::MC6839_LIB)
        cppCommand << " -D_CMOC_MC6839_=1";
    if (!isCharSignedByDefault)
        cppCommand << " -D_CMOC_unsigned_char_=1";
    cppCommand << " -U__GNUC__ -nostdinc -undef";

    for (list<string>::const_iterator it = defines.begin(); it != defines.end(); ++it)
        cppCommand << " -D" << shq(*it);

    cppCommand << " " << inputFilename;  // must be last argument, for portability
    return cppCommand.str();
}


int
Parameters::createAssemblyLanguageFile(const ASMText &asmText,
                                       const string &asmFilename)
{
    if (verbose)
        cout << "Assembly language filename: " << asmFilename << endl;
    ofstream asmFile(asmFilename.c_str(), ios::out);
    if (!asmFile)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "failed to create assembler file " << asmFilename
                << ": " << strerror(e) << endl;
        return EXIT_FAILURE;
    }
    if (!asmText.writeFile(asmFile))
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to write output assembly file " << asmFilename << endl;
        return EXIT_FAILURE;
    }
    asmFile.close();
    if (!asmFile)
    {
        cout << PACKAGE << fatalErrorPrefix << "failed to close output assembly file " << asmFilename << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


// Returns true upon success. Issues an error message upon failure.
//
bool
Parameters::writePrerequisites(const string &compilationOutputFilename)
{
    string dependenciesFilename = prerequisiteFilename.empty()
                                    ? replaceExtension(compilationOutputFilename, ".d")
                                    : prerequisiteFilename;
    ofstream dependenciesFile(dependenciesFilename.c_str(), ios::out);
    if (dependenciesFile.good())
    {
        TranslationUnit::instance().writePrerequisites(dependenciesFile, dependenciesFilename,
                                                       prerequisiteRuleTarget.empty() ? compilationOutputFilename : prerequisiteRuleTarget,
                                                       pkgdatadir);
        return true;
    }
    int e = errno;
    cout << PACKAGE << fatalErrorPrefix << "failed to create dependencies file "
            << dependenciesFilename  << ": " << strerror(e) << endl;
    return false;
}


static bool
copyLinesToCOut(FILE *source)
{
    char buffer[8192];
    while (fgets(buffer, sizeof(buffer), source) != NULL)  // while a line can be read
    {
        cout << buffer;
        if (!cout)
        {
            int e = errno;
            cout << PACKAGE << fatalErrorPrefix
                    << "failed to copy C preprocessor output to standard output:"
                    << " " << strerror(e) << endl;
            return false;
        }
    }
    return true;
}


static void
addPrerequisitesFromIncludeMarkers(FILE *source)
{
    char buffer[8192];
    while (fgets(buffer, sizeof(buffer), source) != NULL)  // while a line can be read
    {
        // Extract an #included file path, if applicable.
        string filename;
        if (parseIncludeMarker(buffer, filename))
            TranslationUnit::instance().addPrerequisiteFilename(filename.c_str());
    }
}


// Generates the assembly file and invokes the assembler on that file.
//
// Returns EXIT_SUCCESS or EXIT_FAILURE.
//
int
Parameters::compileCFile(const string &inputFilename,
                         const string &moduleName,
                         const string &asmFilename,
                         const string &compilationOutputFilename,
                         const char *targetPlatformName,
                         const char *targetPreprocId)
{
    assert(!compilationOutputFilename.empty());

    // Check that we don't have NOT generatePrerequisitesFile AND generatePrerequisitesFileOnly.
    // It might be good to replace these two booleans with a 3-valued enum.
    //
    assert(generatePrerequisitesFile || !generatePrerequisitesFileOnly);

    if (verbose)
    {
        cout << "Version: " << VERSION << "\n";
        cout << "Target platform: " << targetPlatformName << "\n";
        cout << "Preprocessing: " << inputFilename << endl;
    }

    // Call the C preprocessor on the source file and prepare to read its output:
    //
    string cppCommand = buildCppCommand(inputFilename, targetPreprocId);
    if (verbose)
        cout << "Preprocessor command: " << cppCommand << endl;

    extern FILE *yyin;
    yyin = popen(cppCommand.c_str(), "r");
    if (yyin == NULL)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "could not start C preprocessor (through pipe):"
                                            << " " << strerror(e) << endl;
        return EXIT_FAILURE;
    }

    PipeCloser preprocFileCloser("preprocessor", yyin);

    // If this pointer receives an instance of TranslationUnit, that instance will be
    // destroyed before leaving this function.
    //
    std::unique_ptr<TranslationUnit> tuInstance;

    if (preprocOnly)
    {
        // The compiler received -E, so it only generates the output of cpp, to stdout.
        //
        tuInstance.reset(new TranslationUnit(*this));

        if (!copyLinesToCOut(yyin))
            return EXIT_FAILURE;
        return preprocFileCloser.close() != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    if (generatePrerequisitesFileOnly)
    {
        // The compiler received --deps-only, so it only reads the cpp output lines that
        // declare a dependency on another file, and calls addPrerequisiteFilename() for each.
        //
        tuInstance.reset(new TranslationUnit(*this));

        addPrerequisitesFromIncludeMarkers(yyin);
        if (preprocFileCloser.close() != 0)
            return EXIT_FAILURE;
    }


    if (numErrors == 0 && !generatePrerequisitesFileOnly)
    {
        tuInstance.reset(new TranslationUnit(*this));
        TranslationUnit &tu = TranslationUnit::instance();

        if (verbose)
            cout << "Compiling..." << endl;
        assert(yyin != NULL);

        int yyparse(void);
        yyparse();  // invoke parser.yy

        if (preprocFileCloser.close() != 0)
            return EXIT_FAILURE;


        uint16_t pragmaStackSpace = 0;

        tu.processPragmas(codeAddress, codeAddressSetBySwitch,
                          limitAddress, limitAddressSetBySwitch,
                          dataAddress, dataAddressSetBySwitch,
                          pragmaStackSpace, compileOnly);

        /*  Apply #pragma stack_space only if --stack-space not used.
        */
        if (pragmaStackSpace != 0 && !stackSpaceSpecifiedByCommandLine)
            stackSpace = pragmaStackSpace;

        if (targetPlatform == VECTREX)
        {
            // On the Vectrex, the writable globals must be mapped at $C880
            // while the code and read-only globals are mapped at the start of memory.
            //
            dataAddress = 0xC880;  // equivalent to --data=C880

            // The Vectrex is limited in RAM space and shares the stack with freely available memory.
            // $C880 - $CBEA is user RAM (874 bytes).
            // $CBEA is Vec_Default_Stk (Default top-of-stack).
            //
            stackSpace = 256;
        }

        if (verbose && targetPlatform != OS9)
        {
            cout << "Code address: $" << hex << codeAddress << dec << " (" << codeAddress << ")\n";
            cout << "Data address: ";
            if (dataAddress == 0xFFFF)
                cout << "after the code";
            else
                cout << "$" << hex << dataAddress << dec << " (" << dataAddress << ")";
            cout << "\n";
        }

        if (numErrors == 0)
        {
            tu.checkSemantics();  // this is when Scope objects get created in FunctionDefs

            tu.allocateLocalVariables();  // in all FunctionDef objects
        }

        std::unique_ptr<ASMText> asmText;

        // Emit the assembly language instructions and directives (in memory)
        // from the syntax tree, then optimize it.
        //
        if (targetPlatform != INTERM_REP)
        {
            // Create an in-memory object that accumulates 6809 instructions.
            //
            asmText.reset(new ASMText(yRegAllowed));

            if (numErrors == 0)
            {
                tu.emitAssembler(*asmText.get(), dataAddress, initialStackRegValue, stackSpace, extraStackSpace, assumeTrack34);

                asmText->peepholeOptimize(optimizationLevel, framePointerOption);
                if (wholeFunctionOptimization)
                    asmText->optimizeWholeFunctions();
                
                // Emit the utility routine imports and the END directive.
                tu.finishEmittingAssembler(*asmText.get());
            }
        }


        /*  Now that yyparse() and emitAssembler() have been called, free the memory,
            to keep valgrind from reporting a leak.
        */
        extern string sourceFilename;
        sourceFilename.clear();


        if (numErrors == 0)
        {
            if (targetPlatform != INTERM_REP)
            {
                /*  Create an asm file that will receive the assembly language code:
                */
                assert(asmText.get());
                int exitCode = createAssemblyLanguageFile(*asmText.get(), asmFilename);
                if (exitCode != EXIT_SUCCESS)
                    return exitCode;
            }
            else
            {
                ofstream irFile(compilationOutputFilename);
                if (!irFile)
                {
                    cout << PACKAGE << fatalErrorPrefix << "failed to create " << compilationOutputFilename << "\n";
                    return EXIT_FAILURE;
                }
                if (!tu.emitInternalRepresentation(irFile))
                {
                    cout << PACKAGE << fatalErrorPrefix << "generating intermediate representation: Not supported.\n";
                    return EXIT_FAILURE;
                }
            }
        }

        if (verbose)
            cout << numErrors << " error(s)"
                    << ", " << numWarnings << " warning(s)." << endl;

        if (numErrors > 0)
            return EXIT_FAILURE;

        if (numWarnings > 0 && treatWarningsAsErrors)
            return EXIT_FAILURE;
    }

    if (generatePrerequisitesFile)
    {
        if (!writePrerequisites(compilationOutputFilename))  // if writing .d file failed
            return EXIT_FAILURE;
        if (generatePrerequisitesFileOnly)
            return EXIT_SUCCESS;
    }

    // Invoke the assembler as a separate command.
    //
    if (!genAsmOnly)
    {
        string lstFilename = useIntDir(moduleName + ".lst");
        int status = invokeAssembler(asmFilename, compilationOutputFilename, lstFilename, targetPreprocId);
        if (compileOnly || status != EXIT_SUCCESS)
            return status;
    }

    return EXIT_SUCCESS;
}


int
Parameters::invokeAssembler(const string &inputFilename,
                            const string &objectFilename,
                            const string &lstFilename,
                            const string &targetPreprocId) const
{
    // Assemble the asm to a .o object file.

    string lwasmCmdLine = lwasmPath
                          + " -fobj --pragma=" + lwasmPragma
                          + " -D" + targetPreprocId
                          + " --output=" + shq(objectFilename)
                          + (intermediateFilesKept ? " --list=" + shq(lstFilename) : string())
                          + " " + shq(inputFilename);
    if (verbose)
        cout << "Assembler command: " << lwasmCmdLine << endl;

    int status = system(lwasmCmdLine.c_str());
    if (status == -1)
    {
        int e = errno;
        cout << PACKAGE << fatalErrorPrefix << "could not start assembler: "
                                            << strerror(e) << endl;
        return EXIT_FAILURE;
    }

    if (verbose)
        cout << "Exit code from assembler command: " << WEXITSTATUS(status) << "\n";

    if (!WIFEXITED(status))
        return EXIT_FAILURE;
    status = WEXITSTATUS(status);
    if (status != 0)
        return status;

    return EXIT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////


int
PipeCloser::close()
{
    if (file == NULL)
        return 0;  // success: nothing to do
    int status = pclose(file);
    file = NULL;
    if (!WIFEXITED(status))
    {
        cout << PACKAGE << fatalErrorPrefix << runningTool << " terminated abnormally." << endl;
        assert(status != 0);
    }
    else if (WEXITSTATUS(status) != 0)
    {
        cout << PACKAGE << fatalErrorPrefix << runningTool << " failed." << endl;
        assert(status != 0);
    }
    return status;
}
