/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "core/collection/DynamicMap.h"
#include "core/Function.h"

namespace core {

/**
 * @brief Dynamic growing hash map.
 * @sa Map
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmap::DefaultHasher,
		 typename COMPARE = privdynamicmap::EqualCompare, size_t BLOCK_SIZE = 256>
class DynamicParallelMap : public DynamicMap<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE, BLOCK_SIZE> {
private:
	using Super = DynamicMap<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE, BLOCK_SIZE>;

protected:
	using Super::_buckets;

public:
	using Super::DynamicMap;
	void for_parallel(const core::Function<void(const KEYTYPE &, VALUETYPE &)> &fn) const {
		auto func = [this, &fn](int start, int end) {
			for (int i = start; i < end; ++i) {
				typename Super::KeyValue *entry = _buckets[i];
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
