# Zspie Lang

Zspie is a fast, easy, dynamic programming language for beginners. It has all the basic features most languages provide: expressions, variables, scopes, string, globals, if-else, for-loop, while-loop, functions. The compiler is written entirely in C which makes it really fast.

## Building
The project uses Cmake as it's build system.

#### Requirements:
- A C compiler
- [Git](https://git-scm.com/)
- [Cmake](https://cmake.org/)
- Platform specific build tools
    - [make](https://www.gnu.org/software/make/) (linux)
    - [MS Visual Studio](https://visualstudio.microsoft.com/) (windows)
    - [XCode](https://developer.apple.com/xcode/) (OSX)

1. Clone the repo and cd into the project root directory.
```sh
git clone https://github.com/prashantrahul141/zspie && cd zspie
```

2. Run cmake to create build files for your platform.
```sh
cmake
```
additionally you can provide `-DCMAKE_BUILD_TYPE={configuration}` to build in `Debug`, `Release` configurations.

3. Open/run your platform specific build files.

- For Linux:
just run make
```sh
make
```

- For windows:
Open Microsoft Visual Studio Solution file.

- For OSX:
Open XCode Solution file.



## Using the compiler
You can either use the live repl
```sh
zspie
```
or pass a `.zspie` file
```sh
zspie main.zspie
```




# Language documentation

### File Extension

Zspie supports text files with `zspie` file extension.
example: `main.zspie`

### Hello, world!

A simple hello world program in Zspie:

```python
print "Hello, World!";
```

Semi-colons at the end of every line is mandatory in Zspie.

### Datatypes

Zspie has following datatypes

#### Numbers

These can number literals which can be both integers and floating point numbers.

examples: `1`, `2.5`, `9`

#### Strings

These are string literals defined inside `"`

examples: `"Zspie"`, `"Strings are easy"`

### Booleans

These are boolean literals which can be either `true` or `false`.

### Null

Zspie has nulls, It can be defined using the `null` keyword. All uninitialized variables are given the value of `null`.

### Operators.

Zspie has following operators:

`=` - equals

`-` - Unary negation

`and` - logical AND

`or` - logical OR

`!` - logical NOT

`+` - sum

`-` - difference

`*` - product

`/` - division

`==` - is equals

`!=` - is not equals

`>` - is less than

`>=` - is less than or equals

`>` - is greater than

`>=` - is greater than or equals

### Comments

Zspie has only one type of comment, single line comments, which can be defined using `//` at the beginning of a line.

```c
// This is a comment.
// The Lexer completely ignores any line starting with //
// The Whole line is ignored.
```

### Variables

Zspie has variables which can be defined using the `let` keyword without defining any data type, zspie can automatically detect datatype at runtime.

```rust
let a; // default value is null if nothing is assigned.
let b = 2; // numbers: both integers
let c = 2.5; // and floats
let d = "Strings are easy"; // strings
let e = true; // booleans
```

### Scope

Zspie variables have scope like any other modern programming language.

```rust
let a = 1;
{
    let a = 2;
    print a; // 2
}
print a; // 1
```

### Conditionals

Zspie has `if` `else` conditionals. It can be defined using the following syntax:

```rust
let a = 1;
if (a == 1) {
    print "A is infact 1";
} else {
    print "A is not 1";
}
```

### While loop

while loops in zspie can be defined using the following syntax:

```rust
let a = 10;
while (a > 1) {
    print a;
    a = a - 1;
}
```

### For loops

Zspie has support for`for` loops (even though it is just syntatic sugar).

```c
for(let i = 0; i < 10; i = i + 1) {
    print i;
}
```

### Functions

Zspie have user defined functions, and ability to call them.

#### Function declaration

A function in zspie can be defined using the following syntax:

```rust
fn greet(name) {
    print "Hello " + name;
}
```

#### Calling functions

A function can be called using the following syntax:

```c
greet("Zspie");
```
