/**
 * @file
 */

#pragma once

#include "Octree.h"
#include <unordered_map>

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
