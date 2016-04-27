#ifndef SAUCE_INTERNAL_OPAQUE_BINDING_H_
#define SAUCE_INTERNAL_OPAQUE_BINDING_H_

#include <string>
#include <vector>

#include <sauce/memory.h>
#include <sauce/internal/type_id.h>

namespace sauce {

class Injector;

namespace internal {

class OpaqueBinding;

typedef sauce::shared_ptr<OpaqueBinding> OpaqueBindingPtr;

/**
 * An opaque binding.
 *
 * Bindings associate an interface with an implementation.  How that provision
 * comes about is determine by derived types.  Binding itself is a pure
 * interface.
 *
 * To be type-homogenous, OpaqueBinding is not a template, and particularly not a
 * template of any specific interface or implementation types.  It however has
 * a TypeId, which indirectly identifies the interface it is bound to. The
 * interface is raised to the type system in ResolvedBinding, a class template
 * deriving from OpaqueBinding.
 */
class OpaqueBinding {
public:

  virtual ~OpaqueBinding() {}

  /**
   * The dynamic name of this binding.
   */
  virtual std::string getName() const = 0;

  /**
   * Set the dynamic name of this binding.
   */
  virtual void setName(std::string) = 0;

  /**
   * The NamedTypeId of the (hidden) provided interface.
   */
  virtual NamedTypeId getKey() const = 0;

  /**
   * Does this binding modify an existing value?
   *
   * If not, it must provide a new value, and so is "providing" binding.
   */
  virtual bool isModifier() const {
    return false;
  }

  /**
   * The TypeId of the (hidden) scope.
   */
  virtual TypeId getScopeKey() const = 0;

  /**
   * Provide, but do not return the hidden interface.
   *
   * Instead, cache the instance in its appropriate scope, if any.  If the binding is not scoped,
   * do nothing.
   */
  virtual void eagerlyInject(OpaqueBindingPtr, sauce::shared_ptr<Injector>) const = 0;

  /**
   * Accept the list of dynamic dependency names this binding was created with.
   */
  virtual void setDynamicDependencyNames(std::vector<std::string> const &) = 0;

};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_OPAQUE_BINDING_H_
