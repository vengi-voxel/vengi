/**
 * @file
 */

#include "voxelformat/private/slab6/KV6Format.h"
#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class KV6FormatTest : public AbstractFormatTest {};

TEST_F(KV6FormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test.kv6", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	const voxel::Region &region = node->region();
	const voxel::Region expectedRegion(0, 0, 0, 2, 2, 0);
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(expectedRegion, region);
}

TEST_F(KV6FormatTest, testSaveCubeModel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadCube("kv6-savecubemodel.kv6", &f, flags);
}

TEST_F(KV6FormatTest, testSaveSmallVoxel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadVoxel("kv6-smallvolumesavetest.kv6", &f, -16, 15, flags);
}

TEST_F(KV6FormatTest, testLoadSave) {
	KV6Format f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testConvert("voxlap5.kv6", f, "kv6-voxlap5.kv6", f, flags);
}

TEST_F(KV6FormatTest, testAnasplit) {
	struct Object {
		voxel::Region region;
		int cnt;
	} modelRegions[] {
		{voxel::Region{0, 103, 9, 10, 143, 18}, 910},
		{voxel::Region{3, 148, 9, 16, 190, 23}, 1198},
		{voxel::Region{20, 71, 4, 39, 106, 27}, 2257},
		{voxel::Region{20, 110, 0, 61, 143, 27}, 4260},
		{voxel::Region{20, 148, 4, 61, 193, 33}, 5018},
		{voxel::Region{22, 18, 4, 36, 66, 24}, 1927},
		{voxel::Region{24, 0, 6, 36, 13, 34}, 1051},
		{voxel::Region{27, 198, 6, 54, 228, 33}, 2172},
		{voxel::Region{42, 71, 4, 61, 106, 27}, 2257},
		{voxel::Region{45, 0, 6, 57, 13, 34}, 1051},
		{voxel::Region{45, 18, 4, 59, 66, 24}, 1927},
		{voxel::Region{65, 148, 9, 78, 190, 23}, 1198},
		{voxel::Region{71, 103, 9, 81, 143, 18}, 910}
	};

	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "anasplit.kv6", lengthof(modelRegions));
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}

	int compared = 0;
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isModelNode()) {
			continue;
		}
		const voxel::Region &region = node.region();
		bool found = false;
		for (const Object &r : modelRegions) {
			if (r.region != region) {
				continue;
			}
			EXPECT_EQ(r.cnt, voxelutil::countVoxels(*node.volume()));
			++compared;
			found = true;
			break;
		}
		EXPECT_TRUE(found) << "Expected region not found: voxel::Region{" << region.getLowerX() << ", " << region.getLowerY() << ", " << region.getLowerZ() << ", " << region.getUpperX() << ", " << region.getUpperY() << ", " << region.getUpperZ() << "}, "
			<< voxelutil::countVoxels(*node.volume());
	}
	EXPECT_EQ(lengthof(modelRegions), compared);
}

} // namespace voxelformat
