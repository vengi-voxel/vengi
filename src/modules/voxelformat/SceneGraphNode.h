/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxel {

class RawVolume;

/**
 * @brief Struct that holds the metadata and the volume
 * @sa SceneGraph
 */
class SceneGraphNode {
public:
	SceneGraphNode() {
	}
	SceneGraphNode(voxel::RawVolume &volume, const core::String &name = "", bool visible = true);
	SceneGraphNode(voxel::RawVolume *volume, const core::String &name = "", bool visible = true);
	SceneGraphNode(voxel::RawVolume *volume, const core::String &name, bool visible, const glm::ivec3 &pivot);
	SceneGraphNode(const voxel::RawVolume *volume, const core::String &name = "", bool visible = true);
	SceneGraphNode(const voxel::RawVolume *volume, const core::String &name, bool visible, const glm::ivec3 &pivot);
	SceneGraphNode(SceneGraphNode &&move) noexcept;
	SceneGraphNode &operator=(SceneGraphNode &&move) noexcept;

protected:
	core::String _name;
	voxel::RawVolume *_volume = nullptr;
	/**
	 * this will ensure that we are releasing the volume memory in this instance
	 * @sa release()
	 */
	bool _volumeOwned = true;
	bool _visible = true;
	glm::ivec3 _pivot{0};

public:
	/**
	 * @brief Releases the memory of the volume instance (only if owned).
	 */
	void release();

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
	const glm::ivec3 &pivot() const;
	void setPivot(const glm::ivec3 &pivot);
};

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

inline const glm::ivec3 &SceneGraphNode::pivot() const {
	return _pivot;
}

inline void SceneGraphNode::setPivot(const glm::ivec3 &pivot) {
	_pivot = pivot;
}

} // namespace voxel
