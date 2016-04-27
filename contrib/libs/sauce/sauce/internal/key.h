#ifndef SAUCE_INTERNAL_KEY_H_
#define SAUCE_INTERNAL_KEY_H_

#include <sauce/memory.h>
#include <sauce/named.h>
#include <sauce/internal/type_id.h>

namespace sauce {
namespace internal {

/**
 * A complete specification of a dependency request.
 */
template<typename Iface_>
class Key {
public:
  typedef Iface_ Iface;
  typedef sauce::shared_ptr<Iface_> Ptr;
  typedef Unnamed Name;

  /**
   * Note Normalized is not Iface, but Named<Iface, Unnamed>.
   *
   * They are logically equivalent, but have different TypeIds.  Where it is ambiguous, we use
   * the normalized version.
   */
  typedef Named<Iface_, Unnamed> Normalized;
};

/**
 * Template specialization when a reference is used.
 */
template<typename Iface_>
class Key<Iface_ &> {
public:
  typedef typename Key<Iface_>::Iface Iface;
  typedef typename Key<Iface_>::Ptr Ptr;
  typedef typename Key<Iface_>::Name Name;
  typedef typename Key<Iface_>::Normalized Normalized;
};

/**
 * Template specialization when a smart pointer is used.
 */
template<typename Iface_>
class Key<sauce::shared_ptr<Iface_> > {
public:
  typedef typename Key<Iface_>::Iface Iface;
  typedef typename Key<Iface_>::Ptr Ptr;
  typedef typename Key<Iface_>::Name Name;
  typedef typename Key<Iface_>::Normalized Normalized;
};

/**
 * Template specialization when an actual name is used.
 */
template<typename Iface_, typename Name_>
class Key<Named<Iface_, Name_> > {
public:
  typedef Iface_ Iface;
  typedef sauce::shared_ptr<Iface_> Ptr;
  typedef Name_ Name;
  typedef Named<Iface_, Name_> Normalized;
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_KEY_H_
