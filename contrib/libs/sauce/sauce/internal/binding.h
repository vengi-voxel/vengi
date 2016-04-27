#ifndef SAUCE_INTERNAL_BINDING_H_
#define SAUCE_INTERNAL_BINDING_H_

#include <cassert>
#include <string>

#include <sauce/injector.h>
#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/internal/key.h>
#include <sauce/internal/opaque_binding.h>
#include <sauce/internal/resolved_binding.h>
#include <sauce/internal/type_id.h>

namespace sauce {
namespace internal {

/**
 * The base class of all actual binding implementations.
 */
template<typename Dependency_, typename Scope>
class Binding: public ResolvedBinding<Dependency_>, public InjectorFriend {
public:

  typedef typename Key<Dependency_>::Normalized Dependency;
  typedef typename Key<Dependency>::Ptr IfacePtr;
  typedef typename ResolvedBinding<Dependency>::BindingPtr BindingPtr;

private:

  std::string name;

  /**
   * The TypeId of the Scope template parameter.
   */
  TypeId getScopeKey() const {
    return typeIdOf<Scope>();
  }

  /**
   * Inject an Iface.
   *
   * The strategy used is left to derived types.
   */
  virtual void inject(IfacePtr &, BindingPtr, InjectorPtr) const = 0;

  /**
   * Inject an Iface.
   *
   * If a Scope is configured for the injection, this checks the scope cache first before calling
   * inject(), and caches the new Iface on miss.
   *
   * Modifying bindings (those which return true from isModifier()) are never scoped, since they don't provide the
   * would-be scoped value to begin with.
   *
   * Derived classes should not override get(), but rather inject().
   */
  void get(IfacePtr & injected, BindingPtr binding, InjectorPtr injector) const {
    bool unscoped = (getScopeKey() == typeIdOf<NoScope>()) || this->isModifier();

    if (unscoped || !probe<Dependency>(injector, injected, getScopeKey())) {
      // injected is not null if and only if we are a modifier.
      assert((injected.get() != NULL) == this->isModifier());

      inject(injected, binding, injector);
      if (!unscoped) {
        cache<Dependency>(injector, injected, getScopeKey());
      }
    }
  }

  /**
   * Inject, but do not return an Iface.
   *
   * Instead, cache the instance in its appropriate scope, if any.  If the injection is not scoped,
   * do nothing.
   */
  void eagerlyInject(OpaqueBindingPtr opaque, InjectorPtr injector) const {
    if (getScopeKey() != typeIdOf<NoScope>()) {
      BindingPtr binding = resolve<Dependency>(opaque);
      TypeIds ids;
      this->validateAcyclic(injector, ids);
      IfacePtr injected;
      get(injected, binding, injector);
    }
  }

  /**
   * Accept the list of dynamic dependency names this injection was created with.
   */
  virtual void setDynamicDependencyNames(std::vector<std::string> const &) {}

public:

  Binding():
    name(unnamed()) {}

  virtual ~Binding() {}

  /**
   * The dynamic name of this binding.
   */
  std::string getName() const {
    return name;
  }

  /**
   * Set the dynamic name of this binding.
   */
  void setName(std::string const name) {
    this->name = name;
  }

};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_BINDING_H_
