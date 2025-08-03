/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "core/collection/Map.h"
#include <functional>

namespace core {

/**
 * @brief Hash map with a max size given as constructor parameter.
 *
 * @note The allocations are performed with the @c PoolAllocator
 *
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = priv::DefaultHasher,
		 typename COMPARE = priv::EqualCompare>
class ParallelMap : public Map<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE> {
private:
	using Super = Map<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE>;
protected:
	using Super::_buckets;

public:
	using Super::Map;
	void for_parallel(const std::function<void(const KEYTYPE &, const VALUETYPE &)> &fn) const {
		auto func = [this, &fn](int start, int end) {
			for (int i = start; i < end; ++i) {
				const typename Super::KeyValue *entry = _buckets[i];
				while (entry != nullptr) {
					fn(entry->key, entry->value);
					entry = entry->next;
				}
			}
		};
		app::for_parallel(0, _buckets.size(), func);
	}
};

} // namespace core
