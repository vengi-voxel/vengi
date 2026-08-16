/**
 * @file
 */

#include "voxedit-util/modifier/BrushRadiusPreview.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/geometric.hpp>

namespace voxedit {

class BrushRadiusPreviewTest : public app::AbstractTest {};

TEST_F(BrushRadiusPreviewTest, testFlatSurfaceOutline) {
	voxel::Region region(0, 0, 0, 20, 2, 20);
	voxel::RawVolume volume(region);
	for (int z = 0; z <= 20; ++z) {
		for (int x = 0; x <= 20; ++x) {
			volume.setVoxel(x, 0, z, voxel::Voxel(voxel::VoxelType::Generic, 1));
		}
	}

	core::Buffer<glm::vec3> points;
	const glm::ivec3 center(10, 0, 10);
	buildSurfaceRadiusOutline(&volume, center, voxel::FaceNames::PositiveY, 4, points);
	ASSERT_GE(points.size(), 8u);

	for (const glm::vec3 &p : points) {
		EXPECT_NEAR(p.y, 1.01f, 0.02f) << "Outline should sit on top of the flat surface";
		const float dx = p.x - 10.5f;
		const float dz = p.z - 10.5f;
		const float dist = glm::length(glm::vec2(dx, dz));
		EXPECT_NEAR(dist, 4.5f, 1.25f) << "Outline samples should stay near the brush radius";
	}
}

TEST_F(BrushRadiusPreviewTest, testNullVolume) {
	core::Buffer<glm::vec3> points;
	buildSurfaceRadiusOutline(nullptr, glm::ivec3(0), voxel::FaceNames::PositiveY, 3, points);
	EXPECT_TRUE(points.empty());
}

} // namespace voxedit
