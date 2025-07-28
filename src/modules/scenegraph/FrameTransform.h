/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

namespace scenegraph {

// TODO: PERF: this should get cached in the SceneGraph for one frameIdx
//             see SceneGraph::transformForFrame() - this method is called a lot
//             if your scenegraph is large
class FrameTransform {
private:
	mutable glm::vec3 _scale;
	mutable bool _scaleCalculated = false;
	glm::mat4 matrix;
public:
	FrameTransform() : matrix(1.0f) {
	}
	void setWorldMatrix(const glm::mat4 &m) {
		matrix = m;
		_scaleCalculated = false;
	}
	inline const glm::mat4 &worldMatrix() const {
		return matrix;
	}

	glm::vec3 translation() const;
	glm::vec3 scale() const;
	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;

	/**
	 * @brief Calulate the pivot in object space
	 * @param[in] dimensions The dimensions of the object
	 * @param[in] normalizedPivot The pivot in the normalized range (usually [0, 1])
	 */
	glm::vec3 calcPivot(const glm::vec3 &dimensions, const glm::vec3 &normalizedPivot) const;
	/**
	 * @brief Calculate the transformed position for the given input
	 * @param[in] pos The position in object space
	 * @param[in] pivot The pivot in object space (see @c calcPivot() for details)
	 */
	glm::vec3 calcPosition(const glm::vec3 &pos, const glm::vec3 &pivot) const;
};

glm::vec3 calculateExtents(const glm::vec3 &dimensions);
glm::vec3 calculateCenter(const FrameTransform &transform, const glm::vec3 &worldPivot, const glm::vec3 &regionCenter);

} // namespace scenegraph
