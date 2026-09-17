#include <exception>
#include <sstream>
#include <variant>
#include <string>
#include "snek-util.hh"

//
// snek-util.cc
//
// Implementation of some utilities for the Snek interpreter.
//
// See the header (.hh) file for details.
//

//
// Let's define nothing, erm, None.
unit None;

//
// s = snek_message(lo,ms);
//
// Builds and returns a string `s` that gives a Snek error message `ms`
// along with information `lo` about the place in the source file where
// the error occurs.
//
const std::string snek_message(Locn lo, std::string ms) {
    std::stringstream ss { };
    ss << lo.source_name << ":";
    if (lo.column > 0 && lo.line > 0) {
        ss << lo.line << ":" << lo.column << ":";
    }
    ss << " " << ms;
    return ss.str();
}

//
// class SnekError
//
//  - the type of exceptions thrown by the Snek interpreter
//
SnekError::SnekError(Locn lo, std::string ms) :
    std::exception { },
    message { snek_message (lo, ms) }
{ }

const char* SnekError::what() const noexcept {
    return message.c_str();
}

//
// de_escape(s)
//
// Builds a string `t` from string `s` where all the escape sequences
// (e.g. `\\`, `\n`) have been replaced by their actual characters.
//
// Returns that de-escaped string.
//
std::string de_escape(std::string s) {
    std::stringstream de_s;
    bool escape = false;
    for (char c: s) {
        if (escape) {
            if (c == 'n') {
                de_s << '\n';
            } else if (c == 't') {
                de_s << '\t';
            } else if (c == '\\') {
                de_s << '\\';
            } else if (c == '"') {
                de_s << '"';
            }
            escape = false;        
        } else if (c == '\\') {
            escape = true;
        } else {
            de_s << c;
        }
    }
    return de_s.str();
}

//
// re_escape(s)
//
// Builds a string `t` from string `s` where all special characters
// (e.g tab, end of line, etc) are replaced by their escape sequences
// (e.g. `\t`, `\n`, etc).
//
// Returns that escaped string.
//
std::string re_escape(std::string s) {
    std::stringstream re_s;
    for (char c: s) {
        if (c == '\n') {
            re_s << "\\n";
        } else if (c == '\t') {
            re_s << "\\t";
        } else if (c == '\\') {
            re_s << "\\\\";
        } else if (c == '"') {
            re_s << "\\\"";
        } else {
            re_s << c;
        }
    }
    return re_s.str();
}

//
// to_repn(v)
//
// Returns a string that is the lexical representation of a Snek value.
//
std::string to_repn(Valu v) {
    if (std::holds_alternative<std::string>(v)) {
        return "\"" + re_escape(std::get<std::string>(v)) + "\"";
    } else {
        return to_string(v);
    }
}

//
// to_string(v)
//
// Returns a string that is the printable output of a Snek value.
//
std::string to_string(Valu v) {
    if (std::holds_alternative<int>(v)) {
        int iv = std::get<int>(v);
        return std::to_string(iv);
    } else if (std::holds_alternative<std::string>(v)) {
        return std::get<std::string>(v);
    } else if (std::holds_alternative<bool>(v)) {
        if (std::get<bool>(v)) {
            return "True";
        } else {
            return "False";
        }
    } else if (std::holds_alternative<unit>(v)) {
        return "None";
    }
    // Should never happen.
    return "None";
}

//
// to_int(v)
//
// Returns an integer coerced from the Snek value.
//
int to_int(Valu v) {
    if (std::holds_alternative<int>(v)) {
        return std::get<int>(v);
    } else if (std::holds_alternative<std::string>(v)) {
        return std::stoi(std::get<std::string>(v));
    } else if (std::holds_alternative<bool>(v)) {
        if (std::get<bool>(v)) {
            return 1;
        } else {
            return 0;
        }
    } else if (std::holds_alternative<unit>(v)) {
        return 0;
    }
    // Should never happen.
    return 0;
}

//
// to_bool(v)
//
// Returns a boolean coerced from the Snek value.
//
bool to_bool(Valu v) {
    if (std::holds_alternative<int>(v)) {
        return (std::get<int>(v) != 0);
    } else if (std::holds_alternative<std::string>(v)) {
        return (std::get<std::string>(v).length() > 0);
    } else if (std::holds_alternative<bool>(v)) {
        return std::get<bool>(v);
    } else if (std::holds_alternative<unit>(v)) {
        return false;
    }
    // Should never happen.
    return false;
}

//
// ty_string(v)
//
// Returns a string that is the Snek type of a Snek value.
//
std::string ty_string(Valu v) {
    if (std::holds_alternative<int>(v)) {
        return "int";
    } else if (std::holds_alternative<std::string>(v)) {
        return "str";
    } else if (std::holds_alternative<bool>(v)) {
        return "bool";
    } else if (std::holds_alternative<unit>(v)) {
        return "unit";	
    }
    // Should never happen.
    return "unit";
}
