/**
 * @file
 */

#include "voxel/RawVolumeView.h"
#include "VoxelPrinter.h"
#include "app/tests/AbstractTest.h"
#include "math/tests/TestMathHelper.h"
#include "math/Vec3i.h"

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
				ASSERT_TRUE(v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Air, 0)));
			}
		}
	}
	const math::Vec3i pos{viewRegion.getLowerX(), viewRegion.getUpperY() - 1, viewRegion.getLowerZ()};
	const voxel::Voxel expectedVoxel = voxel::createVoxel(VoxelType::Generic, 1);
	ASSERT_TRUE(v.setVoxel(pos, expectedVoxel));

	RawVolumeView view(&v, viewRegion);

	ASSERT_EQ(view.viewPosFromIndex(0), math::Vec3i(0, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(1), math::Vec3i(1, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(2), math::Vec3i(2, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(3), math::Vec3i(3, 0, 0));

	ASSERT_TRUE(voxel::isAir(view[0].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[1].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[2].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[3].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[5].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[6].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[7].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[8].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[9].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[10].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[11].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[12].getMaterial()));

	const math::Vec3i expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	const voxel::Voxel& actualVoxel = v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4));
	ASSERT_TRUE(actualVoxel.isSame(expectedVoxel)) << actualVoxel << " vs " << expectedVoxel;
	ASSERT_TRUE(view[4].isSame(expectedVoxel)) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
}

TEST_F(RawVolumeViewTest, testOperatorWithNegativeRegion) {
	RawVolume v({-10, 10});
	pageIn(v);
	voxel::Region viewRegion(-7, -4, 0, -4, -2, 0);
	for (int x = viewRegion.getLowerX(); x <= viewRegion.getUpperX(); ++x) {
		for (int y = viewRegion.getLowerY(); y <= viewRegion.getUpperY(); ++y) {
			for (int z = viewRegion.getLowerZ(); z <= viewRegion.getUpperZ(); ++z) {
				ASSERT_TRUE(v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Air, 0)));
			}
		}
	}
	const math::Vec3i pos{viewRegion.getLowerX(), viewRegion.getUpperY() - 1, viewRegion.getLowerZ()};
	const voxel::Voxel expectedVoxel = voxel::createVoxel(VoxelType::Generic, 1);
	ASSERT_TRUE(v.setVoxel(pos, expectedVoxel));

	RawVolumeView view(&v, viewRegion);

	ASSERT_EQ(view.viewPosFromIndex(0), math::Vec3i(0, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(1), math::Vec3i(1, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(2), math::Vec3i(2, 0, 0));
	ASSERT_EQ(view.viewPosFromIndex(3), math::Vec3i(3, 0, 0));

	ASSERT_TRUE(voxel::isAir(view[0].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[1].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[2].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[3].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[5].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[6].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[7].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[8].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[9].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[10].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[11].getMaterial()));
	ASSERT_TRUE(voxel::isAir(view[12].getMaterial()));

	const math::Vec3i expectedPos(0, 1, 0);
	ASSERT_EQ(view.viewPosFromIndex(4), expectedPos);
	ASSERT_EQ(viewRegion.getLowerCorner() + view.viewPosFromIndex(4), pos);
	const voxel::Voxel& actualVoxel = v.voxel(viewRegion.getLowerCorner() + view.viewPosFromIndex(4));
	ASSERT_TRUE(actualVoxel.isSame(expectedVoxel)) << actualVoxel << " vs " << expectedVoxel;
	ASSERT_TRUE(view[4].isSame(expectedVoxel)) << "pos in volume: " << pos << ", pos in view: " << expectedPos;
}

} // namespace voxel
