/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/StringMap.h"
#include "voxel/Region.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxel {

class RawVolume;

enum class SceneGraphNodeType {
	Root,
	Model,
	ModelReference,
	Group,
	Camera,
	Unknown,

	Max
};

struct SceneGraphTransform {
	// should be the normalized value between 0 and 1
	glm::vec3 normalizedPivot{0.0f};
	glm::vec3 position{0.0f};
	glm::quat rot{1.0f, 0.0f, 0.0f, 0.0f};
	float scale{1.0f};
	glm::mat4x4 mat{1.0f};

	void print() const;
	void update();
	void updateFromMat();
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
	int _modelId = -1;
	int _referencedNodeId = -1;
	SceneGraphNodeType _type;
	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	SceneGraphTransform _transform;
	/**
	 * this will ensure that we are releasing the volume memory in this instance
	 * @sa release()
	 */
	bool _volumeOwned = true;
	bool _visible = true;
	bool _locked = false;
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

	int modelId() const;
	void setModelId(int id);

	SceneGraphNodeType type() const;

	bool addChild(int id);
	bool removeChild(int id);
	const glm::mat4 matrix() const;
	void setTransform(const SceneGraphTransform &transform, bool updateMatrix);
	const SceneGraphTransform& transform() const;
	SceneGraphTransform& transform();

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
	/**
	 * @brief Return the normalized pivot between 0.0 and 1.0
	 */
	const glm::vec3 &normalizedPivot() const;
	void setPivot(const glm::ivec3 &pos, const glm::ivec3 &size);
	void setNormalizedPivot(const glm::vec3 &pivot);

	const core::Buffer<int, 32> &children() const;
	const core::StringMap<core::String> &properties() const;
	core::StringMap<core::String> &properties();
	core::String property(const core::String& key) const;
	void addProperties(const core::StringMap<core::String>& map);

	void setProperty(const core::String& key, const char *value) {
		_properties.put(key, value);
	}

	void setProperty(const core::String& key, bool value) {
		_properties.put(key, core::string::toString(value));
	}

	void setProperty(const core::String& key, const core::String& value) {
		_properties.put(key, value);
	}
};

inline const SceneGraphTransform& SceneGraphNode::transform() const {
	return _transform;
}

inline SceneGraphTransform& SceneGraphNode::transform() {
	return _transform;
}

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

inline int SceneGraphNode::modelId() const {
	return _modelId;
}

inline void SceneGraphNode::setModelId(int id) {
	_modelId = id;
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

inline const glm::vec3 &SceneGraphNode::normalizedPivot() const {
	return _transform.normalizedPivot;
}

inline const glm::mat4 SceneGraphNode::matrix() const {
	return _transform.mat;
}

} // namespace voxel
