#ifndef SAUCE_INTERNAL_BASE_INJECTOR_H_
#define SAUCE_INTERNAL_BASE_INJECTOR_H_

#include <sauce/exceptions.h>
#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/provider.h>
#include <sauce/internal/bindings.h>
#include <sauce/internal/key.h>
#include <sauce/internal/locker_factory.h>
#include <sauce/internal/self_injector.h>
#include <sauce/internal/type_id.h>

namespace sauce {

class Injector;
class Modules;

namespace internal {

template<typename ImplicitBindings>
class BaseInjector;

/**
 * Detects circular dependencies on behalf of injectors.
 */
template<typename ImplicitBindings, typename Dependency>
class CircularDependencyGuard {
  friend class BaseInjector<ImplicitBindings>;

  TypeIds & ids;
  NamedTypeId id;

  CircularDependencyGuard(TypeIds & ids, std::string const name):
    ids(ids),
    id(namedTypeIdOf<Dependency>(name)) {
    TypeIds::iterator i = ids.find(id);
    if (i == ids.end()) {
      ids.insert(i, id);
    } else {
      throw CircularDependencyExceptionFor<Dependency>();
    }
  }

  ~CircularDependencyGuard() {
    ids.erase(id);
  }
};

template<typename ImplicitBindings>
class BaseInjector {
  typedef sauce::auto_ptr<LockFactory> LockFactoryPtr;
  typedef sauce::shared_ptr<Injector> InjectorPtr;

  Bindings<ImplicitBindings> const bindings;
  LockFactoryPtr lockFactory;

  friend class ::sauce::Modules;

  BaseInjector(Bindings<ImplicitBindings> const & bindings, LockFactoryPtr lockFactory):
    bindings(bindings),
    lockFactory(lockFactory) {}

public:

  template<typename Dependency>
  void validateAcyclic(bool validateProviding, InjectorPtr injector, TypeIds & ids, std::string const name) const {
    typedef typename Key<Dependency>::Normalized Normalized;
    CircularDependencyGuard<ImplicitBindings, Normalized> guard(ids, name);
    bindings.template validateAcyclic<Normalized>(validateProviding, injector, ids, name);
  }

  template<typename Dependency>
  void inject(typename Key<Dependency>::Ptr & injected, InjectorPtr injector, std::string const name) const {
    typedef typename Key<Dependency>::Normalized Normalized;
    typedef typename Key<Dependency>::Iface Iface;
    bindings.template get<Normalized>(injected, injector, name);
    SelfInjector<Iface> selfInjector;
    selfInjector.setSelf(injected);
  }

  template<typename Scope>
  void eagerlyInject(InjectorPtr injector) const {
    bindings.template eagerlyInject<Scope>(injector);
  }

  /**
   * Create an RAII synchronization lock.
   */
  sauce::auto_ptr<Lock> acquireLock() {
    sauce::auto_ptr<Lock> lock = lockFactory->createLock();
    return lock;
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_BASE_INJECTOR_H_
