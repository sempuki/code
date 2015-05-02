#ifndef SRC_CORE_LOGGING_HPP_
#define SRC_CORE_LOGGING_HPP_

#ifdef __GNUC__  // clang supports this header
#include <cxxabi.h>

inline std::string demangle(const std::string &name) {
  size_t size = 1024;  // we *don't* want realloc() to be called
  char buffer[size];  // NOLINT

  int status = 0;
  abi::__cxa_demangle(name.c_str(), buffer, &size, &status);
  assert(status == 0 && "Demanging failed");

  return buffer;
}
#else
inline std::string demangle(const std::string &name) {
  return name;
}
#endif  // __GNUC__


// Only supports maximum 8 arguments
// Zero arguments are also *not* supported

// recurse down from given argument count
#define PRINT_ARG_1(argument) #argument ": " << argument
#define PRINT_ARG_2(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_1(__VA_ARGS__)
#define PRINT_ARG_3(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_2(__VA_ARGS__)
#define PRINT_ARG_4(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_3(__VA_ARGS__)
#define PRINT_ARG_5(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_4(__VA_ARGS__)
#define PRINT_ARG_6(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_5(__VA_ARGS__)
#define PRINT_ARG_7(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_6(__VA_ARGS__)
#define PRINT_ARG_8(argument, ...) #argument ": " << argument << ", " << PRINT_ARG_7(__VA_ARGS__)

// varargs displaces count so that N aligns with the correct size
#define COUNT_ARGS(...) COUNT_ARGS_(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define COUNT_ARGS_(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N

// magic to make nested macro evaluations complete
#define CONCAT(lhs, rhs) CONCAT1(lhs, rhs)
#define CONCAT1(lhs, rhs) CONCAT2(lhs, rhs)
#define CONCAT2(lhs, rhs) lhs##rhs

// print all args with name label and output stream operator
#define PRINT_ARGS(...) CONCAT(PRINT_ARG_, COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)


// Uniform logging macros (assumes the loging object is called "logger")

#define LOG_METHOD_CALL()\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARG_1(this) \
      )

#define LOG_METHOD_CALL_(LEVEL)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARG_1(this) \
      )

#define LOG_METHOD_ARGS(...)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARGS(this, __VA_ARGS__) \
      )

#define LOG_METHOD_ARGS_(LEVEL, ...)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARGS(this, __VA_ARGS__) \
      )

#define LOG_METHOD_MESG(message)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARG_1(this) \
      )

#define LOG_METHOD_MESG_(LEVEL, message)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARG_1(this) \
      )

#define LOG_METHOD_FULL(message, ...)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARGS(this, __VA_ARGS__) \
      )

#define LOG_METHOD_FULL_(LEVEL, message, ...)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARGS(this, __VA_ARGS__) \
      )


#define LOG_CALL()\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      )

#define LOG_CALL_(LEVEL)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      )

#define LOG_ARGS(...)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARGS(__VA_ARGS__) \
      )

#define LOG_ARGS_(LEVEL, ...)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << PRINT_ARGS(__VA_ARGS__) \
      )

#define LOG_MESG(message)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      )

#define LOG_MESG_(LEVEL, message)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      )

#define LOG_FULL(message, ...)\
  LOG4CXX_DEBUG(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARGS(__VA_ARGS__) \
      )

#define LOG_FULL_(LEVEL, message, ...)\
  LOG4CXX_ ## LEVEL(logger, \
      __FUNCTION__ << '[' << __LINE__ << ']' \
      << ": " << message \
      << ", " << PRINT_ARGS(__VA_ARGS__) \
      )

#define LOG(message, ...)\
  LOG4CXX_DEBUG(logger, \
      message << ": " << PRINT_ARGS(__VA_ARGS__) \
      )

#define LOG_(LEVEL, message, ...)\
  LOG4CXX_ ## LEVEL(logger, \
      message << ": " << PRINT_ARGS(__VA_ARGS__) \
      )

#endif
