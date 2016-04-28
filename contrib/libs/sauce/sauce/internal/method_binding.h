#ifndef SAUCE_INTERNAL_METHOD_BINDING_H_
#define SAUCE_INTERNAL_METHOD_BINDING_H_

#include <sauce/injector.h>
#include <sauce/scopes.h>
#include <sauce/internal/apply_variadic.h>
#include <sauce/internal/binding.h>
#include <sauce/internal/resolved_binding.h>

namespace sauce {
namespace internal {

template<typename Dependency, typename Signature>
class MethodBinding: public Binding<Dependency, NoScope> {

  typedef MethodBinding<Dependency, Signature> MethodBinding_;

  /**
   * A mixin for ApplyVariadic parameter concept types.
   */
  struct MethodBindingFriend {
    template<typename T>
    void validateAcyclicHelper(MethodBinding_ const & binding, InjectorPtr injector, TypeIds & ids, std::string name) {
      binding.template validateAcyclicHelper<T>(injector, ids, name);
    }

    template<typename T>
    typename Key<T>::Ptr injectHelper(MethodBinding_ const & binding, InjectorPtr injector, std::string name) {
      typename Key<T>::Ptr injected;
      binding.template injectHelper<typename i::Key<T>::Normalized>(injected, injector, name);
      return injected;
    }
  };

  friend struct MethodBindingFriend;

  struct InjectParameters {
    struct Passed {
      MethodBinding_ const & binding;
      InjectorPtr & injector;

      Passed(MethodBinding_ const & binding, InjectorPtr & injector):
        binding(binding), injector(injector) {}
    };

    template<typename T, int i>
    struct Parameter: public MethodBindingFriend {
      typedef typename Key<T>::Ptr Type;

      template<typename Passed>
      Type yield(Passed passed) {
        MethodBinding_ const & binding = passed.binding;
        InjectorPtr & injector = passed.injector;
        std::string dependencyName = binding.dynamicDependencyNames[i];

        return this->MethodBindingFriend::template injectHelper<T>(binding, injector, dependencyName);
      }
    };
  };

  typedef typename Key<Dependency>::Iface Iface;
  typedef typename Key<Dependency>::Ptr IfacePtr;
  typedef ApplyMethod<InjectParameters, Signature> Inject;

  std::vector<std::string> dynamicDependencyNames;

  struct ValidateAcyclicParameters {
    struct Passed {
      MethodBinding_ const & binding;
      InjectorPtr & injector;
      TypeIds & ids;

      Passed(MethodBinding_ const & binding, InjectorPtr & injector, TypeIds & ids):
        binding(binding), injector(injector), ids(ids) {}
    };

    template<typename T, int i>
    struct Parameter: public MethodBindingFriend {
      typedef typename Key<T>::Ptr Type;

      template<typename Passed>
      void observe(Passed passed) {
        MethodBinding_ const & binding = passed.binding;
        InjectorPtr & injector = passed.injector;
        TypeIds & ids = passed.ids;
        std::string dependencyName = binding.dynamicDependencyNames[i];

        this->MethodBindingFriend::template validateAcyclicHelper<T>(binding, injector, ids, dependencyName);
      }
    };
  };

public:
  typedef typename Inject::Method Method;
private:

  Method method;

  bool isModifier() const {
    return true;
  }

  void validateAcyclic(InjectorPtr injector, TypeIds & ids) const {
    typename ValidateAcyclicParameters::Passed passed(*this, injector, ids);
    observeMethod<ValidateAcyclicParameters, Signature>(method, passed);
  }

  void setDynamicDependencyNames(std::vector<std::string> const & dynamicDependencyNames) {
    this->dynamicDependencyNames = dynamicDependencyNames;
    this->dynamicDependencyNames.resize(Inject::arity(), unnamed());
  }

public:

  typedef typename ResolvedBinding<Dependency>::BindingPtr BindingPtr;

  explicit MethodBinding(Method method):
    dynamicDependencyNames(Inject::arity(), unnamed()),
    method(method) {}

  void inject(IfacePtr & injected, BindingPtr, InjectorPtr injector) const {
    typename InjectParameters::Passed passed(*this, injector);
    applyMethod<InjectParameters, Signature>(*injected.get(), method, passed);
  }

};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_METHOD_BINDING_H_
