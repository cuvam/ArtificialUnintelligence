# Matt Lang Language Specification

## Overview

Matt Lang is a statically-typed, interpreted programming language with C-like syntax designed around the philosophy of explicit behavior and type safety. The language eliminates implicit conversions and undefined behavior common in C while maintaining familiar syntax.

**Design Version:** 0.1  
**Date:** November 2025

---

## Core Design Philosophies

### 1. No Nil Except for Pointers
- All primitive types (int, float, double, long, bool) must be initialized with actual values
- There is no concept of "nil", "null", or "undefined" for non-pointer types
- Pointers may be null as their natural "not pointing at anything" state
- This eliminates an entire class of undefined behavior bugs

### 2. No Implicit Conversions
- Type conversions must be explicit via casting
- No automatic promotion or demotion between types
- Attempting to use a value of one type where another is expected is a compile-time error
- Every conversion must be intentional and visible in the code

### 3. Explicit Returns Required
- Every function must have an explicit return statement
- This includes void functions (must end with `return;`)
- The main function must explicitly `return 0;` or another exit code
- No implicit return values under any circumstances

---

## Type System

### Primitive Types

```
int       - Integer type
float     - Floating point type  
double    - Double-precision floating point
long      - Long integer type
bool      - Boolean type (true/false only)
char      - Character type
```

### Pointer Types

```
type*     - Pointer to type (can be null)
```

### Array Types

```
type[]    - Dynamic array of type
```

Arrays are dynamic (similar to lists in other languages) and automatically manage memory allocation. They grow as needed when elements are added via explicit methods.

### String Type

```
string    - String type (internally char*)
```

### Type Declarations

Variables must be declared with explicit types:

```c
int x = 5;
float y = 3.14;
bool condition = true;
int[] numbers = [1, 2, 3, 4, 5];
```

---

## Variables and Initialization

### Declaration Rules

1. All variables must be explicitly typed
2. All non-pointer variables must be initialized at declaration
3. Variable names follow C identifier rules

### Examples

```c
// Valid
int x = 42;
float pi = 3.14159;
bool flag = false;

// Invalid - uninitialized primitive
int y;  // ERROR: must initialize

// Valid - pointer can be null
int* ptr = null;
```

---

## Operators

### Arithmetic Operators

```
+    Addition
-    Subtraction  
*    Multiplication
/    Division
%    Modulo
```

All operands must be of compatible numeric types. No implicit conversion.

### Comparison Operators

All comparison operators return `bool` type:

```
==   Equal to
!=   Not equal to
<    Less than
>    Greater than
<=   Less than or equal
>=   Greater than or equal
```

### Logical Operators

All logical operators operate on and return `bool`:

```
&&   Logical AND
||   Logical OR
!    Logical NOT
```

### Assignment Operators

```
=    Assignment (returns the assigned value)
```

Assignment is an expression that evaluates to the assigned value, enabling chained assignments:

```c
int a, b, c;
a = b = c = 5;  // All three get value 5

int x;
int y;  
x = (y = 10);  // y gets 10, then x gets 10
```

However, type checking still applies - all variables in the chain must be compatible types.

---

## Control Flow

### If Statements

Condition must be of type `bool`:

```c
if (condition) {
    // code
}

if (x > 5) {
    // code  
} else {
    // code
}

if (x > 5) {
    // code
} else if (x < 0) {
    // code
} else {
    // code
}
```

**Invalid:**
```c
int x = 5;
if (x) { }  // ERROR: x is int, not bool
```

**Valid:**
```c
if (x != 0) { }  // Explicit comparison returns bool
```

### While Loops

Condition must be of type `bool`:

```c
while (condition) {
    // code
}
```

**Infinite loop:**
```c
while (true) {
    // code
}
```

### For Loops

Syntax: `for (init; condition; increment)`

- **init**: Optional - can declare/initialize variables or be empty
- **condition**: REQUIRED - must be a bool expression
- **increment**: Optional - statement executed after each iteration

```c
for (int i = 0; i < 10; i = i + 1) {
    // code
}

// Infinite loop - must be explicit
for (; true; ) {
    // code
}
```

**Invalid:**
```c
for (;;) { }  // ERROR: condition required
```

Variables declared in init are scoped to the for loop.

### Switch Statements

Switch provides one-to-one mapping of values to code blocks:

**Rules:**
1. All cases must be of the same type as the switch expression
2. Each case implicitly breaks (no fallthrough)
3. No multiple case labels for the same block
4. Default case is optional

```c
switch (x) {
    case 1:
        handle_one();
    case 2:
        handle_two();
    case 3:
        handle_three();
    default:
        handle_default();
}
```

**Invalid:**
```c
switch (x) {
    case 1:
    case 2:
        // ERROR: no case stacking allowed
}
```

If multiple values need the same behavior, extract to a function:

```c
switch (x) {
    case 1:
        common_handler();
    case 2:
        common_handler();
}
```

### Break and Continue

Work exactly as in C:
- `break;` - Exit the innermost loop
- `continue;` - Skip to next iteration of innermost loop

---

## Functions

### Function Declaration

```c
return_type function_name(param_type param_name, ...) {
    // body
    return value;  // REQUIRED
}
```

### Return Statement Rules

1. **All functions must explicitly return**
2. **Void functions must have `return;` statement**
3. **All code paths must end in a return**

```c
// Valid
int add(int a, int b) {
    return a + b;
}

void print_hello() {
    printf("Hello");
    return;  // REQUIRED even for void
}

// Invalid
int multiply(int a, int b) {
    int result = a * b;
    // ERROR: missing return statement
}
```

### Main Function

The main function must:
1. Return an int (exit code)
2. Explicitly return a value (no implicit return 0)

```c
int main() {
    printf("Hello, World!");
    return 0;  // REQUIRED
}
```

---

## Arrays

### Dynamic Arrays

Arrays in Matt Lang are dynamically-sized and automatically manage memory:

```c
int[] numbers = [1, 2, 3, 4, 5];
float[] values = [];
```

### Array Operations

**Indexing:**
```c
int[] arr = [10, 20, 30];
int x = arr[0];  // x = 10
arr[1] = 25;     // arr is now [10, 25, 30]
```

**Bounds Checking:**
Arrays enforce strict bounds checking. You can only access existing indices:

```c
int[] arr = [1, 2, 3];  // Size: 3, valid indices: 0, 1, 2

arr[0] = 10;   // OK
arr[2] = 30;   // OK  
arr[3] = 40;   // ERROR: index out of bounds
```

**Growing Arrays:**

Arrays can only grow via explicit methods:

```c
int[] arr = [1, 2, 3];

arr.append(4);      // arr is now [1, 2, 3, 4]
arr[3] = 40;        // NOW this is valid

// Potential future methods:
arr.prepend(0);     // Add to beginning
arr.insert(2, 99);  // Insert at index
```

**No Sparse Arrays:**

Unlike JavaScript, you cannot create gaps in arrays:

```c
int[] arr = [];
arr[100] = 5;  // ERROR: cannot jump to index 100
```

You must grow the array incrementally via append/prepend/insert.

---

## Type Casting

All type conversions must be explicit:

```c
int x = 5;
float y = (float)x;  // Explicit cast required

// Invalid
float z = x;  // ERROR: no implicit conversion
```

---

## Comments

```c
// Single-line comment

/* Multi-line
   comment */
```

---

## Boolean Type

The `bool` type is a first-class type with only two values: `true` and `false`.

**No truthiness:** Integers, pointers, and other types cannot be used where a bool is expected.

```c
bool flag = true;

if (flag) { }        // Valid
if (5) { }           // ERROR: 5 is not bool
if (ptr) { }         // ERROR: pointer is not bool
if (ptr != null) { } // Valid: comparison returns bool
```

---

## Implementation Model

### Compilation Pipeline

Matt Lang uses a two-pass interpretation model:

1. **Lexer**: Source code → Tokens
2. **Parser**: Tokens → Abstract Syntax Tree (AST)
3. **Type Checker**: Validate AST for type safety
4. **Interpreter**: Execute the validated AST

All syntax and type errors are caught before execution begins.

### Runtime Type Representation

Variables are represented internally using a tagged union structure:

```c
typedef enum {
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_BINARY,
    TYPE_ARRAY
} Type;

typedef union {
    int INTEGER;
    float FLOAT;
    long LONG;
    double DOUBLE;
    char *STRING;
    bool BOOL;
    void *BINARY;
    // Array structure with element_type, data, length, capacity
} VarData;

typedef struct {
    char *varname;
    Type type;
    VarData data;
    size_t size;
} Variable;
```

---

## Error Handling

### Compile-Time Errors

The following are compile-time errors:
- Type mismatches
- Uninitialized non-pointer variables
- Missing return statements
- Invalid type conversions without explicit casts
- Array index out of bounds (when statically determinable)
- Switch cases with different types
- Non-bool expressions in conditions

### Runtime Errors

The following are runtime errors:
- Array index out of bounds
- Null pointer dereference
- Division by zero
- Memory allocation failures

---

## Future Considerations

### Not Yet Specified

The following features are under consideration for future versions:

1. Structs and user-defined types
2. Enums beyond internal type system
3. Function pointers
4. Import/module system
5. Standard library
6. String manipulation operations
7. Array slicing operations
8. Memory management for complex types
9. Error handling mechanisms (exceptions/results)
10. Const/immutability modifiers

---

## Examples

### Hello World

```c
int main() {
    printf("Hello, World!");
    return 0;
}
```

### Factorial Function

```c
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    printf("5! = %d", result);
    return 0;
}
```

### Array Processing

```c
int sum_array(int[] numbers) {
    int total = 0;
    for (int i = 0; i < numbers.length; i = i + 1) {
        total = total + numbers[i];
    }
    return total;
}

int main() {
    int[] values = [1, 2, 3, 4, 5];
    int sum = sum_array(values);
    printf("Sum: %d", sum);
    return 0;
}
```

### Type Safety Example

```c
int main() {
    int x = 5;
    float y = 3.14;
    
    // ERROR: cannot assign float to int without cast
    // x = y;
    
    // Valid: explicit cast
    x = (int)y;
    
    // ERROR: cannot use int as bool
    // if (x) { }
    
    // Valid: explicit comparison
    if (x != 0) {
        printf("x is non-zero");
    }
    
    return 0;
}
```

---

## Design Rationale

### Why No Implicit Conversions?

Implicit conversions are a major source of bugs:
- Silent precision loss (float → int)
- Unexpected type coercions
- Hard-to-debug behavior changes

Requiring explicit casts makes the programmer's intent clear and prevents accidental data loss.

### Why Explicit Returns?

Missing return statements are a common bug in C. Requiring explicit returns:
- Makes control flow obvious
- Catches missing return values at compile time
- Eliminates undefined behavior from reaching end of non-void function

### Why Strict Array Bounds?

JavaScript-style sparse arrays lead to:
- Accidental memory exhaustion
- Hard-to-debug typos (arr[10000] instead of arr[100])
- Unclear memory usage

Strict bounds checking catches these errors immediately.

### Why No Switch Fallthrough?

Switch fallthrough is a bug 99% of the time. Requiring explicit code per case:
- Eliminates forgotten break statements
- Makes intent clearer
- Encourages code reuse via functions

---

## Appendix: Comparison with C

| Feature | C Behavior | Matt Lang Behavior |
|---------|-----------|-------------------|
| Uninitialized variables | Undefined behavior | Compile error |
| Implicit type conversion | Allowed | Forbidden (must cast) |
| Bool type | 0=false, non-zero=true | Dedicated bool type only |
| Array bounds | Not checked | Strictly checked |
| Switch fallthrough | Default behavior | Never (implicit break) |
| Missing return | Undefined behavior | Compile error |
| Return in void functions | Optional | Required |
| Array sizing | Static or manual malloc | Dynamic, automatic |
| Null/nil for primitives | Possible (undefined) | Impossible (except pointers) |

---

**End of Specification Document v0.1**
