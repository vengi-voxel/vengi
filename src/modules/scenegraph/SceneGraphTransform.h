/**
 * @file
 */

#pragma once

#include "SceneGraphAnimation.h"
#include "math/Axis.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"

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
	math::Mat4f _worldMat = math::Mat4f::identity();
	math::Mat4f _localMat = math::Mat4f::identity();

	math::Quatf _worldOrientation = math::Quatf::identity();
	math::Quatf _localOrientation = math::Quatf::identity();

	math::Vec3f _worldTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	math::Vec3f _worldScale{1.0f};

	math::Vec3f _localTranslation{0.0f};
	/**
	 * @brief Uniform scale value
	 */
	math::Vec3f _localScale{1.0f};

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
	void setTransforms(const math::Vec3f &worldTranslation, const math::Quatf &worldOrientation,
					   const math::Vec3f &worldScale, const math::Vec3f &localTranslation,
					   const math::Quatf &localOrientation, const math::Vec3f &localScale);

	void setWorldMatrix(const math::Mat4f &matrix);
	void setWorldTranslation(const math::Vec3f &translation);
	void setWorldOrientation(const math::Quatf &orientation);
	void setWorldScale(const math::Vec3f &scale);

	void setLocalMatrix(const math::Mat4f &matrix);
	void setLocalTranslation(const math::Vec3f &translation);
	void setLocalOrientation(const math::Quatf &orientation);
	void setLocalScale(const math::Vec3f &scale);

	void mirrorX();
	void mirrorXYZ();
	void mirrorXZ();
	void rotate(math::Axis axis);

	void lerp(const SceneGraphTransform &dest, double deltaFrameSeconds);

	math::Mat4f worldMatrix() const;
	const math::Vec3f &worldTranslation() const;
	math::Quatf worldOrientation() const;
	const math::Vec3f &worldScale() const;

	const math::Mat4f &localMatrix() const;
	const math::Vec3f &localTranslation() const;
	math::Quatf localOrientation() const;
	const math::Vec3f &localScale() const;

	const math::Mat4f calculateLocalMatrix() const;

	bool update(const SceneGraph &sceneGraph, SceneGraphNode &node, FrameIndex frameIdx, bool updateChildren);

	bool validate() const;

	/**
	 * @brief Uses the matrix to perform the transformation
	 * @note The matrix must be up-to-date
	 * @note The rotation is applied relatively to the given pivot - that's why we need the real size here.
	 */
	math::Vec3f apply(const math::Vec3f &pos, const math::Vec3f &pivot) const;
};

} // namespace scenegraph
