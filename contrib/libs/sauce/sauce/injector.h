#ifndef SAUCE_INJECTOR_H_
#define SAUCE_INJECTOR_H_

#include <cassert>
#include <string>

#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/scopes.h>
#include <sauce/internal/base_injector.h>
#include <sauce/internal/key.h>
#include <sauce/internal/locker_factory.h>
#include <sauce/internal/scope_cache.h>
#include <sauce/internal/type_id.h>

namespace sauce {

class Modules;
class Injector;

namespace internal {
class ImplicitBindings;
class InjectorFriend;
typedef sauce::shared_ptr<Injector> InjectorPtr;
}

class Injector {
  i::TypeId const scopeKey;
  i::ScopeCache scopeCache;
  sauce::weak_ptr<Injector> weak;
  i::InjectorPtr const next;
  sauce::shared_ptr<i::BaseInjector<i::ImplicitBindings> > const base;

  friend class Modules;
  friend class i::InjectorFriend;

  Injector(i::TypeId scopeKey, i::InjectorPtr next):
    scopeKey(scopeKey),
    scopeCache(),
    weak(),
    next(next),
    base() {}

  Injector(sauce::shared_ptr<i::BaseInjector<i::ImplicitBindings> > const base):
    scopeKey(i::typeIdOf<SingletonScope>()),
    scopeCache(),
    weak(),
    next(),
    base(base) {}

  void setSelfPtr(i::InjectorPtr shared) {
    assert(shared.get() == this);
    weak = shared;
  }

  i::InjectorPtr getSelf() const {
    i::InjectorPtr self = weak.lock();
    assert(self.get() == this);
    return self;
  }

  template<typename Dependency>
  void validateAcyclic(bool validateProviding, i::InjectorPtr injector, i::TypeIds & ids, std::string const name) {
    if (base.get() == NULL) {
      next->validateAcyclic<Dependency>(validateProviding, injector, ids, name);
    } else {
      base->validateAcyclic<Dependency>(validateProviding, injector, ids, name);
    }
  }

  template<typename Dependency>
  void inject(typename i::Key<Dependency>::Ptr & injected, i::InjectorPtr injector, std::string const name) {
    if (base.get() == NULL) {
      next->inject<Dependency>(injected, injector, name);
    } else {
      base->inject<Dependency>(injected, injector, name);
    }
  }

  /**
   * Create an RAII synchronization lock.
   */
  sauce::auto_ptr<i::Lock> acquireLock() {
    if (base.get() == NULL) {
      return next->acquireLock();
    } else {
      return base->acquireLock();
    }
  }

  template<typename Scope>
  void eagerlyInject(i::InjectorPtr injector) {
    if (base.get() == NULL) {
      next->eagerlyInject<Scope>(injector);
    } else {
      base->eagerlyInject<Scope>(injector);
    }
  }

  template<typename Dependency>
  void cache(typename i::Key<Dependency>::Ptr pointer, i::TypeId dependencyScopeKey) {
    if (scopeKey == dependencyScopeKey) {
      scopeCache.template put<Dependency>(pointer);
    } else if (next.get() == NULL) {
      dependencyScopeKey.throwOutOfScopeException();
    } else {
      next->cache<Dependency>(pointer, dependencyScopeKey);
    }
  }

  template<typename Dependency>
  bool probe(typename i::Key<Dependency>::Ptr & out, i::TypeId dependencyScopeKey) const {
    if (scopeKey == dependencyScopeKey) {
      return scopeCache.template get<Dependency>(out);
    } else if (next.get() == NULL) {
      dependencyScopeKey.throwOutOfScopeException();
      return false; // never reached
    } else {
      return next->probe<Dependency>(out, dependencyScopeKey);
    }
  }

  template<typename Scope>
  bool alreadyInScope() const {
    if (scopeKey == i::typeIdOf<Scope>()) {
      return true;
    } else if (next.get() == NULL) {
      return false;
    } else {
      return next->alreadyInScope<Scope>();
    }
  }

public:

  template<typename Dependency>
  void inject(typename i::Key<Dependency>::Ptr & injected, std::string const name = unnamed()) {
    typedef typename i::Key<Dependency>::Normalized Normalized;

    sauce::auto_ptr<i::Lock> lock = acquireLock();

    i::TypeIds ids;
    bool validateProviding = (injected.get() == NULL);
    validateAcyclic<Normalized>(validateProviding, getSelf(), ids, name); // TODO Make this check optional.

    inject<Normalized>(injected, getSelf(), name);
  }

  template<typename Iface, typename Name>
  void inject(typename i::Key<Named<Iface, Name> >::Ptr & injected, std::string const name = unnamed()) {
    inject<Named<Iface, Name> >(injected, name);
  }

  template<typename Dependency>
  typename i::Key<Dependency>::Ptr get(std::string const name = unnamed()) {
    typename i::Key<Dependency>::Ptr injected;
    inject<Dependency>(injected, name);
    return injected;
  }

  template<typename Iface, typename Name>
  typename i::Key<Named<Iface, Name> >::Ptr get(std::string const name = unnamed()) {
    return get<Named<Iface, Name> >(name);
  }

  template<typename Scope>
  i::InjectorPtr enter() const {
    if (alreadyInScope<Scope>()) {
      throw AlreadyInScopeExceptionFor<Scope>();
    }

    i::InjectorPtr scoped(new Injector(i::typeIdOf<Scope>(), getSelf()));
    scoped->setSelfPtr(scoped);
    return scoped;
  }

  i::InjectorPtr exit() const {
    if (next.get() == NULL) {
      throw ExitingSingletonScopeException();
    } else {
      return next;
    }
  }

  template<typename Scope>
  void eagerlyInject() {
    sauce::auto_ptr<i::Lock> lock = acquireLock();
    eagerlyInject<Scope>(getSelf());
  }

};

namespace internal {

class InjectorFriend {
protected:

  template<typename Dependency>
  void validateAcyclicHelper(InjectorPtr injector, TypeIds & ids, std::string const name) const {
    injector->validateAcyclic<Dependency>(true, injector, ids, name);
  }

  template<typename Dependency>
  void injectHelper(typename Key<Dependency>::Ptr & injected, InjectorPtr injector, std::string const name) const {
    injector->inject<Dependency>(injected, injector, name);
  }

  template<typename Dependency>
  void cache(InjectorPtr injector, typename Key<Dependency>::Ptr injected, i::TypeId scope) const {
    injector->template cache<Dependency>(injected, scope);
  }

  template<typename Dependency>
  bool probe(InjectorPtr injector, typename Key<Dependency>::Ptr & injected, i::TypeId scope) const {
    return injector->template probe<Dependency>(injected, scope);
  }

};

}

}

#endif // SAUCE_INJECTOR_H_
