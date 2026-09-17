%skeleton "lalr1.cc"
%require  "2.3"
%debug 
%defines 
%define api.namespace {Snek}
%define api.parser.class {Parser}
%define parse.error verbose
    
%code requires{
    
    #include "snek-ast.hh"

    namespace Snek {
        class Driver;
        class Lexer;
    }

    #define YY_NULLPTR nullptr

}

%parse-param { Lexer  &lexer }
%parse-param { Driver &main  }
    
%code{

    #include <sstream>
    #include "snek-util.hh"    
    #include "snki.hh"

    #undef yylex
    #define yylex lexer.yylex
    
}


%define api.value.type variant
%define parse.assert
%define api.token.prefix {Token_}

%locations

%token               EOFL  0  
%token               EOLN
%token               INDT
%token               DEDT
%token               PASS "pass"
%token               PRNT "print"
%token               INPT "input"
%token               INTC "int"
%token               STRC "str"
%token               ASGN "="
%token               PLUS "+"
%token               MNUS "-"
%token               TMES "*"
%token               IDIV "//"
%token               IMOD "%"
%token               LPAR "(" 
%token               RPAR ")"
%token               NONE "None"
%token               TRUE "True"
%token               FALS "False"
%token <int>         NMBR
%token <std::string> NAME
%token <std::string> STRG

%type <Prgm_ptr> prgm
%type <Blck_ptr> blck
%type <Stmt_vec> stms
%type <Stmt_ptr> stmt
%type <Expn_ptr> expn

%%

%start main;

%left PLUS MNUS;
%left TMES IMOD IDIV;
    
main:
  prgm {
      main.set($1);
  }
;

prgm:
  blck {
                      // Hack:
      (void)yynerrs_; // This is to get rid of the "variable set but not used"
                      // warning from the compiler.
      Defs ds { };
      Blck_ptr b = $1; 
      $$ = Prgm_ptr { new Prgm {ds, b, b->where()} };
  }   
;

blck:
  stms {
      Stmt_vec ss = $1;
      $$ = Blck_ptr { new Blck {ss, ss[0]->where()} };
  }
;

stms:
  stms stmt {
      Stmt_vec ss = $1;
      ss.push_back($2);
      $$ = ss;
  }
| stmt {
      Stmt_vec ss { };
      ss.push_back($1);
      $$ = ss;
  }
;
  
stmt: 
  NAME ASGN expn EOLN {
      $$ = std::shared_ptr<Asgn> { new Asgn {$1,$3,lexer.locate(@2)} };
  }
| PASS EOLN {
      $$ = std::shared_ptr<Pass> { new Pass {lexer.locate(@1)} };
  }
| PRNT LPAR expn RPAR EOLN {
      $$ = std::shared_ptr<Prnt> { new Prnt {$3,lexer.locate(@1)} };
  }
;

expn:
  expn PLUS expn {
      $$ = std::shared_ptr<Plus> { new Plus {$1,$3,lexer.locate(@2)} };
  }
| expn MNUS expn {
      $$ = std::shared_ptr<Mnus> { new Mnus {$1,$3,lexer.locate(@2)} };
  }
| expn TMES expn {
      $$ = std::shared_ptr<Tmes> { new Tmes {$1,$3,lexer.locate(@2)} };
  }
| expn IDIV expn {
      $$ = std::shared_ptr<IDiv> { new IDiv {$1,$3,lexer.locate(@2)} };
  }
| expn IMOD expn {
      $$ = std::shared_ptr<IMod> { new IMod {$1,$3,lexer.locate(@2)} };
  }
| INPT LPAR expn RPAR {
      $$ = std::shared_ptr<Inpt> { new Inpt {$3,lexer.locate(@1)} };
  }
| INTC LPAR expn RPAR {
      $$ = std::shared_ptr<IntC> { new IntC {$3,lexer.locate(@1)} };
  }
| STRC LPAR expn RPAR {
      $$ = std::shared_ptr<StrC> { new StrC {$3,lexer.locate(@1)} };
  }
| NAME {
      $$ = std::shared_ptr<Lkup> { new Lkup {$1,lexer.locate(@1)} };
  }
| NMBR {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {$1},lexer.locate(@1)} };
  }
| STRG {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {de_escape($1)},lexer.locate(@1)} };
  }
| TRUE {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {true},lexer.locate(@1)} };
  }
| FALS {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {false},lexer.locate(@1)} };
  }
| NONE {
      $$ = std::shared_ptr<Ltrl> { new Ltrl {Valu {None},lexer.locate(@1)} };
  }    
| LPAR expn RPAR {
      $$ = $2;
  }
;

%%
       
void Snek::Parser::error(const location_type &loc, const std::string &msg) {
    throw SnekError { lexer.locate(loc), msg };
}
