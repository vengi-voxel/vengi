/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "core/RGBA.h"
#include "core/String.h"
#include "core/ArrayLength.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "voxel/Palette.h"

#include <glm/gtc/quaternion.hpp>

namespace voxel {
class RawVolume;
class Region;
class Palette;
}

namespace scenegraph {

class SceneGraph;
class SceneGraphNode;
using FrameIndex = int32_t;
using KeyFrameIndex = int32_t;
#define InvalidKeyFrame ((scenegraph::KeyFrameIndex)-1)
#define InvalidFrame ((scenegraph::FrameIndex)-1)
#define DEFAULT_ANIMATION "Default"

enum class SceneGraphNodeType : uint8_t {
	Root,
	Model,
	ModelReference,
	Group,
	Camera,
	Unknown,

	Max,

	AllModels, // Fake type for the iterator
	All
};

// these identifiers are using in the vengi format for the different node types
// if you change these, VENGIFormat might need a migration path
static constexpr const char* SceneGraphNodeTypeStr[] {
	"Root",
	"Model",
	"ModelReference",
	"Group",
	"Camera",
	"Unknown"
};
static_assert((int)(scenegraph::SceneGraphNodeType::Max) == lengthof(SceneGraphNodeTypeStr), "Array sizes don't match Max");

/**
 * @brief The node transformation
 * @note This needs a call to @c update() to apply the chances that were made by the setters. Not doing so will trigger
 * asserts.
 * @note You can't modify local and world transforms at the same time.
 * @ingroup SceneGraph
 */
class alignas(16) SceneGraphTransform {
private:
	enum {
		DIRTY_WORLDVALUES = 1 << 0,
		DIRTY_LOCALVALUES = 1 << 1,
		DIRTY_PARENT = 1 << 2
	};
	/**
	 * @brief The model matrix that is assembled by the translation, orientation and scale value
	 */
	glm::mat4x4 _worldMat{1.0f};
	glm::mat4x4 _localMat{1.0f};

	glm::quat _worldOrientation;
	glm::quat _localOrientation;

	glm::vec3 _worldTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	glm::vec3 _worldScale{1.0f};

	glm::vec3 _localTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	glm::vec3 _localScale{1.0f};

	// indicated which values were changed
	uint32_t _dirty = 0u;

public:
	SceneGraphTransform();

	inline bool dirty() const {
		return _dirty != 0u;
	}

	/**
	 * @brief This method will set all values into the transform without the need to perform any
	 * @c update() call. It's assumed, that all values for world and local transformations are valid
	 */
	void setTransforms(const glm::vec3 &worldTranslation, const glm::quat &worldOrientation, const glm::vec3 &worldScale,
					   const glm::vec3 &localTranslation, const glm::quat &localOrientation, const glm::vec3 &localScale);

	void setWorldMatrix(const glm::mat4x4 &matrix);
	void setWorldTranslation(const glm::vec3 &translation);
	void setWorldOrientation(const glm::quat& orientation);
	void setWorldScale(const glm::vec3 &scale);

	void setLocalMatrix(const glm::mat4x4 &matrix);
	void setLocalTranslation(const glm::vec3 &translation);
	void setLocalOrientation(const glm::quat& orientation);
	void setLocalScale(const glm::vec3 &scale);

	void lerp(const SceneGraphTransform &dest, double deltaFrameSeconds);

	const glm::mat4x4 &worldMatrix() const;
	const glm::vec3 &worldTranslation() const;
	const glm::quat &worldOrientation() const;
	const glm::vec3 &worldScale() const;

	const glm::mat4x4 &localMatrix() const;
	const glm::vec3 &localTranslation() const;
	const glm::quat &localOrientation() const;
	const glm::vec3 &localScale() const;

	void update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx);

	/**
	 * @brief Uses the matrix to perform the transformation
	 * @note The matrix must be up-to-date
	 * @note The rotation is applied relatively to the given pivot - that's why we need the real size here.
	 */
	glm::vec3 apply(const glm::vec3 &pos, const glm::vec3 &pivot) const;
};

/**
 * @ingroup SceneGraph
 */
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
static_assert(int(scenegraph::InterpolationType::Max) == lengthof(InterpolationTypeStr), "Array sizes don't match Max");

/**
 * @ingroup SceneGraph
 */
class SceneGraphKeyFrame {
private:
	SceneGraphTransform _transform;
public:
	FrameIndex frameIdx = 0;
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
using SceneGraphKeyFramesMap = core::StringMap<SceneGraphKeyFrames>;
using SceneGraphNodeProperties = core::StringMap<core::String>;

#define InvalidNodeId (-1)

/**
 * @brief Struct that holds the metadata and the volume
 * @sa SceneGraph
 * @ingroup SceneGraph
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

	int _id = InvalidNodeId;
	int _parent = 0;
	int _referenceId = InvalidNodeId;
	SceneGraphNodeType _type;
	uint8_t _flags = 0u;
	core::RGBA _color;

	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	SceneGraphKeyFramesMap _keyFramesMap;
	SceneGraphKeyFrames *_keyFrames = nullptr;
	core::Buffer<int, 32> _children;
	SceneGraphNodeProperties _properties;
	mutable core::Optional<voxel::Palette> _palette;

	/**
	 * @brief Called in emplace() if a parent id is given
	 */
	void setParent(int id);
	void setId(int id);
	void sortKeyFrames();

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

	bool isReference() const;
	bool isReferenceable() const;
	bool isModelNode() const;

	core::RGBA color() const;
	void setColor(core::RGBA color);

	int id() const;
	int parent() const;
	int reference() const;
	bool setReference(int nodeId);
	bool unreferenceModelNode(const SceneGraphNode &node);

	voxel::Palette &palette() const;
	void setPalette(const voxel::Palette &palette);

	bool setPivot(const glm::vec3 &pivot);
	const glm::vec3 &pivot() const;
	glm::vec3 worldPivot() const;

	/**
	 * @brief Apply the given @c translation vector to all keyframe transform of this node
	 */
	void translate(const glm::vec3 &translation);

	SceneGraphNodeType type() const;

	bool isLeaf() const;
	bool addChild(int id);
	bool removeChild(int id);

	/**
	 * @note If this node is a reference node ( @c SceneGraphNodeType::ModelReference ) then this will return @c
	 * nullptr, too - use @c SceneGraph::resolveVolume() instead.
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume() const;
	/**
	 * @note If this node is a reference node ( @c SceneGraphNodeType::ModelReference ) then this will return @c
	 * nullptr, too - use @c SceneGraph::resolveVolume() instead.
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	voxel::RawVolume *volume();
	/**
	 * @brief Remaps the voxel colors to the new given palette
	 * @note The palette is not set by this method - you have to call @c setPalette() on your own.
	 */
	voxel::Region remapToPalette(const voxel::Palette &palette, int skipColorIndex = -1);
	/**
	 * @note If this node is a reference node ( @c SceneGraphNodeType::ModelReference ) then this will return an invalid
	 * region - use @c SceneGraph::resolveRegion() instead.
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
	 */
	void setVolume(const voxel::RawVolume *volume);

	// meta data

	const core::String &name() const;
	core::String &name();
	void setName(const core::String &name);
	bool visible() const;
	void setVisible(bool visible);
	bool locked() const;
	void setLocked(bool locked);

	const SceneGraphNodeChildren &children() const;
	const core::StringMap<core::String> &properties() const;
	core::StringMap<core::String> &properties();
	core::String property(const core::String& key) const;
	float propertyf(const core::String& key) const;
	void addProperties(const core::StringMap<core::String>& map);

	bool setProperty(const core::String& key, const char *value);
	bool setProperty(const core::String& key, bool value);
	bool setProperty(const core::String& key, const core::String& value);

	FrameIndex maxFrame(const core::String &animation) const;
	KeyFrameIndex addKeyFrame(FrameIndex frameIdx);
	bool removeKeyFrame(FrameIndex frameIdx);
	bool removeKeyFrameByIndex(KeyFrameIndex keyFrameIdx);
	const SceneGraphKeyFrames &keyFrames() const;
	const SceneGraphKeyFrames &keyFrames(const core::String &anim) const;
	bool duplicateKeyFrames(const core::String &fromAnimation, const core::String &toAnimation);
	/**
	 * @sa hasActiveAnimation()
	 */
	SceneGraphKeyFrames *keyFrames();
	/**
	 * @brief Set the keyframes for the current active animation
	 */
	bool setKeyFrames(const SceneGraphKeyFrames&);
	void setAllKeyFrames(const SceneGraphKeyFramesMap &map, const core::String &animation);
	const SceneGraphKeyFramesMap &allKeyFrames() const;
	SceneGraphKeyFramesMap &allKeyFrames();
	bool hasActiveAnimation() const;
	bool addAnimation(const core::String &anim);
	bool removeAnimation(const core::String &anim);
	bool setAnimation(const core::String &anim);
	/**
	 * @brief Get the index of the keyframe for the given frame
	 */
	KeyFrameIndex keyFrameForFrame(FrameIndex frameIdx) const;
	void setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform);
	SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0);
	const SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0) const;

	/**
	 * @brief Interpolates the transforms for the given frame. It searches the keyframe before and after
	 * the given input frame and interpolates according to the given delta frames between the particular
	 * keyframes.
	 */
	SceneGraphTransform transformForFrame(FrameIndex frameIdx) const;
	SceneGraphTransform transformForFrame(const core::String &animation, FrameIndex frameIdx) const;
	SceneGraphTransform transformForFrame(const SceneGraphKeyFrames &kfs, FrameIndex frameIdx) const;

	/**
	 * @note Only use this accessor if you know that the given key frame index exists
	 */
	SceneGraphKeyFrame &keyFrame(KeyFrameIndex keyFrameIdx);
};

/**
 * @ingroup SceneGraph
 */
class SceneGraphNodeCamera : public SceneGraphNode {
private:
	using Super = SceneGraphNode;
	// no members - just convenience methods
public:
	SceneGraphNodeCamera();

	static constexpr const char *Modes[] = {"orthographic", "perspective"};
	static constexpr const char *PropMode = "cam_mode";
	static constexpr const char *PropNearPlane = "cam_nearplane";
	static constexpr const char *PropFarPlane = "cam_farplane";
	static constexpr const char *PropWidth = "cam_width";
	static constexpr const char *PropHeight = "cam_height";
	static constexpr const char *PropAspect = "cam_aspect";
	static constexpr const char *PropFov = "cam_fov";

	static bool isFloatProperty(const core::String &key) {
		 return key == PropNearPlane || key == PropFarPlane || key == PropAspect;
	}

	static bool isIntProperty(const core::String &key) {
		 return key == PropHeight || key == PropWidth || key == PropFov;
	}

	/**
	 * @brief Field of view in degree
	 */
	int fieldOfView() const;
	void setFieldOfView(int val);

	/**
	 * aspect ratio (width over height) of the field of view, or the aspect ratio of the viewport
	 */
	void setAspectRatio(float val);
	float aspectRatio() const;

	int width() const;
	void setWidth(int val);
	int height() const;
	void setHeight(int val);

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

inline core::String &SceneGraphNode::name() {
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
