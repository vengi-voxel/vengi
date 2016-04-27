#ifndef SAUCE_INTERNAL_IMPLICIT_BINDINGS_H_
#define SAUCE_INTERNAL_IMPLICIT_BINDINGS_H_

#include <string>
#include <vector>

#include <sauce/exceptions.h>
#include <sauce/injector.h>
#include <sauce/memory.h>
#include <sauce/provider.h>
#include <sauce/scopes.h>
#include <sauce/internal/resolved_binding.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/bindings.h>
#include <sauce/internal/key.h>

namespace sauce {
namespace internal {

class ImplicitBindings;

typedef Bindings<ImplicitBindings> Concrete;

template<typename ImplicitInjection_>
class ImplicitBindingTraits {
public:
  typedef ImplicitInjection_ ImplicitInjection;
  typedef typename ImplicitInjection::Dependency Dependency;
  typedef typename ResolvedBinding<Dependency>::BindingPtr BindingPtr;
};

/**
 * Attempts to supply a Binding when the given Dependency is not found.
 */
template<typename Dependency>
class ImplicitBinding {
public:

  typedef sauce::shared_ptr<ResolvedBinding<Dependency> > BindingPtr;

  /**
   * Attempt to supply an unknown providing Binding at injection time.
   */
  static BindingPtr get(Concrete const &, std::string const name) {
    throw UnboundExceptionFor<Dependency>(name);
  }

};

/**
 * Attempts to supply a Binding when none is found for a dependency.
 */
class ImplicitBindings {
public:

  /**
   * Attempt to supply an unknown providing Binding at injection time.
   */
  template<typename Dependency>
  sauce::shared_ptr<ResolvedBinding<Dependency> > getProviding(
    Concrete const & bindings, std::string const name) const {
    return ImplicitBinding<Dependency>::get(bindings, name);
  }

  /**
   * Attempt to supply unknown modifying Bindings at injection time.
   */
  template<typename Dependency>
  std::vector<sauce::shared_ptr<ResolvedBinding<Dependency> > > getModifyings(
    Concrete const &, std::string const) const {
    return std::vector<sauce::shared_ptr<ResolvedBinding<Dependency> > >(); // TODO
  }

};

/**
 * The implicit Injector binding.
 */
template<>
class ImplicitBinding<Named<Injector, Unnamed> >: ImplicitBindingTraits<i::InjectorBinding> {
public:
  static BindingPtr get(Concrete const &, std::string const name) {
    if (name != unnamed()) {
      throw UnboundExceptionFor<Named<Injector, Unnamed> >(name);
    }
    BindingPtr binding(new ImplicitInjection());
    return binding;
  }
};

/**
 * The implicit Provider binding for bound dependencies.
 */
template<typename ProvidedDependency, typename Name>
class ImplicitBinding<Named<Provider<ProvidedDependency>, Name> > {
public:
  typedef ImplicitBindingTraits<i::ImplicitProviderBinding<ProvidedDependency, Name> > Traits;
  typedef typename Traits::ImplicitInjection ImplicitInjection;
  typedef typename Traits::Dependency Dependency;
  typedef typename Traits::BindingPtr BindingPtr;

  static BindingPtr get(Concrete const & bindings, std::string const name) {
    typedef typename Key<ProvidedDependency>::Normalized Normalized;
    typedef typename ResolvedBinding<Normalized>::BindingPtr ProvidedBindingPtr;

    ProvidedBindingPtr providedBinding(bindings.getProvidingBinding<Normalized>(name));
    BindingPtr binding(new ImplicitInjection(providedBinding));
    return binding;
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_IMPLICIT_BINDINGS_H_
