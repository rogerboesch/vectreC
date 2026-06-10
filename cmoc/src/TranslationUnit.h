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

#ifndef _H_TranslationUnit
#define _H_TranslationUnit

#include "TypeManager.h"
#include "Scope.h"

class Tree;
class TreeSequence;
class Declaration;
class FunctionDef;
class ClassDef;
class DeclarationSequence;
class DeclarationSpecifierList;
class Declarator;
class Parameters;


class TranslationUnit
{
public:

    // The constructor must have been called before calling this method.
    //
    static TranslationUnit &instance()
    {
        assert(theInstance);
        return *theInstance;
    }

    static bool hasInstance() { return theInstance != NULL; }

    static TypeManager &getTypeManager() { return instance().typeManager; }

    // After calling this, instance() returns the lone instance of this class.
    // Must not be called again before destroying the instance.
    //
    // The TranslationUnit instance keeps a reference to 'params'.
    //
    TranslationUnit(const Parameters &params);

    // Resets the instance pointer. Do not call instance() after this.
    ~TranslationUnit();

    // defList: Must come from 'new' (or be null). Ownership transferred to TranslationUnit.
    void setDefinitionList(TreeSequence *defList);

    // Stores 'fd' in a table (indexed by function identifiers) used by getFunctionDef().
    void addFunctionDef(FunctionDef *fd);

    // Removes 'fd' from the table used by getFunctionDef().
    void removeFunctionDef(FunctionDef *fd);

    // See also addFunctionDef() and removeFunctionDef().
    FunctionDef *getFunctionDef(const std::string &functionId);
    std::string getFunctionLabel(const std::string &functionId);

    const FunctionDef *getFunctionDefFromScope(const Scope &functionScope) const;

    // Emits an error message if the function is not valid.
    // Calls addFunctionDef() otherwise.
    void registerFunction(FunctionDef *fd);

    // Remembers that the named caller calls the named callee.
    // See detectCalledFunctions().
    void registerFunctionCall(const std::string &callerId, const std::string &calleeId);

    // Returns the register used to pass the first parameter of a function,
    // WHEN the first parameter is passed this way.
    // CMOC's default calling convention is to pass all function parameters on the stack.
    //
    Register getFirstParameterRegister(bool isParam8Bits) const;

    void pushCurrentDefaultCallConvention(CallConvention callConv);

    CallConvention popCurrentDefaultCallConvention();

    CallConvention getCurrentDefaultCallConvention() const;

    void checkSemantics();

    TargetPlatform getTargetPlatform() const;

    FloatingPointLibrary getFloatingPointLibrary() const;

    bool isCharSignedByDefault() const;

    // Indicates if the platform the compiler is generating code for uses the Y register,
    // typically as a data segment pointer. On platforms that do not do this, Y may be
    // used in generated code.
    // See getDataIndexRegister().
    //
    bool targetPlatformUsesY() const;

    // Get the name of the register (i.e., Y or PCR, i.e., data segment or code segment)
    // that is used to refer to global data.
    //
    const char *getDataIndexRegister(bool prefixWithComma, bool readOnly) const;

    // Like getDataIndexRegister(), but for string, long and floating-point literals.
    // Despite being data, these things may reside in the read-only code segment because
    // they are not supposed to be modified by the C program.
    //
    const char *getLiteralIndexRegister(bool prefixWithComma) const;

    void allocateLocalVariables();

    // Calls methods on the ASMText to emit instructions and directives in an in-memory format
    // contained in the ASMText.
    // Does not actually write an assembly language file.
    // allocateLocalVariables() must have been called.
    //
    // N.B.: finishEmittingAssembler() MUST be called soon after this to emit imports for
    //       the utility routines and to emit the END directive.
    //
    // Stops short if an error is detected.
    // dataAddress: 0xFFFF is the data section is NOT separate from the code/read-only section.
    // initialStackRegValue: See Parameters::initialStackRegValue.
    // stackSpace: Ignored when targetting OS-9.
    // extraStackSpace: Only used when targetting OS-9.
    // emitBootLoaderMarker: Only used when targetting COCO_BASIC.
    //
    void emitAssembler(ASMText &out, uint16_t dataAddress,
                       int32_t initialStackRegValue,
                       uint16_t stackSpace, uint16_t extraStackSpace,
                       bool emitBootLoaderMarker);

    // Emits IMPORT directives for each utility routine registered with registerNeededUtility().
    // Emits the END directive.
    //
    void finishEmittingAssembler(ASMText &out);

    void pushScope(Scope *scope);
    Scope *getCurrentScope();
    void popScope();

    Scope &getGlobalScope();

    void pushBreakableLabels(const std::string &brkLabel,
                                        const std::string &contLabel);
    const BreakableLabels *getCurrentBreakableLabels();
    void popBreakableLabels();

    void setCurrentFunctionEndLabel(const std::string &label);
    std::string getCurrentFunctionEndLabel();

    static std::string genLabel(char letter);
    std::string generateLabel(char letter);

    // Returns the assembly label now associated with the contents of sle's literal string.
    // A given string literal is not registered twice, so as to save memory.
    //
    std::string registerStringLiteral(const class StringLiteralExpr &sle);

    std::string getEscapedStringLiteral(const std::string &stringLabel);

    // Returns an empty string if the real constant cannot be represented
    // on the targeted platform.
    // A given real constant is not registered twice, so as to save memory.
    //
    std::string registerRealConstant(const class RealConstantExpr &rce);

    // A given dword constant is not registered twice, so as to save memory.
    //
    std::string registerDWordConstant(const class DWordConstantExpr &rce);

    int16_t getTypeSize(const TypeDesc &typeDesc) const;

    const ClassDef *getClassDef(const std::string &className) const;

    ClassDef *getClassDef(const std::string &className);

    // See Tree::callUtility().
    void registerNeededUtility(const std::string &utilitySubRoutine);

    const std::set<std::string> &getNeededUtilitySubRoutines() const;

    bool isRelocatabilitySupported() const;

    FramePointerOption getFramePointerOption() const;

    // Called during parsing, right after the parser recognized the #pragma directive.
    // May change some fields of this TranslationUnit object.
    // Issues errors or warnings as needed.
    //
    void processParseTimePragma(class Pragma &pragma);

    // Processes #pragma directives that need to be processed AFTER parsing.
    //
    // Changes 'codeAddress' iff a '#pragma org' directive is seen.
    // codeAddressSetBySwitch: Indicates if the code address was set by a command-line argument.
    //
    // Changes 'codeLimitAddress' iff a '#pragma limit' directive is seen.
    // codeLimitAddressSetBySwitch: Indicates if the code limit address was set by a command-line argument.
    //
    // Changes 'dataAddress' iff a '#pragma data' directive is seen.
    //
    // May change some fields of this TranslationUnit object.
    // Issues errors or warnings as needed.
    //
    void processPragmas(uint16_t &codeAddress, bool codeAddressSetBySwitch,
                        uint16_t &codeLimitAddress, bool codeLimitAddressSetBySwitch,
                        uint16_t &dataAddress, bool dataAddressSetBySwitch,
                        uint16_t &stackSpace,
                        bool compileOnly);

    /** Indicates if accesses to pointers must include a run-time null pointer check.
    */
    bool isNullPointerCheckingEnabled() const;

    /** Indicates if stack overflows are checked for at the beginning of a function.
    */
    bool isStackOverflowCheckingEnabled() const;

    DeclarationSequence *createDeclarationSequence(DeclarationSpecifierList *dsl,
                                                   std::vector<Declarator *> *declarators);

    static void checkForEllipsisWithoutNamedArgument(const FormalParamList *formalParamList);

    bool areAllWarningMessagesInhibited() const;

    bool isWarningOnSignCompareEnabled() const;

    bool isWarningOnPassingConstForFuncPtr() const;

    bool isFloatingPointSupported() const;

    bool isProgramMultiThreaded() const;

    void warnIfFloatUnsupported();

    bool warnOnConstIncorrect() const;

    bool warnOnBinaryOpGivingByte() const;

    bool warnOnLocalVariableHidingAnother() const;

    bool warnOnNonLiteralPrintfFormat() const;

    bool warnOnUncalledStaticFunction() const;

    bool warnOnMissingFieldInitializers() const;

    bool warnAboutInlineAsmArrayIndexes() const;

    void enableVolatileWarning();

    bool warnIfForConditionComparesDifferentSizes() const;

    bool warnArrayWithUnknownFirstDimension() const;

    bool warnArraySizeZero() const;

    bool warnTooManyElementsInInitializer() const;

    bool warnShiftAlwaysZero() const;

    bool warnLabelOnDeclaration() const;

    bool warnAssignmentInCondition() const;

    // If true, a type mismatch on a return statement, where the two types are words, pointers or arrays,
    // only gives a warning instead of an error.
    // Example: int foo() { return "bar"; }
    //
    bool onlyWarningOnWordSizedReturnTypeMismatch() const;

    // Adds the given filename to the list of filenames that the current
    // translation unit depends on.
    //
    void addPrerequisiteFilename(const char *filename);

    // Writes a makefile prerequisite rule.
    // Files whose path begins with the string in pkgdatadir (case sensitive) are not listed.
    // dependenciesFilename: If not empty, is written as the second target of the rule.
    // outputFilename: The path of the file that is the target of the rule.
    // pkgdatadir: Files whose path begins with this string (case sensitive) are not listed,
    //             as they are considered system header files.
    //
    void writePrerequisites(std::ostream &out,
                            const std::string &dependenciesFilename,
                            const std::string &outputFilename,
                            const std::string &pkgdatadir) const;

    // Adds decl to globalVariables if not extern.
    // Declares decl in globalScope.
    //
    void declareGlobal(Declaration *decl);

    bool emitInternalRepresentation(std::ostream &irFile);

private:

    void detectCalledFunctions();
    CodeStatus emitPrecedingVerbatimAssemblyBlocks(ASMText &out, std::vector<Tree *>::const_iterator it);
    CodeStatus emitAssemblerStmts(ASMText &out,
                                  std::vector<Tree *>::const_iterator begin,
                                  std::vector<Tree *>::const_iterator end);
    static std::string resolveVectrexMusicAddress(const std::string &symbol);
    void emitProgramEnd(ASMText &out) const;
    CodeStatus emitWritableGlobals(ASMText &out) const;
    CodeStatus emitGlobalVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const;
    CodeStatus emitLocalStaticVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const;
    void determineIfGlobalIsReadOnly(Declaration &decl) const;
    void checkConstDataDeclarationInitializer(const Declaration &decl) const;
    void markGlobalDeclarations();
    void setTypeDescOfGlobalDeclarationClasses();
    void setGlobalDeclarationLabels();
    void declareFunctions();
    void declareFunctionLocalStaticVariables();
    bool adjustFunctionDefReturnTypeFromPrototype(FunctionDef &fd,
                                                  FunctionDef &preexisting);

    struct StandardFunctionDeclaration
    {
        enum { MAXREQS = 1 };

        const char *name;  // C name of the function (e.g., "sprintf")
        const char *required[MAXREQS];  // C names of the functions needed by 'name'() (e.g., "printf", when name is "sprintf")
    };

private:

    typedef std::map<std::string, FunctionDef *> FunctionDefTable;

    static TranslationUnit *theInstance;

    const Parameters &params;
    TypeManager typeManager;
    Scope *globalScope;  // Scope tree must be destroyed after the TreeSequences in definitionList
    TreeSequence *definitionList;  // owns the object, which must have been allocated with 'new'
    FunctionDefTable functionDefs;  // does not own the pointed objects
    StringGraph callGraph;  // maps a function to the functions it calls
    std::vector<Declaration *> globalVariables;  // in order of declaration; does not own the pointed objects
    std::vector<Scope *> scopeStack;  // does not own the pointed objects
    std::vector<BreakableLabels> breakableStack;
    std::string functionEndLabel;  // assembly label of the function currently being emitted
    size_t labelGeneratorIndex;
    std::vector<CallConvention> callConventionStack;

    typedef std::map<std::string, const StringLiteralExpr *> StringLiteralToExprMap;  // key = asm label
    StringLiteralToExprMap stringLiteralLabelToValue;
    std::map<std::string, std::string> stringLiteralValueToLabel;
                        // key = string value, value = asm label

    std::map< std::string, std::vector<uint8_t> > realConstantLabelToValue;
                        // key = asm label, value = float representation
    std::map< std::vector<uint8_t>, std::string > realConstantValueToLabel;
                        // key = float representation, value = asm label

    std::map< std::string, std::vector<uint8_t> > dwordConstantLabelToValue;
                        // key = asm label, value = dword representation
    std::map< std::vector<uint8_t>, std::string > dwordConstantValueToLabel;
                        // key = dword representation, value = asm label

    std::map<std::string, std::string> builtInFunctionDescs;
    bool warnedAboutUnsupportedFloats;
    bool warnedAboutVolatile;
    std::set<std::string> neededUtilitySubRoutines;

    // Vectrex fields:
    std::string vxTitle;    
    std::string vxMusic;
    int8_t vxTitleSizeWidth;
    int8_t vxTitleSizeHeight;
    int8_t vxTitlePosX;
    int8_t vxTitlePosY;
    std::string vxCopyright;

    std::vector<std::string> prerequisiteFilenamesSeen;  // as listed by cpp output

    // Forbidden operations:
    TranslationUnit(const TranslationUnit &);
    TranslationUnit & operator = (const TranslationUnit &);
};


#endif  /* _H_TranslationUnit */
