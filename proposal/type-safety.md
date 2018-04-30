Type Safety

Types always maintain their invariants, and all operations have valid results.
  Types are well defined in their base; and remain well defined after K operations. 

* Disallow type unsafe C constructs that have type safe C++ alternatives
  1. `enum class` instead of `enum`
  1. inline functions instead of macros
  1. inline variables instead of defines
  1. `std::variant` instead of `union`
  1. templates instead of `void` pointer
  1. variadic templates instead of varargs
* Disallow all sources of *implicit* conversion
  1. Do not implicitly generate constructors / assignment / conversion operators
  1. All constructors / conversion operators are considered `explicit`
* Distinguish between safe and unsafe casts
  * Introduce `safe_cast`: like static_cast but only allowing type safe conversions
    1. type promotion (non-narrowing; ie. uint32_t -> int64_t)
    1. base pointer/reference cast (ie. Derived* -> Base*)
    1. `explicit` conversion (invoking `explicit` conversion constructor / operator)
  * Introduce `narrow_cast`: narrowing cast with runtime check
    1. Integer values are unchanged (ie. assert `((int64_t)42) == ((uint32_t)42)`)
    1. Allow `double` -> `float`
* Delegate low-level operations to trusted library types
  1. Disallow pointer arithmetic
  1. Disallow bitshifting
* Make it harder to define types wrong
  1. Non-final class destructors are considered `virtual`
  1. Methods in derived class with same name as base must be declared `override`
  1. Error if `noexcept` method can be proven to throw 
    * Roughly same rules as `const` methods
      1. Disallow throw statements in `noexcept`
      1. Only allow calling other `noexcept` functions
      1. Assume C functions do not throw
  1. Assignment operator is suppressed on r-values
* Make it harder to use types wrong
  1. All values are initialized
    * `T value;` is considered to be `T value{};`
  1. Disallow shadowing
    * Shadowed variables are an error
    * Overload functions that differ only by default variable are an error
  1. Warnings should be produced on
    * Accessing moved-from variables
