/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/StringMap.h"
#include "voxel/Region.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace voxel {

class RawVolume;

enum class SceneGraphNodeType {
	Root,
	Model,
	ModelReference,
	Transform,
	Group,
	Camera,
	Unknown,

	Max
};

/**
 * @brief Struct that holds the metadata and the volume
 * @sa SceneGraph
 */
class SceneGraphNode {
	friend class SceneGraph;
public:
	SceneGraphNode(SceneGraphNodeType type = SceneGraphNodeType::Model) : _type(type) {
	}
	SceneGraphNode(SceneGraphNode &&move) noexcept;
	SceneGraphNode &operator=(SceneGraphNode &&move) noexcept;

protected:
	int _id = -1;
	int _parent = 0;
	int _referencedNodeId = -1;
	SceneGraphNodeType _type;
	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	glm::mat4x4 _mat {1.0f};
	/**
	 * this will ensure that we are releasing the volume memory in this instance
	 * @sa release()
	 */
	bool _volumeOwned = true;
	bool _visible = true;
	bool _locked = false;
	glm::ivec3 _pivot{0};
	core::Buffer<int, 32> _children;
	core::StringMap<core::String> _properties;

	/**
	 * @brief Called in emplace() if a parent id is given
	 */
	void setParent(int id);

public:
	/**
	 * @brief Releases the memory of the volume instance (only if owned).
	 */
	void release();
	/**
	 * @brief Release the ownership without freeing the memory
	 */
	void releaseOwnership();
	bool owns() const;

	int id() const;
	void setId(int id);
	int parent() const;
	SceneGraphNodeType type() const;

	void addChild(int id);
	const glm::mat4 matrix() const;
	void setMatrix(const glm::mat4x4 &mat);

	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume() const;
	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume();
	/**
	 * For SceneGraphNodeType::ModelReference
	 */
	int referencedNodeId() const;
	void setReferencedNodeId(int nodeId);
	/**
	 * @return voxel::Region instance that is invalid when the volume is not set for this instance.
	 */
	const voxel::Region &region() const;
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @param transferOwnership this is @c true if the volume should get deleted by this class, @c false if
	 * you are going to manage the instance on your own.
	 */
	void setVolume(voxel::RawVolume *volume, bool transferOwnership);
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @param transferOwnership this is @c true if the volume should get deleted by this class, @c false if
	 * you are going to manage the instance on your own.
	 */
	void setVolume(const voxel::RawVolume *volume, bool transferOwnership);

	void translate(const glm::ivec3 &v);

	// meta data

	const core::String &name() const;
	void setName(const core::String &name);
	bool visible() const;
	void setVisible(bool visible);
	bool locked() const;
	void setLocked(bool locked);
	const glm::ivec3 &pivot() const;
	void setPivot(const glm::ivec3 &pivot);

	const core::Buffer<int, 32> &children() const;
	const core::StringMap<core::String> &properties() const;
	core::StringMap<core::String> &properties();
	core::String property(const core::String& key) const;
	void addProperties(const core::StringMap<core::String>& map);
	void setProperty(const core::String& key, const core::String& value);
};

inline int SceneGraphNode::referencedNodeId() const {
	return _referencedNodeId;
}

inline void SceneGraphNode::setReferencedNodeId(int nodeId) {
	_referencedNodeId = nodeId;
}

inline bool SceneGraphNode::owns() const {
	return _volume;
}

inline void SceneGraphNode::setId(int id) {
	_id = id;
}

inline int SceneGraphNode::parent() const {
	return _parent;
}

inline void SceneGraphNode::setParent(int id) {
	_parent = id;
}

inline SceneGraphNodeType SceneGraphNode::type() const {
	return _type;
}

inline int SceneGraphNode::id() const {
	return _id;
}

inline voxel::RawVolume *SceneGraphNode::volume() const {
	return _volume;
}

inline voxel::RawVolume *SceneGraphNode::volume() {
	return _volume;
}

inline const core::String &SceneGraphNode::name() const {
	return _name;
}

inline void SceneGraphNode::setName(const core::String &name) {
	_name = name;
}

inline bool SceneGraphNode::visible() const {
	return _visible;
}

inline void SceneGraphNode::setVisible(bool visible) {
	_visible = visible;
}

inline bool SceneGraphNode::locked() const {
	return _locked;
}

inline void SceneGraphNode::setLocked(bool locked) {
	_locked = locked;
}

inline const glm::ivec3 &SceneGraphNode::pivot() const {
	return _pivot;
}

inline void SceneGraphNode::setPivot(const glm::ivec3 &pivot) {
	_pivot = pivot;
}

inline const glm::mat4 SceneGraphNode::matrix() const {
	return _mat;
}

inline void SceneGraphNode::setMatrix(const glm::mat4x4 &mat) {
	_mat = mat;
}

} // namespace voxel
