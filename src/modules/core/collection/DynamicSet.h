/**
 * @file
 */

#pragma once

#include "core/collection/DynamicMap.h"

namespace core {

template<class T, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmap::DefaultHasher, typename COMPARE = privdynamicmap::EqualCompare, size_t BLOCK_SIZE = 256>
class DynamicSet : public DynamicMap<T, bool, BUCKETSIZE, HASHER, COMPARE, BLOCK_SIZE> {
private:
	using Super = DynamicMap<T, bool, BUCKETSIZE, HASHER, COMPARE, BLOCK_SIZE>;
public:
	bool insert(const T& key) {
		if (has(key)) {
			return false;
		}
		this->put(key, true);
		return true;
	}

	template<class ITER>
	void insert(ITER first, ITER last) {
		while (first != last) {
			this->put(*first, true);
			++first;
		}
	}

	template<class FUNC>
	void insert(size_t n, FUNC&& func) {
		for (size_t i = 0u; i < n; ++i) {
			insert(TYPE(func(i)));
		}
	}

	inline bool has(const T& key) const {
		return this->hasKey(key);
	}
};

}
