# archimedes - C++ reflection via code generation

<p align="center">
<img src="https://github.com/jdah/archimedes/raw/master/images/0.png" width=60% height=60%>
</p>

### WARNING: this is ALPHA software. the code is highly unstable hot garbage. use at your own risk!

## Features
* `template`s **are** supported - though only those which are naturally instantiated (ised) by the program itself
* Reflected `class`/`struct` types have:
	* `template` parameter names, types, and values
	* `typedef` and `using` values (i.e. `struct Foo { using F = int; /* ... */}`)
	* base classes
	* class memory layout including `virtual` bases, vtable pointers, etc.
	* fields and get/set values on live objects
	* static fields and their values
	* functions, member and static, which can be invoked on live objects
	* constructors (and destructors) which can be invoked on raw memory to (de)initialize objects
	* various type traits (`is_abstract/is_polymorphic/is_pod/...`)
	* inspect `private` members
* Reflected `enum` types have:
	* `std::string` to value conversion
	* value to `std::string` conversion
	* underlying type information
* Reflected functions:
	* can be `invoke(...)`'d with arbitrary arguments
	* list parameters; with types, names, and qualifiers

## Example
The shortest demo which covers the most features (see `test/example.test.{cpp, hpp}`)

#### TODO

## How does it work?
archimedes works via a clang plugin which runs on already compiled/template-instantiated ASTs.
It traverses ASTs and gathers type information, serializing it and emitting it in an object file emitted alongside the actual compiler output.
These object files can then be linked into any normal executable (or library) and their data loaded via `archimedes::load()` on program startup.
In order to not bloat compile times, the majority of information (that which doesn't rely on function pointers, constexpr values, etc.) is
serialized and embedded into the program via simple byte arrays which are deserialized at runtime.

## Usage
Include `include/archimedes.hpp` for full access, and `include/archimedes/*.hpp` for submodules (`any`, `type_id`, etc.)
See `Makefile` for complex usage.

```
$ clang++
	-o main.o
	-fplugin=<path_to_argimedes>
	-fplugin-arg-archimedes-header-include/archimedes.hpp 	# path to archimedes header
	-fplugin-arg-archimedes-exclude-ns-std 			# use "exclude-ns-<regex>" to exclude namespaces from reflection
	-fplugin-arg-archimedes-file-main.cpp 			# list files in which types should be reflected
	-fplugin-arg-archimedes-file-main.hpp
	-fplugin-arg-archimedes-out-main.types.o 		# reflection output object file
	-c
	main.cpp
$ clang++ -o main main.o main.types.o 				# link *.o and *.types.o to include reflection information
```

## Building
`$ make plugin static shared`

## TODO: Developing
