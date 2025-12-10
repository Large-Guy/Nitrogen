# Nitrogen Language Specification v0.1

## 1. Overview

Nitrogen is designed to be a slightly safer, minimalist, and clean extension (but not a superset) of C.
Hence the name Nitrogen (N comes after C on the Periodic Table). The language is not bound to C grammar 
style completely but takes heavy inspiration from its syntax, with other influences coming from rust.

Note: Much of this is subject to change as the language develops.

Note: IIZ seems to mean Initialize If Zero, I thought it was Initialization Is Zero.

## 2. Lexical Structure

### 2.1 Keywords

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

## 3. Types

### 3.1 Primitives
 - i[N]: signed integers of [N] bits
 - u[N]: unsigned integers of [N] bits
 - f[N]: floating point of [N] bits
 - bool: boolean type, identical to i8.
 - string: unsure how this will be defined.

### 3.2 Pointer and Reference Types
 - T*: Non-nullable pointer of type T. A "reference." Either must be assigned immediately or will initialize IIZ.
Once assigned, the underlying pointer **cannot** change. You can change the value by dereferencing it with `*`
 - T*?: Nullable pointer of type T. A true pointer. Cannot by itself be read or written to, a null check is required first.

example:
```c++
i32*? a = get_nullable();

//assign value
*a = 67; // error: this is not permitted, because it is unknown whether a is null or not.

i32* b = get_non_nullable();

*b = 41; // b is non_nullable, meaning it acts more as a reference. 
```

### 3.3 Array Types
unsure how this will exactly be defined.

## 4. Expressions

### 4.1 Literals
 - Integers: 67, 0xFF
 - Float: 3.14, 3.14f, 3f
 - String: "Hello, World!"
 - Bool: true, false
 - Pointers: null

### 4.2 Null Checking
A null check is **required** to access a pointers value. You are not permitted to dereference the underlying variable 
before null-checking.

```c++
i32*? a = get_nullable();



if (a != null)
{
    *a = 87;
}
```

## 5. Statements

### 5.1 Variables

Follows Standard C syntax:

`[Type] [Name];` <-- follows IIZ.

`[Type] [Name] = [Value];` <-- assigned immediately.

### 5.2 Control Flow

Identical to standard C.

### 5.3 Return

Identical to Standard C.

## 6. Functions

### 6.1 Declaration

Identical to Standard C. Forward Declarations are unnecessary.

```c++
[Return Type] [Name]([Type] [Arg1 Name], [Type] [Arg2 Name])
{
    // body
}
```

## 7. Structures

Structures look identical to Standard C. Nitrogen does not have standard RAII, but does follow IIZ.
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
}

Point p = Point.new(1f, 2f);
```

### 8. Memory Managment

This is unknown for now. Leaning towards manual C Style memory management. Likely with some safety features.