#ifndef SAUCE_INTERNAL_BINDINGS_H_
#define SAUCE_INTERNAL_BINDINGS_H_

#include <cassert>
#include <map>
#include <utility>
#include <vector>

#include <sauce/exceptions.h>
#include <sauce/memory.h>
#include <sauce/internal/opaque_binding.h>
#include <sauce/internal/resolved_binding.h>
#include <sauce/internal/key.h>
#include <sauce/internal/type_id.h>

namespace sauce {

class Injector;

namespace internal {

/**
 * Cast an OpaqueBindingPtr to a Binding smart pointer.
 *
 * This must be done carefully (with static_pointer_cast) in order to not lose the ref count.
 */
template<typename Dependency>
sauce::shared_ptr<ResolvedBinding<Dependency> > resolve(OpaqueBindingPtr binding) {
  assert((namedTypeIdOf<Dependency>(binding->getName())) == binding->getKey());
  return sauce::static_pointer_cast<ResolvedBinding<Dependency> >(binding);
}

/**
 * A container for bindings.
 *
 * Each Modules objects creates a Bindings, and passes a const copy to each Injector it creates.
 * The set of bindings an Injector uses is therefore read-only.
 *
 * When providing instances (eagerly and not) the injector is passed in and the dependency is
 * returned directly, instead of giving the binding to the injector (the binding details stay
 * hidden.)
 *
 * The template parameter is a strategy type that attempts to located unknown bindings dynamically
 * (implicitly.)  Due to the threat of circular dependencies (in Sauce's type space), Bindings
 * itself can't be aware of concrete Binding implementations.  The only time it needs to know
 * about them is when resolving implicit bindings.  So, this functionality is hidden behind the
 * templated strategy.
 */
template<typename ImplicitBindings>
class Bindings {
  typedef std::map<NamedTypeId, OpaqueBindingPtr> ProvidingBindingMap;
  typedef std::multimap<NamedTypeId, OpaqueBindingPtr> ModifyingBindingMap;
  typedef std::multimap<TypeId, OpaqueBindingPtr> ScopeMap;
  typedef sauce::shared_ptr<Injector> InjectorPtr;

  ProvidingBindingMap providingBindingMap;
  ModifyingBindingMap modifyingBindingMap;
  ScopeMap scopeMap;

public:

  Bindings():
    providingBindingMap(),
    modifyingBindingMap(),
    scopeMap() {}

  /**
   * Insert the given binding.
   */
  void put(OpaqueBindingPtr binding) {
    if (binding->isModifier()) {
      modifyingBindingMap.insert(std::make_pair(binding->getKey(), binding));
    } else {
      providingBindingMap.insert(std::make_pair(binding->getKey(), binding));
      TypeId scopeKey = binding->getScopeKey();
      scopeMap.insert(std::make_pair(scopeKey, binding));
    }
  }

  template<typename Dependency>
  sauce::shared_ptr<ResolvedBinding<Dependency> > getProvidingBinding(std::string const name) const {
    sauce::shared_ptr<ResolvedBinding<Dependency> > binding;

    ProvidingBindingMap::const_iterator i = providingBindingMap.find(namedTypeIdOf<Dependency>(name));
    if (i == providingBindingMap.end()) {
      ImplicitBindings implicitBindings;
      binding = implicitBindings.template getProviding<Dependency>(*this, name);
    } else {
      binding = resolve<Dependency>(i->second);
    }

    return binding;
  }

  template<typename Dependency>
  std::vector<sauce::shared_ptr<ResolvedBinding<Dependency> > > getModifierBindings(std::string const name) const {
    ImplicitBindings implicitBindings;
    std::vector<sauce::shared_ptr<ResolvedBinding<Dependency> > > bindings =
      implicitBindings.template getModifyings<Dependency>(*this, name);

    NamedTypeId bindingKey = namedTypeIdOf<Dependency>(name);
    ModifyingBindingMap::const_iterator i = modifyingBindingMap.lower_bound(bindingKey);
    ModifyingBindingMap::const_iterator end = modifyingBindingMap.upper_bound(bindingKey);

    for (; i != end; ++i) {
      OpaqueBindingPtr const & binding = i->second;
      bindings.push_back(resolve<Dependency>(binding));
    }

    return bindings;
  }

  template<typename Dependency>
  void validateAcyclic(bool validateProviding, InjectorPtr injector, TypeIds & ids, std::string const name) const {
    typedef sauce::shared_ptr<ResolvedBinding<Dependency> > BindingPtr;
    BindingPtr binding;

    if (validateProviding) {
      binding = getProvidingBinding<Dependency>(name);
      binding->validateAcyclic(injector, ids);
    }

    typedef std::vector<BindingPtr> BindingPtrs;
    BindingPtrs bindings = getModifierBindings<Dependency>(name);

    for (typename BindingPtrs::iterator i = bindings.begin(); i != bindings.end(); ++i) {
      binding = *i;
      binding->validateAcyclic(injector, ids);
    }
  }

  /**
   * Inject the named Dependency.
   *
   * If no binding is found, the implicit bindings are checked.
   */
  template<typename Dependency>
  void get(typename Key<Dependency>::Ptr & injected, InjectorPtr injector, std::string const name) const {
    typedef sauce::shared_ptr<ResolvedBinding<Dependency> > BindingPtr;
    BindingPtr binding;

    if (injected.get() == NULL) {
      binding = getProvidingBinding<Dependency>(name);
      binding->get(injected, binding, injector);
    }

    typedef std::vector<BindingPtr> BindingPtrs;
    BindingPtrs bindings = getModifierBindings<Dependency>(name);

    for (typename BindingPtrs::iterator i = bindings.begin(); i != bindings.end(); ++i) {
      binding = *i;
      binding->get(injected, binding, injector);
    }
  }

  template<typename Scope>
  void eagerlyInject(InjectorPtr injector) const {
    TypeId scopeKey = typeIdOf<Scope>();
    ScopeMap::const_iterator i = scopeMap.lower_bound(scopeKey);
    ScopeMap::const_iterator end = scopeMap.upper_bound(scopeKey);

    for (; i != end; ++i) {
      OpaqueBindingPtr const & binding = i->second;
      binding->eagerlyInject(binding, injector);
    }
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_BINDINGS_H_
