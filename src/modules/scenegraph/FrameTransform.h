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

// TODO: PERF: this should get cached in the SceneGraph for one frameIdx
//             see SceneGraph::transformForFrame() - this method is called a lot
//             if your scenegraph is large
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
	const glm::mat4 &worldMatrix() const;
	glm::mat4 calculateWorldMatrix(const glm::vec3 &normalizedPivot, const glm::vec3 &dimensions) const;

	glm::vec3 translation() const;
	const glm::vec3 &scale() const;
	void decompose(glm::vec3 &scale, glm::quat &orientation, glm::vec3 &translation) const;
};

glm::vec3 calculateExtents(const glm::vec3 &dimensions);

} // namespace scenegraph
