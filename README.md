<h1 align="center">Project 1: Functions, Loops, and Conditionals</h1>
<h2 align="center">Reed CSCI 394 Fall 2026</h2>

Your assignment is to build a parser and interpreter for a better version of **Snek**,
one that has a richer set of constructs than the "straight line" **Snek** of Project 0.
The parser is written using the **Bison** parser-generator tool and it relies on a lexer written using **Flex**.
The initial code we provide handles straight line **Snek**.
You will extend the code to handle the additional language constructs.

We outline some of the extensions to **Snek** from Project 0, take you through
some of the starter code, and then explain briefly how to work within
**Bison** to complete the assignment. We summarize the exercises and then offer
several bonus exercises at the very end.

# "Crooked" **Snek**

The full **Snek** language for this assignment is given by the following grammar:

~~~ none

<prgm> ::= <defn>  ... <defn> <blck>
<defn> ::= def <name> ( <name> , ... , <name> ) : <nest>
<nest> ::= INDT <blck> DEDT
<blck> ::= <stmt> <stmt> ... <stmt>
<stmt> ::= <name> <updt> <expn> EOLN
         | pass EOLN
         | print ( <expn> , ... , <expn> ) EOLN
         | if <expn> : <nest> else : <nest>
         | while <expn> : <nest>
         | return <expn> EOLN
         | return EOLN
         | <name> ( <expn> , ... , <expn> ) EOLN
<updt> ::= += | -=
<expn> ::= <expn> <bnop> <expn>
         | <unop> <expn>
         | input ( <expn> ) | int ( <expn> ) | str ( <expn> )
         | <name> | <ltrl> | ( <expn> )
         | <name> ( <expn> , ... , <expn> )         
<bnop> ::= + | - | * | // | %
         | < | <= | == 
         | and | or
<unop> ::= not
<ltrl> ::= <nmbr> | <strg> | <bool> | <unit>
<name> ::= x | count | _special | y0 | camelWalk | snake_slither | ...
<nmbr> ::= 0 | 1 | 2 | 3 | ...
<bool> ::= True | False
<unit> ::= None
<strg> ::= "hello" | "" | ...

~~~

The main additions are the ability to define functions and procedures, and to call them;
the addition of loops and conditional statements, and the logic and comparison operations
that support them; and several enhancements to the base code to support all these changes.

Let's go through each of the language additions.

## Function definition, call, and return.

A programmer can now preface their main script with a series of function
definitions. For example, they can now write code like the following:
~~~ python
    def square(x):
        return x * x

    def twoDigit(tens,ones):
        number = tens * 10 + ones
        return number

    def report(number):
        message = "The square of that digit repeated is "
        message += str(square(number))
        message += "."
        print(message)
        return
        
    def get():
        digit = int(input("Please enter a digit: "))
        return twoDigit(digit,digit)

    value = get()
    report(value)
~~~
This code gives four defintions of `square`, `twoDigit`, `report`, and
`get`, respectively. The first two and the last are functions that
return an integer value, and the third is a procedure that returns
when it is done with its work. And then the main script runs, with the
first line calling the `get` function (which relies on `twoDigit`)
and the second line invoking the `report` procedure (which relies on
`square`).

The syntax for `def` relies on indentation, and the indentation rules
are similar to those followed by the **FOO** and **BAZ** languages
from Homework 1. There is the `def` line followed by several lines
indented by the same amount, typically with four spaces. The line
following the function's definition has a "de-dentation" where the
subsequent line (either another `def` or the main script lines,
for this example) is not indented.

The grammar rules that specify this are:
~~~ none
<defn> ::= def <name> ( <name> , ... , <name> ) : <nest>
<nest> ::= INDT <blck> DEDT
<blck> ::= <stmt> <stmt> ... <stmt>
~~~
A parser is expected to get a token stream that includes these
special `INDT` and `DEDT` tokens that summarize the indentation
witnessed when the source code is scanned. After an `INDT` token,
the subsequent statement lines are indented at a certain level.
That level is retracted to a prior level when a `DEDT` is issued.
In a way, `INDT` and `DEDT` serve as the analogue to C's
open and closed curly brace notation, where blocks are marked with
`{` and `}`. 

There are two kinds of `return` statements for exiting a called
routine. If the definition is of a function, then the syntax is
~~~ none
    <stmt> ::= return <expn> EOLN
~~~
and the value being returned is specified just after the `return`
token. If instead a procedure is being defined, then the syntax is
~~~ none
    <stmt> ::= return EOLN
~~~
which exits the procedure immediately.

Your parser need not check whether the use of `return` is consistent
throughout the definition's body, nor does it need to check whether
a `return` occurs. That kind of check is left for later semantic
analysis phases. For future compiler extensions, we will perform
these kinds of checks. For this assignment, those checks occur 
within the interpreter component.

A procedure call occurs as a program statement and has this syntax:
~~~ none
    <stmt> ::= <name> ( <expn> , ... , <expn> ) EOLN
~~~
This means it is invoked by name with 0 or more parameter
values passed as the arguments given by the expressions in parentheses.
The parser does not check whether the number of arguments match
the number of parameters in the procedure's definition, nor does
it check whether there exists a `def` for that name. Again, this
kind of check is for a later semantic analysis phase.

A function call is similar, except it occurs anywhere a value expression
can be used, e.g. on the right-hand side of an assignment statement.

Note that the ability to define "subroutines"---functions and procedures---
gets rid of the "straight line" execution of our programs.

## Loops.

The language now has a while loop that has the syntax:
~~~ none
    <stmt> ::= while <expn> : <nest>
~~~
The expression after the `while` gives the condition for continuing
execution of the loop. It is expected to be of boolean type meaning
it should yield the value `True` or the value `False`. The code that
follows it should be indented. Any loop bodies nested inside a
loop should be indented further. For example:
~~~ none
r = 0
while r < 10:
    c = 0
    while c < 10:
        print(str(r)+str(c))
        c += 1
    r += 1
print("Done.")
~~~
This would print all the values from 0 to 99 with a digit padding of 0
when the value is less than 10. In that code above, the outer `while`
isn't indented. Its body is indented 4 spaces. It has an inner `while`
whose loop's body is indented 8 spaces.

We now have a wider variety of expressions that include ones that
evaluate to `True` and `False`, namely integer comparison checks
and value equality checks, as well as the logical connectives
`and` and `or`. These are expressed as binary operations in the
language. We also have a unary `not` for logical negation.

Because these are `<expn>` constructs, we can assign variables
boolean values, and we can invent boolean predicate functions
that return `True` or `False`.

## Conditionals.

There is a conditional "if-else" statement with the syntax:
~~~ none
    <stmt> ::= if <expn> : <nest> else : <nest>
~~~
Like the `while`, this takes a boolean expression, but here
there are two indented blocks of code---the code that should
be executed when the condition is `True`, and the code that
should instead be executed when the condition is `False`.

Like `while`, each of these blocks could have a `if-then`
within their statements. They could also have a `while`.
And furthermore, they could be nesetd inside an `while`
or a `def`.

## More types.

We have changed the grammar so that expressions can be
of type `str` and `int`. We allow value conversions using
the `int` and `str` functions. There are also the literals
`True` and `False` whose type is `bool` and the
value `None`. ( I call this the `unit` type.)

## More operations.

We've included the left-associative `+`, `-`, `*`, `//`, and `%` operations.
The precedence rules for these are as expected. Addition and subtraction
operations are lower in precendence than multiplication and integer division.

You'll be adding comparisons and logical operations to these.

Having a wider variety of types means that some
of the binary operations can have overloaded meaning. Already, 
we can concatenate strings with `+` and repeat them with `*`. 
We might also want to compare strings (and maybe even booleans)
with `<`, etc. 

Operator precedence rules are richer.
The logical operations have the lowest precedence but, among them, logical
negation with `not` is the highest, then `and`, and then `or`. Both `and` and
`or` are left-associative binary operations.

Sitting at a precedence level between the arithmetic and the logical operations
are the comparison operations. **These have no associativity**, meaning
(for example) you cannot express a condition like `a < b < c` in this
language.  Rather than `a < b < c`, you are forced to use a combination
of parentheses and the logical connectives to reason about the results of
comparisons, writing this as `(a < b) and (b < c)`. Now, focusing on this
example, it turns out also that you can write this as `a < b and b < c`.
This is because comparisons have lower prcedecence than the arithmetic
expressions that they typically reason about, and higher than the logical
operations that string the comparisons together.

This all means, for example, that
~~~ python
x > y + z * 2 and not x * x < w or x == w
~~~
should be parsed the same as
~~~ python
((x > (y + (z * 2))) and (not ((x * x) < w))) or (x == w)
~~~

We also have a variety of assignment/update operations, namely
`=`, `+=`, and `-=`. Assignment statements are not expressions 
and so do not yield a value.

# Starter code

The starting code for this assignment can be obtained by unpacking
this ZIP file:

* [snki-v2.zip](snki-v2.zip) - **Bison**/**Flex**-driven parser starting code. 

It will unpack to yield the following code components:

* `snki.{hh,cc}`: this is the driver of the code and defines `main`. It relies on a `Driver` object that houses the parser and lexer, and keeps a reference to the parsed program.

* `snek-flex.{hh,ll}`: this is the Flex source code that defines the lexer. The lexer is assumed to be driven by a **Bison**-generated parser.

* `snek-bison.{hh,yy}`: this is the **Bison** source code that defines the parser. For now it only defines stuff from Project 1. The parser ultimately builds and returns an AST for the source code it processes.

* `snek-ast.{hh,cc}`: like the first assignment, this defines the AST node classes and implements their methods. Much of your work will involve laying out the additional classes for other syntactic constructs beyond the ones from Project 0.

* `snek-util.{hh,cc}`: this contains a few spare utilities for reporting errors, including giving locations within the source code. Note: this has been changed from Project 0 to work with **Bison**'s code location mechanism.

You can build the unextended parser with the command `make`. And then you
can run the resulting program executable `snki` with a filename argument
like so
~~~ none
./snki some-source-program.snk
~~~
just as you did with Project 0. When run, it
builds the lexer and the parser, reads and processes the source code
specified by the filename in the first argument to the Unix command,
and then runs the interpreter.

You can also include the `--dump` directive as one of the options.
In this case, the program only outputs the AST of the code to the
console without actually running it (in a manner similar to what
was required for Project 0).

# The Assignment

This assignment requires you to:

1. Add AST definitions for all the additional syntactic constructs, methods for printing out their syntax trees. These will be placed in the `snek-ast.*` files.

2. Add token definitions for scanning **Snek** source code. The new token types will be placed in the appropriate section of the **Bison** specification file `snek-bison.yy`. The rules for scanning them will be placed in the Flex specification file `snek-flex.ll`.

3. Add parsing rules for handling all the **Snek** syntax shown in the grammar, plus any additional constructs you choose to provide.

4. Finally, add methods for executing the code for each of the additions.

Below we give directions for changing the starter code, namely:

1. How to add new tokens to the `.yy` file.

2. How to add rules for scanning tokens in the `.ll` file.

3. How to add new grammar variables to the `.yy` file.

4. How to add new parse rules to the `.yy` file.

5. How to extend the interpreter for functions and procedures.

---

# Coding Details

## 1. Specifying a grammar's tokens in **Bison**

If you look at `snek-bison.yy`, you will see a section of code
that defines the enumerated type for all the grammar's tokens, and also give
the description of all the grammar's variables. These are needed by
**Bison** so that it knows what kind of information (what C++ types) are
communicated by the lexer, and also what the resulting types of
the actions associated with each grammar production. Here, for example,
is a summary of the token section:

~~~ none
%token               EOFL  0  
%token               EOLN
%token               INDT
%token               DEDT
%token               PASS "pass"
%token               PRNT "print"
...
%token               RPAR ")"
%token <int>         NMBR
%token <std::string> NAME
%token <std::string> STRG
~~~
These lines ultimately define a enumerated type within the C++ code
generated by **Bison**. What we are seeing is three kinds of tokens:

1. Ones like `pass` and `)` that are standard lexemes.

2. Ones lile `EOFL` and `EOLN` that are special characters handled by the lexer.

3. Ones like `NMBR` and `NAME` that carry additional information.

In the Flex/**Bison** interface, all three kinds have an associated integer
tag that is invented. This `enum` gets built as a **Bison**-generated file
`snek-bison.tab.hh`, and looks like so:
~~~ none
    struct token
    {
      enum yytokentype
      {
        Token_EOFL = 0,
        Token_EOLN = 258,
        Token_INDT = 259,
        Token_DEDT = 260,
        Token_PASS = 261,
        ...
        Token_RPAR = 272,
        Token_NMBR = 273,
        Token_NAME = 274,
        Token_STRG = 275
      };
    };
~~~
These end up being housed under the namespace `SLPY::Parser` but used
to a large extent within the code for `SLPY::Lexer` as defined in
`snek-flex.ll`.

Within the `.yy` file, these tokens get mentioned within grammar production
rules using their four-digit ALL CAPS. For example, here is the rule for
`print` statements:
~~~ none
stmt:
...
| PRNT LPAR expn RPAR EOLN {
    ...
  }
~~~
In the above, we have elided some of the details with `...`.  The important
thing to pay attention to in the above rule is that, instead of a grammar
rule that says:
~~~
<stmt> ::= print ( <expn> ) EOLN
~~~
we instead use those four letter codes for each token like `print` and `(`.

There is nothing truly special distingushing the tokens of kind #1 
(like `PASS`, `LPAR`) and the tokens of kind #2 (like `EOLN`, `INDT`)
as far as the Bison code is concerned. The only information
we need to learn from the lexer when parsing is which token we saw. The
integers associated with each (258 for `EOLN`, 261 for `PASS`) are all
that **Bison** and **Flex** need to share for those kinds of tokens.
In our declaration of the tokens of kind #1, we include the extra literal
string (e.g. `"pass"` for `PASS`, `"("` for `LPAR`) just for clarity's sake.
(And also, I believe **Bison** will let you use these literal strings in
the gramar rules; I just haven't used that feature in the parse rules
of the starter code.)

For tokens of kind #2. like variable names (`NAME`) and literal values
(`NMBR` and `STRG`) we need their information---a `std::string` for names
and string literals, an `int` for number literals---because these tokens
carry that additional information. The last lines of this token section
gives the C++ type specification of that additional information:
~~~ none
%token <int>         NMBR
%token <std::string> NAME
%token <std::string> STRG
~~~
We will describe below how this additional information can be used in
our parsing rules. For now I can give you a short preview. The **Bison**
rule for parsing the assignment statement is given by:
~~~ none
stmt:
  NAME ASGN expn EOLN {
      $$ = Asgn_ptr { new Asgn {$1,$3,lexer.locate(@2)} };
  }
~~~
This rule tells **Bison** that when the scanner finds a `NAME` followed by
an `ASGN` at the start of a `<stmt>` (something like `count =`)
consume those two tokens and then work to parse an `<expn>` followed by
an end-of-line token. The C++ code instructs the parser to then create
a new `Asgn` node with three pieces of information: the name of the variable,
the parsed right-hand-side expression, and the location of the `=` operation.
This code is just a standard C++ block but with special `$` designators that
allow us to access the `NAME` and `expn` information, and to describe
the new AST node being returned by the rule with `$$`.

For our purpose here, the key thing to note is that **Bison** knows that
the information attached with `NAME` in the first position of the rule
is of type `std::string`. And that is because we told **Bison** this
with the token declaration:
~~~ none
%token <std::string> NAME
~~~
And so that means that **Bison** will use the fact that the expression
`$1` is of type `std::string` to generate the C++ parser code 
`snek-bison.tab.cc`.  

### Coding: adding tokens to `snek-bison.yy`:

When you extend the parser to handle more grammar rules, some of
those rules will require you to add more tokens to this list.
For example, the conditional statement requires the tokens
`if`, `:`, and `else`.  You'll want to add these here with new
(capitalized, perhaps four-letter) names for their associated
enumeration value.

## 2. Adding more scanning rules to the lexer in **Flex**

With each token you add to the `.yy` file, you'll need to also add
a scanning rule to the lexer in `snek-flex.ll`. These rules are similar
code to what we demonstrated in lab for Homework 1. They are just the regular
expressions describing each token. 

These rules are pretty simple for this assignment. They will just be the
literal text. For example, the lines for the tokens `"pass"` and `"("` are
given by:
~~~ none
"(" {
    return issue(token::Token_LPAR, yytext, loc);
}

...

"pass" {
    return issue(token::Token_PASS, yytext, loc);
}
~~~
There is nothing too special to see here. It just requires you to
write some code that is very boilerplate. In our `.ll` file, for
convenience, we have set up these scanner firing rules so that
we can say `token::Token_PASS` instead of 
`Snek::Parser::token::Token_PASS`.

### Coding: adding token scanning to `snek-flex.ll`:

For each token type that you introduce to `snek-bison.yy`, you'll
need to add a scanner rule for matching its literal text and for
issuing the associated C++ token. Just mimic the starter code
I've given you and you should be fine here.

## 3. Specifying a grammar's variables in **Bison**

Just below the token section of `snek-bison.yy`, you will see a
section of code that gives a summary description of all the grammar's
variables.  These are told to **Bison** so that it learns the result types
of all the actions associated with each grammar production rule.  Here
is that variable section in the starter code:
~~~ none
%type <Prgm_ptr> prgm
%type <Blck_ptr> blck
%type <Stmt_vec> stms
%type <Stmt_ptr> stmt
%type <Expn_ptr> expn
~~~
I hope it's clear what's going on here. The first line tells **Bison**
that `<prgm>`-related production rules lead to parser actions
that, having fired, return a (smart) pointer to a new AST node of type
`Prgm`. A similar assertion is being made for `<blck>`, `<stmt>`, and
`<expn>` production rules. We essentially saw the use of this information
when we inspected the `.yy` code lines
~~~ none
stmt:
  NAME ASGN expn EOLN {
      $$ = Asgn_ptr { new Asgn {$1,$3,lexer.locate(@2)} };
  }
~~~
Since **Bison** is parsing a `<stmt>` when it consumes source code of the
form `<name> = <expn> EOLN`, the parser will return a new assignment
statement AST node pointer. And the type `Asgn_ptr` happens to be
copacetic with our specification line:
~~~ none
%type <Stmt_ptr> stmt
~~~
That is to say, if our parser builds something of type `Asgn_ptr`
then that value can be used as a `Stmt_ptr` in the code because
the `Asgn` class is derived from the `Stmt` class.

Note that one of the specifications is not like the others namely
~~~ none
%type <Stmt_vec> stms
~~~
Rather than introduce a new AST node type for sequences of statements,
we chose instead to use the type `std::vector<Stmt_ptr>`, which we
aliased as `Stmt_vec`. Our parsing, however, was made less cumbersome
by introducing a new variable `<stms>` and parsing it with the lines
~~~ none
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
~~~
The C++ code for each rule manipulates a `std::vector<Stmt>`. In the
base case of a single statement, we build a vector of length one. In the
recursive case of two or more statements, we extend it with `push_back`
to add the last statement parsed.

The end effect of these two rules means that when a series of  
`stmt` rules are parsed, each yielding a `Stmt_ptr` value, those
statment pointers each get placed into a `Stmt_vec` object.

### Coding: adding grammar variable specifications to `snek-bison.yy`:

In extending the parser rules for **Snek**, you may find a need to
introduce new left-hand sides to the grammar. For example, you may want
to do this to process the formal parameters of functions in their
`def` line. You may also want to do this for processing the actual
parameter expressions passed as arguments in a function call
(say, with a `std::vector>Name<` and a `std:vector<Expn_ptr>`).

If you do need to introduce new variables in your **Bison** spec beyond the
five we've listed, here is where you do it.

## 4. Adding more parse rules in **Bison**.

Finally, we get to the meaty part of the assignment: actually describing
the code for parsing **Snek**. In a way, your coding work can start here.
To describe that work, let's outline an example. Suppose you wanted to
add a new parse rule to our parser. Then you do the following:

1. Determine whether you need to introduce a new left-hand-side variable to the grammar production rules of the parser.

2. If so, add a new `%type` line to the variable section, and start a new section of grammar rules for that variable. You then might also need to add a new AST subclass to the `snek-ast.hh` and `snek-ast.cc` files.

3. If not, find the section for the variable in the existing list of grammar rules.

4. Regardless, describe the right-hand-side of the production rule along with the C++ code that acts when that rule fires. This action will normally be the construction of some component of the AST, and that compoent should get returned by the C++ code using the `$$ = ...` directive. We talk about this more below.

5. If the right-hand-side involves newly introduced tokens, then you need to add their definitions to the `%token` section with the appropriate (new, possibly four-letter, ALL CAPS) name.

We've essentially described all the steps earlier excepting #4. Step #4 is
where all the parsing magic happens and we work to demystify the coding
here.

In **Bison** the syntax of a language is specified by production rules, and
then the parsing of code that follows that rule is given by some C++
code in a corresponding action. The general syntax for a series of
rules are of the form:
~~~ none
lhs:
  rhs_1 { action_1 }
| rhs_2 { action_2 }
    ...
| rhs_k { action_k }
;
~~~
When we say this, we are describe a construct of the language with a variable
`lhs` that corresponds to a grammar's production
~~~ none
<lhs> ::= rhs_1 | rhs_2 | ... | rhs_k
~~~
that is, there are `k` ways to expand the variable using the grammar.

In **Bison**, 
the variable `lhs` is declared with a `%type` line (as we described
earlier). And then each `rhs_i` is made up of a series of **Bison** grammar
variables and Flex-related tokens. For example, our starter code has these
rules for statement syntax:
~~~ none
stmt:
  NAME ASGN expn EOLN { ... }
| PASS EOLN { ... }
| PRNT LPAR expn RPAR EOLN { ... }
;
~~~
These grammar rules correspond to the assignment, `pass`, and output statements
in our language.

A different right-hand-side can be seen with the rule
~~~ none
expn:
  expn PLUS expn { ... }
| expn MNUS expn { ... }
| ...
~~~
This gives the rule for parsing additions and subtractions. There are 
more rules just below those, but we've elided them with `...`. These
describe other binary operations and other integer expression rules.

We've adopted the convention throughout our **Bison** code that uses
lower case four-letter words for variables and ALL CAP four-letter
words for tokens. And so we are seeing, with each of these five
example rules a mix of tokens and variables given as their
ight-hand sides.

In general, **Bison** allows you to write a bit of code that "fires"
when a section of code is consumed by a parse that followed one
a grammar rule. The code for each rule is suggested by `action_i`
in the general format I described above. For the assignment statement
we saw that the action is
~~~ none
      $$ = Asgn_ptr { new Asgn {$1,$3,lexer.locate(@2)} };
~~~
This basically tells **Bison** that, when it parses an assignment statement
it should *return a value* (the `$$` assignment) that is a smart pointer
to a newly allocated `Asgn` node. To build that node we need three pieces
of information:

1. the name of the program variable being assigned,

2. the defining expression for (re-)assigning that variable, and

3. the location in the code where the assignment occurs.

The `$$` code within this C++ is **Bison**'s notation for *the result
returned by this rule*. The `$1`, `$2`, and `$3` codes within this
C++ are **Bison**'s notation for the information resulting from consuming
the `NAME` token, the `ASGN` token, and the text that was parsed
with the `expn`. The information attached to `NAME` was specified to
be a `std::string` according to the `%token` line we gave for `NAME`.
The information attached to the parse of `expn` was specified to be
of type `Expn_ptr` by the `%type` rule we gave for `expn`.  And then
also, it turns out, we can extract the source code location of the `=`
token by using our lexer's `locate` method. The `@2` is **Bison**'s
notation for accessing the source location (line and column info) 
of the token `ASGN`. 

Here is the action for parsing a sum:
~~~ none
$$ = Plus_ptr { new Plus {$1,$3,lexer.locate(@2)}
~~~
Its explanation is very similar.

We have a very regimented structure in how we parse our 
**Snek** code. We process and consume some of the source 
code as a series of tokens provided by the lexer, and then
we build an AST as a result. Hopefully you can get the gist
of what the other rules and actions do in the other parts of
the starting `.yy` file. They all contsruct and return a
portion of the AST for each portion of the source code 
that they parse.

### Coding: add new parse rules to `snek-bison.yy`.

Mimicking my structure, add the missing rules from the grammar
to handle parsing of the entire **Snek** spec, and maybe
also tackle some of the bonus constructs listed at the end of this
document.

### Coding: precedence and associativity

You are introducing several new binary operators for comparison
and for logic, and also the unary `not` operation. You'll need to
tell **Bison** how they sit with the addition and multiplication
operators. Currently, their precedence and associativity
are declared in a section like so:
~~~ none
%left PLUS MNUS;
%left TMES IMOD IDIV;
~~~
This tells **Bison** that `+` and `-` have lower precedence than
`*`, `%`, and `//`. It says that all those operations are left
associative. To include a unary operation in the hierarchy, you
use the `%precedence` directive, something like `%precedence NOT;`
if `NOT` is your token for `not`. To include "non-associative"
binary operations (like `<`) you use the `%nonassoc` directive.
  
## 5. Making AST methods for `dump` and for the interpreter.

You've already had experience in Project 0 of traversing the AST nodes
you invent, namely with your `dump` and interpreter methods. You will
need to continue that work here to test your parser. Each AST node class
you invent should know how to dump its subtrees out to the console.
And each AST node class should know its role within the interpreter
(e.g. how `exec` or `eval` work for it).

There are three major design changes to the interpreter.  
A **Snek** program now consists of a series of function definitions
along with a main block of script code that describes its
execution. And then also, at any place in the code, there might
be a call to a function (within an expression) or a call to
a procedure (as a statement).  And so the first major change 
in the interpreter from Project 0 is that the `eval` and the
`exec` methods need access to the code for those top-level
definitions to do their work. We call this the *global* or
*top-level* context.

And then also we have enhanced the language so that there are
string, integer, and boolean values computed by expressions and
by functions. This means that `eval` has to have the flexibility
to return more than just `int`. This is the second major design
change. 

Finally, as a third major change, note that at any point during the
execution of a procedure or function a `return` statement could be hit.
This means that when we are executing a block of statements, we will
need to immediately exit the code of the procedure or function we
are currently executing. And, when this happens, we might be deeply
digging through a block of statements, nested within a block of statements,
etc. So this means that we'll essentially need to jump out of a series
of recursive calls to `exec` back to the top-level execution of
the defining block of that procedure or function. 

To make this more clear, consider the program:
~~~ none
def is_prime(number):
    if number <= 1:
        return False
    else:
        divisor = 2
        while divisor < number:
            if number % divisor == 0:
                return False
            else:
                divisor += 1
        return True

print(is_prime(int(input("Number? "))))
~~~
The parse tree for the body of `is_prime` will have the form
~~~ none
...
    BLCK
        IFEL
            BLCK
                RTRN 
                    ...
            BLCK
                ASGN
                    ...
                WHLE
                    BLCK
                        IFEL
                            BLCK
==>                             RTRN
                                    ...
                RTRN
~~~
In particular, the `RTRN` line marked with `==>` is within
the fourth nested block of code. (There are 4 `BLCK` descendants
within the tree.) This means that when we handle the `exec`
of that `return`, we will be in the middle of several recursive
calls to `exec` methods (a total of seven; four for `BLCK`
and three for the intervening statements; the eighth one is the
`exec` for this `return`). And so each of these `exec` calls
will need to report back to their callers that a `return` 
statement was hit.

Below we describe each of these coding issues.

### Coding: add AST classes for each new construct

I'll be brief here, because this mimics Project 0.
The key additions to our Python-like language are `if-else`,
`while`, `return`, and `def` along with boolean-valued expressions
for doing value comparison and logic. This means that you'll need
the AST classes for each of these constructs. And then, depending
on what they do, they may need either an `exec` or an `eval`
method. And then for `def`, which I see as a new kind of 
construct (neither `Stmt` nor `Expn`) I encourage you to
invent a `call` method.

And then also you'll need to change `Pgrm` AST node so that, in addition
to the `main` block of script code, it also houses all the functions and
procedures introduced by `def` above the main script. This could be something
like a `std::vector<Defn_ptr>` or maybe an `unordered_map`. I'll let you
decide what you want to use to represent this information.
Within `snek-ast.hh` I've included a `typedef` line for a new
type called `Defs`. This can be the type used for that
collection of function and procedure definitions. I've set it
to be `std::vector<Defn_ptr>` but, again, you can make this
what you want.

### Coding `eval` to handle a variety of types of values

Since expressions can evaluate to several different types of data,
the `eval` method has a more complicated return type. Its type
signature is now of the form
~~~ none
Valu eval( ..., const Ctxt& ctxt) { ... }
~~~
and then we have declared the type `Valu` as
~~~  none
typedef std::variant<int, bool, std::string, unit> Valu;
~~~
(The `unit` type is just `class None`.) This STL allows us to
return several different kinds of values from `eval`, each 
corresponding to the types of values possible within a
**Snek** program. 

If you are new to `std::variant`, you can have a crash course in
it by looking at the code for `Plus::eval`, shown below:
~~~ none
    Valu lv = left->eval(defs,ctxt);
    Valu rv = rght->eval(defs,ctxt);
    if (std::holds_alternative<int>(lv) && std::holds_alternative<int>(rv)) {
        int ln = std::get<int>(lv);
        int rn = std::get<int>(rv);
        return Valu {ln + rn};
    } else if (std::holds_alternative<std::string>(lv)
               && std::holds_alternative<std::string>(rv)) {
        ... // similar code that handles std::string
    } else {
        ... // code that raises an error
    }
~~~
When we get a value back from evaluating a `Plus` node's two subtrees, we
check with `holds_alternative<int>` to see whether they are holding the `int`
variant. If so, we extract those two integer values with `get<int>`.
And then we compute and return the sum by constructing an `int` variant of `Valu`
with `return Valu {ln + rn};`.

### Coding: have `eval` and `exec` handle function and procedure calls

We've modified the starter code so that the interpreter methods
`exec` and `eval` all have access to the global context. We've done
that by referencing a value of type `Defs` everywhere. This (as mentioned
above) contains information about the collection of procedures
that could be invoked within a statement block, and all the functions that
can be called within an expression. For example, the full type signature
for `eval` is
~~~ none
Valu eval(const Defs& defs, const Ctxt& ctxt) { ... }
~~~
The `defs` are just the subtree of `Prgm` that corresponds to all the
`def` blocks.

You'll need to figure out the method `exec` for the subclass of
`Stmt` that invokes procedures and the method `eval`
for the subclass of `Expn` that makes function calls. 
Basically, you need to evaluate the parameter expressions
that are being passed to the procedure/function, and then
you'll need to build a new frame of local variables as a `Ctxt`
for executing that procedure's/functions' block of code.
And then you'll execute the block of code that defines the 
function or procedure.

It is in the execution of the procedure/function body
that you need to be thoughftul about procedure/function return,
describes just below.

### Coding: have `exec` indicate whether a `return` was encountered

Since executing a block of statements could lead to a `return`,
and also since executing an `if-else` or a `while` loop might lead
to a `return` we need a way to exit out of `exec` when we've
encountered a `return` statement. The starter code has this 
type signature for `exec` methods:
~~~ none
VOpt exec(const Defs& defs, Ctxt& ctxt) { ... }
~~~
The type `VOpt` is just a synonym for `std::optional<Valu>`.
It allows us to *optionally* return a `Valu`.
The design here is that, if we execute a block of code and encouter a
`return` that seeks to return the value `rv` then we stop executing
the block's statements and return it. 
If instead no `return` was encountered and we "fall through" the
block of code, then we `return std::nullopt`.
This is summarized by the code for `Blck::exec` below:
~~~ none
    for (Stmt_ptr s : stmts) {
        std::optional<Valu> rv = s->exec(defs,ctxt);
        if (rv.has_value()) {
            return rv;
        } 
    }
    return std::nullopt;
~~~
When you extend the syntax with `while` and `if-then`, you'll need to
be wary of whether a `return` is encountered within their blocks, and
act accordingly. 

And then also you'll invent the behavior of `exec` for the new
AST nodes for function return statments and procedure return
statements.

# Exercises

## Add `+=` and `-=`.

We'll probably do one of these in lab together.

## Add the `if`-`else` statement

Here you will confront parsing the `INDT` and `DEDT` tokens. They aren't too bad.

## Add comparison operations `<`, `==`

You can add the others, too, but these are enough to express quite a bit.

## Add the `while` statement.

It's exciting to make your language's programs never halt!

## Add logical operations `and`, `or`, `not`.

Hey, why not?

## Add `def` and procedure invocation.

Here you are levelling up quite a bit. You need to figure out how to
represent the components of a defined procedure when you invent `class Defn`.
Each has some named formal parameters (maybe initially have them only take one or none). 
It has some indented lines. 
And then there should be a new `Stmt` type for calling one, sending it values for its parameters. 

## Add `return` and function invocation

Same as the above, but you'll now need an `Expn` type for calling a function, 
you'll need a `return` statement, and you'll need a mechanism for getting a returned value.

# BONUS exercises 

Here are a few other things that you might consider.
The last of these is pretty complex, especially
if you haven't taken CSCI 384. If you want to do it,
you should probably talk to me first about what's
involved.

The first two are the same as those we had in Project 0.

## Add a conditional expression.

Python allows the construct given by
~~~ none
<expn> ::= <expn> if <expn> else <expn> expression
~~~
Figure out its precedence in Python relative to the other operators,
and parse these expressions too.

## Allow `print` to take multiple arguments.

Change the `print` syntax so that 0, 1, or more than one arguments
can be passed to it, like suggested below
~~~ none
<expn> ::= print ( <expn> , ... , <expn> )  
~~~
When executed, the values should be printed on the same line, separated
by a space. A call to `print()` should print an empty line.

## Add the unary minus operation.

Add a unary minus operator `-` to our grammar with
~~~ none
<expn> ::= - <expn>
~~~
This should have the highest precedence amongst the unary an binary
operations. 

## Add the `if` and the cascading `elif` statements.

The language as defined forces every `if` to have a corresponding
`else`.  This often requires a programmer to use a lot of `pass`
statements when no `else` action is actually required. Change the
syntax to include a statement
~~~ none
<stmt> ::= if <expn> : <stmt>
~~~
so that the syntax is less cumbersome.

Python also provides "cascading" conditionals with `elif`.
Add this construct to the language.

Note that you can, if you like, parse these without inventing new
AST node types. Instead, the parser can just rewrite these to use
the existing AST node for `if-else`.  

## Shortcircuit the `and` and `or`.

I didn't say so above, but note that `and` and `or` are normally
short-circuited in Python. Write the interpreter so that it does
the same. This means that the second expression may not need to
be evaluated if the first expression's value already resolves
the condition (`False` for conjunction, `True` for disjunction). 

## Add a `repeat-until` statement.

Right all the things that are wrong with this world by adding a
statement with the syntax
~~~ none
<stmt> ::= repeat: <nest> until <expn> EOLN
~~~
like what was in the language **Pascal**.
This will make your instructor very very happy.

## Allow nested functions and function values.

We have made our lives easy by only allowing `def` to occur
at the top level. We instead could have written the grammar
like so:
~~~
<prgm> ::= <blck>
<stmt> ::= def <name> ( <name> , ... , <name> ) : <nest>
~~~
This would suddenly make "higher order functions" possible
because a programmer could then write
~~~ python
def curried_sum(x):
    def added_to(y):
        return x + y
    return added_to

addTen = curried_sum(10)
print(addTen(32))
~~~
to print the value `42`.

This means that function values need to be added to the `Valu` type.
But representing them is complicated. We could call `curried_sum` like so:
~~~ python
addTen = curried_sum(10)
addTwo = curried_sum(2)
~~~
and so that means that the function objects `addTen` and `addTwo` need to
"carry around" their individual notions of `x` with them. This requires
your function values to be *closures*. A function value needs to
know the code of the function it represents, and it also needs reference
to the frame of variables that was active when the function value was
built.

Doing this properly would eliminate the need for a `std::vector<Defn>` to be
passed to the methods for executing statements and evaluating expressions,
but it is conceptually trickier to do this.

Compiling this kind of code is even trickier, and it makes sense that many
compiled languages (like C) do not allow it.
