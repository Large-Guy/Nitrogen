# Nitrogen Language Specification v0.1

## Table of Contents

<!-- TOC -->
* [Nitrogen Language Specification v0.1](#nitrogen-language-specification-v01)
  * [Table of Contents](#table-of-contents)
  * [1. Overview](#1-overview)
  * [2. Lexical Structure](#2-lexical-structure)
    * [2.1 Keywords](#21-keywords)
    * [2.2 Identifiers](#22-identifiers)
    * [2.3 Operators](#23-operators)
    * [2.4 Visibility](#24-visibility)
  * [3. Types](#3-types)
    * [3.1 Primitives](#31-primitives)
    * [3.2 Pointer and Reference Types](#32-pointer-and-reference-types)
    * [3.3 Array Types](#33-array-types)
  * [4. Expressions](#4-expressions)
    * [4.1 Literals](#41-literals)
    * [4.2 Null Checking](#42-null-checking)
  * [5. Statements](#5-statements)
    * [5.1 Variables](#51-variables)
    * [5.2 Control Flow](#52-control-flow)
    * [5.3 Return](#53-return)
  * [6. Functions](#6-functions)
    * [6.1 Declaration](#61-declaration)
  * [7. Structures](#7-structures)
    * [7.1 Constructors](#71-constructors)
    * [7.2 Deconstructors](#72-deconstructors)
  * [8. Interfaces](#8-interfaces)
  * [9. Casting](#9-casting)
  * [10. Memory Management](#10-memory-management)
  * [11. Generics](#11-generics)
    * [11.1 Single Generics](#111-single-generics)
    * [11.2 Variadic Generics](#112-variadic-generics)
    * [11.3 Structural Singular Generics](#113-structural-singular-generics)
    * [11.4 Structural Variadic Generics](#114-structural-variadic-generics)
  * [12. Styling](#12-styling)
    * [12.1 Casing](#121-casing)
    * [12.2 Braces](#122-braces)
    * [12.3 Ordering](#123-ordering)
<!-- TOC -->

## 1. Overview

Nitrogen is designed to be a slightly safer, minimalist, and clean extension (but not a superset) of C.
Hence the name Nitrogen (N comes after C on the Periodic Table). The language is not bound to C grammar 
style completely but takes heavy inspiration from its syntax, with other influences coming from rust.

Note: Much of this is subject to change as the language develops.

## 2. Lexical Structure

### 2.1 Keywords

the only keyword used differently is the `static` keyword. 
`static` is unnecessary because of visibility rules. See 2.4.

| module | import | return | struct   | union  | interface |
|--------|--------|--------|----------|--------|-----------|
| enum   | static | const  | void     | bool   | u8        |
| u16    | u32    | u64    | i8       | i16    | i32       |
| i64    | f32    | f64    | isize    | usize  | string    |
| null   | true   | false  | if       | else   | while     |
| do     | for    | break  | continue | switch | pass      |

### 2.2 Identifiers
`[a-zA-Z_][a-zA-Z0-9_]*`

### 2.3 Operators

| +  | -  | *  | /  | %    | **   |
|----|----|----|----|------|------|
| == | != | \< | \> | \<=  | \>=  |
| &  | \| | ^  | ~  | \<\< | \>\> |
| =  | ++ | -- |    |      |      |

### 2.4 Visibility

Any identifier, (struct, interface, function, methods, fields, global variables) when prefixed with `_` will be marked
private. Meaning things are public by default.

## 3. Types

### 3.1 Primitives
 - **i[N]**: signed integers of [N] bits
 - **u[N]**: unsigned integers of [N] bits
 - **f[N]**: floating point of [N] bits
 - **bool**: boolean type, identical to i8.
 - **string**: unsure how this will be defined.

### 3.2 Pointer and Reference Types
 - T*: Non-nullable pointer of type T. A "reference." It must be assigned before it is used, this should be enforced by the compiler.
Once assigned, the underlying pointer **cannot** change. 
 - T*?: Nullable pointer of type T. A true pointer. Cannot by itself be read or written to, a null check is required first.

example:
```c++
i32*? a = getNullable();

//assign value
*a = 67; // error: this is not permitted, because it is unknown whether a is null or not.

if (a != null)
{
    *a = 67; // this is permitted, because it has been ensured that the pointer is not null.
}

i32 original;
i32* b = &original; // reference pointer assigned immediately

i32 other;
b = other; // this is NOT allowed by the compiler a reference pointer CANNOT change.

*b = 41; // b is non-nullable, meaning it acts as a reference. 
// original is now 41.
```

### 3.3 Array Types
TODO: define this

## 4. Expressions

### 4.1 Literals
 - **Integers**: 67, 0xFF
 - **Float**: 3.14, 3.14f, 3f
 - **String**: "Hello, World!"
 - **Bool**: true, false
 - **Pointers**: null

### 4.2 Null Checking
A null check is **required** to access a pointers value. You are not permitted to dereference the underlying variable 
before null-checking.

```c++
i32*? a = getNullable();

if (a != null)
{
    *a = 87;
}
```

## 5. Statements

### 5.1 Variables

Follows Standard C syntax:

`[Type] [Name];` <-- follows ZII.

`[Type] [Name] = [Value];` <-- assigned immediately.

### 5.2 Control Flow

Identical to standard C.

### 5.3 Return

Identical to Standard C.

## 6. Functions

### 6.1 Declaration

Identical to Standard C. Forward Declarations are unnecessary.

```c++
i32 function(i32 a, f32 b, bool c)
{
    
}
```

Nitrogen also supports function variants, so you can have multiple functions with the same name, just with different
arguments.

## 7. Structures

### 7.1 Constructors

Structures look identical to Standard C. Nitrogen does not have standard RAII, but does follow ZII.
Functions can be placed in structs, static functions can act as constructors.

```c++
struct Point
{
    f32 x; // assignment here is NOT permitted
    f32 y; // same here
    
    static Point new(f32 x, f32 y)
    {
        Point point;
        point.x = x;
        point.y = y;
        return point;
    }
    
    f32 length()
    {
        return sqrt(x ** 2 + y ** 2)   
    }
}

Point p = Point.new(1f, 2f);
```

### 7.2 Deconstructors

Structures also support deconstructors allowing for safe cleanup when a variable goes out of scope. There name is `~`.

```c++
struct File
{
    i32 _handle;
    string _path;
    
    static File open(string path)
    {
        File f;
        f._handle = system_open(path);
        f._path = path;
        return f;
    }
    
    ~()
    {
        if (_handle != 0)
        {
            system_close(_handle);
        }
    }
}
```

## 8. Interfaces

All functions on an interface **must** be implemented. There are no defaults for functions.

```c++
interface IAnimal
{
    void makeSound();
}

struct Dog : IAnimal
{
    void makeSound()
    {
        print("Woof!");
    }
}
```

## 9. Casting

There are two types of casts, conversion, and reinterpret casting (or safe, and unsafe). Reinterpret/Unsafe casts are
denoted with the `!` prefix.

**Conversion/Safe Cast**
```c++
f32 x = 3.14159f;
i32 y = (i32)x;
```

**Reinterpret/Unsafe Cast**
```c++
i32 bits = 0x40490FDB;
f32 f = (!f32)bits;
```

## 10. Memory Management

TODO: This is unknown for now. Leaning towards manual C Style memory management. Likely with some safety features.

## 11. Generics

Structs, interfaces, functions, and methods can be generic. Generics are designed to be as simple as possible, similar
to C# generics, but more powerful. The goal is to avoid metaprogramming that makes code hard to debug or understand.

### 11.1 Single Generics

```c++
void swap<T>(T* a, T* b)
{
    T temp = *a;
    *b = *a;
    *a = temp;
}
```

### 11.2 Variadic Generics

variadic generics are expanded automatically.

```c++
void printValues<T[]>(T[] values)
{
    print(values); // implicitly expanded
}
```

### 11.3 Structural Singular Generics

```c++
struct Pair<Key, Value>
{
    Key key;
    Value value;
}
```

### 11.4 Structural Variadic Generics

```c++
struct Tuple<T[]>
{
    T[] values;
    
    static Tuple<T[]> new(T[] args)
    {
        Tuple<T[]> tuple;
        tuple.values = args;
        return tuple;
    }
    
    void print()
    {
        print(values); // expanded implicitly
        print("\n");
    }
}
```

## 12. Styling

Note: `_` makes anything private.

### 12.1 Casing

 - **Variables**: `snake_case`
 - **Structs**: `PascalCase`
 - **Interfaces**: `IPascalCase`
 - **Functions**: `camelCase`

### 12.2 Braces

**Braces**: always begin on the next line.

### 12.3 Ordering

Standard ordering / formatting for a program.

```c++
module program; // module is always on the first line

// interfaces

interface IInterface
{
    void interfaceFunction();
}

// structures

struct Struct
{
    // nesting follows the same ordering
    struct Inner : IInterface
    {
        i32 value;
        
        void interfaceFunction()
        {
            print(value);
        }
    }
    
    //statics
    
    static i32 count;
    
    //publics
    Inner value;
    
    //privates
    i32 _private;
    
    // constructor destructor
    
    static Struct new(i32 value)
    {
        count++;
        Struct s;
        s.value.value = value;
        return s;
    }
    
    ~()
    {
        count--;
    }
    
    //functions
    
    void printValue()
    {
        value.interfaceFunction();
    }
}

// functions
void function()
{
    Struct s = Struct.new(variable);
    s.printValue();
}

// variables

static i32 variable = 64;

// optional main function, will be called during module initialization

i32 main()
{
    function();
    return 0;
}
```