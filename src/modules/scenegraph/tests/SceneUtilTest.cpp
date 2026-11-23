/**
 * @file
 */

#include "scenegraph/SceneUtil.h"
#include "app/tests/AbstractTest.h"
#include "math/Ray.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "math/Mat3f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace scenegraph {

class SceneUtilTest : public app::AbstractTest {};

TEST_F(SceneUtilTest, testCalcAdjacentRegionPositiveX) {
	const voxel::Region source(0, 0, 0, 3, 5, 7);
	const math::Vec3i dims(4, 6, 8);
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
	const math::Vec3i dims(2, 2, 2);
	const voxel::Region adjacent = calcAdjacentRegion(source, voxel::FaceNames::NegativeZ, dims);
	EXPECT_EQ(10, adjacent.getLowerX());
	EXPECT_EQ(10, adjacent.getLowerY());
	EXPECT_EQ(8, adjacent.getLowerZ());
	EXPECT_EQ(11, adjacent.getUpperX());
	EXPECT_EQ(11, adjacent.getUpperY());
	EXPECT_EQ(9, adjacent.getUpperZ());
}

TEST_F(SceneUtilTest, testTraceObbFacePositiveY) {
	const math::OBBF obb(math::Vec3f(0.0f), math::Vec3f(2.0f, 3.0f, 4.0f), math::Mat3f(1.0f));
	const math::Ray ray(math::Vec3f(0.0f, 10.0f, 0.0f), math::Vec3f(0.0f, -1.0f, 0.0f));
	const ObbFaceHit hit = traceObbFace(obb, ray);
	ASSERT_NE(voxel::FaceNames::Max, hit.face);
	EXPECT_EQ(voxel::FaceNames::PositiveY, hit.face);
	EXPECT_GT(hit.distance, 0.0f);
}

TEST_F(SceneUtilTest, testCalcAdjacentObbPositiveXRotated) {
	const math::Mat3f rotMat(0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	const math::OBBF source(math::Vec3f(0.0f), math::Vec3f(2.0f, 2.0f, 2.0f), rotMat);
	const math::Vec3f newExtents(2.0f, 2.0f, 2.0f);
	const math::OBBF adjacent = calcAdjacentObb(source, voxel::FaceNames::PositiveX, newExtents);
	const math::Vec3f expectedOrigin = source.origin() + rotMat * math::Vec3f(4.0f, 0.0f, 0.0f);
	EXPECT_VEC_NEAR(expectedOrigin, adjacent.origin(), 0.0001f);
	EXPECT_VEC_NEAR(newExtents, adjacent.extents(), 0.0001f);
	EXPECT_VEC_NEAR(rotMat[0], math::Mat3f(adjacent.rotation())[0], 0.0001f);
	EXPECT_VEC_NEAR(rotMat[1], math::Mat3f(adjacent.rotation())[1], 0.0001f);
	EXPECT_VEC_NEAR(rotMat[2], math::Mat3f(adjacent.rotation())[2], 0.0001f);
}

TEST_F(SceneUtilTest, testAdjacentPreviewAabbSharesFourCorners) {
	const voxel::Region source(2, 4, 6, 5, 7, 9);
	const math::Vec3i dims = source.getDimensionsInVoxels();
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
