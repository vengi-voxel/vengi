/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class CrocotileFormatTest : public AbstractFormatTest {};

TEST_F(CrocotileFormatTest, testLoadSwamp) {
	scenegraph::SceneGraph sceneGraph;
	// Optional local fixture - skipped when the file is not available
	testLoad(sceneGraph, "swamp.crocotile", 1, true);
}

TEST_F(CrocotileFormatTest, testLoadWaterfall) {
	scenegraph::SceneGraph sceneGraph;
	// Optional local fixture - skipped when the file is not available
	testLoad(sceneGraph, "waterfall.crocotile", 1, true);
}

TEST_F(CrocotileFormatTest, testLoadWaterfallScaledMatchesObjExportDensity) {
	// Geometry should match crocotile_to_obj.py (raw UVs, no extrusion hacks).
	util::ScopedVarChange scaleVar(cfg::VoxformatScale, "16");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "waterfall.crocotile", 1, true);
	if (IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	const voxel::RawVolume *volume = node->volume();
	ASSERT_NE(volume, nullptr);
	EXPECT_GT(voxelutil::countVoxels(*volume), 20000);
}

} // namespace voxelformat
