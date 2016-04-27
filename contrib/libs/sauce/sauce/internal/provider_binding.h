#ifndef SAUCE_INTERNAL_PROVIDER_BINDING_H_
#define SAUCE_INTERNAL_PROVIDER_BINDING_H_

#include <cassert>

#include <sauce/injector.h>
#include <sauce/memory.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/key.h>

namespace sauce {
namespace internal {

/**
 * An injection that provides from the configured provider.
 */
template<typename Dependency, typename Scope, typename Provider>
class ProviderBinding: public Binding<Dependency, Scope> {
public:
  typedef typename ResolvedBinding<Dependency>::BindingPtr BindingPtr;

  void validateAcyclic(InjectorPtr injector, TypeIds & ids) const {
    this->template validateAcyclicHelper<Provider>(injector, ids, this->getName());
  }

  void inject(typename Key<Dependency>::Ptr & injected, BindingPtr binding, InjectorPtr injector) const {
    typename Key<Provider>::Ptr provider;
    this->template injectHelper<Provider>(provider, injector, binding->getName());
    injected = provider->get();
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_PROVIDER_BINDING_H_
