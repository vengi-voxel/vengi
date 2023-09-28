/**
 * @file
 */

#include "voxelutil/VolumeVisitor.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"

namespace voxelutil {

class VolumeVisitorTest : public app::AbstractTest {};

TEST_F(VolumeVisitorTest, testVisitSurface) {
	const voxel::Region region(0, 2);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			for (int z = 0; z < 3; ++z) {
				volume.setVoxel(x, y, z, voxel);
			}
		}
	}

	int cnt = visitSurfaceVolume(
		volume, [&](int, int, int, const voxel::Voxel &) {}, VisitorOrder::XZY);
	EXPECT_EQ(26, cnt);
}

TEST_F(VolumeVisitorTest, testVisitSurfaceCorners) {
	const voxel::Region region(0, 2);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 2, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 2, voxel));

	EXPECT_TRUE(volume.setVoxel(0, 2, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 0, voxel));

	int cnt = visitSurfaceVolume(
		volume, [&](int, int, int, const voxel::Voxel &) {}, VisitorOrder::XZY);
	EXPECT_EQ(8, cnt);
}

TEST_F(VolumeVisitorTest, testVisitVisibleSurface) {
	const voxel::Region region(0, 0, 0, 3, 5, 3);
	const voxel::Voxel voxel1 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel voxel2 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel voxel3 = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(1, 1, 1, voxel1));
	EXPECT_TRUE(volume.setVoxel(1, 2, 1, voxel2));
	EXPECT_TRUE(volume.setVoxel(1, 3, 1, voxel3));

	int idx = 0;
	int cnt = visitSurfaceVolume(
		volume, [&](int, int, int, const voxel::Voxel &voxel) {
			if (idx == 0) {
				EXPECT_EQ(1, voxel.getColor());
			} else if (idx == 1) {
				EXPECT_EQ(2, voxel.getColor());
			} else if (idx == 2) {
				EXPECT_EQ(3, voxel.getColor());
			}
			++idx;
		}, VisitorOrder::XZY);
	EXPECT_EQ(3, cnt);

	idx = 0;
	cnt = visitSurfaceVolume(
		volume, [&](int, int, int, const voxel::Voxel &voxel) {
			if (idx == 0) {
				EXPECT_EQ(3, voxel.getColor());
			} else if (idx == 1) {
				EXPECT_EQ(2, voxel.getColor());
			} else if (idx == 2) {
				EXPECT_EQ(1, voxel.getColor());
			}
			++idx;
		}, VisitorOrder::XZmY);
	EXPECT_EQ(3, cnt);
}

class VolumeVisitorParamTest :
		public app::AbstractTest,
		public ::testing::WithParamInterface<VisitorOrder> {
};

class VolumeVisitorOrderTest : public VolumeVisitorParamTest {};

TEST_P(VolumeVisitorOrderTest, testVisitor) {
	const voxel::Region region(0, 200);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 2, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 2, voxel));

	EXPECT_TRUE(volume.setVoxel(0, 2, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 0, voxel));

	int cnt = visitVolume(
		volume, [&](int, int, int, const voxel::Voxel &) {}, SkipEmpty(), GetParam());
	EXPECT_EQ(8, cnt);
}

inline ::std::ostream& operator<<(::std::ostream& os, const VisitorOrder& state) {
	static const char *strings[] = {
		"XYZ",
		"ZYX",
		"ZXY",
		"XmZY",
		"mXZY",
		"mXmZY",
		"mXZmY",
		"XmZmY",
		"mXmZmY",
		"XZY",
		"XZmY",
		"YXZ",
		"YZX",
		"mYZX",
		"YZmX"
	};
	static_assert(lengthof(strings) == (int)VisitorOrder::Max, "Array size mismatch");
	return os << "order[" << strings[(int)state] << " - " << (int)state << "]";
}

INSTANTIATE_TEST_SUITE_P(VisitorOrder, VolumeVisitorOrderTest,
						 ::testing::Values(
						VisitorOrder::XYZ,
						VisitorOrder::ZYX,
						VisitorOrder::ZXY,
						VisitorOrder::XmZY,
						VisitorOrder::mXZY,
						VisitorOrder::mXmZY,
						VisitorOrder::mXZmY,
						VisitorOrder::XmZmY,
						VisitorOrder::mXmZmY,
						VisitorOrder::XZY,
						VisitorOrder::XZmY,
						VisitorOrder::YXZ,
						VisitorOrder::YZX,
						VisitorOrder::mYZX,
						VisitorOrder::YZmX));

} // namespace voxelutil
