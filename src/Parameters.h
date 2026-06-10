/*  CMOC - A C-like cross-compiler
    Copyright (C) 2023-2025 Pierre Sarrazin <http://sarrazip.com/>

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

#ifndef _H_Parameters
#define _H_Parameters

#include "SwitchStmt.h"


class Parameters
{
public:

    // Argument for the ORG directive.
    // The default is 512 bytes past the default start of a Basic program
    // on a Disk Basic CoCo ($2600).
    //
    uint16_t codeAddress;
    uint16_t dataAddress;  // by default, data section follows code section
    bool codeAddressSetBySwitch;  // true if --org or --dos used
    bool dataAddressSetBySwitch;  // true if --data used

    int32_t initialStackRegValue;  // if in 0..0xFFFF, then LDS #initialStackRegValue is generated as 1st instruction
    uint16_t stackSpace;  // applies to --check-stack and sbrk(); not for OS-9
    uint16_t extraStackSpace;  // OS-9 only

    uint32_t functionStackSpace;  // uint32_t(-1) means not set by command-line argument

    std::string pkgdatadir;  // directory where CMOC support files (header files, etc.) are installed
    std::string cmocfloatlibdir;  // directory where CMOC floating-point library files and object files are installed

    std::string cppExecutablePath;  // path of the executable of the C preprocessor (or basename that must be in $PATH)
    std::string lwasmPath;
    std::string lwasmPragma;  // comma-separated list
    std::string lwlinkPath;

    bool intermediateFilesKept;
    std::string intermediateDir;  // files where intermediate files (e.g., .lst, .i) are created;
                             // if empty, use user-specified directory

    bool generatePrerequisitesFile;      // --deps option
    bool generatePrerequisitesFileOnly;  // --deps-only option
    std::string prerequisiteFilename;  // if not empty, used instead of forming .d from .o
    std::string prerequisiteRuleTarget;  // string that must appear as the target of the dependency rule;
                                         // if empty, the compilation output filename is used instead by compileCFile()

    bool preprocOnly;
    bool genAsmOnly;  // generate assembly language file (.s) only, i.e., -S option was used
    bool compileOnly;
    bool verbose;
    bool treatWarningsAsErrors;
    bool inhibitAllWarningMessages;
    bool nullPointerCheckingEnabled;
    bool stackOverflowCheckingEnabled;
    TargetPlatform targetPlatform;
    bool assumeTrack34;  // true = CoCo DECB Track 34 (relevant only with COCO_BASIC)
    std::string forcedLWLinkFormat;  // if not empty, passed to --format option of lwlink
    bool callToUndefinedFunctionAllowed;
    bool warnSignCompare;
    bool warnPassingConstForFuncPtr;
    bool isConstIncorrectWarningEnabled;
    bool isBinaryOpGivingByteWarningEnabled;
    bool isLocalVariableHidingAnotherWarningEnabled;
    bool isNonLiteralPrintfFormatWarningEnabled;
    bool isUncalledStaticFunctionWarningEnabled;
    bool isMissingFieldInitializersWarningEnabled;
    bool inlineAsmArrayIndexesWarningEnabled;
    bool forConditionComparesDifferentSizesWarningEnabled;
    bool warnArrayWithUnknownFirstDimension;
    bool warnArraySizeZero;
    bool warnTooManyElementsInInitializer;
    bool warnShiftAlwaysZero;
    bool warnLabelOnDeclaration;
    bool warnAssignmentInCondition;
    bool onlyWarningOnWordSizedReturnTypeMismatch;

    bool wholeFunctionOptimization;
    bool forceJumpMode;
    SwitchStmt::JumpMode forcedJumpMode;
    size_t optimizationLevel;
    bool yRegAllowed;  // allow low-level optimizer to use Y register (see ASMText::isYRegisterAllowed())
    FramePointerOption framePointerOption;
    bool stackSpaceSpecifiedByCommandLine;
    uint16_t limitAddress;  // see --limit; 0xFFFF means not applicable
    bool limitAddressSetBySwitch;  // true if --limit used
    bool assumeCFileByDefault;  // if true, any file other than .s, .asm, .o, .a is assumed to be a C file; if false, only .c files compiled as C
    std::string outputFilename;
    std::vector<std::string> libDirs;  // list of directories to pass to lwlink via -L options
    bool useDefaultLibraries;  // if false, only libcmoc-crt-*.a is used
    FloatingPointLibrary floatingPointLibrary;  // library that provides floating point arithmetic routines
    bool relocatabilitySupported;
    bool isCharSignedByDefault;

    std::list<std::string> includeDirList;
    bool searchDefaultIncludeDirs;

    std::list<std::string> defines;

    static uint32_t getVersionInteger();

public:

    Parameters();

    // May change some parameters of this object.
    //
    int compileCFile(const std::string &inputFilename,
                     const std::string &moduleName,
                     const std::string &asmFilename,
                     const std::string &compilationOutputFilename,
                     const char *targetPlatformName,
                     const char *targetPreprocId);

    int invokeAssembler(const std::string &inputFilename,
                        const std::string &objectFilename,
                        const std::string &lstFilename,
                        const std::string &targetPreprocId) const;

    // Returns 's' if no intermediate directory has been specified or
    // if 's' already contains a directory specification.
    // Otherwise, returns the basename of 's' preceded by the specified intermediate directory.
    //
    std::string useIntDir(const std::string &s) const;

    // True only for ".c", unless assumeCFileByDefault is true.
    //
    bool isCFileExtension(const std::string &extension) const;

    // Prints to cout.
    // Returns 1.
    //
    int declareInvalidOption(const std::string &opt) const;

    // Prints to cout.
    //
    void displayVersionNo() const;

    // Prints to cout.
    //
    void displayHelp() const;

private:

    std::string buildCppCommand(const std::string &inputFilename,
                                const char *targetPreprocId);
    int createAssemblyLanguageFile(const ASMText &asmText,
                                   const std::string &asmFilename);
    bool writePrerequisites(const std::string &compilationOutputFilename);

};


class PipeCloser
{
public:
    PipeCloser(const char *_runningTool, FILE *_file)
        : runningTool(_runningTool), file(_file) {}
    ~PipeCloser() { (void) close(); }

    // Issues error message to cout upon error.
    // Returns value returned by pclose() (0 means success).
    int close();

private:
    // Forbidden:
    PipeCloser(const PipeCloser &);
    PipeCloser &operator = (const PipeCloser &);
private:
    std::string runningTool;
    FILE *file;
};


#endif  /* _H_Parameters */
