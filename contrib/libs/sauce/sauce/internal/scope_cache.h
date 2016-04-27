#ifndef SAUCE_INTERNAL_SCOPE_CACHE_H_
#define SAUCE_INTERNAL_SCOPE_CACHE_H_

#include <sauce/internal/key.h>
#include <sauce/internal/type_id.h>

namespace sauce {
namespace internal {

template<typename Dependency>
class ScopeCacheLineDeleter {
public:
  typedef typename Key<Dependency>::Ptr SmartPtr;
  void operator()(void * smartPtrPtr) const {
    delete static_cast<SmartPtr *>(smartPtrPtr);
  }
};

class ScopeCache {
  typedef sauce::shared_ptr<void> CachedPtr;
  typedef std::map<TypeId, CachedPtr> Cache;

  Cache cache;

public:

  ScopeCache():
    cache() {}

  /**
   * Insert a dependency into the cache.
   */
  template<typename Dependency>
  void put(typename Key<Dependency>::Ptr pointer) {
    typedef typename Key<Dependency>::Normalized Normalized;
    typedef typename Key<Normalized>::Ptr SmartPtr;

    /*
     * A voice! a voice! It rang deep to the very last. It survived his strength to hide in the
     * magnificent folds of eloquence the barren darkness of his heart. Oh, he struggled! he
     * struggled! The wastes of his weary brain were haunted by shadowy images now – images of
     * wealth and fame revolving obsequiously round his unextinguishable gift of noble and lofty
     * expression. My Intended, my station, my career, my ideas – these were the subjects for the
     * occasional utterances of elevated sentiments.
     */

    /*
     * (Make the new smart ptr type agnostic by shoving it into *another* smart pointer.
     * The deleter casts it back so the reference count isn't leaked.)
     */

    CachedPtr cachedPtr(
      static_cast<void *>(new SmartPtr(pointer)),
      ScopeCacheLineDeleter<Normalized>());

    cache.insert(std::make_pair(typeIdOf<Normalized>(), cachedPtr));
  }

  /**
   * Probe the cache for a dependency.
   *
   * The return value indicates if a hit was found.  On a hit, the out argument will be
   * overwritten with the discovered value.
   */
  template<typename Dependency>
  bool get(typename Key<Dependency>::Ptr & out) const {
    typedef typename Key<Dependency>::Normalized Normalized;
    typedef typename Key<Normalized>::Ptr SmartPtr;

    Cache::const_iterator cachedPtr = cache.find(typeIdOf<Normalized>());
    if (cachedPtr == cache.end()) {
      return false;
    }

    out = *static_cast<SmartPtr *>(cachedPtr->second.get());
    return true;
  }
};

}

namespace i = ::sauce::internal;

}

#endif // SAUCE_INTERNAL_SCOPE_CACHE_H_
