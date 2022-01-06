/**
 * @file
 */

#pragma once

#include "SceneGraphNode.h"
#include "core/collection/DynamicArray.h"

namespace voxel {

class RawVolume;

/**
 * @brief The internal format for the save/load methods.
 * @note Does not free the attached volumes by default!
 *
 * @sa ScopedSceneGraph
 * @sa SceneGraphNode
 */
class SceneGraph {
protected:
	core::DynamicArray<SceneGraphNode> _nodes;

	SceneGraph() {}
public:
	~SceneGraph();

	bool emplace_back(SceneGraphNode &&node);

	/**
	 * @brief Pre-allocated memory in the graph without added the nodes
	 */
	void reserve(size_t size);
	bool empty() const;
	/**
	 * @return Amount of nodes in the graph
	 */
	size_t size() const;
	/**
	 * @brief Merge all available nodes into one big volume.
	 * @note If the graph is empty, this returns @c nullptr
	 * @note The caller is responsible for deleting the returned volume
	 */
	voxel::RawVolume *merge() const;

	/**
	 * @brief Delete the owned volumes
	 */
	void clear();

	const SceneGraphNode &operator[](size_t idx) const;
	SceneGraphNode &operator[](size_t idx);

	inline auto begin() {
		return _nodes.begin();
	}

	inline auto end() {
		return _nodes.end();
	}

	inline auto begin() const {
		return _nodes.begin();
	}

	inline auto end() const {
		return _nodes.end();
	}
};

/**
 * @brief Using this class will automatically free the allocated memory of the volumes once the scope
 * was left.
 * @sa SceneGraph
 */
class ScopedSceneGraph : public SceneGraph {
public:
	~ScopedSceneGraph() {
		clear();
	}
};

} // namespace voxel
