#ifndef SAUCE_INTERNAL_INSTANCE_BINDING_H_
#define SAUCE_INTERNAL_INSTANCE_BINDING_H_

#include <sauce/injector.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/key.h>
#include <sauce/memory.h>
#include <sauce/scopes.h>

namespace sauce {
namespace internal {

/**
 * An injection that provides the value passed at construction.
 */
template<typename Dependency>
class InstanceBinding: public Binding<Dependency, NoScope> {
  typedef typename Key<Dependency>::Ptr IfacePtr;
  IfacePtr iface;

  void validateAcyclic(InjectorPtr, TypeIds &) const {}

public:

  typedef typename ResolvedBinding<Dependency>::BindingPtr BindingPtr;

  InstanceBinding(IfacePtr iface):
    Binding<Dependency, NoScope>(),
    iface(iface) {}

  /**
   * Inject the instance passed at construction.
   */
  void inject(IfacePtr & injected, BindingPtr, InjectorPtr) const {
    injected = iface;
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_INSTANCE_BINDING_H_
