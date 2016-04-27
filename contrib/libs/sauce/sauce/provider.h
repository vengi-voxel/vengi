#ifndef SAUCE_PROVIDER_H_
#define SAUCE_PROVIDER_H_

#include <cassert>

#include <sauce/memory.h>
#include <sauce/internal/disposal_deleter.h>
#include <sauce/internal/key.h>

namespace sauce {

class Injector;

template<typename Dependency>
class Provider;

/**
 * An interface for including custom factories in an Injector.
 */
template<typename Dependency>
class Provider {
public:

  /**
   * Indicates to template magic that this type exposes sauce::shared_ptr<Provides> get().
   */
  typedef typename i::Key<Dependency>::Iface Provides;

  virtual ~Provider() {}

  /**
   * Provide an Iface.
   */
  virtual typename i::Key<Dependency>::Ptr get() = 0;

};

/**
 * An interface for including custom factories in an Injector.
 */
template<typename Dependency>
class AbstractProvider: public Provider<Dependency> {

  typedef typename i::Key<Dependency>::Iface Iface;
  typedef AbstractProvider<Dependency> Abstract;
  typedef i::DisposalDeleter<Iface, Abstract> Deleter;

  friend class i::DisposalDeleter<Iface, Abstract>;

  sauce::weak_ptr<Abstract> weak;

  /**
   * Provide a naked Iface pointer.
   */
  virtual Iface * provide() = 0;

  /**
   * Dispose of an Iface provided by this provider.
   */
  virtual void dispose(Iface *) = 0;

public:

  typedef Abstract RequestsSelfInjection;

  virtual ~AbstractProvider() {}

  void setSelf(sauce::weak_ptr<Abstract> weak) {
    assert(weak.lock().get() == this);
    this->weak = weak;
  }

  /**
   * Provide an Iface.
   *
   * A naked instance pointer is obtained with provide(), and wrapped in a shared_ptr.  It is also
   * given a custom deleter, to dispose of the naked pointer with dispose(Iface *).
   */
  sauce::shared_ptr<Iface> get() {
    sauce::shared_ptr<Abstract> self = weak.lock();
    assert(self.get() == this);
    Deleter deleter(sauce::static_pointer_cast<Abstract>(self));
    return sauce::shared_ptr<Iface>(provide(), deleter);
  }

};

}

#endif // SAUCE_PROVIDER_H_
