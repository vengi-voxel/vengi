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

	ASSERT_TRUE(view[0].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[1].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[2].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[3].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[5].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[6].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[7].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[8].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[9].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[10].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[11].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[12].isSameType(voxel::createVoxel(VoxelType::Air, 0)));

	const glm::ivec3 expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	ASSERT_TRUE(v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4)).isSame(expectedVoxel));
	ASSERT_TRUE(view[4].isSame(expectedVoxel)) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
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

	ASSERT_TRUE(view[0].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[1].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[2].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[3].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[5].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[6].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[7].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[8].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[9].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[10].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[11].isSameType(voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_TRUE(view[12].isSameType(voxel::createVoxel(VoxelType::Air, 0)));

	const glm::ivec3 expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	ASSERT_TRUE(v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4)).isSame(expectedVoxel));
	ASSERT_TRUE(view[4].isSame(expectedVoxel)) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
}

} // namespace voxel
