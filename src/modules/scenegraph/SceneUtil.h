/**
 * @file
 */

#include "math/AABB.h"
#include "math/OBB.h"
#include "math/Ray.h"

#include "scenegraph/FrameTransform.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace scenegraph {

math::OBBF toOBB(bool sceneMode, const voxel::Region &region, const glm::vec3 &normalizedPivot,
					   const scenegraph::FrameTransform &transform);
math::AABB<float> toAABB(const voxel::Region &region);
math::AABB<float> toAABB(const math::OBBF &obb);
voxel::Region toRegion(const math::AABB<float> &aabb);
voxel::Region toRegion(const math::OBBF &obb);

/**
 * @brief Calculate the world region for a new node placed adjacent to @c sourceWorld on @c face
 */
voxel::Region calcAdjacentRegion(const voxel::Region &sourceWorld, voxel::FaceNames face,
								 const glm::ivec3 &newDims);

/**
 * @brief Calculate the world OBB for a new node placed adjacent to @c source on @c face
 */
math::OBBF calcAdjacentObb(const math::OBBF &source, voxel::FaceNames face, const glm::vec3 &newExtents);

struct ObbFaceHit {
	voxel::FaceNames face = voxel::FaceNames::Max;
	float distance = -1.0f;
	glm::vec3 hitPoint = glm::vec3(0.0f);
};

/**
 * @brief Trace a ray against an OBB and return the hit face in OBB local space
 */
ObbFaceHit traceObbFace(const math::OBBF &obb, const math::Ray &ray);

/**
 * @brief Fill @c quad with the four world-space corners of an OBB face
 */
void obbFaceQuad(const math::OBBF &obb, voxel::FaceNames face, glm::vec3 quad[4]);

/**
 * @brief Count how many corner vertices two axis-aligned bounding boxes share
 */
int countSharedAabbCorners(const math::AABB<float> &a, const math::AABB<float> &b);

} // namespace scenegraph
