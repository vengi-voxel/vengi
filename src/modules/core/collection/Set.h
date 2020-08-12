/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"

namespace core {

template<class T, size_t BUCKETSIZE = 11, typename HASHER = priv::DefaultHasher, typename COMPARE = priv::EqualCompare>
class Set : public Map<T, bool, BUCKETSIZE, HASHER, COMPARE> {
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

	inline bool has(const T& key) const {
		return this->hasKey(key);
	}
};

}
