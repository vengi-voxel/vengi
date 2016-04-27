#ifndef SAUCE_INTERNAL_SELF_INJECTOR_H_
#define SAUCE_INTERNAL_SELF_INJECTOR_H_

#include <sauce/memory.h>

namespace sauce {
namespace internal {

/**
 * If a type requests injection of its own smart pointer, do so.
 *
 * A type requests self-injection by defining the RequestsSelfInjection typedef.  If this typedef is present,
 * void setSelf(sauce::weak_ptr<RequestsSelfInjection>) must be defined and will be passes a self weak pointer.
 */
template<typename T>
class SelfInjector { // TODO Ditch this thing.
  typedef sauce::shared_ptr<T> Ptr;

  template<typename DoesNotRequest>
  void setSelfIfRequested(Ptr, ...) {}

  template<typename Requests>
  void setSelfIfRequested(Ptr ptr, typename Requests::RequestsSelfInjection *) {
    sauce::weak_ptr<typename Requests::RequestsSelfInjection> weak(ptr);
    ptr->setSelf(weak);
  }

public:

  void setSelf(Ptr ptr) {
    setSelfIfRequested<T>(ptr, 0);
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_SELF_INJECTOR_H_
