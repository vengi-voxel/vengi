#ifndef SAUCE_EXCEPTIONS_H_
#define SAUCE_EXCEPTIONS_H_

#include <string>
#include <stdexcept>

namespace sauce {

/**
 * Base class for all sauce exceptions.
 */
class Exception: public std::runtime_error {
public:
  Exception(std::string message):
    std::runtime_error(message) {}
};

/**
 * Thrown when a binding cannot be found.
 */
class UnboundException: public Exception {
public:
  UnboundException(std::string const name):
    Exception("Request for unbound interface " + name + ".") {}
};

/**
 * Thrown when a binding cannot be found for the given interface.
 */
template<typename Dependency>
class UnboundExceptionFor: public UnboundException {
public:
  UnboundExceptionFor(std::string const name): UnboundException(name) {}
};

/**
 * Thrown when a binding hasn't been completely specified.
 */
class PartialBindingException: public Exception {
public:
  PartialBindingException():
    Exception("Binding is incomplete.") {}
};

/**
 * Thrown when a binding hasn't been completely specified for the given interface.
 */
template<typename Dependency>
class PartialBindingExceptionFor: public PartialBindingException {
public:
  PartialBindingExceptionFor(): PartialBindingException() {}
};

/**
 * Thrown when a dependency cycle is found.
 */
class CircularDependencyException: public Exception {
public:
  CircularDependencyException():
    Exception("Request for unbound interface.") {}
};

/**
 * Thrown when a dependency cycle is found for the given interface.
 */
template<typename Dependency>
class CircularDependencyExceptionFor: public CircularDependencyException {
public:
  CircularDependencyExceptionFor(): CircularDependencyException() {}
};

/**
 * Thrown when a provision is requested outside of its bound scope.
 */
class OutOfScopeException: public Exception {
public:
  OutOfScopeException():
    Exception("Out of dependency scope.") {}
};

/**
 * Thrown when a provision is requested outside of its given, bound scope.
 */
template<typename Scope>
class OutOfScopeExceptionFor: public OutOfScopeException {
public:
  OutOfScopeExceptionFor(): OutOfScopeException() {}
};

/**
 * Thrown when re-entering a scope that is already open.
 */
class AlreadyInScopeException: public Exception {
public:
  AlreadyInScopeException():
    Exception("Already in scope.") {}
};

/**
 * Thrown when re-entering the given scope, which is already open.
 */
template<typename Scope>
class AlreadyInScopeExceptionFor: public AlreadyInScopeException {
public:
  AlreadyInScopeExceptionFor(): AlreadyInScopeException() {}
};

/**
 * Thrown when exiting the singleton scope.
 */
class ExitingSingletonScopeException: public Exception {
public:
  ExitingSingletonScopeException():
    Exception("Can't exit SingletonScope") {}
};

}

#endif // SAUCE_EXCEPTIONS_H_
