#ifndef SAUCE_MODULES_H_
#define SAUCE_MODULES_H_

#include <sauce/binder.h>
#include <sauce/injector.h>
#include <sauce/memory.h>
#include <sauce/internal/base_injector.h>
#include <sauce/internal/resolved_binding.h>
#include <sauce/internal/bindings.h>
#include <sauce/internal/implicit_bindings.h>
#include <sauce/internal/locker_factory.h>

namespace sauce {

/**
 * A base class that modules implemented as classes might derive from.
 *
 * Such a module would override configure() and call bind() directly, instead of handling an
 * explicit Binder.
 */
class AbstractModule {
  mutable Binder * binder;

  /**
   * RAII object to protect the state of AbtractModule::binder.
   */
  class BinderGuard {
    AbstractModule const * module;
    Binder * previousBinder;

    friend class AbstractModule;

    BinderGuard(AbstractModule const * module, Binder * binder):
      module(module),
      previousBinder(module->binder) {
      module->binder = binder;
    }

    ~BinderGuard() {
      module->binder = previousBinder;
    }
  };

  friend class BinderGuard;

protected:

  AbstractModule():
    binder(NULL) {}

  /**
   * Override in derived classes to declare bindings.
   */
  virtual void configure() const = 0;

  /**
   * Begin binding the chosen interface.
   */
  template<typename Iface>
  BindClause<Iface> bind() const {
    return binder->bind<Iface>();
  }

public:

  virtual ~AbstractModule() {}

  void operator()(Binder & binder) const {
    BinderGuard guard(this, &binder);
    configure();
  }
};

/**
 * A factory that accepts Modules and creates Injectors.
 */
class Modules {
  i::Bindings<i::ImplicitBindings> bindings;
  Binder binder;

  sauce::shared_ptr<Injector> createInjector(sauce::auto_ptr<i::LockFactory> lockFactory) const {
    sauce::shared_ptr<i::BaseInjector<i::ImplicitBindings> > base(
      new i::BaseInjector<i::ImplicitBindings>(bindings, lockFactory));
    sauce::shared_ptr<Injector> injector(new Injector(base));
    injector->setSelfPtr(injector);
    return injector;
  }

public:

  /**
   * Create an empty Modules.
   */
  Modules():
    bindings(),
    binder(bindings) {}

  /**
   * Add the bindings defined by the given module function.
   *
   * An Injector created after adding a module will understand how to provide dependencies
   * specified by that module.
   */
  Modules & add(void (* module)(Binder &)) {
    module(binder);
    binder.throwAnyPending();
    return *this;
  }

  /**
   * Add the bindings defined by the given Module type.
   *
   * The module here is any default constructable type providing operator()(Binding & bindings).
   *
   * An Injector created after adding a module will understand how to provide dependencies
   * specified by that module.
   */
  template<typename Module>
  Modules & add() {
    Module module;
    module(binder);
    binder.throwAnyPending();
    return *this;
  }

  /**
   * Add the bindings defined by the given Module instance.
   *
   * The module here is any value providing operator()(Binding & bindings).
   *
   * An Injector created after adding a module will understand how to provide dependencies
   * specified by that module.
   */
  template<typename Module>
  Modules & add(Module & module) {
    module(binder);
    binder.throwAnyPending();
    return *this;
  }

  /**
   * Create an Injector that can provide dependencies specified by all added Modules.
   *
   * Any modules added after an Injector is created will have no effect on that
   * Injector.
   */
  sauce::shared_ptr<Injector> createInjector() const {
    sauce::auto_ptr<i::LockFactory> lockFactory(new i::NullLockFactory());
    return createInjector(lockFactory);
  }

  /**
   * Create an Injector that can provide dependencies specified by all added Modules.
   *
   * Any modules added after an Injector is created will have no effect on that
   * Injector.
   */
  template<typename Locker, typename Lockable>
  sauce::shared_ptr<Injector> createInjector(Lockable & lockable) const {
    sauce::auto_ptr<i::LockFactory> lockFactory(new i::LockerLockFactory<Locker, Lockable>(lockable));
    return createInjector(lockFactory);
  }

};

}

#endif // SAUCE_MODULES_H_
