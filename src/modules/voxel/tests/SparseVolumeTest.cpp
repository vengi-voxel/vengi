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
	core::Vector<voxel::Voxel, 7> voxels;
	voxels.assign(voxel, voxels.capacity());
	voxel::SparseVolume v{voxel::Region{0, 0, 0, 3, 6, 3}};
	voxel::setVoxels(v, 0, 0, 0, v.region().getWidthInVoxels(), v.region().getDepthInVoxels(), &voxels.front(),
				v.region().getHeightInVoxels());
	const int vxls = voxelutil::countVoxels(v);
	ASSERT_EQ(v.region().voxels(), vxls);
}

TEST_F(SparseVolumeTest, testFullSamplerLoop) {
	const voxel::Region &region = voxel::Region::fromSize(glm::ivec3(64));
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

TEST_F(SparseVolumeTest, testChunkBoundariesAndRegion) {
	SparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 2);
	ASSERT_TRUE(v.setVoxel(255, 255, 255, voxel));
	ASSERT_TRUE(v.setVoxel(256, 0, 0, voxel));
	ASSERT_TRUE(v.setVoxel(-1, -1, -1, voxel));

	EXPECT_EQ(3u, v.size());
	EXPECT_TRUE(v.hasVoxel(255, 255, 255));
	EXPECT_TRUE(v.hasVoxel(256, 0, 0));
	EXPECT_TRUE(v.hasVoxel(-1, -1, -1));

	const Region region = v.calculateRegion();
	ASSERT_TRUE(region.isValid());
	EXPECT_EQ(-1, region.getLowerX());
	EXPECT_EQ(-1, region.getLowerY());
	EXPECT_EQ(-1, region.getLowerZ());
	EXPECT_EQ(256, region.getUpperX());
	EXPECT_EQ(255, region.getUpperY());
	EXPECT_EQ(255, region.getUpperZ());
}

TEST_F(SparseVolumeTest, testSetVoxelsRowAcrossChunks) {
	SparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 2);
	v.setVoxelsRow(-300, -1, 257, 600, voxel);
	EXPECT_EQ(600u, v.size());
	EXPECT_FALSE(v.hasVoxel(-301, -1, 257));
	for (int x = -300; x < 300; ++x) {
		ASSERT_TRUE(v.hasVoxel(x, -1, 257)) << x;
		ASSERT_EQ(2, v.voxel(x, -1, 257).getColor()) << x;
	}
	EXPECT_FALSE(v.hasVoxel(300, -1, 257));
	v.setVoxelsRow(-10, -1, 257, 20, voxel);
	EXPECT_EQ(600u, v.size());
	const Region region = v.calculateRegion();
	EXPECT_EQ(glm::ivec3(-300, -1, 257), region.getLowerCorner());
	EXPECT_EQ(glm::ivec3(299, -1, 257), region.getUpperCorner());

	v.setVoxelsRow(-300, -1, 257, 600, voxel::Voxel());
	EXPECT_TRUE(v.empty());
	EXPECT_FALSE(v.calculateRegion().isValid());
}

TEST_F(SparseVolumeTest, testStoreEmptyVoxels) {
	SparseVolume v;
	v.setStoreEmptyVoxels(true);
	const voxel::Voxel air = voxel::createVoxel(VoxelType::Air, 5);
	ASSERT_TRUE(v.setVoxel(3, 4, 5, air));
	EXPECT_EQ(1u, v.size());
	EXPECT_TRUE(v.hasVoxel(3, 4, 5));
	EXPECT_EQ(5, v.voxel(3, 4, 5).getColor());
	EXPECT_FALSE(v.hasVoxel(3, 4, 6));

	int visited = 0;
	voxel::RawVolume rv(voxel::Region(0, 7));
	rv.setVoxel(3, 4, 5, voxel::createVoxel(VoxelType::Generic, 1));
	v.copyTo(rv);
	EXPECT_TRUE(voxel::isAir(rv.voxel(3, 4, 5).getMaterial()));
	EXPECT_EQ(5, rv.voxel(3, 4, 5).getColor());
	visited = voxelutil::countVoxels(rv);
	EXPECT_EQ(0, visited);
}

TEST_F(SparseVolumeTest, testCopyToRegion) {
	SparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1);
	ASSERT_TRUE(v.setVoxel(1, 1, 1, voxel));
	ASSERT_TRUE(v.setVoxel(5, 5, 5, voxel));
	ASSERT_TRUE(v.setVoxel(-300, 1, 1, voxel));
	ASSERT_TRUE(v.setVoxel(600, 1, 1, voxel));
	voxel::RawVolume rv(voxel::Region(0, 7));
	v.copyTo(rv, voxel::Region(0, 3));
	EXPECT_EQ(1, voxelutil::countVoxels(rv));
	EXPECT_TRUE(voxel::isBlocked(rv.voxel(1, 1, 1).getMaterial()));
}

TEST_F(SparseVolumeTest, testRemoveReleasesChunk) {
	SparseVolume v;
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1, 3);
	ASSERT_TRUE(v.setVoxel(-5, 7, 300, voxel));
	ASSERT_TRUE(v.setVoxel(-5, 7, 300, voxel::Voxel()));
	EXPECT_TRUE(v.empty());
	EXPECT_FALSE(v.hasVoxel(-5, 7, 300));
	ASSERT_TRUE(v.setVoxel(-5, 7, 300, voxel::createVoxel(VoxelType::Generic, 2)));
	EXPECT_EQ(1u, v.size());
	EXPECT_EQ(2, v.voxel(-5, 7, 300).getColor());
	EXPECT_EQ(NO_NORMAL, v.voxel(-5, 7, 300).getNormal());
}

TEST_F(SparseVolumeTest, testSamplerSetVoxelAcrossChunks) {
	SparseVolume v;
	SparseVolume::Sampler sampler(v);
	const voxel::Voxel voxel = voxel::createVoxel(VoxelType::Generic, 1);
	sampler.setPosition(250, -3, 0);
	for (int i = 0; i < 12; ++i) {
		ASSERT_TRUE(sampler.setVoxel(voxel));
		sampler.movePositiveX();
	}
	EXPECT_EQ(12u, v.size());
	for (int x = 250; x < 262; ++x) {
		EXPECT_TRUE(v.hasVoxel(x, -3, 0)) << x;
	}
	sampler.setPosition(255, -3, 0);
	EXPECT_TRUE(voxel::isBlocked(sampler.voxel().getMaterial()));
	EXPECT_TRUE(sampler.setVoxel(voxel::Voxel()));
	EXPECT_TRUE(voxel::isAir(sampler.voxel().getMaterial()));
	EXPECT_EQ(11u, v.size());
	sampler.movePositiveX();
	EXPECT_TRUE(voxel::isBlocked(sampler.voxel().getMaterial()));
	sampler.moveNegativeX(2);
	EXPECT_TRUE(voxel::isBlocked(sampler.voxel().getMaterial()));
}

TEST_F(SparseVolumeTest, testSamplerMovesAcrossBricks) {
	SparseVolume v;
	// unique color per position to detect wrong index arithmetic
	auto colorAt = [](int x, int y, int z) { return (uint8_t)(1 + ((x * 7 + y * 13 + z * 29) & 0x7f)); };
	for (int z = -10; z <= 10; ++z) {
		for (int y = -10; y <= 10; ++y) {
			for (int x = -10; x <= 10; ++x) {
				v.setVoxel(x, y, z, voxel::createVoxel(VoxelType::Generic, colorAt(x, y, z)));
			}
		}
	}
	SparseVolume::Sampler sampler(v);
	sampler.setPosition(-10, -10, -10);
	for (int i = -10; i <= 10; ++i) {
		EXPECT_EQ(colorAt(i, -10, -10), sampler.voxel().getColor()) << "x " << i;
		sampler.movePositiveX();
	}
	sampler.setPosition(10, 10, 10);
	for (int i = 10; i >= -10; --i) {
		EXPECT_EQ(colorAt(10, i, 10), sampler.voxel().getColor()) << "y " << i;
		sampler.moveNegativeY();
	}
	sampler.setPosition(3, -2, -10);
	for (int i = -10; i <= 10; i += 3) {
		EXPECT_EQ(colorAt(3, -2, i), sampler.voxel().getColor()) << "z " << i;
		sampler.movePositiveZ(3);
	}
	// chunk border: 255 -> 256 -> 255
	v.setVoxel(255, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(256, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
	sampler.setPosition(255, 0, 0);
	EXPECT_EQ(1, sampler.voxel().getColor());
	sampler.movePositiveX();
	EXPECT_EQ(2, sampler.voxel().getColor());
	sampler.moveNegativeX();
	EXPECT_EQ(1, sampler.voxel().getColor());
}

TEST_F(SparseVolumeTest, testSetVoxelsClipsToRegion) {
	SparseVolume v(voxel::Region(0, 3));
	const voxel::Voxel column[6] = {
		voxel::createVoxel(VoxelType::Generic, 1), voxel::createVoxel(VoxelType::Generic, 2),
		voxel::createVoxel(VoxelType::Generic, 3), voxel::createVoxel(VoxelType::Generic, 4),
		voxel::createVoxel(VoxelType::Generic, 5), voxel::createVoxel(VoxelType::Generic, 6)};
	// starts two voxels below the lower corner on every axis - the first two column entries are below the region
	voxel::setVoxels(v, -2, -2, -2, 6, 6, column, 6);
	EXPECT_EQ(64u, v.size());
	EXPECT_EQ(3, v.voxel(0, 0, 0).getColor());
	EXPECT_EQ(4, v.voxel(0, 1, 0).getColor());
	EXPECT_EQ(6, v.voxel(3, 3, 3).getColor());
}

TEST_F(SparseVolumeTest, testSamplerLeavesAndReentersRegion) {
	SparseVolume v(voxel::Region(0, 3));
	v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(3, 0, 0, voxel::createVoxel(VoxelType::Generic, 2));
	SparseVolume::Sampler sampler(v);
	EXPECT_TRUE(sampler.setPosition(3, 0, 0));
	sampler.movePositiveX();
	EXPECT_FALSE(sampler.currentPositionValid());
	EXPECT_FALSE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 3)));
	EXPECT_TRUE(voxel::isAir(sampler.voxel().getMaterial()));
	EXPECT_EQ(2, sampler.peekVoxel1nx0py0pz().getColor());
	sampler.moveNegativeX();
	EXPECT_TRUE(sampler.currentPositionValid());
	EXPECT_EQ(2, sampler.voxel().getColor());
	sampler.moveNegativeX(3);
	EXPECT_EQ(1, sampler.voxel().getColor());
	sampler.moveNegativeY();
	EXPECT_FALSE(sampler.currentPositionValid());
	sampler.movePositiveY();
	EXPECT_EQ(1, sampler.voxel().getColor());
}

TEST_F(SparseVolumeTest, testSamplerSeesBrickReleasedByVolume) {
	SparseVolume v;
	v.setVoxel(1, 1, 1, voxel::createVoxel(VoxelType::Generic, 1));
	v.setVoxel(2, 1, 1, voxel::createVoxel(VoxelType::Generic, 2));
	SparseVolume::Sampler sampler(v);
	sampler.setPosition(1, 1, 1);
	EXPECT_EQ(1, sampler.voxel().getColor());

	// releases the brick (and the chunk) the sampler points into
	v.setVoxel(1, 1, 1, voxel::Voxel());
	v.setVoxel(2, 1, 1, voxel::Voxel());
	EXPECT_EQ(0u, v.size());
	sampler.movePositiveX();
	EXPECT_TRUE(voxel::isAir(sampler.voxel().getMaterial()));
	EXPECT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 3)));
	EXPECT_EQ(3, v.voxel(2, 1, 1).getColor());
	EXPECT_EQ(1u, v.size());

	// a brick created by another writer is picked up
	v.setVoxel(3, 1, 1, voxel::createVoxel(VoxelType::Generic, 4));
	sampler.movePositiveX();
	EXPECT_EQ(4, sampler.voxel().getColor());

	// sampler removal releases the brick - writing again has to allocate a new one
	sampler.setPosition(2, 1, 1);
	EXPECT_TRUE(sampler.setVoxel(voxel::Voxel()));
	sampler.movePositiveX();
	EXPECT_TRUE(sampler.setVoxel(voxel::Voxel()));
	EXPECT_EQ(0u, v.size());
	EXPECT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 5)));
	EXPECT_EQ(5, v.voxel(3, 1, 1).getColor());
	EXPECT_EQ(1u, v.size());
}

TEST_F(SparseVolumeTest, testSamplerCurrentVoxelMatchesStoredVoxel) {
	SparseVolume v;
	SparseVolume::Sampler sampler(v);
	sampler.setPosition(3, 4, 5);
	ASSERT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 1, 7)));
	EXPECT_EQ(v.voxel(3, 4, 5), sampler.voxel());

	// no normal given - the stored normal is kept
	ASSERT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 2)));
	EXPECT_EQ(7, v.voxel(3, 4, 5).getNormal());
	EXPECT_EQ(v.voxel(3, 4, 5), sampler.voxel());

	voxel::Voxel reset = voxel::createVoxel(VoxelType::Generic, 3);
	reset.setNormalReset();
	ASSERT_TRUE(sampler.setVoxel(reset));
	EXPECT_EQ(NO_NORMAL, v.voxel(3, 4, 5).getNormal());
	EXPECT_EQ(v.voxel(3, 4, 5), sampler.voxel());
}

} // namespace voxel
