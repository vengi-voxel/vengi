/**
 * @file
 */

#include "voxelutil/VolumeSelect.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelutil {

class VolumeSelectTest : public app::AbstractTest {};

static void fillBox(voxel::RawVolume &volume, const voxel::Region &r, const voxel::Voxel &v) {
	const glm::ivec3 &lo = r.getLowerCorner();
	const glm::ivec3 &hi = r.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				volume.setVoxel(x, y, z, v);
			}
		}
	}
}

TEST_F(VolumeSelectTest, testLineMarkSolidDiagonal) {
	voxel::Region region(-5, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillBox(volume, voxel::Region(glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2)), solid);

	int marked = 0;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) {
		EXPECT_EQ(x, y);
		EXPECT_EQ(y, z);
		++marked;
	};
	lineMarkSolid(volume, glm::ivec3(-2, -2, -2), glm::ivec3(2, 2, 2), 1, region, markFunc);
	EXPECT_EQ(marked, 5);
}

TEST_F(VolumeSelectTest, testLineDrapeSurfaceAcrossStep) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x <= 10; ++x) {
		const int top = x <= 5 ? 10 : 9;
		for (int z = 0; z <= 2; ++z) {
			for (int y = 0; y <= top; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	core::DynamicSet<glm::ivec3, 257, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lineDrapeSurface(volume, glm::ivec3(0, 10, 1), glm::ivec3(10, 9, 1), 0, 2, 1, true, 1, region, markFunc);

	EXPECT_TRUE(selected.has(glm::ivec3(5, 10, 1)));
	EXPECT_TRUE(selected.has(glm::ivec3(6, 9, 1)));
	EXPECT_TRUE(selected.has(glm::ivec3(10, 9, 1)));
	EXPECT_FALSE(selected.has(glm::ivec3(5, 5, 1)));
}

} // namespace voxelutil
