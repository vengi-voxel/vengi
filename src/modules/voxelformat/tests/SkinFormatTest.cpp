/**
 * @file
 */

#include "voxelformat/private/minecraft/SkinFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class SkinFormatTest : public AbstractFormatTest {};

TEST_F(SkinFormatTest, DISABLED_testLoadSave) {
	SkinFormat src;
	SkinFormat target;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testLoadSaveAndLoadSceneGraph("minecraft-skin.png", src, "minecraft-skin-test.mcskin", target, flags);
}

} // namespace voxelformat
