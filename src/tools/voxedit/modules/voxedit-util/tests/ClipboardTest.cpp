/**
 * @file
 */

#include "voxedit-util/Clipboard.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

class ClipboardTest : public app::AbstractTest {
protected:
	void paste(const voxel::ClipboardData &clipboardData, const voxel::Voxel &voxel) {
		// paste into a new volume
		const voxel::Region targetRegion(0, 5);
		voxel::RawVolume targetVolume(targetRegion);
		palette::Palette targetPalette;
		targetPalette.nippon();
		voxel::ClipboardData outData(&targetVolume, &targetPalette, false);
		const glm::ivec3 pastePos(0, 0, 0);
		voxel::Region modifiedRegion = voxel::Region::InvalidRegion;
		tool::paste(outData, clipboardData, pastePos, modifiedRegion);

		EXPECT_TRUE(modifiedRegion.isValid());
		EXPECT_EQ(voxel.getColor(), targetVolume.voxel(1, 1, 1).getColor());
	}

	void prepare(scenegraph::SceneGraphNode &node, const voxel::Voxel &voxel) {
		const voxel::Region region(0, 3);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		palette::Palette palette;
		palette.nippon();

		// place a voxel and select it
		volume->setVoxel(1, 1, 1, voxel);

		node.setVolume(volume, true);
		node.setPalette(palette);
		// select the region to enable cut
		node.select(region);
	}
};

TEST_F(ClipboardTest, testCopyPaste) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	prepare(node, voxel);

	// copy
	voxel::ClipboardData clipboardData = tool::copy(node);
	ASSERT_TRUE((bool)clipboardData);
	ASSERT_NE(nullptr, clipboardData.volume);
	EXPECT_EQ(voxel.getColor(), clipboardData.volume->voxel(1, 1, 1).getColor());

	paste(clipboardData, voxel);

	// original volume should be unchanged
	EXPECT_EQ(voxel.getColor(), node.volume()->voxel(1, 1, 1).getColor());
}

TEST_F(ClipboardTest, testCutPaste) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	prepare(node, voxel);

	// cut
	voxel::Region cutModifiedRegion = voxel::Region::InvalidRegion;
	voxel::ClipboardData clipboardData = tool::cut(node, cutModifiedRegion);
	ASSERT_TRUE((bool)clipboardData);
	ASSERT_NE(nullptr, clipboardData.volume);
	EXPECT_EQ(voxel.getColor(), clipboardData.volume->voxel(1, 1, 1).getColor());

	paste(clipboardData, voxel);

	// the original volume should have the voxel removed
	EXPECT_TRUE(voxel::isAir(node.volume()->voxel(1, 1, 1).getMaterial()));
}

} // namespace voxedit
