/**
 * @file
 */

#include "voxel/RawVolumeView.h"
#include "VoxelPrinter.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"

namespace voxel {

class RawVolumeViewTest : public app::AbstractTest {
protected:
	bool pageIn(RawVolume &v) {
		for (int x = v.region().getLowerX(); x <= v.region().getUpperX(); ++x) {
			for (int y = v.region().getLowerY(); y <= v.region().getUpperY(); ++y) {
				for (int z = v.region().getLowerZ(); z <= v.region().getUpperZ(); ++z) {
					v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Generic, 0));
				}
			}
		}
		return true;
	}
};

TEST_F(RawVolumeViewTest, testOperator) {
	RawVolume v({0, 10});
	pageIn(v);
	voxel::Region viewRegion(4, 2, 0, 7, 4, 0);
	for (int x = viewRegion.getLowerX(); x <= viewRegion.getUpperX(); ++x) {
		for (int y = viewRegion.getLowerY(); y <= viewRegion.getUpperY(); ++y) {
			for (int z = viewRegion.getLowerZ(); z <= viewRegion.getUpperZ(); ++z) {
				v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Air, 0));
			}
		}
	}
	const glm::ivec3 pos{viewRegion.getLowerX(), viewRegion.getUpperY() - 1, viewRegion.getLowerZ()};
	const voxel::Voxel expectedVoxel = voxel::createVoxel(VoxelType::Generic, 1);
	v.setVoxel(pos, expectedVoxel);

	RawVolumeView view(&v, viewRegion);

	ASSERT_EQ(view.viewPosFromIndex(0), glm::ivec3(0, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(1), glm::ivec3(1, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(2), glm::ivec3(2, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(3), glm::ivec3(3, 0, 0));

	ASSERT_EQ(view[0], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[1], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[2], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[3], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[5], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[6], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[7], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[8], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[9], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[10], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[11], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[12], voxel::createVoxel(VoxelType::Air, 0));

	const glm::ivec3 expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	ASSERT_EQ(v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4)), expectedVoxel);
	ASSERT_EQ(view[4], expectedVoxel) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
}

TEST_F(RawVolumeViewTest, testOperatorWithNegativeRegion) {
	RawVolume v({-10, 10});
	pageIn(v);
	voxel::Region viewRegion(-7, -4, 0, -4, -2, 0);
	for (int x = viewRegion.getLowerX(); x <= viewRegion.getUpperX(); ++x) {
		for (int y = viewRegion.getLowerY(); y <= viewRegion.getUpperY(); ++y) {
			for (int z = viewRegion.getLowerZ(); z <= viewRegion.getUpperZ(); ++z) {
				v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Air, 0));
			}
		}
	}
	const glm::ivec3 pos{viewRegion.getLowerX(), viewRegion.getUpperY() - 1, viewRegion.getLowerZ()};
	const voxel::Voxel expectedVoxel = voxel::createVoxel(VoxelType::Generic, 1);
	v.setVoxel(pos, expectedVoxel);

	RawVolumeView view(&v, viewRegion);

	ASSERT_EQ(view.viewPosFromIndex(0), glm::ivec3(0, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(1), glm::ivec3(1, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(2), glm::ivec3(2, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(3), glm::ivec3(3, 0, 0));

	ASSERT_EQ(view[0], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[1], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[2], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[3], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[5], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[6], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[7], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[8], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[9], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[10], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[11], voxel::createVoxel(VoxelType::Air, 0));
	ASSERT_EQ(view[12], voxel::createVoxel(VoxelType::Air, 0));

	const glm::ivec3 expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	ASSERT_EQ(v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4)), expectedVoxel);
	ASSERT_EQ(view[4], expectedVoxel) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
}

} // namespace voxel
