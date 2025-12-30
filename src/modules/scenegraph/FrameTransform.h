/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/type_aligned.hpp>

namespace voxel {
class Region;
}

namespace scenegraph {

class FrameTransform {
private:
	mutable glm::vec3 _scale;
	mutable bool _scaleCalculated = false;
	glm::mat4 _matrix{1.0f};

	void resetCache() {
		_scaleCalculated = false;
	}

public:
	void setWorldMatrix(const glm::mat4 &m);
	/**
	 * @brief World matrix without pivot offset
	 * @sa calculateWorldMatrix()
	 */
	const glm::mat4 &worldMatrix() const;
	/**
	 * @brief World translation without pivot offset
	 */
	glm::vec3 worldTranslation() const;
	const glm::vec3 &worldScale() const;

	/**
	 * @brief World matrix with pivot offset
	 * @sa worldMatrix()
	 */
	glm::mat4 calculateWorldMatrix(const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions) const;

	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;
};

glm::vec3 calculateExtents(const glm::vec3 &dimensions);

} // namespace scenegraph
