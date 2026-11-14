/**
 * @file
 */

#include "voxel/private/LinearVolumeStorage.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <gtest/gtest.h>
#include <glm/vec3.hpp>

namespace voxel {

TEST(LinearVolumeStorageTest, AllocateFillCopyAndMove) {
	LinearVolumeStorage storage({8, 4, 2});
	EXPECT_EQ(8 * 4 * 2 * sizeof(Voxel), storage.allocatedBytes());
	EXPECT_EQ(Voxel(), storage.voxel({0, 0, 0}));
	EXPECT_EQ(Voxel(), storage.voxel({7, 3, 1}));

	const Voxel value(VoxelType::Generic, 17, 23, FlagOutline, 4);
	EXPECT_TRUE(storage.setVoxel({7, 3, 1}, value));
	EXPECT_FALSE(storage.setVoxel({7, 3, 1}, value));
	EXPECT_EQ(value, storage.voxel({7, 3, 1}));

	storage.fill(value);
	EXPECT_EQ(value, storage.voxel({0, 0, 0}));
	EXPECT_EQ(value, storage.voxel({7, 3, 1}));

	LinearVolumeStorage copy(storage);
	EXPECT_EQ(storage.allocatedBytes(), copy.allocatedBytes());
	EXPECT_EQ(value, copy.voxel({3, 1, 0}));
	EXPECT_TRUE(copy.setVoxel({0, 0, 0}, Voxel(VoxelType::Transparent, 9)));
	EXPECT_EQ(value, storage.voxel({0, 0, 0}));

	Voxel *linear = (Voxel *)core_malloc(storage.allocatedBytes());
	ASSERT_NE(nullptr, linear);
	copy.copyTo(linear);
	EXPECT_EQ(copy.voxel({0, 0, 0}), linear[0]);
	EXPECT_EQ(copy.voxel({7, 3, 1}), linear[7 + 8 * 3 + 8 * 4 * 1]);
	core_free(linear);

	LinearVolumeStorage moved({2, 1, 1});
	ASSERT_TRUE(moved.setVoxel({0, 0, 0}, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(moved.move({1, 0, 0}));
	EXPECT_EQ((int)VoxelType::Air, (int)moved.voxel({0, 0, 0}).getMaterial());
	EXPECT_EQ(0, (int)moved.voxel({0, 0, 0}).getColor());
	EXPECT_EQ((int)VoxelType::Generic, (int)moved.voxel({1, 0, 0}).getMaterial());
	EXPECT_EQ(1, (int)moved.voxel({1, 0, 0}).getColor());
}

TEST(LinearVolumeStorageTest, MoveConstructAndAssign) {
	LinearVolumeStorage storage({4, 2, 1});
	const Voxel value = createVoxel(VoxelType::Generic, 5);
	ASSERT_TRUE(storage.setVoxel({3, 1, 0}, value));
	Voxel *payload = storage.voxels();

	LinearVolumeStorage moved(core::move(storage));
	EXPECT_EQ(payload, moved.voxels());
	EXPECT_EQ(value, moved.voxel({3, 1, 0}));
	EXPECT_NE(payload, storage.voxels());
	EXPECT_EQ(glm::ivec3(1), storage.dimensions());

	LinearVolumeStorage assigned({1, 1, 1});
	assigned = core::move(moved);
	EXPECT_EQ(payload, assigned.voxels());
	EXPECT_EQ(value, assigned.voxel({3, 1, 0}));
	EXPECT_NE(payload, moved.voxels());
	EXPECT_EQ(glm::ivec3(1), moved.dimensions());
}

TEST(LinearVolumeStorageTest, SampleDataAndRow) {
	LinearVolumeStorage storage({5, 3, 2});
	const Voxel value(VoxelType::Generic, 3);
	ASSERT_TRUE(storage.setVoxel({2, 1, 0}, value));

	int count = 0;
	const Voxel *row = storage.row({2, 1, 0}, count);
	ASSERT_NE(nullptr, row);
	EXPECT_EQ(3, count);
	EXPECT_EQ(value, row[0]);

	glm::ivec3 dataLower(0);
	glm::ivec3 dataUpper(0);
	int64_t yStride = 0;
	int64_t zStride = 0;
	Voxel *sample = storage.sampleData({2, 1, 0}, dataLower, dataUpper, yStride, zStride);
	ASSERT_NE(nullptr, sample);
	EXPECT_EQ(glm::ivec3(0), dataLower);
	EXPECT_EQ(glm::ivec3(4, 2, 1), dataUpper);
	EXPECT_EQ(5, yStride);
	EXPECT_EQ(15, zStride);
	EXPECT_EQ(value, *sample);
}

TEST(LinearVolumeStorageTest, CopyFromHasFlagsAndRegionForFlag) {
	LinearVolumeStorage src({4, 3, 2});
	const Voxel value = createVoxel(VoxelType::Generic, 7);
	ASSERT_TRUE(src.setVoxel({1, 1, 0}, value));
	src.setFlags({1, 1, 0}, {2, 2, 0}, FlagOutline);

	LinearVolumeStorage dst({4, 3, 2});
	dst.copyFrom(src, {1, 1, 0}, {0, 0, 0}, {2, 2, 1});
	EXPECT_EQ(value.getMaterial(), dst.voxel({0, 0, 0}).getMaterial());
	EXPECT_EQ(value.getColor(), dst.voxel({0, 0, 0}).getColor());
	EXPECT_TRUE(dst.hasFlags({0, 0, 0}, {1, 1, 0}, FlagOutline));
	EXPECT_FALSE(dst.hasFlags({2, 0, 0}, {3, 2, 1}, FlagOutline));

	glm::ivec3 mins(0);
	glm::ivec3 maxs(0);
	ASSERT_TRUE(src.regionForFlag(FlagOutline, mins, maxs));
	EXPECT_EQ(glm::ivec3(1, 1, 0), mins);
	EXPECT_EQ(glm::ivec3(2, 2, 0), maxs);
	EXPECT_FALSE(src.isEmpty({1, 1, 0}, {1, 1, 0}));
}

TEST(LinearVolumeStorageTest, AdoptExternalBuffer) {
	const glm::ivec3 dimensions(2, 2, 1);
	Voxel *data = (Voxel *)core_malloc(2 * 2 * sizeof(Voxel));
	ASSERT_NE(nullptr, data);
	const Voxel air;
	const Voxel solid = createVoxel(VoxelType::Generic, 1);
	core_memset4(data, 0, 4);
	core_memcpy((void *)data, &solid, sizeof(Voxel));
	{
		LinearVolumeStorage storage(data, dimensions, true);
		EXPECT_EQ(data, storage.voxels());
		EXPECT_EQ(solid, storage.voxel({0, 0, 0}));
		EXPECT_EQ(air, storage.voxel({1, 0, 0}));
	}
}

} // namespace voxel
