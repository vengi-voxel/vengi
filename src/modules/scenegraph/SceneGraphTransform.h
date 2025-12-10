/**
 * @file
 */

#pragma once

#include "SceneGraphAnimation.h"
#include "math/Axis.h"
#include <glm/gtc/quaternion.hpp>

namespace scenegraph {

class SceneGraph;
class SceneGraphNode;
/**
 * @brief The node transformation
 * @note This needs a call to @c update() to apply the chances that were made by the setters. Not doing so will trigger
 * asserts.
 * @note You can't modify local and world transforms at the same time.
 * @ingroup SceneGraph
 */
class alignas(16) SceneGraphTransform {
private:
	enum { DIRTY_WORLDVALUES = 1 << 0, DIRTY_LOCALVALUES = 1 << 1, DIRTY_PARENT = 1 << 2 };
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

	void markClean() {
		_dirty = 0u;
	}

	void markDirtyParent() {
		_dirty = DIRTY_PARENT;
	}

	/**
	 * @brief This method will set all values into the transform without the need to perform any
	 * @c update() call. It's assumed, that all values for world and local transformations are valid
	 */
	void setTransforms(const glm::vec3 &worldTranslation, const glm::quat &worldOrientation,
					   const glm::vec3 &worldScale, const glm::vec3 &localTranslation,
					   const glm::quat &localOrientation, const glm::vec3 &localScale);

	void setWorldMatrix(const glm::mat4x4 &matrix);
	void setWorldTranslation(const glm::vec3 &translation);
	void setWorldOrientation(const glm::quat &orientation);
	void setWorldScale(const glm::vec3 &scale);

	void setLocalMatrix(const glm::mat4x4 &matrix);
	void setLocalTranslation(const glm::vec3 &translation);
	void setLocalOrientation(const glm::quat &orientation);
	void setLocalScale(const glm::vec3 &scale);

	void mirrorX();
	void mirrorXYZ();
	void mirrorXZ();
	void rotate(math::Axis axis);

	void lerp(const SceneGraphTransform &dest, double deltaFrameSeconds);

	const glm::mat4x4 &worldMatrix() const;
	const glm::vec3 &worldTranslation() const;
	const glm::quat &worldOrientation() const;
	const glm::vec3 &worldScale() const;

	const glm::mat4x4 &localMatrix() const;
	const glm::vec3 &localTranslation() const;
	const glm::quat &localOrientation() const;
	const glm::vec3 &localScale() const;

	const glm::mat4x4 calculateLocalMatrix() const;

	bool update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx, bool updateChildren);

	bool validate() const;

	/**
	 * @brief Uses the matrix to perform the transformation
	 * @note The matrix must be up-to-date
	 * @note The rotation is applied relatively to the given pivot - that's why we need the real size here.
	 */
	glm::vec3 apply(const glm::vec3 &pos, const glm::vec3 &pivot) const;
};

} // namespace scenegraph
