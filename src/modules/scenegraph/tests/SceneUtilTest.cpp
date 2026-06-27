/**
 * @file
 */

#include "scenegraph/SceneUtil.h"
#include "app/tests/AbstractTest.h"
#include "math/Ray.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace scenegraph {

class SceneUtilTest : public app::AbstractTest {};

TEST_F(SceneUtilTest, testCalcAdjacentRegionPositiveX) {
	const voxel::Region source(0, 0, 0, 3, 5, 7);
	const glm::ivec3 dims(4, 6, 8);
	const voxel::Region adjacent = calcAdjacentRegion(source, voxel::FaceNames::PositiveX, dims);
	EXPECT_EQ(4, adjacent.getLowerX());
	EXPECT_EQ(0, adjacent.getLowerY());
	EXPECT_EQ(0, adjacent.getLowerZ());
	EXPECT_EQ(7, adjacent.getUpperX());
	EXPECT_EQ(5, adjacent.getUpperY());
	EXPECT_EQ(7, adjacent.getUpperZ());
}

TEST_F(SceneUtilTest, testCalcAdjacentRegionNegativeZ) {
	const voxel::Region source(10, 10, 10, 11, 11, 11);
	const glm::ivec3 dims(2, 2, 2);
	const voxel::Region adjacent = calcAdjacentRegion(source, voxel::FaceNames::NegativeZ, dims);
	EXPECT_EQ(10, adjacent.getLowerX());
	EXPECT_EQ(10, adjacent.getLowerY());
	EXPECT_EQ(8, adjacent.getLowerZ());
	EXPECT_EQ(11, adjacent.getUpperX());
	EXPECT_EQ(11, adjacent.getUpperY());
	EXPECT_EQ(9, adjacent.getUpperZ());
}

TEST_F(SceneUtilTest, testTraceObbFacePositiveY) {
	const math::OBBF obb(glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 4.0f), glm::mat3(1.0f));
	const math::Ray ray(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	const ObbFaceHit hit = traceObbFace(obb, ray);
	ASSERT_NE(voxel::FaceNames::Max, hit.face);
	EXPECT_EQ(voxel::FaceNames::PositiveY, hit.face);
	EXPECT_GT(hit.distance, 0.0f);
}

TEST_F(SceneUtilTest, testCalcAdjacentObbPositiveXRotated) {
	const glm::mat3 rotMat(0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	const math::OBBF source(glm::vec3(0.0f), glm::vec3(2.0f, 2.0f, 2.0f), rotMat);
	const glm::vec3 newExtents(2.0f, 2.0f, 2.0f);
	const math::OBBF adjacent = calcAdjacentObb(source, voxel::FaceNames::PositiveX, newExtents);
	const glm::vec3 expectedOrigin = source.origin() + rotMat * glm::vec3(4.0f, 0.0f, 0.0f);
	EXPECT_VEC_NEAR(expectedOrigin, adjacent.origin(), 0.0001f);
	EXPECT_VEC_NEAR(newExtents, adjacent.extents(), 0.0001f);
	EXPECT_VEC_NEAR(rotMat[0], glm::mat3(adjacent.rotation())[0], 0.0001f);
	EXPECT_VEC_NEAR(rotMat[1], glm::mat3(adjacent.rotation())[1], 0.0001f);
	EXPECT_VEC_NEAR(rotMat[2], glm::mat3(adjacent.rotation())[2], 0.0001f);
}

TEST_F(SceneUtilTest, testAdjacentPreviewAabbSharesFourCorners) {
	const voxel::Region source(2, 4, 6, 5, 7, 9);
	const glm::ivec3 dims = source.getDimensionsInVoxels();
	const math::AABB<float> sourceAabb = toAABB(source);
	const voxel::FaceNames faces[] = {voxel::FaceNames::PositiveX, voxel::FaceNames::NegativeX,
									  voxel::FaceNames::PositiveY, voxel::FaceNames::NegativeY,
									  voxel::FaceNames::PositiveZ, voxel::FaceNames::NegativeZ};
	for (voxel::FaceNames face : faces) {
		const voxel::Region preview = calcAdjacentRegion(source, face, dims);
		ASSERT_TRUE(preview.isValid()) << "face " << (int)face;
		const math::AABB<float> previewAabb = toAABB(preview);
		EXPECT_EQ(4, countSharedAabbCorners(sourceAabb, previewAabb)) << "face " << (int)face;
	}
}

} // namespace scenegraph
