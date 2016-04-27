#ifndef SAUCE_INTERNAL_INJECTOR_BINDING_H_
#define SAUCE_INTERNAL_INJECTOR_BINDING_H_

#include <sauce/injector.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/key.h>
#include <sauce/memory.h>
#include <sauce/scopes.h>

namespace sauce {
namespace internal {

/**
 * An injection that provides the injector itself.
 */
class InjectorBinding: public Binding<Key<Injector>::Normalized, NoScope> {

  void validateAcyclic(InjectorPtr, TypeIds &) const {}

public:

  /**
   * Inject the injector.
   *
   * Just return the passed injector.
   */
  void inject(InjectorPtr & injected, BindingPtr, InjectorPtr injector) const {
    injected = injector;
  }

};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_INJECTOR_BINDING_H_
