/*  $Id: TranslationUnit.h,v 1.63 2020/05/07 01:04:08 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2018 Pierre Sarrazin <http://sarrazip.com/>

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


class TranslationUnit
{
public:

    static TranslationUnit &instance()
    {
        assert(theInstance);
        return *theInstance;
    }

    static TypeManager &getTypeManager() { return instance().typeManager; }

    static void createInstance(TargetPlatform targetPlatform,
                               bool callToUndefinedFunctionAllowed,
                               bool warnSignCompare,
                               bool warnPassingConstForFuncPtr,
                               bool isConstIncorrectWarningEnabled,
                               bool isBinaryOpGivingByteWarningEnabled,
                               bool isLocalVariableHidingAnotherWarningEnabled,
                               bool relocatabilitySupported);

    static void destroyInstance();

    // Resets the instance pointer. Do not call instance() after this.
    ~TranslationUnit();

    // defList: Must come from 'new' (or be null). Ownership transfered to TranslationUnit.
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

    void checkSemantics();

    void setTargetPlatform(TargetPlatform platform);

    TargetPlatform getTargetPlatform() const;

    // Get the register (i.e., Y or PCR, i.e., data segment or code segment) that is used
    // to refer to global data.
    //
    const char *getDataIndexRegister(bool prefixWithComma, bool readOnly) const;

    // Like getDataIndexRegister(), but for string, long and floating-point literals.
    // Despite being data, these things may reside in the read-only code segment because
    // they are not supposed to be modified by the C program.
    //
    const char *getLiteralIndexRegister(bool prefixWithComma) const;

    void allocateLocalVariables();

    // allocateLocalVariables() must have been called.
    void emitAssembler(ASMText &out, uint16_t dataAddress, uint16_t stackSpace, bool emitBootLoaderMarker);

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

    void processPragmas(uint16_t &codeAddress, bool codeAddressSetBySwitch,
                        uint16_t &codeLimitAddress, bool codeLimitAddressSetBySwitch,
                        uint16_t &dataAddress, bool dataAddressSetBySwitch,
                        uint16_t &stackSpace,
                        bool compileOnly);

    /** Determines if accesses to pointers will include a run-time null pointer check.
    */
    void enableNullPointerChecking(bool enable);

    /** Indicates if accesses to pointers must include a run-time null pointer check.
    */
    bool isNullPointerCheckingEnabled() const;

    /** Determines if stack overflows are checked for at the beginning of a function.
    */
    void enableStackOverflowChecking(bool enable);

    /** Indicates if stack overflows are checked for at the beginning of a function.
    */
    bool isStackOverflowCheckingEnabled() const;

    DeclarationSequence *createDeclarationSequence(DeclarationSpecifierList *dsl,
                                                   std::vector<Declarator *> *declarators);

    static void checkForEllipsisWithoutNamedArgument(const FormalParamList *formalParamList);

    bool isCallToUndefinedFunctionAllowed() const;

    bool isWarningOnSignCompareEnabled() const;

    bool isWarningOnPassingConstForFuncPtr() const;

    void warnIfFloatUnsupported();

    bool warnOnConstIncorrect() const;

    bool warnOnBinaryOpGivingByte() const;

    bool warnOnLocalVariableHidingAnother() const;

    void warnAboutVolatile();

    // Adds the given filename to the list of filenames that the current
    // translation unit depends on.
    //
    void addPrerequisiteFilename(const char *filename);

    // Writes a makefile prerequisite rule.
    // Files whose path begins with the string in pkgdatadir (case sensitive) are not listed.
    //
    void writePrerequisites(std::ostream &out,
                            const std::string &dependenciesFilename,
                            const std::string &objectFilename,
                            const std::string &pkgdatadir) const;

    // Adds decl to globalVariables if not extern.
    // Declares decl in globalScope.
    //
    void declareGlobal(Declaration *decl);

private:

    TranslationUnit(TargetPlatform targetPlatform,
                    bool _callToUndefinedFunctionAllowed,
                    bool _warnSignCompare,
                    bool _warnPassingConstForFuncPtr,
                    bool _isConstIncorrectWarningEnabled,
                    bool _isBinaryOpGivingByteWarningEnabled,
                    bool _isLocalVariableHidingAnotherWarningEnabled,
                    bool _relocatabilitySupported);

    void detectCalledFunctions();
    static std::string resolveVectrexMusicAddress(const std::string &symbol);
    void emitProgramEnd(ASMText &out) const;
    CodeStatus emitWritableGlobals(ASMText &out) const;
    CodeStatus emitGlobalVariables(ASMText &out, bool readOnlySection, bool withStaticInitializer) const;
    void checkConstDataDeclarationInitializer(const Declaration &decl) const;
    void markGlobalDeclarations();
    void setTypeDescOfGlobalDeclarationClasses();
    void setGlobalDeclarationLabels();
    void declareFunctions();

    struct StandardFunctionDeclaration
    {
        enum { MAXREQS = 1 };

        const char *name;  // C name of the function (e.g., "sprintf")
        const char *required[MAXREQS];  // C names of the functions needed by 'name'() (e.g., "printf", when name is "sprintf")
    };

private:

    typedef std::map<std::string, FunctionDef *> FunctionDefTable;

    static TranslationUnit *theInstance;

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
    bool relocatabilitySupported;
    bool isProgramExecutableOnlyOnce;
    bool nullPointerCheckingEnabled;
    bool stackOverflowCheckingEnabled;
    bool callToUndefinedFunctionAllowed;
    bool warnSignCompare;  // warn if <, <=, >, >= used on operands of differing signedness
    bool warnPassingConstForFuncPtr;
    bool warnedAboutUnsupportedFloats;
    bool isConstIncorrectWarningEnabled;
    bool isBinaryOpGivingByteWarningEnabled;
    bool isLocalVariableHidingAnotherWarningEnabled;
    bool warnedAboutVolatile;
    std::set<std::string> neededUtilitySubRoutines;
    TargetPlatform targetPlatform;

    // Vectrex fields:
    std::string vxTitle;    
    std::string vxMusic;
    int8_t vxTitleSizeWidth;
    int8_t vxTitleSizeHeight;
    int8_t vxTitlePosX;
    int8_t vxTitlePosY;
    std::string vxCopyright;

    std::vector<std::string> sourceFilenamesSeen;  // as listed by cpp output

    // Forbidden operations:
    TranslationUnit(const TranslationUnit &);
    TranslationUnit & operator = (const TranslationUnit &);
};


// inline because called often by parser.yy
inline void
TranslationUnit::warnIfFloatUnsupported()
{
    if (warnedAboutUnsupportedFloats)
        return;
    if (targetPlatform != COCO_BASIC && targetPlatform != OS9)
        warnmsg("floating-point arithmetic is not supported on this platform");
    warnedAboutUnsupportedFloats = true;
}


#endif  /* _H_TranslationUnit */
