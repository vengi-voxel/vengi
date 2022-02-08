/**
 * @file
 */

#pragma once

#include "SceneGraphNode.h"
#include "core/collection/DynamicArray.h"
#include <functional>

namespace voxel {

class RawVolume;

/**
 * @brief The internal format for the save/load methods.
 *
 * @sa SceneGraph
 * @sa SceneGraphNode
 */
class SceneGraph {
protected:
	core::Map<int, SceneGraphNode> _nodes;
	int _nextNodeId = 0;
	int _nextModelId = 0;
	int _activeNodeId = -1;

public:
	SceneGraph();
	~SceneGraph();

	int activeNode() const;
	bool setActiveNode(int nodeId);
	void foreachGroup(const std::function<void(int)>& f);

	/**
	 * @return The full region of the whole scene
	 */
	voxel::Region region() const;

	/**
	 * @brief We move into the scene graph to make it clear who is owning the volume.
	 *
	 * @param node The node to move
	 * @param parent The parent node id - by default this is 0 which is the root node
	 * @sa core::move()
	 * @return the node id that was assigned - or a negative number in case the node wasn't added and an error happened.
	 * @note If an error happened, the node is released.
	 */
	int emplace(SceneGraphNode &&node, int parent = 0);

	SceneGraphNode* findNodeByName(const core::String& name);

	const SceneGraphNode& root() const;
	/**
	 * @brief Get the current scene graph node
	 * @note It's important to check whether the node exists before calling this method!
	 * @param nodeId The node id that was assigned to the node after adding it to the scene graph
	 * @sa hasNode()
	 * @sa emplace_back()
	 * @return @c SceneGraphNode refernence. Undefined if no node was found for the given id!
	 */
	SceneGraphNode& node(int nodeId) const;
	bool hasNode(int nodeId) const;
	bool removeNode(int nodeId, bool recursive = false);

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

	const SceneGraphNode *operator[](int modelIdx) const;
	SceneGraphNode *operator[](int modelIdx);

	class iterator {
	private:
		int _startNodeId = -1;
		int _endNodeId = -1;
		SceneGraphNodeType _filter = SceneGraphNodeType::Max;
		const SceneGraph *_sceneGraph = nullptr;
	public:
		constexpr iterator() {
		}

		iterator(int startNodeId, int endNodeId, SceneGraphNodeType filter, const SceneGraph *sceneGraph) :
				_startNodeId(startNodeId), _endNodeId(endNodeId), _filter(filter), _sceneGraph(sceneGraph) {
			while (_startNodeId != _endNodeId) {
				if (!_sceneGraph->hasNode(_startNodeId)) {
					++_startNodeId;
					continue;
				}
				if (_sceneGraph->node(_startNodeId).type() == filter) {
					break;
				}
				++_startNodeId;
			}
		}

		inline SceneGraphNode& operator*() const {
			return _sceneGraph->node(_startNodeId);
		}

		iterator &operator++() {
			if (_startNodeId != _endNodeId) {
				for (;;) {
					++_startNodeId;
					if (_startNodeId == _endNodeId) {
						break;
					}
					if (!_sceneGraph->hasNode(_startNodeId)) {
						continue;
					}
					if (_sceneGraph->node(_startNodeId).type() == _filter) {
						break;
					}
				}
			}

			return *this;
		}

		inline SceneGraphNode& operator->() const {
			return _sceneGraph->node(_startNodeId);
		}

		inline bool operator!=(const iterator& rhs) const {
			return _startNodeId != rhs._startNodeId;
		}

		inline bool operator==(const iterator& rhs) const {
			return _startNodeId == rhs._startNodeId;
		}
	};

	inline auto begin(SceneGraphNodeType filter = SceneGraphNodeType::Model) {
		return iterator(0, _nextNodeId, filter, this);
	}

	inline auto end() {
		return iterator(0, _nextNodeId, SceneGraphNodeType::Max, this);
	}

	inline auto begin(SceneGraphNodeType filter = SceneGraphNodeType::Model) const {
		return iterator(0, _nextNodeId, filter, this);
	}

	inline auto end() const {
		return iterator(0, _nextNodeId, SceneGraphNodeType::Max, this);
	}
};

} // namespace voxel
