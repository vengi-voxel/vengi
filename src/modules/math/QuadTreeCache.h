/**
 * @file
 */

#include "QuadTree.h"
#include <unordered_map>

namespace math {

#define CACHE 1
template<class NODE, typename TYPE>
class QuadTreeCache {
private:
	QuadTree<NODE, TYPE>& _tree;
#if CACHE
	std::unordered_map<Rect<TYPE>, typename QuadTree<NODE, TYPE>::Contents> _cache;
#endif
public:
	QuadTreeCache(QuadTree<NODE, TYPE>& tree) :
			_tree(tree) {
	}

	inline void clear() {
#if CACHE
		_cache.clear();
#endif
	}

	inline bool query(const Rect<TYPE>& area, typename QuadTree<NODE, TYPE>::Contents& contents) {
#if CACHE
		if (_tree.isDirty()) {
			_tree.markAsClean();
			clear();
		}
		// TODO: normalize to quad tree cells to improve the cache hits
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
