/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "scenegraph/SceneGraphNode.h" // InvalidNodeId

namespace voxedit {

/**
 * @brief Caches a computed value for a specific scene graph node with lazy invalidation.
 *
 * The cached value is invalidated when @c invalidate() is called with the matching node id,
 * or unconditionally without a node id. Use @c valid() to check if the cache holds a value.
 */
template<typename T>
class SceneGraphNodeCachedValue {
public:
	SceneGraphNodeCachedValue() : _nodeId(InvalidNodeId) {
	}

	bool valid(int nodeId) const {
        return _nodeId == nodeId && _value.hasValue();
	}

	int nodeId() const {
		return _nodeId;
	}

	const T *value() const {
		return _value.value();
	}

	void set(int nodeId, const T &val) {
		_nodeId = nodeId;
		_value.setValue(val);
	}

	void invalidate() {
		_nodeId = InvalidNodeId;
		_value = core::Optional<T>();
	}

	void invalidate(int nodeId) {
		if (_nodeId == nodeId) {
			invalidate();
		}
	}

private:
	int _nodeId;
	core::Optional<T> _value;
};

} // namespace voxedit
