/**
 * @file
 */

#pragma once

#include "core/UUID.h"
#include "core/collection/DynamicMap.h"

namespace voxedit {

/**
 * @brief Caches computed values for scene graph nodes with lazy invalidation.
 *
 * Supports caching values for multiple nodes simultaneously. The cached value for a node
 * is invalidated when @c invalidate() is called with the matching node UUID,
 * or all entries are cleared when called without a UUID.
 * Use @c valid() to check if the cache holds a value for a given node.
 */
template<typename T>
class SceneGraphNodeValueCache {
public:
	SceneGraphNodeValueCache() {
	}

	bool valid(const core::UUID &nodeUUID) const {
		return nodeUUID.isValid() && _map.hasKey(nodeUUID);
	}

	const T *value(const core::UUID &nodeUUID) const {
		auto iter = _map.find(nodeUUID);
		if (iter == _map.end()) {
			return nullptr;
		}
		return &iter->second;
	}

	void set(const core::UUID &nodeUUID, const T &val) {
		if (!nodeUUID.isValid()) {
			return;
		}
		_map.put(nodeUUID, val);
	}

	void invalidate() {
		_map.clear();
	}

	void invalidate(const core::UUID &nodeUUID) {
		_map.remove(nodeUUID);
	}

	int size() const {
		return (int)_map.size();
	}

private:
	core::DynamicMap<core::UUID, T, 251, core::UUIDHash> _map;
};

} // namespace voxedit
