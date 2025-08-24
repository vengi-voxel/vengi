/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "core/collection/DynamicMap.h"
#include <functional>

namespace core {

/**
 * @brief Dynamic growing hash map.
 * @sa Map
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmap::DefaultHasher,
		 typename COMPARE = privdynamicmap::EqualCompare>
class DynamicParallelMap : public DynamicMap<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE> {
private:
	using Super = DynamicMap<KEYTYPE, VALUETYPE, BUCKETSIZE, HASHER, COMPARE>;

protected:
	using Super::_buckets;

public:
	using typename Super::DynamicMap;
	void for_parallel(const std::function<void(const KEYTYPE &, VALUETYPE &)> &fn) const {
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
