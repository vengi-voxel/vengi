/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/VolumeDataAccessor.h"
#include "core/StandardLib.h"
#include <limits.h>

namespace voxel {

class VolumeDataAccessorTest : public AbstractVoxelTest {};

TEST_F(VolumeDataAccessorTest, FullDenseViewBorrowsStorage) {
	RawVolume volume(Region(-3, 3));
	volume.fill(Voxel(VoxelType::Generic, 7, 9, FlagOutline, 3));
	VolumeDataAccessor view(volume);
	ASSERT_TRUE(view.valid());
	EXPECT_FALSE(view.ownsBuffer());
	EXPECT_EQ(volume.linearVoxels(), view.voxels());
	EXPECT_EQ(volume.region(), view.region());
	EXPECT_EQ(volume.width(), view.width());
	EXPECT_EQ(volume.height(), view.height());
	EXPECT_EQ(volume.depth(), view.depth());
	EXPECT_EQ(volume.region().stride(), view.stride());
	view.voxels()[0].setNormal(11);
	EXPECT_EQ(11, volume.voxel(-3, -3, -3).getNormal());
	const VolumeDataAccessor &read = view;
	EXPECT_EQ((const uint8_t *)read.voxels(), read.data());
}

TEST_F(VolumeDataAccessorTest, RegionsAreSnapshotsWithAirPadding) {
	RawVolume volume(Region(-3, 3));
	const Voxel value(VoxelType::Generic, 17, 11, FlagOutline, 4);
	volume.fill(value);
	volume.setBorderValue(Voxel(VoxelType::Generic, 99));
	const Region requested(-4, -1);
	VolumeDataAccessor copy(volume, requested);
	ASSERT_TRUE(copy.valid());
	EXPECT_TRUE(copy.ownsBuffer());
	EXPECT_EQ(requested, copy.region());
	for (int z = -4; z <= -1; ++z) {
		for (int y = -4; y <= -1; ++y) {
			for (int x = -4; x <= -1; ++x) {
				EXPECT_EQ(volume.region().containsPoint(x, y, z) ? value : Voxel(),
					copy.voxels()[requested.index(x, y, z)]);
			}
		}
	}
	copy.voxels()[requested.index(-3, -3, -3)].setNormal(0);
	EXPECT_EQ(value, volume.voxel(-3, -3, -3));
	VolumeDataAccessor outside(volume, Region(10, 12));
	ASSERT_TRUE(outside.valid());
	for (int i = 0; i < 27; ++i) {
		EXPECT_EQ(Voxel(), outside.voxels()[i]);
	}
}

TEST_F(VolumeDataAccessorTest, ChunkSnapshotPreservesMetadataAndSourceSamplers) {
	RawVolume volume(Region(-40, 216));
	const Voxel background(VoxelType::Air, 12, 9, FlagOutline, 3);
	volume.fill(background);
	volume.setVoxel(-9, -9, -9, Voxel(VoxelType::Generic, 17, 11, 0, 4));
	volume.setVoxel(-8, -8, -8, Voxel(VoxelType::Transparent, 19, 0, 2, 5));
	const size_t bytes = volume.allocatedBytes();
	RawVolume::Sampler sampler(volume);
	ASSERT_TRUE(sampler.setPosition(-9, -9, -9));
	const Voxel *stable = &sampler.voxel();
	const Region requested(-42, -5);
	VolumeDataAccessor copy(volume, requested);
	ASSERT_TRUE(copy.valid());
	EXPECT_TRUE(copy.ownsBuffer());
	for (int z = -42; z <= -5; ++z) {
		for (int y = -42; y <= -5; ++y) {
			for (int x = -42; x <= -5; ++x) {
				const Voxel expected = volume.region().containsPoint(x, y, z) ? volume.voxel(x, y, z) : Voxel();
				EXPECT_EQ(0, core_memcmp(&expected, copy.voxels() + requested.index(x, y, z), sizeof(Voxel)));
			}
		}
	}
	EXPECT_EQ(bytes, volume.allocatedBytes());
	EXPECT_FALSE(volume.isContiguous());
	EXPECT_EQ(stable, &sampler.voxel());
	copy.voxels()[requested.index(-9, -9, -9)].setNormal(0);
	EXPECT_EQ(11, sampler.voxel().getNormal());
	VolumeDataAccessor full(volume);
	ASSERT_TRUE(full.valid());
	EXPECT_TRUE(full.ownsBuffer());
	EXPECT_EQ(background, full.voxels()[0]);
	EXPECT_FALSE(volume.isContiguous());
}

TEST_F(VolumeDataAccessorTest, MoveTransfersOwnedAndBorrowedBuffers) {
	RawVolume volume(Region(0, 3));
	VolumeDataAccessor owned(volume, Region(0, 1));
	Voxel *original = owned.voxels();
	VolumeDataAccessor moved(core::move(owned));
	EXPECT_EQ(original, moved.voxels());
	EXPECT_TRUE(moved.ownsBuffer());
	EXPECT_FALSE(owned.valid());
	EXPECT_FALSE(owned.region().isValid());
	VolumeDataAccessor borrowed(volume);
	borrowed = core::move(moved);
	EXPECT_EQ(original, borrowed.voxels());
	EXPECT_TRUE(borrowed.ownsBuffer());
	EXPECT_FALSE(moved.valid());
	VolumeDataAccessor another(volume);
	borrowed = core::move(another);
	EXPECT_EQ(volume.linearVoxels(), borrowed.voxels());
	EXPECT_FALSE(borrowed.ownsBuffer());
	VolumeDataAccessor &same = borrowed;
	borrowed = core::move(same);
	EXPECT_EQ(volume.linearVoxels(), borrowed.voxels());
}

TEST_F(VolumeDataAccessorTest, ReinitializationReleasesOwnedBuffersAndResetsState) {
	RawVolume volume(Region(0, 3));
	const Region subregion(0, 1);
	VolumeDataAccessor accessor(volume, subregion);
	ASSERT_TRUE(accessor.ownsBuffer());
	accessor.init(volume, volume.region());
	EXPECT_TRUE(accessor.valid());
	EXPECT_FALSE(accessor.ownsBuffer());
	EXPECT_EQ(volume.linearVoxels(), accessor.voxels());
	accessor.init(volume, subregion);
	ASSERT_TRUE(accessor.valid());
	ASSERT_TRUE(accessor.ownsBuffer());
	accessor.init(volume, accessor.region());
	EXPECT_TRUE(accessor.valid());
	EXPECT_TRUE(accessor.ownsBuffer());
	EXPECT_EQ(subregion, accessor.region());
	accessor.init(volume, Region::InvalidRegion);
	EXPECT_FALSE(accessor.valid());
	EXPECT_FALSE(accessor.ownsBuffer());
	EXPECT_FALSE(accessor.region().isValid());
	accessor.init(volume, subregion);
	ASSERT_TRUE(accessor.ownsBuffer());
	accessor.init(volume, Region(0, INT_MAX - 1));
	EXPECT_FALSE(accessor.valid());
	EXPECT_FALSE(accessor.ownsBuffer());
	EXPECT_FALSE(accessor.region().isValid());
	accessor.init(volume, volume.region());
	EXPECT_EQ(volume.linearVoxels(), accessor.voxels());
	EXPECT_FALSE(accessor.ownsBuffer());
}

TEST_F(VolumeDataAccessorTest, InvalidAndOverflowRegionsFailCleanly) {
	RawVolume volume(Region(0, 1));
	VolumeDataAccessor invalid(volume, Region::InvalidRegion);
	EXPECT_FALSE(invalid.valid());
	EXPECT_FALSE(invalid.ownsBuffer());
	VolumeDataAccessor overflow(volume, Region(0, INT_MAX - 1));
	EXPECT_FALSE(overflow.valid());
	EXPECT_FALSE(overflow.region().isValid());
}

} // namespace voxel
