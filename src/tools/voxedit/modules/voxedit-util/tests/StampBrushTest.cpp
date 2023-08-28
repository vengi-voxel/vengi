/**
 * @file
 */

#include "voxedit-util/modifier/brush/StampBrush.h"
#include "app/tests/AbstractTest.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Palette.h"
#include "voxel/Voxel.h"

namespace voxedit {

class StampBrushTest : public app::AbstractTest {
protected:
};

TEST_F(StampBrushTest, testExecute) {
	StampBrush brush;
	BrushContext brushContext;
	ASSERT_TRUE(brush.init());
	voxel::RawVolume volume(voxel::Region(-3, 3));
	scenegraph::SceneGraph sceneGraph;
	ModifierVolumeWrapper wrapper(&volume, ModifierType::Place, {});
	voxel::Palette palette;
	palette.nippon();

	EXPECT_FALSE(brush.active());
	brush.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 0), palette);
	EXPECT_TRUE(brush.active());

	EXPECT_FALSE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));
	EXPECT_TRUE(brush.execute(sceneGraph, wrapper, brushContext));
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(brushContext.cursorPosition).getMaterial()));

	brush.shutdown();
}

} // namespace voxedit
