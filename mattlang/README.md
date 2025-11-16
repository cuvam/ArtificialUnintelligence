# Matt Language Interpreter

A C-based interpreter for the Matt programming language, a statically-typed, C-like language with explicit type safety and no implicit conversions.

## Overview

This interpreter implements the Matt Language specification (v0.1) as described in `matt-lang-spec.md`. Matt is designed around three core philosophies:

1. **No Nil Except for Pointers** - All primitive types must be initialized
2. **No Implicit Conversions** - All type conversions must be explicit via casting
3. **Explicit Returns Required** - Every function must have an explicit return statement

## Building

```bash
make
```

This will compile the interpreter and create the `matt` executable.

## Usage

```bash
./matt <program.matt>
```

Example:
```bash
./matt tests/01_hello_world.matt
```

## Language Features Implemented

### ✅ Fully Implemented
- **Lexer/Tokenizer** - Complete tokenization of Matt source code
- **Parser** - Full recursive descent parser generating AST
- **Data Types** - int, float, double, long, bool, char, string
- **Arrays** - Dynamic arrays with `.length` property
- **Arithmetic Operators** - +, -, *, /, %
- **Comparison Operators** - ==, !=, <, >, <=, >=
- **Logical Operators** - &&, ||, !
- **Control Flow** - if/else, while, for loops
- **Functions** - Function declarations and calls
- **Variable Declarations** - With mandatory initialization
- **Type Casting** - Explicit type conversions
- **Built-in Functions** - printf with format specifiers (%d, %f, %s, %c)
- **Break/Continue** - Loop control statements

### ⚠️ Partially Implemented
- **Function Recursion** - Implemented but has memory management issues
- **Switch Statements** - Parsed but not fully tested

### ❌ Not Yet Implemented
- **Type Checker** - Parsing works, but comprehensive type checking is minimal
- **Pointer Types** - Type system supports it, runtime doesn't
- **Structs** - Not in current spec version
- **Memory Management** - Some memory leaks exist (see Known Issues)

## Test Suite

The `tests/` directory contains 10 test programs demonstrating various language features:

1. **01_hello_world.matt** - Basic "Hello, World!" program
2. **02_arithmetic.matt** - Arithmetic operations (+, -, *, /, %)
3. **03_factorial.matt** - Recursive factorial (has memory issues)
4. **04_loops.matt** - For and while loops
5. **05_arrays.matt** - Array creation and access
6. **06_conditionals.matt** - If/else statements
7. **07_boolean.matt** - Boolean logic (&&, ||, !)
8. **08_type_casting.matt** - Explicit type conversions
9. **09_fibonacci.matt** - Fibonacci sequence (recursive, has issues)
10. **10_comparisons.matt** - Comparison operators

### Running Tests

Run individual tests:
```bash
./matt tests/01_hello_world.matt
```

Or use the Makefile:
```bash
make test
```

## Example Programs

### Hello World
```c
int main() {
    printf("Hello, World!\n");
    return 0;
}
```

### Loops and Arrays
```c
int main() {
    int[] numbers = [1, 2, 3, 4, 5];

    for (int i = 0; i < numbers.length; i = i + 1) {
        printf("%d ", numbers[i]);
    }
    printf("\n");

    return 0;
}
```

### Type Casting
```c
int main() {
    int x = 5;
    float y = (float)x;  // Explicit cast required

    printf("x: %d, y: %g\n", x, y);
    return 0;
}
```

## Architecture

The interpreter follows a traditional pipeline:

```
Source Code → Lexer → Tokens → Parser → AST → Interpreter → Output
```

### Components

1. **lexer.c** - Tokenization of source code
2. **parser.c** - Recursive descent parser building AST
3. **interpreter.c** - Tree-walking interpreter
4. **utils.c** - Utility functions and AST management
5. **matt.h** - Header with all type definitions
6. **main.c** - Entry point and file handling

## Known Issues

1. **Memory Management** - There are some memory leaks to avoid double-free errors. The cleanup code in `main.c` is commented out. Since the interpreter is short-lived, this is acceptable for a prototype.

2. **Recursion** - Recursive functions work but may cause memory issues in some cases due to value copying.

3. **Type Checking** - While the parser validates syntax, comprehensive static type checking is minimal.

## Spec Compliance

This interpreter implements most of the Matt Language Specification v0.1:

- ✅ Explicit type declarations
- ✅ Mandatory variable initialization
- ✅ Explicit return statements required
- ✅ No implicit type conversions
- ✅ Boolean as first-class type
- ✅ Dynamic arrays with bounds checking
- ✅ Explicit casting syntax
- ⚠️ Switch statements (parsed but not fully implemented)
- ❌ Pointer types (not in runtime)

## Future Improvements

- Comprehensive type checker pass before interpretation
- Fix memory management for proper cleanup
- Implement switch statements fully
- Add more built-in functions
- Better error messages with line numbers
- Support for the full language spec (structs, pointers, etc.)

## License

This is an educational implementation of the Matt language specification.
