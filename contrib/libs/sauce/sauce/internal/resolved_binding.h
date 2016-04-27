#ifndef SAUCE_INTERNAL_RESOLVED_BINDING_H_
#define SAUCE_INTERNAL_RESOLVED_BINDING_H_

#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/internal/key.h>
#include <sauce/internal/type_id.h>
#include <sauce/internal/opaque_binding.h>

namespace sauce {

class Injector;

namespace internal {

/**
 * A binding for an acknowledged interface.
 */
template<typename Dependency>
class ResolvedBinding: public OpaqueBinding {
public:

  typedef typename Key<Dependency>::Ptr IfacePtr;
  typedef sauce::shared_ptr<ResolvedBinding<Dependency> > BindingPtr;

  virtual ~ResolvedBinding() {}

  /**
   * The TypeId of the Dependency template parameter.
   */
  NamedTypeId getKey() const {
    return namedTypeIdOf<Dependency>(getName());
  }

  /**
   * Establish that further dependencies do not introduce cycles with ones already accumulated.
   *
   * This is Tarjan's algorithm using the call stack.  When a cycle is detected a
   * CircularDependencyException is thrown.
   */
  virtual void validateAcyclic(sauce::shared_ptr<Injector>, TypeIds &) const = 0;

  /**
   * Get an Iface, using the given injector to provide dependencies.
   *
   * The binding pointer must point to this same binding instance.
   */
  virtual void get(IfacePtr &, BindingPtr, sauce::shared_ptr<Injector>) const = 0;

};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_RESOLVED_BINDING_H_
