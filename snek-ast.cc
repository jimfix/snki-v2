#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include <iostream>
#include <variant>
#include <optional>
#include <exception>
#include <algorithm>
#include <sstream>

#include "snek-ast.hh"
#include "snek-util.hh"

//
// snek-ast.cc
//
// Below are the implementations of methods of AST nodes. They are organized
// into groups. The first group represents the Snek interpreter using
//
//    Prgm::run, Blck::exec, Stmt::exec, Expn::eval
//
// The second group AST::output performs pretty printing of SLP code.
//

// * * * * *
// The Snek interpreter
//

//
// Constructors
//

AST::AST(Locn lo) : locn {lo} { }

Prgm::Prgm(Defs ds, Blck_ptr mn, Locn lo) : AST {lo}, defs {ds}, main {mn} { }
Defn::Defn(Locn lo) : AST {lo} { } // This will be changed!
Stmt::Stmt(Locn lo) : AST {lo} { }
//
Asgn::Asgn(Name x, Expn_ptr e, Locn l) : Stmt {l}, name {x}, expn {e} { }
Prnt::Prnt(Expn_ptr e, Locn l) : Stmt {l}, expn {e} { }
Pass::Pass(Locn l) : Stmt {l} { }
Blck::Blck(Stmt_vec ss, Locn lo) : AST {lo}, stmts {ss} { }

Expn::Expn(Locn lo) : AST {lo} { }
//
BiOp::BiOp(std::string op, Expn_ptr lf, Expn_ptr rg, Locn lo)
  : Expn {lo}, oper {op}, left {lf}, rght {rg}, sigs {} { }
Plus::Plus(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"+", lf, rg, lo} {
    // Add two integers.
    addRule("int", "int", [](Valu l, Valu r) {
        return Valu {std::get<int>(l) + std::get<int>(r)};
    });
    // Concatenate two strings.
    addRule("str","str", [](Valu l, Valu r) {
        return Valu {std::get<std::string>(l) + std::get<std::string>(r)};
    });
}
Tmes::Tmes(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"*", lf, rg, lo} {
    // Multiply two integers.
    addRule("int", "int",
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) * std::get<int>(r)};
        });
    // Repeat a string an integer number of times.
    addRule("int", "str", 
            [](Valu l, Valu r) {
                int n = std::get<int>(l);
                std::string s = std::get<std::string>(r);
                std::string t = "";
                while (n > 0) { t += s; n--; }
                return Valu {t};
            });
    // Same.
    addRule("str", "int", 
            [](Valu l, Valu r) {
                int n = std::get<int>(r);
                std::string s = std::get<std::string>(l);
                std::string t = "";
                while (n > 0) { t += s; n--; }
                return Valu {t};
            });
}
Mnus::Mnus(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"-", lf, rg, lo} {
    // Subtract two integers.
    addRule("int", "int", 
            [](Valu l, Valu r) {
                return Valu {std::get<int>(l) - std::get<int>(r)};
            });
}
IDiv::IDiv(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"//", lf, rg, lo} {
    // Compute the quotient of two integers.
    addRule("int", "int",
            [this](Valu l, Valu r) {
                int lv = std::get<int>(l);
                int rv = std::get<int>(r);
                if (rv == 0) {
                    throw SnekError {
                        where(), "Run-time error: division by 0 for '" + oper +"'."
                    };
                }
                return Valu {
                    lv / rv - ((lv % rv != 0) && ((lv ^ rv) < 0))
                };
            });
}
IMod::IMod(Expn_ptr lf, Expn_ptr rg, Locn lo)
: BiOp{"%", lf, rg, lo} {
    // Compute the remainder due to an integer division.
    addRule("int", "int",
        [this](Valu l, Valu r) {
        int lv = std::get<int>(l);
        int rv = std::get<int>(r);
        if (rv == 0) {
            throw SnekError {
                where(), "Run-time error: division by 0 for '" + oper +"'."
            };
        }
        int mv = lv % rv;
        return Valu {
            mv != 0 && ((lv < 0) ^ (rv < 0)) ? mv + rv : mv
        };
    });
}
//
Ltrl::Ltrl(Valu vl, Locn lo) : Expn {lo}, valu {vl} { }
Lkup::Lkup(Name nm, Locn lo) : Expn {lo}, name {nm} { }
//
Inpt::Inpt(Expn_ptr e, Locn lo) : Expn {lo}, prpt {e} { }
IntC::IntC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
StrC::StrC(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }
SLen::SLen(Expn_ptr e, Locn lo) : Expn {lo}, expn {e} { }

//
// Prgm::run, Blck::exec, Stmt:: exec
//
//  - execute Snek statements, changing the runtime context mapping
//    variables to their current values.
//

void Prgm::run(void) const {
    Ctxt main_ctxt { };
    // Execute the main block within the empty context.
    main->exec(defs, main_ctxt);
}

VOpt Blck::exec(const Defs& defs, Ctxt& ctxt) const {
    // Execute each statement of a block.
    for (Stmt_ptr s : stmts) {
        VOpt rv = s->exec(defs, ctxt);
        if (rv.has_value()) {
            return rv;
        }
    }
    return std::nullopt;
}

VOpt Asgn::exec(const Defs& defs, Ctxt& ctxt) const {
    // Add or update the context with this named variable's new value.
    ctxt[name] = expn->eval(defs, ctxt);
    return std::nullopt;
}

VOpt Pass::exec([[maybe_unused]]const Defs& defs, [[maybe_unused]]Ctxt& ctxt) const {
    // Do nothing!
    return std::nullopt;
}
  
VOpt Prnt::exec(const Defs& defs, Ctxt& ctxt) const {
    // Output the value of the expression to STDOUT.
    std::cout << to_string(expn->eval(defs, ctxt)) << std::endl;
    return std::nullopt;
}

//
// Expn:: eval
//
// - Evaluate a Snek expression within the runtime context to calculate
//   its resulting Snek value.

//
// BiOp::eval relies heavily on...

// BiOp::addRule
//
// - This is the key helper method for defining binary operations.
//   You can specify a series of type-driven rules for caclulating
//   a binary operator's result from the values of its left and right
//   subtrees. It works by simply adding a hash table entry to the set
//   of "signatures" for that binary operation. It supports the 
//   `BiOp::eval` method's operator dispatch.
//
//  For example, we might define string repetition as taking 
//  an integer and a string, and so we would add a rule for "int", "str"
//  and the function attached to it would concatenate a string several
//  times.
//
void BiOp::addRule(std::string lt, 
                   std::string rt,
                   std::function<Valu(Valu,Valu)> ef) {
    std::string lt_rt = lt + "," + rt;
    sigs.insert({lt_rt, ef});
}
//
Valu BiOp::eval(const Defs& defs, const Ctxt& ctxt) const {

    // Evaluate each subtree.
    Valu lv = left->eval(defs, ctxt);
    Valu rv = rght->eval(defs, ctxt);
    
    // Determine their resulting value's types.
    std::string lt = ty_string(lv);
    std::string rt = ty_string(rv);
    std::string lt_rt = lt + "," + rt;

    // "Dispatch" the rule for evaluation according to the types.
    if (sigs.count(lt_rt) > 0) {
        return sigs.at(lt_rt)(lv,rv);
    }

    throw SnekError {
        where(), "Run-time error: bad operand type(s) for '" + oper +"'."
    };
}

Valu Ltrl::eval([[maybe_unused]]const Defs& defs, [[maybe_unused]]const Ctxt& ctxt) const {
    // Just return the literal value.
    return valu;
}

Valu Lkup::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Look up the named variable within the current context.
    if (ctxt.count(name) > 0) {
        return ctxt.at(name);
    } else {
        throw SnekError { where(), "Run-time error: variable '" + name +"' not defined."};
    }
}

Valu Inpt::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Output the prompt. Await input from STDIN. Return its string.
    std::string vl;
    Valu valu = prpt->eval(defs, ctxt);
    std::cout << to_string(valu);
    std::cin >> vl;
    return Valu {vl};
}

Valu IntC::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Convert a Snek value to a Snek integer.
    int iv = 0;
    try {
        iv = to_int(expn->eval(defs, ctxt));
    }
    catch (const std::runtime_error& e) {
        throw SnekError { where(), 
            "Run-time error: cannot convert value to int."
        };
    }
    return Valu {iv};
}

Valu StrC::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Convert a Snek value to a Snek string.
    return Valu {to_string(expn->eval(defs, ctxt))};
}

Valu SLen::eval([[maybe_unused]]const Defs& defs, const Ctxt& ctxt) const {
    // Compute the length of a Snek string.
    Valu v = expn->eval(defs, ctxt);
    if (std::holds_alternative<std::string>(v)) {
        std::string s = std::get<std::string>(v);
        int sl = s.length();
        return Valu {sl};
    } else {
        throw SnekError { where(), 
            "Run-time error: attempted to take the length of a non-string."
        };
    }
}

// * * * * *
//
// AST::output
//
// - Pretty printer for Snek code represented in an AST.
//
// The code below is an implementation of a pretty printer. For each case
// of an AST node (each subclass) the `output` method provides the means for
// printing the code of the Snek construct it represents.
//
//

void Prgm::output(std::ostream& os) const {
    main->output(os, "");
}

void Defn::output(std::ostream& os) const {
    // Your code goes here.
    os << "def" << std::endl; // BOGUS to shut up the compiler warning. 
}

void Blck::output(std::ostream& os, std::string indent) const {
    for (Stmt_ptr s : stmts) {
        s->output(os,indent);
    }
}

void Asgn::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << name << " = ";
    expn->output(os);
    os << std::endl;
}

void Pass::output(std::ostream& os, std::string indent) const {
    os << indent << "pass" << std::endl;
}

void Prnt::output(std::ostream& os, std::string indent) const {
    os << indent;
    os << "print";
    os << "(";
    expn->output(os);
    os << ")";
    os << std::endl;
}

void BiOp::output(std::ostream& os) const {
    os << "(";
    left->output(os);
    os << oper;
    rght->output(os);
    os << ")";
}

void Ltrl::output(std::ostream& os) const {
    os << to_repn(valu); 
}

void Lkup::output(std::ostream& os) const {
    os << name;
}

void Inpt::output(std::ostream& os) const {
    os << "input(";
    prpt->output(os);
    os << ")";
}

void IntC::output(std::ostream& os) const {
    os << "int(";
    expn->output(os);
    os << ")";
}

void StrC::output(std::ostream& os) const {
    os << "str(";
    expn->output(os);
    os << ")";
}

void SLen::output(std::ostream& os) const {
    os << "len(";
    expn->output(os);
    os << ")";
}

// * * * * *
//
// AST::dump
//
// - outputs an AST.
//
// The code below prints the contents of an AST. For each case we output a
// header describing the node's type, and then we (with indents) dump each
// of the node's subtrees (if any).
//

void Prgm::dump(std::ostream& os) const {
    dump(os, "");
}

void Prgm::dump(std::ostream& os, std::string indent) const {
    // You should probably also dump the definitions. 
    os << indent << "Prgm" << std::endl;
    main->dump(os, indent+"    ");
}

void Defn::dump(std::ostream& os, std::string indent) const {
    os << indent << "DEFN" << std::endl;
    // Your code goes here.
}


void Blck::dump(std::ostream& os, std::string indent) const {
    os << indent << "Blck" << std::endl;
    for (Stmt_ptr s : stmts) {
        s->dump(os, indent+"    ");
    }
}

void Asgn::dump(std::ostream& os, std::string indent) const {
    os << indent << "Asgn" << std::endl;
    os << indent+"    " << name << std::endl;
    expn->dump(os, indent+"    ");
}

void Pass::dump(std::ostream& os, std::string indent) const {
    os << indent << "Pass" << std::endl;
}

void Prnt::dump(std::ostream& os, std::string indent) const {
    os << indent << "Prnt" << std::endl;
    expn->dump(os, indent+"    ");
}

void BiOp::dump(std::ostream& os, std::string indent) const {
    os << indent << "BiOp" << std::endl;
    os << indent << "    " << oper << std::endl;
    left->dump(os, indent+"    ");
    rght->dump(os, indent+"    ");
}

void Ltrl::dump(std::ostream& os, std::string indent) const {
    os << indent << "Ltrl" << std::endl;
    os << indent << "    " << to_repn(valu) << std::endl;
}

void Lkup::dump(std::ostream& os, std::string indent) const {
    os << indent << "Lkup" << std::endl;
    os << indent << "    " << name << std::endl;
}

void Inpt::dump(std::ostream& os, std::string indent) const {
    os << indent << "Inpt" << std::endl;
    prpt->dump(os, indent+"    ");
}

void IntC::dump(std::ostream& os, std::string indent) const {
    os << indent << "IntC" << std::endl;
    expn->dump(os, indent+"    ");
}

void StrC::dump(std::ostream& os, std::string indent) const {
    os << indent << "StrC" << std::endl;
    expn->dump(os, indent+"    ");
}

void SLen::dump(std::ostream& os, std::string indent) const {
    os << indent << "SLen" << std::endl;
    expn->dump(os, indent+"    ");
}
