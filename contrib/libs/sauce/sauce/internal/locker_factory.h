#ifndef SAUCE_INTERNAL_LOCKER_FACTORY_H_
#define SAUCE_INTERNAL_LOCKER_FACTORY_H_

namespace sauce {
namespace internal {

/**
 * An RAII wrapper to hide locker details.
 */
class Lock {
public:
  virtual ~Lock() {}
};

/**
 * Not a lock at all, but used when no synchronization is configured.
 */
class NullLock: public Lock {};

/**
 * An adapter to hide the type of the given Locker.
 */
template<typename Locker>
class LockerLock: public Lock {
  sauce::auto_ptr<Locker> locker;

public:

  LockerLock(sauce::auto_ptr<Locker> locker):
    locker(locker) {}
};

/**
 * Facility to create opaque RAII guards, intended to be synchronization lockers.
 *
 * Since lockers usually need access to a long-lived mutex, support for passing an opaque reference
 * on construction is provided.  RAII being what it is, ownership of the locker is passed out to
 * the caller via an auto_ptr.
 *
 * This formulation is specifically chosen to cater to boost/thread's approach to RAII lockers.
 * Specifically, a lock_guard used with a mutex should be sufficient for all needs.
 */
class LockFactory {
public:
  virtual ~LockFactory() {}

  virtual sauce::auto_ptr<Lock> createLock() = 0;
};

class NullLockFactory: public LockFactory {
public:
  sauce::auto_ptr<Lock> createLock() {
    sauce::auto_ptr<Lock> lock(new NullLock());
    return lock;
  }
};

template<typename Locker, typename Lockable>
class LockerLockFactory: public LockFactory {
  Lockable & lockable;

public:

  LockerLockFactory(Lockable & lockable):
    lockable(lockable) {}

  sauce::auto_ptr<Lock> createLock() {
    sauce::auto_ptr<Locker> locker(new Locker(lockable));
    sauce::auto_ptr<Lock> lock(new LockerLock<Locker>(locker));
    return lock;
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_LOCKER_FACTORY_H_
