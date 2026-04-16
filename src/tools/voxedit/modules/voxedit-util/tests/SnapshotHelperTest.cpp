/**
 * @file
 */

#include "../modifier/brush/SnapshotHelper.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SnapshotHelperTest : public app::AbstractTest {
protected:
	static voxel::Voxel selectedVoxel(uint8_t color = 1) {
		voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, color);
		v.setFlags(voxel::FlagOutline);
		return v;
	}

	static voxel::Voxel solidVoxel(uint8_t color = 2) {
		return voxel::createVoxel(voxel::VoxelType::Generic, color);
	}
};

TEST_F(SnapshotHelperTest, testCaptureSnapshot) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());
	volume.setVoxel(1, 0, 0, selectedVoxel());
	volume.setVoxel(2, 0, 0, selectedVoxel());

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	ASSERT_TRUE(helper.hasSnapshot());
	EXPECT_EQ(3u, helper.snapshotVoxelCount());
	EXPECT_EQ(glm::ivec3(0, 0, 0), helper.snapshotRegion().getLowerCorner());
	EXPECT_EQ(glm::ivec3(2, 0, 0), helper.snapshotRegion().getUpperCorner());
	EXPECT_EQ(volume.region().getLowerCorner(), helper.capturedVolumeLower());
}

TEST_F(SnapshotHelperTest, testCaptureSnapshotIgnoresNonOutline) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, solidVoxel());
	volume.setVoxel(1, 0, 0, selectedVoxel());

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	ASSERT_TRUE(helper.hasSnapshot());
	EXPECT_EQ(1u, helper.snapshotVoxelCount());
}

TEST_F(SnapshotHelperTest, testCaptureEmptyVolume) {
	voxel::RawVolume volume(voxel::Region(-5, 5));

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	EXPECT_FALSE(helper.hasSnapshot());
	EXPECT_EQ(0u, helper.snapshotVoxelCount());
}

TEST_F(SnapshotHelperTest, testWriteVoxelSavesToHistory) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel(1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Write a new voxel at an existing position
	voxel::Voxel newVoxel = selectedVoxel(5);
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), newVoxel);

	// History should have the original voxel
	EXPECT_FALSE(helper.historyEmpty());
	EXPECT_EQ(5, volume.voxel(0, 0, 0).getColor());
}

TEST_F(SnapshotHelperTest, testWriteVoxelOutOfBounds) {
	voxel::RawVolume volume(voxel::Region(-1, 1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	// Writing out of bounds should not crash
	helper.writeVoxel(wrapper, glm::ivec3(100, 100, 100), selectedVoxel());
	EXPECT_TRUE(helper.historyEmpty());
}

TEST_F(SnapshotHelperTest, testRevertChanges) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel(1));
	volume.setVoxel(1, 0, 0, selectedVoxel(2));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Overwrite voxels
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(10));
	helper.writeVoxel(wrapper, glm::ivec3(1, 0, 0), selectedVoxel(20));

	EXPECT_EQ(10, volume.voxel(0, 0, 0).getColor());
	EXPECT_EQ(20, volume.voxel(1, 0, 0).getColor());

	// Revert
	voxel::Region dirty = helper.revertChanges(&volume);
	EXPECT_TRUE(dirty.isValid());
	EXPECT_EQ(1, volume.voxel(0, 0, 0).getColor());
	EXPECT_EQ(2, volume.voxel(1, 0, 0).getColor());
	EXPECT_TRUE(helper.historyEmpty());
}

TEST_F(SnapshotHelperTest, testRestoreHistory) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel(1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Write new voxel
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(10));
	EXPECT_EQ(10, volume.voxel(0, 0, 0).getColor());

	// Restore history (like generate() does between frames)
	helper.restoreHistory(&volume, wrapper);
	EXPECT_EQ(1, volume.voxel(0, 0, 0).getColor());
	EXPECT_TRUE(helper.historyEmpty());
}

TEST_F(SnapshotHelperTest, testAdjustForRegionShift) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());
	volume.setVoxel(1, 0, 0, selectedVoxel());

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	ASSERT_TRUE(helper.hasSnapshot());
	EXPECT_EQ(glm::ivec3(0, 0, 0), helper.snapshotRegion().getLowerCorner());

	// Shift by (3, 0, 0)
	const glm::ivec3 delta(3, 0, 0);
	helper.adjustForRegionShift(delta);

	EXPECT_EQ(glm::ivec3(3, 0, 0), helper.snapshotRegion().getLowerCorner());
	EXPECT_EQ(glm::ivec3(4, 0, 0), helper.snapshotRegion().getUpperCorner());
	EXPECT_EQ(volume.region().getLowerCorner() + delta, helper.capturedVolumeLower());

	// Verify snapshot data was shifted
	EXPECT_TRUE(helper.snapshot().hasVoxel(3, 0, 0));
	EXPECT_TRUE(helper.snapshot().hasVoxel(4, 0, 0));
	EXPECT_FALSE(helper.snapshot().hasVoxel(0, 0, 0));
	EXPECT_FALSE(helper.snapshot().hasVoxel(1, 0, 0));
}

TEST_F(SnapshotHelperTest, testAdjustForRegionShiftWithHistory) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel(1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Create history entry by writing
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(10));
	EXPECT_FALSE(helper.historyEmpty());

	// Shift
	const glm::ivec3 delta(2, 0, 0);
	helper.adjustForRegionShift(delta);

	// History should also be shifted
	EXPECT_TRUE(helper.history().hasVoxel(2, 0, 0));
}

TEST_F(SnapshotHelperTest, testClear) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(10));

	ASSERT_TRUE(helper.hasSnapshot());
	ASSERT_FALSE(helper.historyEmpty());

	helper.clear();

	EXPECT_FALSE(helper.hasSnapshot());
	EXPECT_EQ(0u, helper.snapshotVoxelCount());
	EXPECT_TRUE(helper.historyEmpty());
}

TEST_F(SnapshotHelperTest, testSaveToHistoryDeduplicates) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	volume.setVoxel(0, 0, 0, selectedVoxel(1));

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Write twice to same position
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(10));
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(20));

	// History should still have the original value (color 1)
	EXPECT_EQ(20, volume.voxel(0, 0, 0).getColor());

	// Revert should restore original
	helper.revertChanges(&volume);
	EXPECT_EQ(1, volume.voxel(0, 0, 0).getColor());
}

TEST_F(SnapshotHelperTest, testWriteVoxelToAirPosition) {
	voxel::RawVolume volume(voxel::Region(-5, 5));
	// Position (0,0,0) is air
	volume.setVoxel(1, 0, 0, selectedVoxel());

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setUnownedVolume(&volume);
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);

	SnapshotHelper helper;
	helper.captureSnapshot(&volume, volume.region());

	// Write to air position - history should track the air voxel
	helper.writeVoxel(wrapper, glm::ivec3(0, 0, 0), selectedVoxel(5));
	EXPECT_EQ(5, volume.voxel(0, 0, 0).getColor());

	// Revert should restore air
	helper.revertChanges(&volume);
	EXPECT_TRUE(voxel::isAir(volume.voxel(0, 0, 0).getMaterial()));
}

} // namespace voxedit
