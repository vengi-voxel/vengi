/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "core/RGBA.h"
#include "core/String.h"
#include "core/ArrayLength.h"
#include "core/collection/Buffer.h"
#include "core/collection/StringMap.h"
#include "SceneGraphKeyFrame.h"

namespace voxel {
class RawVolume;
class Region;
}
namespace palette {
class Palette;
}

namespace scenegraph {

class SceneGraph;
class SceneGraphNode;
#define DEFAULT_ANIMATION "Default"

enum class SceneGraphNodeType : uint8_t {
	Root,
	Model,
	ModelReference,
	Group,
	Camera,
	Point, // a point in space with a transform and a name
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
	"Point",
	"Unknown"
};
static_assert((int)(scenegraph::SceneGraphNodeType::Max) == lengthof(SceneGraphNodeTypeStr), "Array sizes don't match Max");

using SceneGraphNodeChildren = const core::Buffer<int, 32>;
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
	SceneGraphNode(SceneGraphNodeType type = SceneGraphNodeType::Model, const core::String &uuid = "");
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
	glm::vec3 _pivot {0.0f};

	core::String _uuid;
	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	SceneGraphKeyFramesMap _keyFramesMap;
	SceneGraphKeyFrames *_keyFrames = nullptr;
	core::Buffer<int, 32> _children;
	SceneGraphNodeProperties _properties;
	mutable core::Optional<palette::Palette> _palette;

	/**
	 * @brief Called in emplace() if a parent id is given
	 */
	void setParent(int id);
	void setId(int id);
	void sortKeyFrames();

public:
	~SceneGraphNode();
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
	bool isAnyModelNode() const;
	bool isModelNode() const;
	bool isGroupNode() const;
	bool isRootNode() const;

	void fixErrors();
	bool validate() const;

	core::RGBA color() const;
	void setColor(core::RGBA color);

	int id() const;
	int parent() const;
	int reference() const;
	bool setReference(int nodeId, bool forceChangeNodeType = false);
	bool unreferenceModelNode(const SceneGraphNode &node);

	palette::Palette &palette() const;
	void setPalette(const palette::Palette &palette);

	// normalized pivot of [0-1] to be somewhere inside the volume region
	bool setPivot(const glm::vec3 &pivot);
	const glm::vec3 &pivot() const;
	glm::vec3 worldPivot() const;

	/**
	 * @brief Apply the given @c translation vector to all keyframe transform of this node
	 */
	void translate(const glm::vec3 &translation);
	/**
	 * @brief Set the given @c translation vector to all keyframe transform of this node
	 */
	void setTranslation(const glm::vec3 &translation, bool world = false);
	/**
	 * @brief Set the given @c rotation to all keyframe transform of this node
	 */
	void setRotation(const glm::quat &rotation, bool world = false);

	SceneGraphNodeType type() const;

	/**
	 * @brief A node is a leaf if it doesn't have any children
	 */
	bool isLeaf() const;
	/**
	 * @param[in] id The node id to add to the child relation
	 * @note This doesn't add the node itself to the graph
	 */
	bool addChild(int id);
	/**
	 * @param[in] id The node id to remove from the child relation
	 * @note This doesn't remove the node itself from the graph
	 */
	bool removeChild(int id);

	/**
	 * @note If this node is a reference node ( @c SceneGraphNodeType::ModelReference ) then this will return @c
	 * nullptr, too - use @c SceneGraph::resolveVolume() instead.
	 * @return voxel::RawVolume - might be @c nullptr
	 */
	const voxel::RawVolume *volume() const;
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
	voxel::Region remapToPalette(const palette::Palette &palette, int skipColorIndex = -1);
	/**
	 * @note If this node is a reference node ( @c SceneGraphNodeType::ModelReference ) then this will return an invalid
	 * region - use @c SceneGraph::resolveRegion() instead.
	 * @return voxel::Region instance that is invalid when the volume is not set for this instance.
	 */
	const voxel::Region &region() const;
	voxel::Region sceneRegion(const voxel::Region &volumeRegion, const glm::vec3 &pivot, KeyFrameIndex keyFrameIdx) const;
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @param transferOwnership this is @c true if the volume should get deleted by this class, @c false if
	 * you are going to manage the instance on your own.
	 */
	void setVolume(voxel::RawVolume *volume, bool transferOwnership);
	/**
	 * @param volume voxel::RawVolume instance. Might be @c nullptr.
	 * @note This will not take ownership of the volume instance
	 */
	void setVolume(const voxel::RawVolume *volume);

	// meta data

	const core::String &name() const;
	const core::String &uuid() const;
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

	/**
	 * @return @c false if no change was made to the properties. This can have the reason of exceeding the max allowed
	 * properties or that the key already has the given value. If the properties were changed, this returns @c true
	 */
	bool setProperty(const core::String& key, const char *value);
	bool setProperty(const core::String& key, bool value);
	bool setProperty(const core::String& key, float value);
	bool setProperty(const core::String& key, uint32_t value);
	bool setProperty(const core::String& key, core::RGBA value);
	bool setProperty(const core::String& key, const core::String& value);

	FrameIndex maxFrame() const;
	KeyFrameIndex addKeyFrame(FrameIndex frameIdx);
	bool hasKeyFrame(FrameIndex frameIdx) const;
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
	/**
	 * @brief Check that all key frames are valid. This basically means that they are sorted in the right order
	 */
	bool keyFramesValidate() const;
	bool hasActiveAnimation() const;
	bool addAnimation(const core::String &anim);
	bool removeAnimation(const core::String &anim);
	bool setAnimation(const core::String &anim);
	/**
	 * @brief Get the index of the keyframe for the given frame.
	 * Or if no direct key frame exists, the key frame that is interpolated towards
	 */
	KeyFrameIndex keyFrameForFrame(FrameIndex frameIdx) const;
	bool hasKeyFrameForFrame(FrameIndex frameIdx, KeyFrameIndex *existingIndex = nullptr) const;
	KeyFrameIndex previousKeyFrameForFrame(FrameIndex frameIdx) const;
	KeyFrameIndex nextKeyFrameForFrame(FrameIndex frameIdx) const;
	void setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform);
	SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0);
	const SceneGraphTransform &transform(KeyFrameIndex keyFrameIdx = 0) const;

	/**
	 * @note Only use this accessor if you know that the given key frame index exists
	 */
	SceneGraphKeyFrame &keyFrame(KeyFrameIndex keyFrameIdx);
	const SceneGraphKeyFrame *keyFrame(KeyFrameIndex keyFrameIdx) const;
};

/**
 * @ingroup SceneGraph
 */
class SceneGraphNodeCamera : public SceneGraphNode {
private:
	using Super = SceneGraphNode;
	// no members - just convenience methods
public:
	SceneGraphNodeCamera(const core::String &uuid = "");

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

inline const voxel::RawVolume *SceneGraphNode::volume() const {
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

inline const core::String &SceneGraphNode::uuid() const {
	return _uuid;
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
