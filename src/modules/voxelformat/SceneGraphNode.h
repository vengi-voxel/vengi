/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "voxel/Region.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace voxel {
class RawVolume;
}

namespace voxelformat {

enum class SceneGraphNodeType {
	Root,
	Model,
	Group,
	Camera,
	Unknown,

	Max
};

struct SceneGraphTransform {
	// should be the normalized value between 0 and 1
	glm::vec3 normalizedPivot{0.0f};
	glm::vec3 position{0.0f};
	glm::quat rot{glm::quat_identity<float, glm::defaultp>()};
	float scale{1.0f};
	glm::mat4x4 mat{1.0f};

	void print() const;
	void update();
	void updateFromMat();
	glm::vec3 apply(const glm::vec3 &pos, const glm::vec3 &size) const;
};

struct SceneGraphKeyFrame {
	uint32_t frame = 0;
	SceneGraphTransform transform;
};

/**
 * @brief Struct that holds the metadata and the volume
 * @sa SceneGraph
 */
class SceneGraphNode {
	friend class SceneGraph;
public:
	SceneGraphNode(SceneGraphNodeType type = SceneGraphNodeType::Model);
	SceneGraphNode(SceneGraphNode &&move) noexcept;
	SceneGraphNode &operator=(SceneGraphNode &&move) noexcept;

protected:
	int _id = -1;
	int _parent = 0;
	SceneGraphNodeType _type;
	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	core::DynamicArray<SceneGraphKeyFrame> _keyFrames;
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
	~SceneGraphNode() { release(); }
	/**
	 * @brief Releases the memory of the volume instance (only if owned).
	 */
	void release();
	/**
	 * @brief Release the ownership without freeing the memory
	 */
	void releaseOwnership();
	bool owns() const;

	const core::DynamicArray<SceneGraphKeyFrame> &keyFrames() const;
	void setKeyFrames(const core::DynamicArray<SceneGraphKeyFrame>&);

	int id() const;
	void setId(int id);
	int parent() const;

	SceneGraphNodeType type() const;

	bool addChild(int id);
	bool removeChild(int id);
	const glm::mat4 &matrix(uint8_t frameIdx = 0) const;
	void setTransform(uint8_t frameIdx, const SceneGraphTransform &transform, bool updateMatrix);
	SceneGraphTransform &transform(uint8_t frameIdx = 0);
	const SceneGraphTransform &transform(uint8_t frameIdx = 0) const;

	SceneGraphKeyFrame &keyFrame(uint8_t frameIdx);

	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume() const;
	/**
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume();
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
	const glm::vec3 &normalizedPivot(uint8_t frameIdx = 0) const;
	void setPivot(uint8_t frameIdx, const glm::ivec3 &pos, const glm::ivec3 &size);
	void setNormalizedPivot(uint8_t frameIdx, const glm::vec3 &pivot);

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

inline SceneGraphKeyFrame& SceneGraphNode::keyFrame(uint8_t frameIdx) {
	if ((int)_keyFrames.size() <= frameIdx) {
		_keyFrames.resize((int)frameIdx + 1);
	}
	return _keyFrames[frameIdx];
}

inline SceneGraphTransform& SceneGraphNode::transform(uint8_t frameIdx) {
	return _keyFrames[frameIdx].transform;
}

inline const SceneGraphTransform& SceneGraphNode::transform(uint8_t frameIdx) const {
	return _keyFrames[frameIdx].transform;
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

inline const glm::vec3 &SceneGraphNode::normalizedPivot(uint8_t frameIdx) const {
	return _keyFrames[frameIdx].transform.normalizedPivot;
}

inline const glm::mat4 &SceneGraphNode::matrix(uint8_t frameIdx) const {
	return _keyFrames[frameIdx].transform.mat;
}

} // namespace voxel
