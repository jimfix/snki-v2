#ifndef _snek_flex_hh
#define _snek_flex_hh

#if ! defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include <iostream>
#include <string>
#include "snek-bison.tab.hh"
#include "snek-util.hh"

namespace Snek {

    class Lexer : public yyFlexLexer{
    public:
        Lexer(std::istream *in, std::string fn) :
            yyFlexLexer {in},
            src_name {fn},
            indents { }
        {
            indents.push_back(1); // Top-level indent is at column 1.
        }

        // Get rid of override virtual function warning
        using FlexLexer::yylex;

        // Critical method for supporting Bison parsing.
        virtual int yylex(Snek::Parser::semantic_type* const lval,
                          Snek::Parser::location_type* loc);
        // YY_DECL defined in snek-flex.ll
        // Method body created by flex in snek-flex.cc

        // Helper for Bison parsing.
        Locn locate(const Snek::Parser::location_type& l);
        
    private:
        // Tie-in to Bison.
        Snek::Parser::semantic_type *yylval = nullptr;

        // Other additional state.
        std::string src_name;
        std::vector<int> indents;

        using location_type = Snek::Parser::location_type;
        
        // Used to issue tokens, update token locations, and determine indents.
        void advance_by_text(const std::string txt, location_type* l);
        void advance_by_char(char curr_char, location_type* l);
        int indent_column(std::string text);
        int issue(int tkn_typ, std::string txt, location_type* l);

        // Terminate with an error.
        void bail(location_type* l, std::string msg);
    };

} 

#endif 
