#ifndef SAUCE_INTERNAL_IMPLICIT_PROVIDER_BINDING_H_
#define SAUCE_INTERNAL_IMPLICIT_PROVIDER_BINDING_H_

#include <cassert>

#include <sauce/injector.h>
#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/internal/resolved_binding.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/key.h>

namespace sauce {
namespace internal {

template<typename Dependency, typename Name>
class ImplicitProviderBinding;

/**
 * A Provider for an explicitly-bound Dependency.
 */
template<typename Dependency, typename Name>
class ImplicitProvider: public Provider<Dependency> {
  typedef typename Key<Dependency>::Normalized Normalized;
  typedef typename ResolvedBinding<Normalized>::BindingPtr BindingPtr;

  BindingPtr binding;
  InjectorPtr injector;

  friend class ImplicitProviderBinding<Dependency, Name>;

  ImplicitProvider(BindingPtr binding, InjectorPtr injector):
    Provider<Dependency>(),
    binding(binding),
    injector(injector) {}

public:

  typename Key<Dependency>::Ptr get() {
    typename Key<Dependency>::Ptr injected;
    binding->get(injected, binding, injector);
    return injected;
  }
};

/**
 * An injection that provides Providers for an already-bound dependency.
 */
template<typename BoundDependency, typename Name>
class ImplicitProviderBinding: public Binding<Named<Provider<BoundDependency>, Name>, NoScope> {

  typedef typename Key<BoundDependency>::Normalized Normalized;
  typedef typename ResolvedBinding<Normalized>::BindingPtr ProvidedBindingPtr;
  typedef Named<Provider<BoundDependency>, Name> ProviderDependency;
  typedef typename Key<ProviderDependency>::Ptr ProviderPtr;

  ProvidedBindingPtr providedBinding;

  void validateAcyclic(InjectorPtr, TypeIds &) const {}

public:

  typedef typename ResolvedBinding<ProviderDependency>::BindingPtr BindingPtr;

  ImplicitProviderBinding(ProvidedBindingPtr providedBinding):
    providedBinding(providedBinding) {}

  void inject(ProviderPtr & injected, BindingPtr, InjectorPtr injector) const {
    injected.reset(new ImplicitProvider<BoundDependency, Name>(providedBinding, injector));
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_IMPLICIT_PROVIDER_BINDING_H_
