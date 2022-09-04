/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/Optional.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/ArrayLength.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "voxel/Palette.h"
#include "voxel/Region.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace voxel {
class RawVolume;
}

namespace voxelformat {

class SceneGraph;
class SceneGraphNode;
using FrameIndex = uint32_t;
using KeyFrameIndex = uint32_t;

enum class SceneGraphNodeType : uint8_t {
	Root,
	Model,
	Group,
	Camera,
	Unknown,

	Max
};

class alignas(16) SceneGraphTransform {
private:
	enum {
		DIRTY_PIVOT = 1 << 0,
		DIRTY_WORLDTRANSLATION = 1 << 1,
		DIRTY_WORLDORIENTATION = 1 << 2,
		DIRTY_WORLDSCALE = 1 << 3,
		DIRTY_WORLDMATRIX = 1 << 4,
		DIRTY_LOCALTRANSLATION = 1 << 5,
		DIRTY_LOCALORIENTATION = 1 << 6,
		DIRTY_LOCALSCALE = 1 << 7,
		DIRTY_LOCALMATRIX = 1 << 8
	};
	/**
	 * @brief The model matrix that is assembled by the translation, orientation and scale value
	 */
	glm::mat4x4 _worldMat{1.0f};
	glm::mat4x4 _localMat{1.0f};

	glm::quat _worldOrientation{glm::quat_identity<float, glm::defaultp>()};
	glm::quat _localOrientation{glm::quat_identity<float, glm::defaultp>()};

	glm::vec3 _worldTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	float _worldScale = 1.0f;

	glm::vec3 _localTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	float _localScale = 1.0f;

	// should be the normalized value between 0 and 1
	glm::vec3 _normalizedPivot{0.0f};

	uint32_t _dirty = 0u;

	void updateLocal(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx);
	void updateWorld();
public:
	void setPivot(const glm::vec3 &normalizedPivot);

	inline bool dirty() const {
		return _dirty != 0u;
	}

	void setWorldMatrix(const glm::mat4x4 &matrix);
	void setWorldTranslation(const glm::vec3 &translation);
	void setWorldOrientation(const glm::quat& orientation);
	void setWorldScale(float scale);

	void setLocalMatrix(const glm::mat4x4 &matrix);
	void setLocalTranslation(const glm::vec3 &translation);
	void setLocalOrientation(const glm::quat& orientation);
	void setLocalScale(float scale);

	void lerp(const SceneGraphTransform &dest, double deltaFrameSeconds);

	const glm::vec3 &pivot() const;

	const glm::mat4x4 &worldMatrix() const;
	const glm::vec3 &worldTranslation() const;
	const glm::quat &worldOrientation() const;
	float worldScale() const;

	const glm::mat4x4 &localMatrix() const;
	const glm::vec3 &localTranslation() const;
	const glm::quat &localOrientation() const;
	float localScale() const;

	void update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx);

	void updateFromWorldMatrix();

	/**
	 * @brief Uses the matrix to perform the transformation
	 * @note The matrix must be up-to-date
	 * @note The rotation is applied relatively to the given pivot - that's why we need the real size here.
	 */
	glm::vec3 apply(const glm::vec3 &pos, const glm::vec3 &size) const;
};

enum class InterpolationType : uint8_t {
	Instant = 0,
	Linear = 1,
	QuadEaseIn = 2,
	QuadEaseOut = 3,
	QuadEaseInOut = 4,
	CubicEaseIn = 5,
	CubicEaseOut = 6,
	CubicEaseInOut = 7,
	Max
};

static constexpr const char *InterpolationTypeStr[] {
	"Instant",
	"Linear",
	"QuadEaseIn",
	"QuadEaseOut",
	"QuadEaseInOut",
	"CubicEaseIn",
	"CubicEaseOut",
	"CubicEaseInOut"
};
static_assert(core::enumVal(voxelformat::InterpolationType::Max) == lengthof(InterpolationTypeStr), "Array sizes don't match Max");

class SceneGraphKeyFrame {
private:
	SceneGraphTransform _transform;
public:
	FrameIndex frame = 0;
	InterpolationType interpolation = InterpolationType::Linear;
	bool longRotation = false;

	inline void setTransform(const SceneGraphTransform &transform) {
		_transform = transform;
	}

	inline SceneGraphTransform &transform() {
		return _transform;
	}

	inline const SceneGraphTransform &transform() const {
		return _transform;
	}
};

using SceneGraphNodeChildren = const core::Buffer<int, 32>;
using SceneGraphKeyFrames = core::DynamicArray<SceneGraphKeyFrame>;

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
	/**
	 * this will ensure that we are releasing the volume memory in this instance
	 * @sa release()
	 */
	static constexpr uint8_t VolumeOwned = 1 << 0;
	static constexpr uint8_t Visible = 1 << 1;
	static constexpr uint8_t Locked = 1 << 2;

	int _id = -1;
	int _parent = 0;
	SceneGraphNodeType _type;
	uint8_t _flags = 0u;
	core::RGBA _color;

	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	SceneGraphKeyFrames _keyFrames;
	core::Buffer<int, 32> _children;
	core::StringMap<core::String> _properties;
	core::Optional<voxel::Palette> _palette;

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

	core::RGBA color() const;
	void setColor(core::RGBA color);

	FrameIndex maxFrame() const;

	bool addKeyFrame(FrameIndex frame);
	bool removeKeyFrame(FrameIndex frame);
	void sortKeyFrames();
	const SceneGraphKeyFrames &keyFrames() const;
	bool setKeyFrames(const SceneGraphKeyFrames&);
	/**
	 * @brief Get the index of the keyframe for the given frame
	 */
	uint32_t keyFrameForFrame(FrameIndex frame) const;

	int id() const;
	void setId(int id);
	int parent() const;
	const voxel::Palette &palette() const;
	voxel::Palette &palette();
	void setPalette(const voxel::Palette &palette);

	SceneGraphNodeType type() const;

	bool addChild(int id);
	bool removeChild(int id);
	void setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform);
	SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0);
	const SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0) const;

	/**
	 * @brief Interpolates the transforms for the given frame. It searches the keyframe before and after
	 * the given input frame and interpolates according to the given delta frames between the particular
	 * keyframes.
	 */
	SceneGraphTransform transformForFrame(FrameIndex frame) const;

	SceneGraphKeyFrame &keyFrame(KeyFrameIndex frameIdx);

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

	void translate(const glm::ivec3 &v, int frame = -1);

	// meta data

	const core::String &name() const;
	void setName(const core::String &name);
	bool visible() const;
	void setVisible(bool visible);
	bool locked() const;
	void setLocked(bool locked);
	void setPivot(FrameIndex frameIdx, const glm::ivec3 &pos, const glm::ivec3 &size);

	const SceneGraphNodeChildren &children() const;
	const core::StringMap<core::String> &properties() const;
	core::StringMap<core::String> &properties();
	core::String property(const core::String& key) const;
	float propertyf(const core::String& key) const;
	void addProperties(const core::StringMap<core::String>& map);

	bool setProperty(const core::String& key, const char *value);
	bool setProperty(const core::String& key, bool value);
	bool setProperty(const core::String& key, const core::String& value);
};

class SceneGraphNodeCamera : public SceneGraphNode {
private:
	using Super = SceneGraphNode;
	// no members - just convenience methods
public:
	SceneGraphNodeCamera();

	int fieldOfView() const;
	void setFieldOfView(int val);

	float farPlane() const;
	void setFarPlane(float val);

	float nearPlane() const;
	void setNearPlane(float val);

	bool isOrthographic() const;
	void setOrthographic();

	bool isPerspective() const;
	void setPerspective();
};
static_assert(sizeof(SceneGraphNodeCamera) == sizeof(SceneGraphNode), "Sizes must match - direct casting is performed");

inline SceneGraphNodeCamera& toCameraNode(SceneGraphNode& node) {
	core_assert(node.type() == SceneGraphNodeType::Camera);
	return (SceneGraphNodeCamera&)node;
}

inline const SceneGraphNodeCamera& toCameraNode(const SceneGraphNode& node) {
	core_assert(node.type() == SceneGraphNodeType::Camera);
	return (const SceneGraphNodeCamera&)node;
}

inline bool SceneGraphNode::owns() const {
	return _volume;
}

inline core::RGBA SceneGraphNode::color() const {
	return _color;
}

inline void SceneGraphNode::setColor(core::RGBA color) {
	_color = color;
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
	if (_type != SceneGraphNodeType::Model) {
		return nullptr;
	}
	return _volume;
}

inline voxel::RawVolume *SceneGraphNode::volume() {
	if (_type != SceneGraphNodeType::Model) {
		return nullptr;
	}
	return _volume;
}

inline const core::String &SceneGraphNode::name() const {
	return _name;
}

inline void SceneGraphNode::setName(const core::String &name) {
	_name = name;
}

inline bool SceneGraphNode::visible() const {
	return _flags & Visible;
}

inline void SceneGraphNode::setVisible(bool visible) {
	if (visible) {
		_flags |= Visible;
	} else {
		_flags &= ~Visible;
	}
}

inline bool SceneGraphNode::locked() const {
	return _flags & Locked;
}

inline void SceneGraphNode::setLocked(bool locked) {
	if (locked) {
		_flags |= Locked;
	} else {
		_flags &= ~Locked;
	}

}

} // namespace voxel
