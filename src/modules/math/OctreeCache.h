/**
 * @file
 */

#pragma once

#include "Octree.h"
#include <unordered_map>

namespace std
{
template<typename TYPE>
struct hash<math::AABB<TYPE> > {
	static inline void hash_combine(size_t &seed, size_t hash) {
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}

	inline size_t operator()(const math::AABB<TYPE>& v) const {
		size_t seed = 0;
		hash<TYPE> hasher;
		const glm::tvec3<TYPE>& mins = v.mins();
		const glm::tvec3<TYPE>& maxs = v.maxs();
		hash_combine(seed, hasher(mins.x));
		hash_combine(seed, hasher(mins.y));
		hash_combine(seed, hasher(mins.z));
		hash_combine(seed, hasher(maxs.x));
		hash_combine(seed, hasher(maxs.y));
		hash_combine(seed, hasher(maxs.z));
		return seed;
	}
};

}

namespace math {

#define CACHE 1
template<class NODE, typename TYPE>
class OctreeCache {
private:
	Octree<NODE, TYPE>& _tree;
#if CACHE
	std::unordered_map<AABB<TYPE>, typename Octree<NODE, TYPE>::Contents> _cache;
#endif
public:
	OctreeCache(Octree<NODE, TYPE>& tree) :
			_tree(tree) {
	}

	inline void clear() {
#if CACHE
		_cache.clear();
#endif
	}

	inline bool query(const AABB<TYPE>& area, typename Octree<NODE, TYPE>::Contents& contents) {
#if CACHE
		if (_tree.isDirty()) {
			_tree.markAsClean();
			clear();
		}
		// TODO: normalize to octree cells to improve the cache hits
		auto iter = _cache.find(area);
		if (iter != _cache.end()) {
			contents = iter->second;
			return true;
		}
		_tree.query(area, contents);
		_cache.insert(std::make_pair(area, contents));
#else
		_tree.query(area, contents);
#endif
		return false;
	}
};

#undef CACHE

}
