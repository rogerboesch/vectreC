%{
/*  $Id: parser.yy,v 1.83 2020/02/10 01:58:22 sarrazip Exp $

    CMOC - A C-like cross-compiler
    Copyright (C) 2003-2016 Pierre Sarrazin <http://sarrazip.com/>

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

#include "DeclarationSequence.h"
#include "WordConstantExpr.h"
#include "RealConstantExpr.h"
#include "DWordConstantExpr.h"
#include "IdentifierExpr.h"
#include "VariableExpr.h"
#include "FunctionCallExpr.h"
#include "StringLiteralExpr.h"
#include "BinaryOpExpr.h"
#include "UnaryOpExpr.h"
#include "CastExpr.h"
#include "CompoundStmt.h"
#include "IfStmt.h"
#include "SwitchStmt.h"
#include "LabeledStmt.h"
#include "WhileStmt.h"
#include "ForStmt.h"
#include "JumpStmt.h"
#include "Scope.h"
#include "FunctionDef.h"
#include "FormalParameter.h"
#include "FormalParamList.h"
#include "AssemblerStmt.h"
#include "TranslationUnit.h"
#include "ObjectMemberExpr.h"
#include "ClassDef.h"
#include "ConditionalExpr.h"
#include "DeclarationSpecifierList.h"
#include "Pragma.h"
#include "CommaExpr.h"

#include <string.h>
#include <time.h>

using namespace std;

int yyparse(void);
int yylex(void);
void yyerror(const char *msg);
extern char *yytext;

extern string sourceFilename;  // defined in lexer.ll
extern int lineno;  // defined in lexer.ll
int numErrors = 0;  // error counter for yyerror()
int numWarnings = 0;
static bool doubleTypeWarningIssued = false;

void _PARSERTRACE(int parserLineNo, const char *fmt, ...);
#define PARSERTRACE(...) _PARSERTRACE(__LINE__, __VA_ARGS__)

%}

%expect 20  /* 1 shift/reduce conflict expected for if-else */

%union {
    char *str;
    signed char character;
    double real;
    int integer;
    BasicType basicType;
    const TypeDesc *typeDesc;
    TypeSpecifier *typeSpecifier;
    DeclarationSpecifierList *declarationSpecifierList;
    Tree *tree;
    CompoundStmt *compoundStmt;
    TreeSequence *treeSequence;
    FormalParamList *formalParamList;
    FormalParameter *formalParameter;
    DeclarationSequence *declarationSequence;
    Declarator *declarator;
    std::vector<Declarator *> *declaratorVector;
    BinaryOpExpr::Op binop;
    UnaryOpExpr::Op unop;
    Scope *scope;
    FunctionDef *functionDef;
    std::vector<std::string> *strList;
    ClassDef *classDef;
    ClassDef::ClassMember *classMember;
    FilenameAndLineNo *filenameAndLineNo;
    std::vector<ClassDef::ClassMember *> *classMemberList;
    Enumerator *enumerator;
    std::vector<Enumerator *> *enumeratorList;
    TypeQualifierBitFieldVector *typeQualifierBitFieldVector;
}

%token <str> ID STRLIT PRAGMA
%token <character> CHARLIT
%token <real> REAL
%token <typeDesc> TYPE_NAME
%token INT CHAR SHORT LONG FLOAT DOUBLE SIGNED UNSIGNED VOID PLUS_PLUS MINUS_MINUS IF ELSE WHILE DO FOR
%token EQUALS_EQUALS BANG_EQUALS LOWER_EQUALS GREATER_EQUALS AMP_AMP PIPE_PIPE
%token LT_LT GT_GT BREAK CONTINUE RETURN ASM NORTS VERBATIM_ASM STRUCT UNION THIS
%token PLUS_EQUALS MINUS_EQUALS ASTERISK_EQUALS SLASH_EQUALS PERCENT_EQUALS LT_LT_EQUALS GT_GT_EQUALS
%token CARET_EQUALS AMP_EQUALS PIPE_EQUALS
%token RIGHT_ARROW INTERRUPT SIZEOF ELLIPSIS TYPEDEF ENUM SWITCH CASE DEFAULT REGISTER GOTO EXTERN STATIC CONST VOLATILE AUTO
%token FUNC_RECEIVES_FIRST_PARAM_IN_REG

%type <tree> external_declaration stmt selection_stmt else_part_opt while_stmt do_while_stmt for_stmt expr_stmt labeled_stmt
%type <tree> expr expr_opt logical_or_expr logical_and_expr rel_expr add_expr mul_expr
%type <tree> inclusive_or_expr exclusive_or_expr and_expr
%type <tree> if_cond while_cond assignment_expr equality_expr shift_expr conditional_expr constant_expr
%type <tree> unary_expr cast_expr postfix_expr primary_expr initializer for_init subscript
%type <declarationSequence> declaration
%type <compoundStmt> compound_stmt stmt_list stmt_list_opt
%type <treeSequence> expr_list_opt expr_list translation_unit arg_expr_list
%type <formalParamList> parameter_list parameter_type_list parameter_type_list_opt function_pointer_cast function_pointer_cast_opt
%type <formalParameter> parameter_declaration
%type <treeSequence> initializer_list subscript_list
%type <declaratorVector> init_declarator_list struct_declarator_list
%type <declarator> init_declarator declarator direct_declarator struct_declarator
%type <functionDef> function_definition
%type <binop> add_op mul_op rel_op equality_op assignment_op
%type <unop> unary_op
%type <typeQualifierBitFieldVector> pointer abstract_declarator
%type <integer> save_line_no struct_or_union
%type <typeDesc> basic_type non_void_basic_type type_name struct_or_union_specifier
%type <typeSpecifier> type_specifier enum_specifier
%type <declarationSpecifierList> declaration_specifiers specifier_qualifier_list
%type <integer> storage_class_specifier type_qualifier type_qualifier_list
%type <str> save_src_fn strlit_seq
%type <classDef> struct_declaration_list struct_declaration_list_opt
%type <classMemberList> struct_declaration
%type <enumerator> enumerator
%type <enumeratorList> enumerator_list

%%

goal:
      translation_unit          { TranslationUnit::instance().setDefinitionList($1); }
    |                           { TranslationUnit::instance().setDefinitionList(NULL); }
    ;

translation_unit:
      external_declaration                      
            {
                $$ = new TreeSequence();
                if ($1)
                    $$->addTree($1); 
            }
    | translation_unit external_declaration
            {
                $$ = $1;
                if ($2)
                    $$->addTree($2);
            }
    ;

external_declaration:
      function_definition       { $$ = $1; }
    | declaration               { $$ = $1; }  // can be null in the case of a typedef
    | PRAGMA                    { $$ = new Pragma($1); free($1); }
    | ';'                       { $$ = NULL; }  // tolerate semi-colon after function body
    ;

function_definition:
      declaration_specifiers declarator compound_stmt       /* return type, name, body */
            {
                DeclarationSpecifierList *dsl = $1;
                Declarator *di = $2;

                if (dsl->hasEnumeratorList())
                {
                    errormsg("enum with enumerated names is not supported in a function's return type");
                    dsl->detachEnumeratorList();
                }
                if (dsl->isStaticDeclaration() && dsl->isExternDeclaration())
                {
                    errormsg("function definition must not be both static and extern");
                }

                // Example: In byte **f() {}, dsl represents "byte" and
                // di represents **f. Hence, di contains a pointer level of 2,
                // which is applied to the TypeDesc found in dsl, i.e., "byte".
                // di also contains the name of the function, "f".
                //
                $$ = new FunctionDef(*dsl, *di);
                $$->setLineNo(di->getSourceFilename(), di->getLineNo());
                $$->setBody($3);
                delete di;
                delete dsl;
            }
    ;

parameter_type_list:
      parameter_list                 { $$ = $1; }
    | parameter_list ',' ELLIPSIS    { $$ = $1; $$->endWithEllipsis(); }
    | ELLIPSIS                       { $$ = new FormalParamList(); $$->endWithEllipsis(); }
                                       /* Lone ellipsis tolerated here;
                                          error message in FunctionDef::declareFormalParams(). */
    ;

parameter_list:
      parameter_declaration          { $$ = new FormalParamList(); if ($1) $$->addTree($1); }
    | parameter_list ',' parameter_declaration
                                     { $$ = $1; if ($3) $$->addTree($3); }
    ;

parameter_declaration:
      declaration_specifiers declarator
                {
                    DeclarationSpecifierList *dsl = $1;
                    $$ = $2->createFormalParameter(*dsl);
                    delete dsl;
                    delete $2;
                }
    ;

type_name:
      specifier_qualifier_list function_pointer_cast_opt
                {
                    if ($2)
                    {
                        $$ = TranslationUnit::getTypeManager().getFunctionPointerType(
                                $1->getTypeDesc(), *$2,
                                $1->isInterruptServiceFunction(), $1->isFunctionReceivingFirstParamInReg());
                        delete $2;
                    }
                    else
                        $$ = $1->getTypeDesc();
                    delete $1;
                }
    | specifier_qualifier_list abstract_declarator function_pointer_cast_opt
                {
                    const TypeDesc *td = TranslationUnit::getTypeManager().getPointerTo($1->getTypeDesc(), *$2);
                    if ($3)
                    {
                        $$ = TranslationUnit::getTypeManager().getFunctionPointerType(
                                td, *$3,
                                $1->isInterruptServiceFunction(), $1->isFunctionReceivingFirstParamInReg());
                        delete $3;
                    }
                    else
                        $$ = td;
                    delete $2;  /* originally created by 'pointer' rule */
                    delete $1;
                }
    ;

abstract_declarator:
      pointer           { $$ = $1; }
    ;

function_pointer_cast_opt:
      /* empty */                               { $$ = NULL; }
    | function_pointer_cast                     { $$ = $1; }
    ;

function_pointer_cast:
      '(' '*' ')' '(' parameter_type_list ')'   { $$ = $5; }
    | '(' '*' ')' '(' ')'                       { $$ = new FormalParamList(); }
    ;

pointer:
      '*'                               { $$ = new TypeQualifierBitFieldVector(); $$->push_back(0); }  /* add pointer level with neither CONST_BIT nor VOLATILE_BIT */
    | '*' type_qualifier_list           { $$ = new TypeQualifierBitFieldVector(); $$->push_back(TypeQualifierBitField($2)); }  /* add pointer level with CONST_BIT and/or VOLATILE_BIT */
    | '*' pointer                       { $$ = $2; $$->push_back(0); }
    | '*' type_qualifier_list pointer   { $$ = $3; $$->push_back($2); }
    ;

specifier_qualifier_list:
      type_specifier specifier_qualifier_list   { $$ = $2; $$->add(*$1); delete $1; }
    | type_specifier                            { $$ = new DeclarationSpecifierList(); $$->add(*$1); delete $1; }
    | type_qualifier specifier_qualifier_list   { $$ = $2; if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    | type_qualifier                            { $$ = new DeclarationSpecifierList(); if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    ;

compound_stmt:
      '{' save_src_fn save_line_no stmt_list_opt '}'
                        { $$ = $4; $$->setLineNo($2, $3); free($2); }
    ;

stmt_list_opt:
      /* empty */       { $$ = new CompoundStmt(); }
    | stmt_list         { $$ = $1; }
    ;

/*  May be null in the case of a typedef.
*/
declaration:
      declaration_specifiers ';'  // to define a struct without declaring an instance of it 
                        {
                            $$ = TranslationUnit::instance().createDeclarationSequence($1, NULL);  // deletes $1
                        }
    | declaration_specifiers init_declarator_list ';'  // includes function prototypes
                        {
                            $$ = TranslationUnit::instance().createDeclarationSequence($1, $2);  // deletes $1 and $2
                        }
    ;

// In the future, should be a list of storage_class_specifier, type_specifier or type_qualifier.
//
declaration_specifiers:
      storage_class_specifier
            { $$ = new DeclarationSpecifierList(); if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    | storage_class_specifier declaration_specifiers
            { $$ = $2; if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    | type_specifier
            { $$ = new DeclarationSpecifierList(); $$->add(*$1); delete $1; }
    | type_specifier declaration_specifiers
            { $$ = $2; $$->add(*$1); delete $1; }
    | type_qualifier
            { $$ = new DeclarationSpecifierList(); if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    | type_qualifier declaration_specifiers
            { $$ = $2; if ($1 != -1) $$->add(DeclarationSpecifierList::Specifier($1)); }
    ;

storage_class_specifier:
      INTERRUPT     { $$ = DeclarationSpecifierList::INTERRUPT_SPEC; }
    | FUNC_RECEIVES_FIRST_PARAM_IN_REG { $$ = DeclarationSpecifierList::FUNC_RECEIVES_FIRST_PARAM_IN_REG_SPEC; }
    | TYPEDEF       { $$ = DeclarationSpecifierList::TYPEDEF_SPEC; }
    | ASM           { $$ = DeclarationSpecifierList::ASSEMBLY_ONLY_SPEC; }
    | NORTS         { $$ = DeclarationSpecifierList::NO_RETURN_INSTRUCTION; }
    | REGISTER      { $$ = -1; /* not supported, ignored */ }
    | AUTO          { $$ = -1; /* not supported, ignored */ }          
    | STATIC        { $$ = DeclarationSpecifierList::STATIC_SPEC; }
    | EXTERN        { $$ = DeclarationSpecifierList::EXTERN_SPEC; }
    ;

type_specifier:
      basic_type                    { $$ = new TypeSpecifier($1, "", NULL); }
    | struct_or_union_specifier     { $$ = new TypeSpecifier($1, "", NULL); }
    | struct_or_union ID            { const TypeDesc *td = TranslationUnit::getTypeManager().getClassType($2, $1 == UNION, true);
                                      $$ = new TypeSpecifier(td, "", NULL);
                                      free($2); }
    | struct_or_union TYPE_NAME     { $$ = new TypeSpecifier($2, "", NULL); }
    | enum_specifier                { $$ = $1; }  /* already a TypeSpecifier */
    | TYPE_NAME                     { $$ = new TypeSpecifier($1, "", NULL); }
    ;

type_qualifier:
      CONST                         { $$ = DeclarationSpecifierList::CONST_QUALIFIER; }
    | VOLATILE                      { $$ = DeclarationSpecifierList::VOLATILE_QUALIFIER; TranslationUnit::instance().warnAboutVolatile(); }
    ;

type_qualifier_list:  /* bit field made of CONST_BIT, VOLATILE_BIT */
      type_qualifier                        { $$ = ($1 == DeclarationSpecifierList::CONST_QUALIFIER ? CONST_BIT : VOLATILE_BIT); }
    | type_qualifier_list type_qualifier    { $$ = $1 | ($2 == DeclarationSpecifierList::CONST_QUALIFIER ? CONST_BIT : VOLATILE_BIT); } 
    ;

struct_or_union_specifier:
      struct_or_union ID '{' struct_declaration_list_opt '}'
                    {
                        ClassDef *classDef = $4;
                        assert(classDef);
                        classDef->setName($2);
                        classDef->setUnion($1 == UNION);
                        TranslationUnit::instance().getGlobalScope().declareClass(classDef);
                        const TypeDesc *td = TranslationUnit::getTypeManager().getClassType($2, $1 == UNION, true);
                        assert(td);
                        classDef->setTypeDesc(td);
                        $$ = td;
                        free($2);
                    }
    | struct_or_union '{' struct_declaration_list_opt '}'
                    {
                        string anonStructName = "AnonStruct_" + $3->getLineNo();
                        ClassDef *classDef = $3;
                        assert(classDef);
                        classDef->setName(anonStructName);
                        classDef->setUnion($1 == UNION);
                        TranslationUnit::instance().getGlobalScope().declareClass(classDef);
                        const TypeDesc *td = TranslationUnit::getTypeManager().getClassType(anonStructName, $1 == UNION, true);
                        assert(td);
                        classDef->setTypeDesc(td);
                        $$ = td;
                    }

    ;

struct_or_union:
      STRUCT        { $$ = STRUCT; }
    | UNION         { $$ = UNION;  }
    ;

enum_specifier:
      ENUM ID '{' enumerator_list comma_opt '}'
                        {
                            const TypeDesc *td = TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true);
                            $$ = new TypeSpecifier(td, $2, $4);
                            free($2);
                        }
    | ENUM '{' enumerator_list comma_opt '}'
                        {
                            const TypeDesc *td = TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true);
                            $$ = new TypeSpecifier(td, "", $3);
                        }
    | ENUM ID
                        {
                            const TypeDesc *td = TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true);
                            $$ = new TypeSpecifier(td, $2, NULL);
                            free($2);
                        }
    ;

enumerator_list:
      enumerator                            { $$ = new vector<Enumerator *>(); $$->push_back($1); }
    | enumerator_list ',' enumerator        { $$ = $1; $$->push_back($3); }
    ;

enumerator
    : ID                                    { $$ = new Enumerator($1, NULL, getSourceLineNo()); free($1); }
    | ID '=' constant_expr                  { $$ = new Enumerator($1, $3,   getSourceLineNo()); free($1); }
    ;

comma_opt:
      ','
    | /* empty */
    ;

non_void_basic_type:
      INT       { $$ = TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true); }
    | CHAR      { $$ = TranslationUnit::getTypeManager().getIntType(BYTE_TYPE, true); } 
    | SHORT     { $$ = TranslationUnit::getTypeManager().getIntType(WORD_TYPE, true); }
    | SIGNED    { $$ = TranslationUnit::getTypeManager().getSizelessType(true);  }
    | UNSIGNED  { $$ = TranslationUnit::getTypeManager().getSizelessType(false); }
    | LONG      { $$ = TranslationUnit::getTypeManager().getLongType(true); }
    | FLOAT     {
                    TranslationUnit::instance().warnIfFloatUnsupported();
                    $$ = TranslationUnit::getTypeManager().getRealType(false);
                }
    | DOUBLE    {
                    TranslationUnit::instance().warnIfFloatUnsupported();
                    $$ = TranslationUnit::getTypeManager().getRealType(false);
                    if (!doubleTypeWarningIssued)
                    {
                        warnmsg("`double' is an alias for `float' for this compiler");
                        doubleTypeWarningIssued = true;
                    }
                }
    ;

basic_type:
      non_void_basic_type   { $$ = $1; }
    | VOID                  { $$ = TranslationUnit::getTypeManager().getVoidType(); }
    ;

save_src_fn:
      /* empty */               { $$ = strdup(sourceFilename.c_str()); }
    ;

save_line_no:
      /* empty */               { $$ = lineno; }
    ;

init_declarator_list:
      init_declarator                           { $$ = new std::vector<Declarator *>(); $$->push_back($1); }
    | init_declarator_list ',' init_declarator	{ $$ = $1; $$->push_back($3); }
    ;

// Declarator with optional initializer.
//
init_declarator:
      declarator                        { $$ = $1; }  // Declarator *
    | declarator '=' initializer        { $$ = $1; $$->setInitExpr($3); }
    ;

declarator:
      pointer declarator                {
                                            $$ = $2;
                                            $$->setPointerLevel($1);  // ownership of $1 transfered to the Declarator
                                        }
    | direct_declarator                 { $$ = $1; }
    ;

direct_declarator:
      ID
            {
                $$ = new Declarator($1, sourceFilename, lineno);
                free($1);
            }
    | /* empty (unnamed function parameter) */
            {
                $$ = new Declarator(string(), sourceFilename, lineno);
            }
    | direct_declarator '[' expr_opt ']'
            {
                $$ = $1;
                $$->checkForFunctionReturningArray();
                $$->addArraySizeExpr($3);
            }
    | direct_declarator '(' parameter_type_list ')'
            {
                $$ = $1;
                $$->setFormalParamList($3);
            }
    | direct_declarator '(' ')'
            {
                $$ = $1;
                $$->setFormalParamList(new FormalParamList());
            }
    | direct_declarator '(' VOID ')'
            {
                $$ = $1;
                $$->setFormalParamList(new FormalParamList());
            }
    | '(' '*' ID ')' '(' parameter_type_list_opt ')'  /* function pointer variable */
            {
                $$ = new Declarator($3, sourceFilename, lineno);
                $$->setAsFunctionPointer($6);  // takes ownership of FormalParamList
                free($3);
                TranslationUnit::checkForEllipsisWithoutNamedArgument($6);
            }
    | '(' '*' ')' '(' parameter_type_list_opt ')'  /* unnamed function pointer variable */
            {
                $$ = new Declarator(string(), sourceFilename, lineno);
                $$->setAsFunctionPointer($5);  // takes ownership of FormalParamList
                TranslationUnit::checkForEllipsisWithoutNamedArgument($5);
            }
    | '(' '*' ID subscript_list ')' '(' parameter_type_list_opt ')'  /* array of function pointer variables */
            {
                $$ = new Declarator($3, sourceFilename, lineno);
                $$->setAsArrayOfFunctionPointers($7, $4);  // takes ownership of FormalParamList ($7), deletes $4
                free($3);
                TranslationUnit::checkForEllipsisWithoutNamedArgument($7);
            }
    ;

/* Returns a non-null TreeSequence *.*/
subscript_list:
      subscript                      { $$ = new TreeSequence(); $$->addTree($1); }
    | subscript_list subscript       { $$ = $1; $$->addTree($2); }
    ;
    
subscript:
      '[' expr_opt ']'               { $$ = $2; }
    ;

parameter_type_list_opt:
      /* empty */                    { $$ = new FormalParamList(); }
    | parameter_type_list            { $$ = $1; }
    ;

initializer:
      assignment_expr                { $$ = $1; }
    | '{' initializer_list '}'       { $$ = $2; }
    | '{' initializer_list ',' '}'   { $$ = $2; }
    | '{' '}'       { $$ = new TreeSequence(); }
    | '{' ',' '}'   { $$ = new TreeSequence(); }
    ; 

initializer_list:
      initializer                           { $$ = new TreeSequence(); $$->addTree($1); }
    | initializer_list ',' initializer      { $$ = $1; $$->addTree($3); }
    ;

struct_declaration_list_opt:
      /* empty */                { $$ = new ClassDef(); }
    | struct_declaration_list    { $$ = $1; }
    ;

struct_declaration_list:
      struct_declaration
                {
                    $$ = new ClassDef();
                    if ($1)
                        for (std::vector<ClassDef::ClassMember *>::iterator it = $1->begin(); it != $1->end(); ++it)
                            $$->addDataMember(*it);
                    else
                        assert(0);
                    delete $1;  // destroy the std::vector<ClassDef::ClassMember *>
                }
    | struct_declaration_list struct_declaration
                {
                    $$ = $1;
                    if ($2)
                        for (std::vector<ClassDef::ClassMember *>::iterator it = $2->begin(); it != $2->end(); ++it)
                            $$->addDataMember(*it);
                    else
                        assert(0);
                    delete $2;  // destroy the std::vector<ClassDef::ClassMember *>
                }
    ;

struct_declaration:
      specifier_qualifier_list struct_declarator_list ';'    { $$ = ClassDef::createClassMembers($1, $2); }
    ;

struct_declarator_list:
      struct_declarator                             { $$ = new std::vector<Declarator *>(); if ($1) $$->push_back($1); }
    | struct_declarator_list ',' struct_declarator  { $$ = $1; if ($3) $$->push_back($3); }
    ;

/* May return a null pointer if the member declarator is ignored. */
struct_declarator:
      declarator                        { $$ = $1; }
    | ':' conditional_expr              { $$ = NULL; }  // unnamed bit field member ignored
    | declarator ':' conditional_expr   // named bit field (dimension ignored, except to choose between char/int/long)
                {
                    $$ = $1;
                    $$->setBitFieldWidth(*$3);  // emits error if $3 is not constant expression
                    delete $3;
                }
    ;

stmt_list:
      stmt              { $$ = new CompoundStmt(); if ($1 != NULL) $$->addTree($1); }
    | stmt_list stmt    { $$ = $1; if ($2 != NULL) $$->addTree($2); }
    ;

stmt:
      expr_stmt         { $$ = $1; }
    | declaration       { $$ = ($1 ? $1 : new TreeSequence()); }  // expect null in case of local typedef
    | compound_stmt     { $$ = $1; }
    | selection_stmt    { $$ = $1; }
    | labeled_stmt      { $$ = $1; }
    | while_stmt        { $$ = $1; }
    | do_while_stmt     { $$ = $1; }
    | for_stmt          { $$ = $1; }
    | GOTO ID ';'       { $$ = new JumpStmt($2); free($2); }
    | BREAK ';'         { $$ = new JumpStmt(JumpStmt::BRK, NULL); }
    | CONTINUE ';'      { $$ = new JumpStmt(JumpStmt::CONT, NULL); }
    | RETURN ';'        { $$ = new JumpStmt(JumpStmt::RET, NULL); }
    | RETURN expr ';'   { $$ = new JumpStmt(JumpStmt::RET, $2); }
    | ASM '(' STRLIT ',' ID ')' ';'
                        { $$ = new AssemblerStmt($3, $5, true);
                          free($3); free($5); }
    | ASM '(' STRLIT ',' STRLIT ')' ';'
                        { $$ = new AssemblerStmt($3, $5, false);
                          free($3); free($5); }
    | ASM '(' STRLIT ')' ';'
                        { $$ = new AssemblerStmt($3, "", false);
                          free($3); }
    | VERBATIM_ASM      { $$ = new AssemblerStmt(yytext); }
    ;

labeled_stmt:
      ID save_src_fn save_line_no ':' stmt
                                    {
                                      $$ = new LabeledStmt($1, TranslationUnit::instance().generateLabel('L'), $5);
                                      $$->setLineNo($2, $3);
                                      free($1); free($2);
                                    }
    | CASE constant_expr ':' stmt       { $$ = new LabeledStmt($2, $4); }
    | DEFAULT ':' stmt                  { $$ = new LabeledStmt($3); }   
    ;

constant_expr:
      conditional_expr          { $$ = $1; }
    ;

expr_stmt:
      ';'                       { $$ = new TreeSequence(); }
    | expr ';'                  { $$ = $1; }
    ;

expr_opt:
      /* empty */               { $$ = NULL; }
    | expr                      { $$ = $1; }
    ;

expr:
      assignment_expr           { $$ = $1; }
    | expr ',' assignment_expr  {
                                    Tree *left = $1;
                                    Tree *right = $3;
                                    if (CommaExpr *ts = dynamic_cast<CommaExpr *>(left))
                                    {
                                        ts->addTree(right);
                                        $$ = ts;
                                    }
                                    else
                                        $$ = new CommaExpr(left, right);
                                }
    ;

arg_expr_list:
      assignment_expr                       { $$ = new TreeSequence(); $$->addTree($1); }
    | arg_expr_list ',' assignment_expr     { $1->addTree($3); $$ = $1; }
    ;

assignment_expr:
      conditional_expr                          { $$ = $1; }
    | cast_expr save_src_fn save_line_no assignment_op assignment_expr
                  /* Using cast_expr here (instead of unary_expr) allows parsing the invalid
                     statement "(void *) p = q;", which allows the compiler to issue the
                     "l-value required as left operand of assignment" error message.
                     With unary_expr, the statement would be reported as a syntax error. */
                { $$ = new BinaryOpExpr($4, $1, $5); free($2); }
    ;

assignment_op:
      '='                                       { $$ = BinaryOpExpr::ASSIGNMENT; }
    | PLUS_EQUALS                               { $$ = BinaryOpExpr::INC_ASSIGN; }
    | MINUS_EQUALS                              { $$ = BinaryOpExpr::DEC_ASSIGN; }
    | ASTERISK_EQUALS                           { $$ = BinaryOpExpr::MUL_ASSIGN; }
    | SLASH_EQUALS                              { $$ = BinaryOpExpr::DIV_ASSIGN; }
    | PERCENT_EQUALS                            { $$ = BinaryOpExpr::MOD_ASSIGN; }
    | CARET_EQUALS                              { $$ = BinaryOpExpr::XOR_ASSIGN; }
    | AMP_EQUALS                                { $$ = BinaryOpExpr::AND_ASSIGN; }
    | PIPE_EQUALS                               { $$ = BinaryOpExpr::OR_ASSIGN; }
    | LT_LT_EQUALS                              { $$ = BinaryOpExpr::LEFT_ASSIGN; }
    | GT_GT_EQUALS                              { $$ = BinaryOpExpr::RIGHT_ASSIGN; }
    ;

conditional_expr:
      logical_or_expr                           { $$ = $1; }
    | logical_or_expr '?' expr ':' conditional_expr
                                                { $$ = new ConditionalExpr($1, $3, $5); }
    ;

logical_or_expr:
      logical_and_expr                          { $$ = $1; }
    | logical_or_expr PIPE_PIPE logical_and_expr
                { $$ = new BinaryOpExpr(BinaryOpExpr::LOGICAL_OR, $1, $3); }
    ;

logical_and_expr:
      inclusive_or_expr                         { $$ = $1; }
    | logical_and_expr AMP_AMP inclusive_or_expr
                { $$ = new BinaryOpExpr(BinaryOpExpr::LOGICAL_AND, $1, $3); }
    ;

inclusive_or_expr:
      exclusive_or_expr                         { $$ = $1; }
    | inclusive_or_expr '|' exclusive_or_expr
                { $$ = new BinaryOpExpr(BinaryOpExpr::BITWISE_OR, $1, $3); }

exclusive_or_expr:
      and_expr                                  { $$ = $1; }
    | exclusive_or_expr '^' and_expr
                { $$ = new BinaryOpExpr(BinaryOpExpr::BITWISE_XOR, $1, $3); }

and_expr:
      equality_expr                             { $$ = $1; }
    | and_expr '&' equality_expr
                { $$ = new BinaryOpExpr(BinaryOpExpr::BITWISE_AND, $1, $3); }

equality_expr:
      rel_expr                  { $$ = $1; }
    | equality_expr equality_op rel_expr
                                { $$ = new BinaryOpExpr($2, $1, $3); }
    ;

equality_op:
      EQUALS_EQUALS             { $$ = BinaryOpExpr::EQUALITY; }
    | BANG_EQUALS               { $$ = BinaryOpExpr::INEQUALITY; }
    ;

rel_expr:
      shift_expr                        { $$ = $1; }
    | rel_expr rel_op shift_expr        { $$ = new BinaryOpExpr($2, $1, $3); }
    ;

shift_expr:
      add_expr                  { $$ = $1; }
    | shift_expr LT_LT add_expr { $$ = new BinaryOpExpr(
                                        BinaryOpExpr::LEFT_SHIFT, $1, $3); }
    | shift_expr GT_GT add_expr { $$ = new BinaryOpExpr(
                                        BinaryOpExpr::RIGHT_SHIFT, $1, $3); }
    ;

rel_op:
      '<'                       { $$ = BinaryOpExpr::INFERIOR; }
    | LOWER_EQUALS              { $$ = BinaryOpExpr::INFERIOR_OR_EQUAL; }
    | '>'                       { $$ = BinaryOpExpr::SUPERIOR; }
    | GREATER_EQUALS            { $$ = BinaryOpExpr::SUPERIOR_OR_EQUAL; }
    ;

add_expr:
      mul_expr                  { $$ = $1; }
    | add_expr add_op mul_expr  { $$ = new BinaryOpExpr($2, $1, $3); }
    ;

add_op:
      '+'                       { $$ = BinaryOpExpr::ADD; }
    | '-'                       { $$ = BinaryOpExpr::SUB; }
    ;

mul_expr:
      cast_expr                 { $$ = $1; }
    | mul_expr mul_op cast_expr { $$ = new BinaryOpExpr($2, $1, $3); }
    ;

mul_op:
      '*'                       { $$ = BinaryOpExpr::MUL; }
    | '/'                       { $$ = BinaryOpExpr::DIV; }
    | '%'                       { $$ = BinaryOpExpr::MOD; }
    ;

unary_expr:
      postfix_expr              { $$ = $1; }
    | unary_op cast_expr        {
                                    RealConstantExpr *rce;
                                    DWordConstantExpr *dwce;
                                    if ($1 == UnaryOpExpr::NEG && (rce = dynamic_cast<RealConstantExpr *>($2)) != NULL)
                                    {
                                        // We have the negation of a real constant.
                                        // Simplify by negating the value in the RealConstantExpr and getting rid of the negation operator.
                                        rce->negateValue();
                                        $$ = rce;
                                    }
                                    else if ($1 == UnaryOpExpr::NEG && (dwce = dynamic_cast<DWordConstantExpr *>($2)) != NULL)
                                    {
                                        dwce->negateValue();
                                        $$ = dwce;
                                    }
                                    else
                                    {
                                        $$ = new UnaryOpExpr($1, $2);
                                    }
                                }
    | PLUS_PLUS unary_expr      { $$ = new UnaryOpExpr(UnaryOpExpr::PREINC, $2); }
    | MINUS_MINUS unary_expr    { $$ = new UnaryOpExpr(UnaryOpExpr::PREDEC, $2); }
    | SIZEOF '(' type_name ')'  { $$ = new UnaryOpExpr($3); }
    | SIZEOF unary_expr         { $$ = new UnaryOpExpr(UnaryOpExpr::SIZE_OF, $2); }
    ;

unary_op:
      '+'               { $$ = UnaryOpExpr::IDENTITY; }
    | '-'               { $$ = UnaryOpExpr::NEG; }
    | '&'               { $$ = UnaryOpExpr::ADDRESS_OF; }
    | '*'               { $$ = UnaryOpExpr::INDIRECTION; }
    | '!'               { $$ = UnaryOpExpr::BOOLEAN_NEG; }
    | '~'               { $$ = UnaryOpExpr::BITWISE_NOT; }
    ;

cast_expr:
      unary_expr                    { $$ = $1; }
    | '(' type_name ')' cast_expr   { $$ = new CastExpr($2, $4); }
    ;

postfix_expr:
      primary_expr              { $$ = $1; }
    | postfix_expr save_src_fn save_line_no '(' ')'
                        { $$ = new FunctionCallExpr($1, new TreeSequence());
                          free($2); }
    | postfix_expr save_src_fn save_line_no '(' arg_expr_list ')'
                        { $$ = new FunctionCallExpr($1, $5);
                          free($2); }
    | postfix_expr '[' expr ']'
                        { $$ = new BinaryOpExpr(BinaryOpExpr::ARRAY_REF, $1, $3); }
    | postfix_expr PLUS_PLUS
                        { $$ = new UnaryOpExpr(UnaryOpExpr::POSTINC, $1); }
    | postfix_expr MINUS_MINUS
                        { $$ = new UnaryOpExpr(UnaryOpExpr::POSTDEC, $1); }
    | postfix_expr '.' ID
                        { $$ = new ObjectMemberExpr($1, $3, true); free($3); }
    | postfix_expr RIGHT_ARROW ID
                        { $$ = new ObjectMemberExpr($1, $3, false); free($3); }
    ;

primary_expr:
      ID                { $$ = new IdentifierExpr($1); free($1); }
    | REAL              {
                            bool isHexOrBin = (yytext[0] == '0' && (tolower(yytext[1]) == 'x' || tolower(yytext[1]) == 'b'));
                            double value = $1;
                            if (!isHexOrBin
                                    && (strchr(yytext, '.') || strchr(yytext, 'e') || strchr(yytext, 'E')))  // if point or exponential
                            {
                                $$ = new RealConstantExpr(value, yytext);
                            }
                            else if (strchr(yytext, 'l') || strchr(yytext, 'L') || value > 65535.0 || value <= -32769.0)
                            {
                                bool uSuffix = (strchr(yytext, 'u') || strchr(yytext, 'U'));
                                $$ = new DWordConstantExpr(value, !uSuffix && value <= 0x7FFFFFFFUL);
                            } 
                            else
                            {
                                $$ = new WordConstantExpr(value, yytext);
                            }
                        }
    | CHARLIT           { $$ = new WordConstantExpr((int8_t) $1, false, true); }  /* char literal always signed */
    | strlit_seq        { $$ = new StringLiteralExpr($1); free($1); }
    | '(' expr ')'      { $$ = $2; }
    ;

strlit_seq:
      STRLIT            { $$ = $1; }
    | strlit_seq STRLIT {
                            $$ = (char *) malloc(strlen($1) + strlen($2) + 1);
                            strcpy($$, $1);
                            strcat($$, $2);
                            free($1); free($2);
                        }
    ;

expr_list_opt:
      /* empty */       { $$ = NULL; }
    | expr_list         { $$ = $1; }
    ;

expr_list:
      expr                      { $$ = new TreeSequence(); $$->addTree($1); }
    | expr_list ',' expr        { $$ = $1; $$->addTree($3); }
    ;

selection_stmt:
      IF '(' if_cond ')' stmt else_part_opt     { $$ = new IfStmt($3, $5, $6); }
    | SWITCH '(' expr ')' stmt                  { $$ = new SwitchStmt($3, $5); }
    ;

if_cond:
      expr                      { $$ = $1; }
    ;

else_part_opt:
      /* empty */               { $$ = NULL; }
    | ELSE stmt                 { $$ = $2; }
    ;

while_stmt:
      WHILE '(' while_cond ')' stmt     { $$ = new WhileStmt($3, $5, false); }
    ;

while_cond:
      expr                      { $$ = $1; }
    ;

do_while_stmt:
      DO stmt WHILE '(' while_cond ')' ';'      { $$ = new WhileStmt($5, $2, true); }
    ;

for_init:
      expr_list_opt ';'         { $$ = $1; }
    | declaration               { $$ = $1; }
    ;

for_stmt:
      FOR '(' save_src_fn save_line_no for_init
              save_src_fn save_line_no expr_opt      ';'
              save_src_fn save_line_no expr_list_opt ')'
              save_src_fn save_line_no stmt
                                { if ( $5)  $5->setLineNo($3, $4);
                                  if ( $8)  $8->setLineNo($6, $7);
                                  if ($12) $12->setLineNo($10, $11);
                                  $$ = new ForStmt($5, $8, $12, $16);
                                  free($3); free($6); free($10); free($14);
                                }
    ;

%%

#if 0
void _PARSERTRACE(int parserLineNo, const char *fmt, ...)
{
    printf("# P%d U%d: ", parserLineNo, lineno);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    fflush(stdout);
}
#endif
