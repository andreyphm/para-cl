<h1 align="center">ParaCL</h1>

<p align="center">A language front end and interpreter for ParaCL programs</p>

<p align="center">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white">
  <img alt="CMake 3.16+" src="https://img.shields.io/badge/CMake-3.16%2B-064F8C?logo=cmake&logoColor=white">
</p>

---

`para_cl` reads a source file, parses the whole program into an abstract syntax tree (AST), and executes it. A program can read numbers with `?`, store them in variables, and write results with `print`. All values have type `int`.

## Contents

- [Getting started](#getting-started)
- [Usage](#usage)
- [Grammar](#grammar)
- [How a program runs](#how-a-program-runs)
  - [Lexer](#lexer)
  - [Parser](#parser)
  - [AST execution](#ast-execution)
  - [Runtime context](#runtime-context)
- [Testing](#testing)
- [Project layout](#project-layout)

## Getting started

> [!NOTE]
> The Release build and tests were verified on Windows (MSYS2 UCRT64, GCC 16.2) and on Ubuntu 24.04 under WSL (GCC 13.3).

| Tool | Version | Used for |
|---|---|---|
| C++ compiler and standard library | C++20 | Building the interpreter |
| CMake | 3.16+ | Configuring the build and running tests with [CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html) |
| Git | Any recent version | Cloning the repository and fetching [GoogleTest](https://github.com/google/googletest) |
| Python | 3.7+ | Running the end-to-end tests |

From an MSYS2 UCRT64 terminal, install the build tools and test dependencies with

```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python
```

Clone and build the project with

```bash
git clone https://github.com/andreyphm/para-cl.git
cd para-cl
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Tests are enabled by default. CMake fetches GoogleTest v1.17.0 on the first configure. To build only the interpreter and its core library, add `-DBUILD_TESTING=OFF` to the configure command.

## Usage

Pass a source file to `para_cl` and supply its input on stdin. Try the [factorial](examples/factorial.paracl) and [Fibonacci](examples/fibonacci.paracl) examples from the project root.

Bash (WSL or MSYS2 UCRT64)

```bash
echo 5 | ./build/para_cl examples/factorial.paracl     # prints 120
echo 10 | ./build/para_cl examples/fibonacci.paracl   # prints 89
```

PowerShell

```powershell
5 | .\build\para_cl.exe examples/factorial.paracl     # prints 120
10 | .\build\para_cl.exe examples/fibonacci.paracl    # prints 89
```

## Grammar

The [ParaCL grammar](docs/grammar.md) uses token kinds. You can change how a keyword, operator, or delimiter is written by editing its entry in the [token spelling table](include/token_spellings.hpp). The grammar rules still refer to the same token kind.

## How a program runs

### Lexer

Implementation: [source/lexer.cpp](source/lexer.cpp), [include/token_spellings.hpp](include/token_spellings.hpp).

The lexer splits the source into tokens and records the starting line and column of each one. It skips whitespace and `//` line comments. When two operators share a prefix, it takes the longest match. It reads a whole identifier before checking for keywords.

### Parser

Implementation: [source/parser.cpp](source/parser.cpp).

The recursive-descent parser consumes the token sequence and builds the AST. Each level of expression precedence has its own rule.

### AST execution

Implementation: [include/ast.hpp](include/ast.hpp), [source/ast.cpp](source/ast.cpp).

The interpreter walks the tree. Expressions evaluate to an `int`, while statements perform actions.

Zero is false and any other value is true.
Binary operators evaluate the left operand first. Comparisons and logical operators return `0` or `1`. The logical operators short-circuit, so `0 && ?` evaluates to `0` without reading input. Arithmetic results must fit in `int`. Division by zero is a runtime error.

### Runtime context

Implementation: [source/runtime_context.cpp](source/runtime_context.cpp).

Execution starts in a global scope. Entering a block opens a scope, and leaving the block removes the variables created there. Assignment updates the nearest existing variable with that name or creates it in the current scope. Reading an unknown variable is a runtime error. The context also supplies the streams used by `?` and `print`.

## Testing

Run the full suite or select a group by its label using CTest.

```bash
cd build
ctest --output-on-failure
ctest -L unit --output-on-failure
ctest -L e2e --output-on-failure
```

The unit tests exercise the lexer, parser, AST nodes, and variable scopes separately. [Unit tests](tests/unit)

The end-to-end tests launch the executable with a source file and input data, then check stdout, stderr, and the exit status. The programs cover factorial, Fibonacci numbers, the greatest common divisor, nested loops, and short-circuit evaluation. [End-to-end tests](tests/e2e)

## Project layout

```text
.
├── .gitignore
├── CMakeLists.txt                # Core library, CLI, and test configuration
├── README.md
├── cmake/
│   └── warnings.cmake            # Compiler warning options
├── docs/
│   └── grammar.md                # ParaCL grammar
├── examples/
│   ├── factorial.paracl
│   └── fibonacci.paracl
├── include/
│   ├── ast.hpp                   # Expression and statement nodes
│   ├── lexer.hpp
│   ├── parser.hpp
│   ├── runtime_context.hpp       # Variable scopes and I/O streams
│   ├── token.hpp                 # Token kinds and source positions
│   └── token_spellings.hpp       # Keywords, operators, and delimiters
├── source/
│   ├── ast.cpp
│   ├── input.cpp                 # Source file reader
│   ├── input.hpp
│   ├── lexer.cpp
│   ├── main.cpp                  # CLI entry point
│   ├── parser.cpp
│   ├── runtime_context.cpp
│   └── token.cpp
└── tests/
    ├── CMakeLists.txt
    ├── unit/
    │   ├── ast_tests.cpp
    │   ├── lexer_tests.cpp
    │   ├── parser_tests.cpp
    │   ├── runtime_context_tests.cpp
    │   └── statement_tests.cpp
    └── e2e/
        ├── CMakeLists.txt
        ├── programs/             # ParaCL source templates for CLI tests
        └── test_cli.py           # Runs the CLI and checks its output
```
