# TODO
* **CLEANUP**
* Multithreading
* NTTP support (non-type template parameters)
* Allow `invoke()`ing functions with `requires` clauses
* ~~convert from `std::type_info` -> archimedes types and vice versa (allow lookups for `dynamic_cast` with `any`s)~~ **DONE**
* Check if types are serializable at compile time through `constexpr` arrays of registered `type_id`s
* ~~Get fields independent on class location in inheritance hierarchy~~ **NOT GONNA HAPPEN**
* `using enum ...;` support for string lookup
* `constexpr` as much of the library as possible
* Respect `using` declarations in the global namespace
* ~~Generate object files directly (instead of source, remove need for extra compiler invoke)~~ **DONE**
* Ad-hoc type checking for `any` types
* Redesign top-level interface around ranges (or something like them) to avoid heap allocations
* Template methods that don't act like template methods: see `test/requires.cpp`
* ~~Generate constructor invokers for all possible constructors (excluding templates) for reflected types~~ **DONE**

## Never-gonna-happen-s
|                                           | Reason
|-------------------------------------------|--------------------------------------------------------
| Invoke private methods                    | not supported by C++
| Values of `private` `constexpr` fields    | can't `decltype` or get values outside of class
| Anything in anonymous namespaces          | too complex to resolve type names, not supported by C++
| Inspecting non-specialized `template`s    | too complex, doesn't make a lot of sense at runtime?
