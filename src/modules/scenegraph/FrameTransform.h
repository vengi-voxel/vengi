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
struct FrameTransform {
	glm::mat4 matrix;

	void setWorldMatrix(const glm::mat4 &m) {
		matrix = m;
	}
	inline const glm::mat4 &worldMatrix() const {
		return matrix;
	}

	glm::vec3 translation() const;
	// TODO: PERF: cache this scale value, as the decompose() method isn't really cheap
	glm::vec3 scale() const;
	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;
};

glm::vec3 calculateWorldPivot(const FrameTransform &transform, const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions);
glm::vec3 calculateExtents(const glm::vec3 &dimensions);
glm::vec3 calculateCenter(const FrameTransform &transform, const glm::vec3 &worldPivot, const glm::vec3 &regionCenter);

} // namespace scenegraph
