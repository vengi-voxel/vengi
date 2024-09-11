/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

class MDLFormatTest : public AbstractFormatTest {};

TEST_F(MDLFormatTest, testVoxelize) {
	// model from https://www.moddb.com/groups/share-and-mod/downloads/quake-1-mdl-droid
	// or from https://github.com/QW-Group/ezquake-media/blob/master/game/progs/flame0.mdl
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "flame0.mdl", 1);
}

} // namespace voxelformat
