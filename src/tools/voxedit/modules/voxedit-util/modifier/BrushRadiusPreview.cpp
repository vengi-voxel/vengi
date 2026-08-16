/**
 * @file
 */

#include "BrushRadiusPreview.h"
#include "math/Axis.h"
#include "voxelutil/VolumeSelect.h"
#include <limits>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

namespace voxedit {

void buildSurfaceRadiusOutline(const voxel::RawVolume *volume, const glm::ivec3 &center, voxel::FaceNames face,
							   int radius, core::Buffer<glm::vec3> &out) {
	out.clear();
	if (volume == nullptr || radius < 0) {
		return;
	}
	voxel::FaceNames outlineFace = face;
	if (outlineFace == voxel::FaceNames::Max) {
		outlineFace = voxel::FaceNames::PositiveY;
	}
	const math::Axis faceAxis = voxel::faceToAxis(outlineFace);
	const int wAxis = math::getIndexForAxis(faceAxis);
	const int uAxis = (wAxis + 1) % 3;
	const int vAxis = (wAxis + 2) % 3;
	const bool positiveNormal = voxel::isPositiveFace(outlineFace);
	const voxel::Region &region = volume->region();
	const int refW = center[wAxis];
	const int tolerance = glm::max(radius * 2 + 4, 8);
	const int segments = glm::clamp(radius * 8 + 24, 24, 96);
	const float r = (float)radius + 0.5f;
	const glm::vec3 faceN = voxel::faceNormal(outlineFace);

	glm::ivec3 lastVoxel(std::numeric_limits<int>::min());
	for (int i = 0; i < segments; ++i) {
		const float angle = (float)i / (float)segments * glm::two_pi<float>();
		const int u = center[uAxis] + (int)glm::round(glm::cos(angle) * r);
		const int v = center[vAxis] + (int)glm::round(glm::sin(angle) * r);
		int w = refW;
		if (!voxelutil::findSurfaceNear(*volume, u, v, refW, tolerance, uAxis, vAxis, wAxis, positiveNormal, region,
										w)) {
			w = refW;
		}
		glm::ivec3 voxelPos(0);
		voxelPos[uAxis] = u;
		voxelPos[vAxis] = v;
		voxelPos[wAxis] = w;
		if (voxelPos == lastVoxel) {
			continue;
		}
		lastVoxel = voxelPos;
		out.push_back(glm::vec3(voxelPos) + 0.5f + faceN * 0.51f);
	}
}

} // namespace voxedit
