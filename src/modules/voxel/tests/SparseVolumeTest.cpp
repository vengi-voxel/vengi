/**
 * @file
 */

#include "voxel/SparseVolume.h"
#include "app/tests/AbstractTest.h"
#include "core/collection/Vector.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxel {

class SparseVolumeTest : public app::AbstractTest {};

TEST_F(SparseVolumeTest, testSetVoxel) {
	voxel::SparseVolume v(voxel::Region(0, 10));
	ASSERT_EQ(0u, v.size());
	ASSERT_TRUE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(10, 10, 10, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_FALSE(v.setVoxel(11, 11, 11, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
}

TEST_F(SparseVolumeTest, testCopyToRawVolume) {
	const voxel::Region region(0, 30);
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 0);
	voxel::SparseVolume v(region);
	voxel::RawVolume rv(region);
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				ASSERT_TRUE(v.setVoxel(x, y, z, voxel));
			}
		}
	}
	voxel::RawVolumeWrapper rvw(&rv);
	v.copyTo(rvw);
	for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
				ASSERT_TRUE(voxel.isSameType(rv.voxel(x, y, z)));
			}
		}
	}
}

TEST_F(SparseVolumeTest, testSetVoxels) {
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	core::Vector<voxel::Voxel, 6> voxels;
	voxels.assign(voxel, voxels.capacity());
	voxel::SparseVolume v{voxel::Region{0, 0, 0, 3, 6, 3}};
	voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				v.region().getHeightInVoxels());
	const int vxls = voxelutil::countVoxels(v);
	ASSERT_EQ(v.region().voxels(), vxls);
}

TEST_F(SparseVolumeTest, testFullSamplerLoop) {
	const voxel::Region region{glm::ivec3(0), glm::ivec3(63)};
	SparseVolume v(region);
	v.setVoxel(1, 2, 1, voxel::createVoxel(VoxelType::Generic, 0));

	v.setVoxel(0, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 0, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(0, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 1, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(0, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(1, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));
	v.setVoxel(2, 1, 2, voxel::createVoxel(VoxelType::Generic, 0));

	v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(1, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
	v.setVoxel(2, 0, 0, voxel::createVoxel(VoxelType::Generic, 3));
	v.setVoxel(0, 0, 1, voxel::createVoxel(VoxelType::Generic, 4));
	v.setVoxel(1, 0, 1, voxel::createVoxel(VoxelType::Generic, 5));
	v.setVoxel(2, 0, 1, voxel::createVoxel(VoxelType::Generic, 6));
	v.setVoxel(0, 0, 2, voxel::createVoxel(VoxelType::Generic, 7));
	v.setVoxel(1, 0, 2, voxel::createVoxel(VoxelType::Generic, 8));
	v.setVoxel(2, 0, 2, voxel::createVoxel(VoxelType::Generic, 9));
	SparseVolume::Sampler volumeSampler(v);

	ASSERT_EQ(0, region.getLowerX());
	ASSERT_EQ(0, region.getLowerY());
	ASSERT_EQ(0, region.getLowerZ());

	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y++) {
			volumeSampler.setPosition(region.getLowerX(), y, z);

			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x++) {
				const Voxel voxelCurrent = volumeSampler.voxel();
				const Voxel voxelLeft = volumeSampler.peekVoxel1nx0py0pz();
				const Voxel voxelRight = volumeSampler.peekVoxel1px0py0pz();
				const Voxel voxelBefore = volumeSampler.peekVoxel0px0py1nz();
				const Voxel voxelBehind = volumeSampler.peekVoxel0px0py1pz();
				const Voxel voxelLeftBefore = volumeSampler.peekVoxel1nx0py1nz();
				const Voxel voxelRightBefore = volumeSampler.peekVoxel1px0py1nz();
				const Voxel voxelLeftBehind = volumeSampler.peekVoxel1nx0py1pz();
				const Voxel voxelRightBehind = volumeSampler.peekVoxel1px0py1pz();

				const Voxel voxelAbove = volumeSampler.peekVoxel0px1py0pz();
				const Voxel voxelAboveLeft = volumeSampler.peekVoxel1nx1py0pz();
				const Voxel voxelAboveRight = volumeSampler.peekVoxel1px1py0pz();
				const Voxel voxelAboveBefore = volumeSampler.peekVoxel0px1py1nz();
				const Voxel voxelAboveBehind = volumeSampler.peekVoxel0px1py1pz();
				const Voxel voxelAboveLeftBefore = volumeSampler.peekVoxel1nx1py1nz();
				const Voxel voxelAboveRightBefore = volumeSampler.peekVoxel1px1py1nz();
				const Voxel voxelAboveLeftBehind = volumeSampler.peekVoxel1nx1py1pz();
				const Voxel voxelAboveRightBehind = volumeSampler.peekVoxel1px1py1pz();

				const Voxel voxelBelow = volumeSampler.peekVoxel0px1ny0pz();

				if (y == 0) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 1) {
					if (x == 0 && z == 0) {
						ASSERT_EQ(VoxelType::Air, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Air, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelLeft.getMaterial())
							<< "Wrong left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRight.getMaterial())
							<< "Wrong right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBehind.getMaterial())
							<< "Wrong behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelBefore.getMaterial())
							<< "Wrong before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBefore.getMaterial())
							<< "Wrong left before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBefore.getMaterial())
							<< "Wrong right before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelLeftBehind.getMaterial())
							<< "Wrong left behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Generic, voxelRightBehind.getMaterial())
							<< "Wrong right behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelAbove.getMaterial())
							<< "Wrong above voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeft.getMaterial())
							<< "Wrong above left voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRight.getMaterial())
							<< "Wrong above right voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveBefore.getMaterial())
							<< "Wrong above before voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBefore.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveLeftBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;
						ASSERT_EQ(VoxelType::Air, voxelAboveRightBehind.getMaterial())
							<< "Wrong above behind voxel " << x << ":" << y << ":" << z;

						ASSERT_EQ(VoxelType::Generic, voxelBelow.getMaterial())
							<< "Wrong below voxel " << x << ":" << y << ":" << z;
					}
				} else if (y == 2) {
					// 25
					if (x == 1 && z == 1) {
						ASSERT_EQ(VoxelType::Generic, voxelCurrent.getMaterial())
							<< "Wrong voxel at coordinate " << x << ":" << y << ":" << z;
					}
				}

				volumeSampler.movePositiveX();
			}
		}
	}
}

} // namespace voxel
