/**
 * @file
 */

#include "../modifier/SelectionManager.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "voxel/RawVolume.h"

namespace voxedit {

class SelectionManagerTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;
};

TEST_F(SelectionManagerTest, testSelectAndInvert) {
	voxel::Region region(0, 16);
	voxel::RawVolume volume(region);
	SelectionManager mgr;
	EXPECT_FALSE(mgr.hasSelection());
	EXPECT_TRUE(mgr.select(volume, glm::ivec3(4), glm::ivec3(12)));
	EXPECT_TRUE(mgr.hasSelection());
	mgr.invert(volume);
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(0)));
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(16)));
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(3)));
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(13)));
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(4)));
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(12)));
}

TEST_F(SelectionManagerTest, testUnselectHole) {
	voxel::Region region(0, 32);
	voxel::RawVolume volume(region);
	SelectionManager mgr;

	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(20, 20, 20);
	EXPECT_TRUE(mgr.select(volume, mins, maxs));
	EXPECT_TRUE(mgr.isSelected(mins));
	EXPECT_TRUE(mgr.isSelected(maxs));

	const glm::ivec3 unselectMins(12, 12, 12);
	const glm::ivec3 unselectMaxs(18, 18, 18);
	EXPECT_TRUE(mgr.unselect(volume, unselectMins, unselectMaxs));

	// Check that the hole is indeed unselected
	for (int x = unselectMins.x; x <= unselectMaxs.x; ++x) {
		for (int y = unselectMins.y; y <= unselectMaxs.y; ++y) {
			for (int z = unselectMins.z; z <= unselectMaxs.z; ++z) {
				EXPECT_FALSE(mgr.isSelected(glm::ivec3(x, y, z)))
					<< "Position at " << x << ", " << y << ", " << z << " should not be selected";
			}
		}
	}

	// Check that the border is still selected
	EXPECT_TRUE(mgr.isSelected(mins));
	EXPECT_TRUE(mgr.isSelected(maxs));

	// Check that we didn't select anything outside the original bounds
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(9, 10, 10)));
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(21, 20, 20)));
}

TEST_F(SelectionManagerTest, testUnselectCorner) {
	voxel::Region region(0, 32);
	voxel::RawVolume volume(region);
	SelectionManager mgr;

	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(20, 20, 20);
	EXPECT_TRUE(mgr.select(volume, mins, maxs));

	const glm::ivec3 unselectMins(15, 15, 15);
	const glm::ivec3 unselectMaxs(25, 25, 25); // Goes outside
	EXPECT_TRUE(mgr.unselect(volume, unselectMins, unselectMaxs));

	// Check unselected part inside original
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(16, 16, 16)));

	// Check selected part
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(10, 10, 10)));

	// Check outside original (should still be unselected)
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(22, 22, 22)));
}

TEST_F(SelectionManagerTest, testUnselectExtendsOutside) {
	voxel::Region region(0, 32);
	voxel::RawVolume volume(region);
	SelectionManager mgr;

	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(10, 10, 10);
	EXPECT_TRUE(mgr.select(volume, mins, maxs));

	// Unselect region that extends outside in Z
	const glm::ivec3 unselectMins(0, 0, -5);
	const glm::ivec3 unselectMaxs(10, 5, 15);
	EXPECT_TRUE(mgr.unselect(volume, unselectMins, unselectMaxs));

	// Check that we didn't select anything outside the original bounds
	// The bug would cause (0, 6, -5) to be selected.
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(5, 8, -1)));
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(5, 8, 11)));

	// Check that the remaining part is correct
	EXPECT_TRUE(mgr.isSelected(glm::ivec3(5, 8, 5)));

	// Check that the unselected part is unselected
	EXPECT_FALSE(mgr.isSelected(glm::ivec3(5, 2, 5)));
}

TEST_F(SelectionManagerTest, testCopy) {
	voxel::Region region(0, 32);
	voxel::RawVolume volume(region);
	// Fill volume
	for (int x = 10; x <= 20; ++x) {
		volume.setVoxel(x, 10, 10, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	}

	SelectionManager mgr;
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(20, 10, 10);
	EXPECT_TRUE(mgr.select(volume, mins, maxs));

	core::ScopedPtr<voxel::RawVolume> copy(mgr.copy(volume));
	ASSERT_NE(nullptr, copy);
	EXPECT_EQ(voxel::Region(mins, maxs), copy->region());
	// verify content
	for (int x = 10; x <= 20; ++x) {
		EXPECT_EQ(voxel::VoxelType::Generic, copy->voxel(x, 10, 10).getMaterial());
	}
}

TEST_F(SelectionManagerTest, testCut) {
	voxel::Region region(0, 32);
	voxel::RawVolume volume(region);
	// Fill volume
	for (int x = 10; x <= 20; ++x) {
		volume.setVoxel(x, 10, 10, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	}

	SelectionManager mgr;
	const glm::ivec3 mins(10, 10, 10);
	const glm::ivec3 maxs(20, 10, 10);
	EXPECT_TRUE(mgr.select(volume, mins, maxs));

	core::ScopedPtr<voxel::RawVolume> cut(mgr.cut(volume));
	ASSERT_NE(nullptr, cut);
	EXPECT_EQ(voxel::Region(mins, maxs), cut->region());
	// verify content in cut
	for (int x = 10; x <= 20; ++x) {
		EXPECT_EQ(voxel::VoxelType::Generic, cut->voxel(x, 10, 10).getMaterial());
	}
	// verify removed from volume
	for (int x = 10; x <= 20; ++x) {
		EXPECT_EQ(voxel::VoxelType::Air, volume.voxel(x, 10, 10).getMaterial());
	}
}

} // namespace voxedit
