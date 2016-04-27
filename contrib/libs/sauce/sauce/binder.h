#ifndef SAUCE_BINDER_H_
#define SAUCE_BINDER_H_

#include <memory>
#include <string>
#include <utility>

#include <sauce/named.h>
#include <sauce/provider.h>
#include <sauce/scopes.h>
#include <sauce/internal/bindings.h>
#include <sauce/internal/clause.h>
#include <sauce/internal/implicit_provider_binding.h>
#include <sauce/internal/injector_binding.h>
#include <sauce/internal/key.h>
#include <sauce/internal/new_binding.h>
#include <sauce/internal/opaque_binding.h>
#include <sauce/internal/pending_thrower.h>
#include <sauce/internal/provider_binding.h>

namespace sauce {

/**
 * Binds to a specific constructor and allocator.
 */
template<typename Dependency, typename Scope, typename Ctor, typename Allocator>
class AllocateFromClause: public i::Clause<Dependency> {
  virtual void onComplete() {
    i::OpaqueBindingPtr pendingBinding(new i::NewBinding<Dependency, Scope, Ctor, Allocator>());
    this->getState()->bind(pendingBinding);
  }
};

/**
 * Binds to a specific constructor, allocating from the heap.
 */
template<typename Dependency, typename Scope, typename Ctor>
class ToClause: public i::Clause<Dependency> {
  typedef typename i::Key<Dependency>::Iface Iface;

  virtual void onComplete() {
    i::OpaqueBindingPtr pendingBinding(new i::NewBinding<Dependency, Scope, Ctor, std::allocator<int> >());
    this->getState()->bind(pendingBinding);
  }

public:

  template<typename Allocator>
  AllocateFromClause<Dependency, Scope, Ctor, Allocator> allocatedFrom() {
    return this->pass(AllocateFromClause<Dependency, Scope, Ctor, Allocator>());
  }
};

/**
 * Binds to a provider with a specific constructor, allocating from the heap.
 */
template<typename ProviderDependency, typename Scope, typename ProviderCtor>
class ToProviderClause: public i::Clause<ProviderDependency> {
  void onComplete() {
    typedef typename i::Key<ProviderDependency>::Iface Provider;
    typedef typename Provider::Provides Iface;
    typedef typename i::Key<ProviderDependency>::Name Name;
    typedef Named<Iface, Name> ProvidedDependency;

    i::OpaqueBindingPtr pendingBinding(new i::NewBinding<ProviderDependency, Scope, ProviderCtor, std::allocator<int> >());
    this->getState()->bind(pendingBinding);

    i::OpaqueBindingPtr providerBinding(new i::ProviderBinding<ProvidedDependency, Scope, ProviderDependency>());
    this->getState()->bindProvider(providerBinding);
  }

public:

  template<typename Allocator>
  AllocateFromClause<ProviderDependency, Scope, ProviderCtor, Allocator> allocatedFrom() {
    return this->pass(AllocateFromClause<ProviderDependency, Scope, ProviderCtor, Allocator>());
  }
};

/**
 * Scopes the binding.
 */
template<typename Dependency, typename Scope>
class InClause: public i::Clause<Dependency> {
  typedef typename i::Key<Dependency>::Iface Iface;
  typedef typename i::Key<Dependency>::Name Name;
  typedef Named<Provider<Iface>, Name> ProviderDependency;

public:

  template<typename Ctor>
  ToClause<Dependency, Scope, Ctor> to() {
    return this->pass(ToClause<Dependency, Scope, Ctor>());
  }

  template<typename ProviderCtor>
  ToProviderClause<ProviderDependency, Scope, ProviderCtor> toProvider() {
    return this->pass(ToProviderClause<ProviderDependency, Scope, ProviderCtor>());
  }
};

template<typename Dependency>
class NamedClause;

template<typename Iface>
class BindClause;

/**
 * Binds to a specific method.
 */
template<typename Dependency, typename Signature>
class ToMethodClause: public i::Clause<Dependency> {
  typedef typename i::MethodBinding<Dependency, Signature> MethodBinding_;
  typedef typename MethodBinding_::Method Method;
  Method method;

  friend class NamedClause<Dependency>;
  friend class BindClause<typename i::Key<Dependency>::Iface>;

  void onComplete() {
    i::OpaqueBindingPtr methodBinding(new MethodBinding_(method));
    this->getState()->bind(methodBinding);
  }

  ToMethodClause(Method method):
    i::Clause<Dependency>(),
    method(method) {}
};

/**
 * Binds to a specific method with possible static dependency names.
 */
template<typename Dependency, typename Signature>
class ToMethodNamingClause: public i::Clause<Dependency> {
  typedef typename i::MethodBinding<Dependency, Signature> MethodBinding_;
  typedef typename MethodBinding_::Method Method;
  Method method;

  friend class NamedClause<Dependency>;
  friend class BindClause<typename i::Key<Dependency>::Iface>;

  void onComplete() {
    i::OpaqueBindingPtr methodBinding(new MethodBinding_(method));
    this->getState()->bind(methodBinding);
  }

  ToMethodNamingClause(Method method):
    i::Clause<Dependency>(),
    method(method) {}
};

/**
 * Binds to a specific single instance.
 *
 * By virtue of always returning the passed instance, this dependency is inheretingly singleton scoped.  It also means
 * there is no need to use the actual singleton scope cache, and so we actually declare it as NoScope here.
 *
 * The constructor template parameter is unused, but a default constructor for the interface is assumed.
 */
template<typename Dependency>
class ToInstanceClause: public i::Clause<Dependency> {
  typedef typename i::InstanceBinding<Dependency> InstanceBinding_;
  typedef typename i::Key<Dependency>::Ptr IfacePtr;
  IfacePtr iface;

  void onComplete() {
    i::OpaqueBindingPtr instanceBinding(new InstanceBinding_(iface));
    this->getState()->bind(instanceBinding);
  }

public:

  ToInstanceClause(IfacePtr iface):
    i::Clause<Dependency>(),
    iface(iface) {}
};

/**
 * Names the binding.
 *
 * There are two kinds of names: static and dynamic.  Static names are given by template parameter
 * type tags.  Dynamic ones are string arguments.  Each binding can use only one kind, but bindings
 * using both kinds can be mixed in the same module.
 */
template<typename Dependency>
class NamedClause: public i::Clause<Dependency> {
  typedef typename i::Key<Dependency>::Iface Iface;
  typedef typename i::Key<Dependency>::Ptr IfacePtr;
  typedef typename i::Key<Dependency>::Name Name;
  typedef Named<Provider<Iface>, Name> ProviderDependency;

public:

  ToInstanceClause<Dependency> toInstance(IfacePtr iface) {
    return this->pass(ToInstanceClause<Dependency>(iface));
  }

  template<typename Method>
  ToMethodClause<Dependency, Method> toMethod(Method method) {
    return this->pass(ToMethodClause<Dependency, Method>(method));
  }

  template<typename Signature>
  ToMethodNamingClause<Dependency, Signature> toMethodNaming(
    typename ToMethodNamingClause<Dependency, Signature>::Method method) {
    return this->pass(ToMethodNamingClause<Dependency, Signature>(method));
  }

  template<typename SetDependency>
  ToMethodNamingClause<Dependency, void(Iface::*) (SetDependency)> setting(
    typename ToMethodNamingClause<Dependency, void(Iface::*) (SetDependency)>::Method method,
    std::string name = unnamed()) {
    ToMethodNamingClause<Dependency, void(Iface::*) (SetDependency)> toMethodNamingClause(method);
    toMethodNamingClause = this->pass(toMethodNamingClause);
    toMethodNamingClause.bindDynamicDependencyName(0, name);
    return toMethodNamingClause;
  }

  template<typename Iface, typename Name>
  ToMethodNamingClause<Dependency, void(Iface::*) (Named<Iface, Name>)> setting(
    typename ToMethodNamingClause<Dependency, void(Iface::*) (Named<Iface, Name>)>::Method method) {
    return this->pass(ToMethodNamingClause<Dependency, void(Iface::*) (Named<Iface, Name>)>(method));
  }

  template<typename Scope>
  InClause<Dependency, Scope> in() {
    return this->pass(InClause<Dependency, Scope>());
  }

  template<typename Ctor>
  ToClause<Dependency, NoScope, Ctor> to() {
    return this->pass(ToClause<Dependency, NoScope, Ctor>());
  }

  template<typename ProviderCtor>
  ToProviderClause<ProviderDependency, NoScope, ProviderCtor> toProvider() {
    return this->pass(ToProviderClause<ProviderDependency, NoScope, ProviderCtor>());
  }
};

class Binder;

/**
 * A builder that creates a single binding.
 */
template<typename Iface>
class BindClause: public i::Clause<Named<Iface, Unnamed> > {
  typedef typename i::Key<Iface>::Ptr IfacePtr;
  typedef Named<Provider<Iface>, Unnamed> ProviderDependency;

  BindClause(i::ClauseStatePtr state):
    i::Clause<Named<Iface, Unnamed> >(state) {}

  friend class Binder;

public:

  template<typename Name>
  NamedClause<Named<Iface, Name> > named() {
    return this->pass(NamedClause<Named<Iface, Name> >());
  }

  NamedClause<Named<Iface, Unnamed> > named(std::string const name) {
    this->setDynamicName(name);
    return this->pass(NamedClause<Named<Iface, Unnamed> >());
  }

  ToInstanceClause<Named<Iface, Unnamed> > toInstance(IfacePtr iface) {
    return this->pass(ToInstanceClause<Named<Iface, Unnamed> >(iface));
  }

  template<typename Method>
  ToMethodClause<Named<Iface, Unnamed>, Method> toMethod(Method method) {
    return this->pass(ToMethodClause<Named<Iface, Unnamed>, Method>(method));
  }

  template<typename Signature>
  ToMethodNamingClause<Named<Iface, Unnamed>, Signature> toMethodNaming(
    typename ToMethodNamingClause<Named<Iface, Unnamed>, Signature>::Method method) {
    return this->pass(ToMethodNamingClause<Named<Iface, Unnamed>, Signature>(method));
  }

  template<typename SetDependency>
  ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (SetDependency)> setting(
    typename ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (SetDependency)>::Method method,
    std::string name = unnamed()) {
    ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (SetDependency)> toMethodNamingClause(method);
    toMethodNamingClause = this->pass(toMethodNamingClause);
    toMethodNamingClause.bindDynamicDependencyName(0, name);
    return toMethodNamingClause;
  }

  template<typename SetIface, typename Name>
  ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (Named<SetIface, Name>)> setting(
    typename ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (Named<SetIface, Name>)>::Method method) {
    return this->pass(ToMethodNamingClause<Named<Iface, Unnamed>, void(Iface::*) (Named<SetIface, Name>)>(method));
  }

  template<typename Scope>
  InClause<Named<Iface, Unnamed>, Scope> in() {
    return this->pass(InClause<Named<Iface, Unnamed>, Scope>());
  }

  template<typename Ctor>
  ToClause<Named<Iface, Unnamed>, NoScope, Ctor> to() {
    return this->pass(ToClause<Named<Iface, Unnamed>, NoScope, Ctor>());
  }

  template<typename ProviderCtor>
  ToProviderClause<ProviderDependency, NoScope, ProviderCtor> toProvider() {
    return this->pass(ToProviderClause<ProviderDependency, NoScope, ProviderCtor>());
  }
};

class Modules;

namespace internal {
class ImplicitBindings;
}

/**
 * Passed to modules to create bindings.
 */
class Binder: public i::PendingThrower {
  i::Bindings<i::ImplicitBindings> & bindings;

  friend class Modules;

  Binder(i::Bindings<i::ImplicitBindings> & bindings):
    PendingThrower(),
    bindings(bindings) {}

public:

  /**
   * Begin binding the chosen interface.
   */
  template<typename Iface>
  BindClause<Iface> bind() {
    i::ClauseStatePtr clauseState(new i::ClauseState(bindings, *this));
    BindClause<Iface> bindClause(clauseState);
    return bindClause;
  }
};

}

#endif // SAUCE_BINDER_H_
