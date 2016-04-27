#ifndef SAUCE_INTERNAL_DISPOSAL_DELETER_H_
#define SAUCE_INTERNAL_DISPOSAL_DELETER_H_

#include <sauce/memory.h>

namespace sauce {
namespace internal {

/**
 * A smart pointer deleter that diposes with the given disposal.
 */
template<typename Iface, typename Disposal>
class DisposalDeleter {
  typedef sauce::shared_ptr<Disposal> DisposalPtr;

  DisposalPtr disposal;

public:

  DisposalDeleter(DisposalPtr disposal):
    disposal(disposal) {}

  /**
   * Cast and dispose the given Iface instance.
   */
  void operator()(Iface * iface) const {
    disposal->dispose(iface);
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_DISPOSAL_DELETER_H_
