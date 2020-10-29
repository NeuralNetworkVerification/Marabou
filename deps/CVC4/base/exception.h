/*********************                                                        */
/*! \file exception.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Morgan Deters, Tim King, Andres Noetzli
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief CVC4's exception base class and some associated utilities
 **
 ** CVC4's exception base class and some associated utilities.
 **/

#include "cvc4_public.h"

#ifndef CVC4__EXCEPTION_H
#define CVC4__EXCEPTION_H

#include <cstdarg>
#include <cstdlib>
#include <exception>
#include <iosfwd>
#include <sstream>
#include <stdexcept>
#include <string>

namespace CVC4 {

class CVC4_PUBLIC Exception : public std::exception {
 protected:
  std::string d_msg;

 public:
  // Constructors
  Exception() : d_msg("Unknown exception") {}
  Exception(const std::string& msg) : d_msg(msg) {}
  Exception(const char* msg) : d_msg(msg) {}

  // Destructor
  virtual ~Exception() {}

  // NON-VIRTUAL METHOD for setting and printing the error message
  void setMessage(const std::string& msg) { d_msg = msg; }
  std::string getMessage() const { return d_msg; }

  // overridden from base class std::exception
  const char* what() const noexcept override { return d_msg.c_str(); }

  /**
   * Get this exception as a string.  Note that
   *   cout << ex.toString();
   * is subtly different from
   *   cout << ex;
   * which is equivalent to
   *   ex.toStream(cout);
   * That is because with the latter two, the output language (and
   * other preferences) for exprs on the stream is respected.  In
   * toString(), there is no stream, so the parameters are default
   * and you'll get exprs and types printed using the AST language.
   */
  std::string toString() const
  {
    std::stringstream ss;
    toStream(ss);
    return ss.str();
  }

  /**
   * Printing: feel free to redefine toStream().  When overridden in
   * a derived class, it's recommended that this method print the
   * type of exception before the actual message.
   */
  virtual void toStream(std::ostream& os) const { os << d_msg; }

};/* class Exception */

class CVC4_PUBLIC IllegalArgumentException : public Exception {
protected:
  IllegalArgumentException() : Exception() {}

  void construct(const char* header, const char* extra,
                 const char* function, const char* tail);

  void construct(const char* header, const char* extra,
                 const char* function);

  static std::string format_extra(const char* condStr, const char* argDesc);

  static const char* s_header;

public:

  IllegalArgumentException(const char* condStr, const char* argDesc,
                           const char* function, const char* tail) :
    Exception() {
    construct(s_header, format_extra(condStr, argDesc).c_str(), function, tail);
  }

  IllegalArgumentException(const char* condStr, const char* argDesc,
                           const char* function) :
    Exception() {
    construct(s_header, format_extra(condStr, argDesc).c_str(), function);
  }

  /**
   * This is a convenience function for building usages that are variadic.
   *
   * Having IllegalArgumentException itself be variadic is problematic for
   * making sure calls to IllegalArgumentException clean up memory.
   */
  static std::string formatVariadic();
  static std::string formatVariadic(const char* format, ...);
};/* class IllegalArgumentException */

inline std::ostream& operator<<(std::ostream& os,
                                const Exception& e) CVC4_PUBLIC;
inline std::ostream& operator<<(std::ostream& os, const Exception& e)
{
  e.toStream(os);
  return os;
}

template <class T> inline void CheckArgument(bool cond, const T& arg,
                                             const char* tail) CVC4_PUBLIC;
template <class T> inline void CheckArgument(bool cond, const T& arg CVC4_UNUSED,
                                             const char* tail CVC4_UNUSED) {
  if(__builtin_expect( ( !cond ), false )) { \
    throw ::CVC4::IllegalArgumentException("", "", tail); \
  } \
}
template <class T> inline void CheckArgument(bool cond, const T& arg)
  CVC4_PUBLIC;
template <class T> inline void CheckArgument(bool cond, const T& arg CVC4_UNUSED) {
  if(__builtin_expect( ( !cond ), false )) { \
    throw ::CVC4::IllegalArgumentException("", "", ""); \
  } \
}

class CVC4_PUBLIC LastExceptionBuffer {
public:
  LastExceptionBuffer();
  ~LastExceptionBuffer();

  void setContents(const char* string);
  const char* getContents() const { return d_contents; }

  static LastExceptionBuffer* getCurrent() { return s_currentBuffer; }
  static void setCurrent(LastExceptionBuffer* buffer) { s_currentBuffer = buffer; }

  static const char* currentContents() {
    return (getCurrent() == NULL) ? NULL : getCurrent()->getContents();
  }

private:
  /* Disallow copies */
  LastExceptionBuffer(const LastExceptionBuffer&) = delete;
  LastExceptionBuffer& operator=(const LastExceptionBuffer&) = delete;

  char* d_contents;

  static thread_local LastExceptionBuffer* s_currentBuffer;
}; /* class LastExceptionBuffer */

}/* CVC4 namespace */

#endif /* CVC4__EXCEPTION_H */
