# LLVM: Motivation and goals

I'm doing this to understand how Clang + LLVM works so I can generate metadata for C++ types. The only way to do this is to either use a metaprogramming utility like [Metadesk](https://github.com/Dion-Systems/metadesk) or use [Clang LibTooling](https://clang.llvm.org/docs/LibTooling.html) to write a custom code generator.

## Goals

0) Understand what is LLVM  		DONE
1) Understand what is Clang  		DONE
2) Understand how compilers work  	DONE
	- Front-end:
		- 1) Generates AST from lexical and syntatical analysis, which is just a tree with all keywords, operators, identifiers, constants, symbols etc
		- 2) Performs semantic analysis to see if AST makes sense. Ex: Checks if an identifier exists in the scope
		- 3) Generates IR
		- Input: String with source code
		- Output: Code AST, IR
	- Middle-end: Performs multiple non-platform specific optimization passes
		- Input: IR
		- Output: Optimized IR
	- Back-end: Performs platform specific optimization passes and generates assembly code
		- Input: Optimized IR
		- Output: Optimized Assembly
	- (OPTIONAL) Linker: C++ has a linker step after the back-end is executed
3) Understand how Clang works
	- Clang is a **compiler front-end** that uses LLVM as its back-end. It's made in C++
	- Some of the LLVM tools that Clang uses are:
		- opt: Optimizes LIR
		- llc: Converts IR to assembly
		- Output format is: <token_type> <token> <extra_annotations>
	- Clang uses LLVM tools by calling them via C++
	- You **don't need to know the middle/back-end** tools to use Clang
	- A standard clang compilation would look something like:
	```
		clang main.cpp -o main.exe
	```
	- Alternatively, you could do a step-by-step compilation calling the middle/back-end manually:
	```
		clang -S -emit-llvm main.cpp -o main.bc
		opt -mem2reg main.bc -o main.opt.bc
		llc main.opt.bc -o main.x86
	```
	- Clang front-end steps:
		- Source code > `Preprocessor/Lexer` > Output: tokens > `clang::Parser` > `clang::Sema` (semantic check) > Output: AST > `clang::CodeGen` -> Output: LLVM IR
	- ASTs:
		- Represents the source code in a faithful way
		- Almost immutable, so it's very hard to change after generated
		- Clang represents ASTs using Nodes with object oriented structure
			- Ex: Type -> QualType, BuiltInType, PointerType etc 
4) Learn basic Clang commands (TODO: Expand to complete examples with comments showing output)
	- Compile a trivial program:
	- Generate IR: -emit-llvm
	- Generate executable from IR: 
	- Optimize: -On, -Ofast 
	- Print tokens generated by lexer: -Xclang -dump-tokens -E
		- `-Xclang <args>` passes subsequent flags to clang compiler
		- `-c` runs all stages of compilation except linking
5) **MINI-PROJECT 1**: LLVM Kaleidoscope tutorial
	- Lexer: Finds tokens. This handles the logic for separating tokens so they 
6) **MINI-PROJECT 2**: Simpler version of Metareflect
	- Start with LibTooling docs, it seems pretty well done


# LLVM for dummies:

## WTF is LLVM?

LLVM is a modular compiler, consisting of an umbrella project with lots of subprojects. It allows you to reuse existing compiling tools, so if you want to modify your compiler toolchain it's a lot easier, as you only have to worry about the specific phases you want to modify.

Compiling a language with LLVM goes something like this:

1) Convert code to LLVM IR (intermediate representation)
2) Run multiple passes of optimization on the IR
	- All of those passes return IR code, but each time more optimized
3) Convert IR to platform-specific Assembly code

So, on summary:

Source code -> LLVM IR -> Optimized LLVM IR -> Assembly



## Why use LLVM instead of 'x' compiler? 

All compilers have a monolithic architecture that operates on specific constraints. This means that, for instance, if you need to compile C++ code for multiple hardwares, you would need a compiler for each of them. This is **really bad** because you can't reuse code between those implementations, so each time you need to add a new platform, you have to make a new compiler version from scratch and lose all the work you've done.

LLVM solves this problem by breaking the compilation process into multiples steps that you can mix and match, so if you need to modify/create a compiler, all you have to do is install LLVM and program the parts you want to modify

#### Example: Building a parser for a language

To be honest I don't know the specifics about that yet lol. But the things is, if you wanted to build a new language, you could simply use LLVM tools to generate LLVM IR for this language and not really care about all the rest, like optimizations and converting it to assembly code. This is really awesome and would cut you a lot of work.


## LLVM structure

The [LLVM repository's](https://github.com/llvm/llvm-project) structure is just a folder with multiple subfolders. Each of those is a separate project and possibly a module for a compilation pipeline. Some of the most used projects are:

- **clang**:
- **clang-extra-tools**:
- **llvm**: Core LLVM tools


# Setting up LLVM on Windows

## Installing LLVM and Clang

0) Clone LLVM repo

```
git clone https://github.com/llvm/llvm-project.git
```

1) Create build directory

```
mkdir build
cd build
```

2) Run CMake to generate project files

- -DLLVM_ENABLE_PROJECTS is used to specify which projects you want to build
	- Ex: `-DLLVM_ENABLE_PROJECTS=clang` would only build clang, which is actually fucking huge
- You should build in release mode unless you wanna debug the compiler itself.

```
cmake -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -G "Visual Studio 16 2019" -A x64 -Thost=x64 ..\llvm
```

3) Run CMake inside build folder to build the project

```
cmake --build .
```

4) Run CMake again from inside build folder to install
```
cmake --build . --target install
```

5) Add the binaries' directory to Path environment variable
- Should be something like path/to/build/Release/bin


## Integrating Clang with CMake/Visual Studio/VSCode

Clang is LLVM's compiler for C/C++. In order to make it work with CMake in Visual Studio, you must add Clang as a component from the Visual Studio installer. After the installation, it should work with both Visual Studio and VSCode. Note that **compiling LLVM manually is not enough to make it work with VS**, so if you want to use CMake's integration with VS, you actually have to use the installer, which is kinda bad imo.

## Embedding LLVM to your CMake project
- Follow [this tutorial](https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project).
- On Windows, this **only works if LLVM was built manually**.

# References

- [How LLVM works](https://www.youtube.com/watch?v=IR_L1xf4PrU&t=776s)
- [A Brief Introduction to LLVM](https://www.youtube.com/watch?v=a5-WaD8VV38) (more geared towards LLVM IR)
- [Core overview of Clang](https://www.youtube.com/watch?v=5kkMpJpIGYU)