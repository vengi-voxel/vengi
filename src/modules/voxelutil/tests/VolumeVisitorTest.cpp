/**
 * @file
 */

#include "voxelutil/VolumeVisitor.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

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

	int cnt = visitSurfaceVolume(volume, [&](int, int, int, const voxel::Voxel &) {}, VisitorOrder::XZY);
	EXPECT_EQ(26, cnt);
}

TEST_F(VolumeVisitorTest, testVisitFaces) {
	const voxel::Region region(0, 2);
	voxel::RawVolume volume(region);
	for (int i = 0; i < 6; ++i) {
		const voxel::FaceNames faceName = (voxel::FaceNames)i;
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, i);
		const math::Axis axis = voxel::faceToAxis(faceName);
		const int idx = math::getIndexForAxis(axis);
		glm::ivec3 pos;
		if (voxel::isNegativeFace(faceName)) {
			pos[idx] = region.getLowerCorner()[idx];
		} else {
			pos[idx] = region.getUpperCorner()[idx];
		}
		for (int j = 0; j < 3; ++j) {
			for (int k = 0; k < 3; ++k) {
				pos[(idx + 1) % 3] = j;
				pos[(idx + 2) % 3] = k;
				volume.setVoxel(pos, voxel);
			}
		}
	}
	for (int i = 0; i < 6; ++i) {
		const voxel::FaceNames faceName = (voxel::FaceNames)i;
		int expectedColorFound = 0;
		auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			if (voxel.getColor() == i) {
				++expectedColorFound;
			}
		};
		const int cnt = visitFace(volume, faceName, visitor);
		EXPECT_EQ(9, cnt) << "Did not visit all voxels on face " << voxel::faceNameString(faceName);
		EXPECT_GE(expectedColorFound, 1) << "Did not find expected color on face " << voxel::faceNameString(faceName);
	}
}

TEST_F(VolumeVisitorTest, testVisitFacesSurface) {
	const voxel::Region region(0, 16);
	voxel::RawVolume volume(region);
	{
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		for (int x = 0; x <= 16; ++x) {
			for (int z = 0; z <= 16; ++z) {
				for (int y = 0; y < z; ++y) {
					volume.setVoxel({x, y, z}, voxel);
				}
			}
		}
	}
	voxel::Region visitRegion(0, 0, 0, 16, 16, 3);
	const voxel::FaceNames faceName = voxel::FaceNames::Front;
	int expectedVoxelVisit = 0;
	auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		if (!voxel::isAir(voxel.getMaterial())) {
			++expectedVoxelVisit;
		}
	};
	visitFace(volume, visitRegion, faceName, visitor, true);
	EXPECT_GE(expectedVoxelVisit, 3 * (16 + 1)) << "Did not find expected color on face " << voxel::faceNameString(faceName);
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

	int cnt = visitSurfaceVolume(volume, [&](int, int, int, const voxel::Voxel &) {}, VisitorOrder::XZY);
	EXPECT_EQ(8, cnt);
}

TEST_F(VolumeVisitorTest, testVisitConnectedByVoxel) {
	const voxel::Region region(0, 0, 0, 3, 5, 3);
	voxel::RawVolume volume(region);
	const voxel::Voxel voxel1 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel voxel2 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel voxel3 = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	EXPECT_TRUE(volume.setVoxel(1, 0, 1, voxel1));
	EXPECT_TRUE(volume.setVoxel(1, 1, 1, voxel1));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, voxel1));
	EXPECT_TRUE(volume.setVoxel(1, 2, 1, voxel2));
	EXPECT_TRUE(volume.setVoxel(1, 3, 1, voxel3));

	int cnt = visitConnectedByVoxel(volume, {1, 1, 1}, [&](int, int, int, const voxel::Voxel &) {});
	EXPECT_EQ(3, cnt);
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
		volume,
		[&](int, int, int, const voxel::Voxel &voxel) {
			if (idx == 0) {
				EXPECT_EQ(1, voxel.getColor());
			} else if (idx == 1) {
				EXPECT_EQ(2, voxel.getColor());
			} else if (idx == 2) {
				EXPECT_EQ(3, voxel.getColor());
			}
			++idx;
		},
		VisitorOrder::XZY);
	EXPECT_EQ(3, cnt);

	idx = 0;
	cnt = visitSurfaceVolume(
		volume,
		[&](int, int, int, const voxel::Voxel &voxel) {
			if (idx == 0) {
				EXPECT_EQ(3, voxel.getColor());
			} else if (idx == 1) {
				EXPECT_EQ(2, voxel.getColor());
			} else if (idx == 2) {
				EXPECT_EQ(1, voxel.getColor());
			}
			++idx;
		},
		VisitorOrder::XZmY);
	EXPECT_EQ(3, cnt);
}

class VolumeVisitorParamTest : public app::AbstractTest, public ::testing::WithParamInterface<VisitorOrder> {};

class VolumeVisitorOrderTest : public VolumeVisitorParamTest {};

TEST_P(VolumeVisitorOrderTest, testVisitor) {
	const voxel::Region region(-1, 4);
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

	int cnt = visitVolume(volume, [&](int, int, int, const voxel::Voxel &) {}, SkipEmpty(), GetParam());
	EXPECT_EQ(8, cnt);
	int parallelCnt = visitVolumeParallel(volume, [&](int, int, int, const voxel::Voxel &) {}, SkipEmpty(), GetParam());
	EXPECT_EQ(8, parallelCnt);
}

inline ::std::ostream& operator<<(::std::ostream& os, const VisitorOrder& state) {
	return os << "order[" << VisitorOrderStr[(int)state] << " - " << (int)state << "]";
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
						VisitorOrder::YmXZ,
						VisitorOrder::mYZX,
						VisitorOrder::YZmX,
						VisitorOrder::mYmXZ,
						VisitorOrder::mYXmZ,
						VisitorOrder::mYmZmX,
						VisitorOrder::mYmXmZ,
						VisitorOrder::mZmXmY,
						VisitorOrder::ZmXmY,
						VisitorOrder::ZmXY,
						VisitorOrder::YXmZ,
						VisitorOrder::ZXmY,
						VisitorOrder::mZXY,
						VisitorOrder::mYZmX,
						VisitorOrder::mYXZ,
						VisitorOrder::mZXmY,
						VisitorOrder::mZmXY
				));

} // namespace voxelutil
