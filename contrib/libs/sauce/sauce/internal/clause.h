#ifndef SAUCE_CLAUSE_H_
#define SAUCE_CLAUSE_H_

#include <string>
#include <vector>

#include <sauce/exceptions.h>
#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/internal/bindings.h>
#include <sauce/internal/instance_binding.h>
#include <sauce/internal/method_binding.h>
#include <sauce/internal/new_binding.h>
#include <sauce/internal/opaque_binding.h>
#include <sauce/internal/pending_thrower.h>
#include <sauce/internal/provider_binding.h>

namespace sauce {
namespace internal {

class ImplicitBindings;

/**
 * The accumulated state passed between Clauses that ultimately results in a new Binding.
 */
class ClauseState {
  Bindings<ImplicitBindings> & bindings;
  PendingThrower & pendingThrower;
  OpaqueBindingPtr pendingBinding;
  OpaqueBindingPtr providerBinding;
  std::string dynamicName;
  std::vector<std::string> dynamicDependencyNames;

public:

  ClauseState(Bindings<ImplicitBindings> & bindings, PendingThrower & pendingThrower):
    bindings(bindings),
    pendingThrower(pendingThrower),
    pendingBinding(),
    providerBinding(),
    dynamicName(unnamed()),
    dynamicDependencyNames() {
    pendingThrower.throwAnyPending();
  }

  virtual ~ClauseState() {
    if (pendingBinding.get() == NULL) {
      return;
    }

    pendingBinding->setName(dynamicName);
    pendingBinding->setDynamicDependencyNames(dynamicDependencyNames);

    if (pendingBinding.get() != NULL) {
      bindings.put(pendingBinding);
    }

    if (providerBinding.get() != NULL) {
      bindings.put(providerBinding);
    }
  }

  void bind(OpaqueBindingPtr pendingBinding) {
    this->pendingBinding = pendingBinding;
  }

  void bindProvider(OpaqueBindingPtr providerBinding) {
    this->providerBinding = providerBinding;
  }

  void setDynamicName(std::string const name) {
    this->dynamicName = name;
  }

  void bindDynamicDependencyName(unsigned int position, std::string const name) {
    if (dynamicDependencyNames.size() <= position) {
      dynamicDependencyNames.resize(position + 1, unnamed());
    }
    dynamicDependencyNames[position] = name;
  }

  template<typename Exception>
  void throwLater() {
    pendingThrower.template throwLater<Exception>();
  }

  void clearException() {
    pendingThrower.clear();
  }
};

typedef sauce::shared_ptr<ClauseState> ClauseStatePtr;

/**
 * Base class for parts of the fluent binding API.
 */
template<typename Dependency>
class Clause {
  ClauseStatePtr state;

protected:

  virtual void onComplete() {
    throwLater(PartialBindingExceptionFor<Dependency>());
  }

  Clause():
    state() {}

  Clause(ClauseStatePtr state):
    state(state) {
    throwLater(PartialBindingExceptionFor<Dependency>());
  }

  ClauseStatePtr getState() {
    return state;
  }

  template<typename Next>
  Next pass(Next next) {
    next.setState(state);
    return next;
  }

  void setDynamicName(std::string const name) {
    state->setDynamicName(name);
  }

  template<typename Exception>
  void throwLater(Exception) {
    state->template throwLater<Exception>();
  }

  void bindDynamicDependencyName(unsigned int position, std::string const name) {
    state->bindDynamicDependencyName(position, name);
  }

public:

  virtual ~Clause() {}

  Clause<Dependency> & naming(unsigned int position, std::string const name) {
    this->bindDynamicDependencyName(position, name);
    return *this;
  }

  void setState(ClauseStatePtr state) {
    this->state = state;
    getState()->clearException();
    onComplete();
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_CLAUSE_H_
