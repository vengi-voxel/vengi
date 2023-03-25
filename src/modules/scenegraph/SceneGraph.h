/**
 * @file
 */

#pragma once

#include "SceneGraphNode.h"
#include "core/Pair.h"
#include "core/collection/DynamicArray.h"

namespace voxel {
class RawVolume;
}

namespace scenegraph {

using SceneGraphAnimationIds = core::DynamicArray<core::String>;

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
	int _activeNodeId = InvalidNodeId;
	SceneGraphAnimationIds _animations;
	core::String _activeAnimation;

	void updateTransforms_r(SceneGraphNode &node);

public:
	SceneGraph(int nodes = 262144);
	~SceneGraph();

	SceneGraph(SceneGraph&& other) noexcept;
	SceneGraph &operator=(SceneGraph &&move) noexcept;

	int activeNode() const;
	bool setActiveNode(int nodeId);
	/**
	 * @brief Returns the first valid palette from any of the nodes
	 */
	voxel::Palette &firstPalette() const;

	/**
	 * @brief Merge the palettes of all scene graph model nodes
	 * @param[in] removeUnused If the colors exceed the max palette colors, this will remove the unused colors
	 * besides merging similar colors.
	 * @note The resulting palette can be used to find similar colors in all nodes when e.g. a format only supports
	 * one palette for all nodes
	 * @param[in] emptyIndex Some formats can't e.g. use the first palette index because 0 indicates an empty voxel.
	 * Inform the merge process about skipping that voxel slot
	 */
	voxel::Palette mergePalettes(bool removeUnused, int emptyIndex = -1) const;

	/**
	 * @return The full region of the whole scene
	 */
	voxel::Region region() const;
	/**
	 * @return The region of the locked/grouped (model) nodes
	 * @note May return voxel::Region::InvalidRegion if the current active node is no model node
	 */
	voxel::Region groupRegion() const;

	/**
	 * @brief The list of known animation ids
	 */
	const SceneGraphAnimationIds &animations() const;
	bool addAnimation(const core::String &animation);
	bool removeAnimation(const core::String &animation);
	FrameIndex maxFrames(const core::String &animation) const;
	/**
	 * Checks if at least one of the nodes has multiple keyframes
	 */
	bool hasAnimations() const;

	/**
	 * @brief Change the active animation for all nodes to the given animation
	 * @note This must be called once all nodes are added
	 */
	bool setAnimation(const core::String &animation);
	const core::String &activeAnimation() const;

	void updateTransforms();

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
	SceneGraphNode* first();

	const SceneGraphNode& root() const;

	/**
	 * @brief Return the next model node in the group first, then continue the search outside the own group
	 */
	int nextModelNode(int nodeId) const;
	int prevModelNode(int nodeId) const;

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
	bool removeNode(int nodeId, bool recursive);
	bool changeParent(int nodeId, int newParentId);
	bool nodeHasChildren(const SceneGraphNode &node, int childId) const;
	bool canChangeParent(const SceneGraphNode &node, int newParentId) const;

	/**
	 * @brief Pre-allocated memory in the graph without adding the nodes
	 */
	void reserve(size_t size);
	/**
	 * @return whether the given node type isn't available in the current scene graph instance.
	 */
	bool empty(SceneGraphNodeType type = SceneGraphNodeType::Model) const;
	/**
	 * @return Amount of nodes in the graph
	 */
	size_t size(SceneGraphNodeType type = SceneGraphNodeType::Model) const;
	size_t nodeSize() const {
		return _nodes.size();
	}

	using MergedVolumePalette = core::Pair<voxel::RawVolume*, voxel::Palette>;
	/**
	 * @brief Merge all available nodes into one big volume.
	 * @note If the graph is empty, this returns @c nullptr for the volume and a dummy value for the palette
	 * @note The caller is responsible for deleting the returned volume
	 * @note The palette indices are just taken as they come in. There is no quantization here.
	 */
	MergedVolumePalette merge(bool transform = true) const;

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
				SceneGraphNodeType type = _sceneGraph->node(_startNodeId).type();
				if (type == _filter || _filter == SceneGraphNodeType::All) {
					break;
				} else if (_filter == SceneGraphNodeType::AllModels) {
					if (type == SceneGraphNodeType::Model || type == SceneGraphNodeType::ModelReference) {
						break;
					}
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
					SceneGraphNodeType type = _sceneGraph->node(_startNodeId).type();
					if (type == _filter || _filter == SceneGraphNodeType::All) {
						break;
					} else if (_filter == SceneGraphNodeType::AllModels) {
						if (type == SceneGraphNodeType::Model || type == SceneGraphNodeType::ModelReference) {
							break;
						}
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

	inline auto beginAll() const {
		return begin(SceneGraphNodeType::All);
	}

	inline auto beginAllModels() const {
		return begin(SceneGraphNodeType::AllModels);
	}

	inline auto end() const {
		return iterator(0, _nextNodeId, SceneGraphNodeType::Max, this);
	}

	/**
	 * @brief Loops over the locked/groups (model) nodes with the given function that receives the node id
	 */
	template<class FUNC>
	void foreachGroup(FUNC&& f) {
		int nodeId = activeNode();
		if (node(nodeId).locked()) {
			for (iterator iter = begin(SceneGraphNodeType::Model); iter != end(); ++iter) {
				if ((*iter).locked()) {
					f((*iter).id());
				}
			}
		} else {
			f(nodeId);
		}
	}

	/**
	 * @brief Loops over the child nodes
	 */
	template<class FUNC>
	void visitChildren(int nodeId, bool recursive, FUNC&& f) {
		if (!hasNode(nodeId)) {
			return;
		}
		const SceneGraphNodeChildren childrenCopy = node(nodeId).children();
		for (int childNodeId : childrenCopy) {
			if (hasNode(childNodeId)) {
				f(node(childNodeId));
				if (recursive) {
					visitChildren(childNodeId, recursive, f);
				}
			}
		}
	}
};

} // namespace voxel
