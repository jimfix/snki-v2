#ifndef __snek_util_
#define __snek_util_

#include <string>
#include <variant>
#include <optional>

//
// snek-util.hh
//
// Some useful utilities for the Snek interpreter. Some are for error
// reporting, namely
//
//  * SnekError    - an exception for reporting Snek errors
//  * Locn         - a (filename, line number, column number) for an error
//  * snek_message - builds an error string 
//
// Some are for converting string literals to their actual strings, and
// back, namely
//
//   * de_escape, re_escape
//

//
// class Locn
//
// Houses information about the Snek source file's name, along with a
// line (the "row") and column within that source file.
//
// This is typically used to report errors in the Snek source code.
//
class Locn {
public:
    std::string source_name;
    int line;
    int column;
    //
    Locn(std::string fn, int li, int co)
        : source_name {fn}, line {li}, column {co} { }
    Locn(std::string fn) 
        : source_name {fn}, line {-1}, column {-1} { }
    Locn(void) : Locn {"",0,0} { }
};
    
//
// s = snek_message(lo,ms);
//
// Builds and returns a string `s` that gives a Snek error message
// `ms` along with information `lo` about the place in the source file
// where the error occurs.
//
const std::string snek_message(Locn lo, std::string ms);

//
// class SnekError
//
// Thrown when an error is discovered while processing a Snek source
// file.
//
class SnekError: public std::exception {
private:
    Locn location;
    const std::string message;
    
public:    
    SnekError(Locn lo, std::string ms);
    const char* what();
};

//
// Utility functions for dealing with string literals.
//
std::string re_escape(std::string s); // Replace special chars with \d ones.
std::string de_escape(std::string s); // Replace \d sequences with actuals.


//
// unit - type of the None value
// None - the None value
// Valu - defines values calculated and stored by Snek programs.
// VOpt - either a value or no value, used for modeling `return`.
//
struct unit { };
extern unit None;
typedef std::variant<int, std::string, bool, unit> Valu;
typedef std::optional<Valu> VOpt;

//
// Utility functions for dealing with Snek values.
//
std::string to_repn(Valu valu);
int to_int(Valu valu);
std::string to_string(Valu valu);
bool to_bool(Valu valu);
std::string ty_string(Valu valu);

#endif
