/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

namespace voxel {
class Region;
}

namespace scenegraph {

// TODO: PERF: this should get cached in the SceneGraph for one frameIdx
//             see SceneGraph::transformForFrame() - this method is called a lot
//             if your scenegraph is large
class FrameTransform {
private:
	mutable glm::vec3 _scale;
	mutable glm::mat4 _inverseMatrix;
	mutable bool _scaleCalculated = false;
	mutable bool _inverseCalculated = false;
	glm::mat4 _matrix{1.0f};

	void resetCache() {
		_scaleCalculated = false;
		_inverseCalculated = false;
	}

public:
	void setWorldMatrix(const glm::mat4 &m);
	const glm::mat4 &worldMatrix() const;
	glm::mat4 calculateWorldMatrix(const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions) const;
	glm::vec3 calcWorldNormal(const glm::vec3 &normal) const;
	glm::mat3 calcNormalMatrix() const;

	glm::vec3 translation() const;
	glm::vec3 scale() const;
	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;

	/**
	 * @brief Calculate the transformed position for the given input
	 * @param[in] pos The position in model/object space
	 * @param[in] pivot The pivot in model/object space
	 */
	glm::vec3 calcPosition(const glm::vec3 &pos, const glm::vec3 &pivot) const;
	glm::vec3 calcModelSpace(const glm::vec3 &worldPos) const;
};

glm::vec3 calculateExtents(const glm::vec3 &dimensions);

} // namespace scenegraph
