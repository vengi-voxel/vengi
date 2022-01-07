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
	core::Map<int, SceneGraphNode> _nodes;
	int _nextNodeId = 0;

	SceneGraph();
public:
	~SceneGraph();

	void emplace_back(SceneGraphNode &&node, int parent = 0);

	const SceneGraphNode& root() const;

	/**
	 * @brief Pre-allocated memory in the graph without added the nodes
	 */
	void reserve(size_t size);
	bool empty(SceneGraphNodeType type = SceneGraphNodeType::Model) const;
	/**
	 * @return Amount of nodes in the graph
	 */
	size_t size(SceneGraphNodeType type = SceneGraphNodeType::Model) const;
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

	const SceneGraphNode &operator[](int modelIdx) const;
	SceneGraphNode &operator[](int modelIdx);

	class iterator {
	private:
		using parent_iter = core::Map<int, SceneGraphNode>::iterator;
		parent_iter _begin;
		parent_iter _end;
		SceneGraphNodeType _filter = SceneGraphNodeType::Max;
	public:
		constexpr iterator() {
		}

		iterator(parent_iter begin, parent_iter end, SceneGraphNodeType filter) :
				_begin(begin), _end(end), _filter(filter) {
			while (_begin != _end && _begin->second.type() != filter) {
				++_begin;
			}
		}

		inline SceneGraphNode& operator*() const {
			return _begin->value;
		}

		iterator& operator++() {
			if (_begin != _end) {
				do {
					++_begin;
				} while (_begin != _end && _begin->second.type() != _filter);
			}

			return *this;
		}

		inline SceneGraphNode& operator->() const {
			return _begin->value;
		}

		inline bool operator!=(const iterator& rhs) const {
			return _begin != rhs._begin;
		}

		inline bool operator==(const iterator& rhs) const {
			return _begin == rhs._begin;
		}
	};

	inline auto begin(SceneGraphNodeType filter = SceneGraphNodeType::Model) {
		return iterator(_nodes.begin(), _nodes.end(), filter);
	}

	inline auto end() {
		return iterator(_nodes.end(), _nodes.end(), SceneGraphNodeType::Max);
	}

	inline auto begin(SceneGraphNodeType filter = SceneGraphNodeType::Model) const {
		return iterator(_nodes.begin(), _nodes.end(), filter);
	}

	inline auto end() const {
		return iterator(_nodes.end(), _nodes.end(), SceneGraphNodeType::Max);
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
