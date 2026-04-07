/**
 * @file
 */

#pragma once

#include "core/collection/DynamicMap.h"

namespace voxedit {

/**
 * @brief Caches computed values for scene graph nodes with lazy invalidation.
 *
 * Supports caching values for multiple nodes simultaneously. The cached value for a node
 * is invalidated when @c invalidate() is called with the matching node id,
 * or all entries are cleared when called without a node id.
 * Use @c valid() to check if the cache holds a value for a given node.
 */
template<typename T>
class SceneGraphNodeValueCache {
public:
	SceneGraphNodeValueCache() {
	}

	bool valid(int nodeId) const {
		return _map.hasKey(nodeId);
	}

	const T *value(int nodeId) const {
		auto iter = _map.find(nodeId);
		if (iter == _map.end()) {
			return nullptr;
		}
		return &iter->second;
	}

	void set(int nodeId, const T &val) {
		_map.put(nodeId, val);
	}

	void invalidate() {
		_map.clear();
	}

	void invalidate(int nodeId) {
		_map.remove(nodeId);
	}

	int size() const {
		return (int)_map.size();
	}

private:
	core::DynamicMap<int, T> _map;
};

} // namespace voxedit
