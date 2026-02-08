/**
 * @file
 */

#include "voxelformat/private/vengi/VENGIFormat.h"
#include "AbstractFormatTest.h"
#include "palette/Palette.h"
#include "scenegraph/IKConstraint.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelformat {

class VENGIFormatTest : public AbstractFormatTest {};

TEST_F(VENGIFormatTest, testSaveSmallVolume) {
	VENGIFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadVoxel) {
	VENGIFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vengi", &f);
}

TEST_F(VENGIFormatTest, testSaveLoadIKConstraint) {
	VENGIFormat f;
	palette::Palette pal;
	pal.magicaVoxel();
	const voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, voxel::createVoxel(pal, 1)));

	scenegraph::SceneGraph sceneGraphSave;
	int effectorNodeId;
	{
		scenegraph::SceneGraphNode effectorNode(scenegraph::SceneGraphNodeType::Model);
		effectorNode.setVolume(&original, false);
		effectorNode.setPalette(pal);
		effectorNode.setName("effector-node");
		effectorNodeId = sceneGraphSave.emplace(core::move(effectorNode));
		ASSERT_NE(InvalidNodeId, effectorNodeId);
	}
	{
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(&original, false);
		node.setPalette(pal);
		node.setName("ik-node");

		scenegraph::IKConstraint ik;
		ik.effectorNodeId = effectorNodeId;
		ik.rollMin = -1.5f;
		ik.rollMax = 2.0f;
		ik.visible = false;
		ik.anchor = true;
		scenegraph::IKConstraint::RadiusConstraint swing;
		swing.center = glm::vec2(0.5f, 1.0f);
		swing.radius = 0.75f;
		ik.swingLimits.push_back(swing);
		node.setIkConstraint(ik);

		sceneGraphSave.emplace(core::move(node));
	}

	const io::ArchivePtr &archive = helper_archive();
	ASSERT_TRUE(f.saveGroups(sceneGraphSave, "testIK.vengi", archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(f.loadGroups("testIK.vengi", archive, sceneGraphLoad, testLoadCtx));

	// find the ik-node by name
	const scenegraph::SceneGraphNode *loadedIKNode = nullptr;
	const scenegraph::SceneGraphNode *loadedEffectorNode = nullptr;
	for (auto iter = sceneGraphLoad.beginModel(); iter != sceneGraphLoad.end(); ++iter) {
		if ((*iter).name() == "ik-node") {
			loadedIKNode = &*iter;
		} else if ((*iter).name() == "effector-node") {
			loadedEffectorNode = &*iter;
		}
	}
	ASSERT_NE(nullptr, loadedIKNode);
	ASSERT_NE(nullptr, loadedEffectorNode);
	ASSERT_TRUE(loadedIKNode->hasIKConstraint());
	const scenegraph::IKConstraint *loadedIK = loadedIKNode->ikConstraint();
	ASSERT_NE(nullptr, loadedIK);
	EXPECT_EQ(loadedEffectorNode->id(), loadedIK->effectorNodeId);
	EXPECT_FLOAT_EQ(-1.5f, loadedIK->rollMin);
	EXPECT_FLOAT_EQ(2.0f, loadedIK->rollMax);
	EXPECT_FALSE(loadedIK->visible);
	EXPECT_TRUE(loadedIK->anchor);
	ASSERT_EQ(1u, loadedIK->swingLimits.size());
	EXPECT_FLOAT_EQ(0.5f, loadedIK->swingLimits[0].center.x);
	EXPECT_FLOAT_EQ(1.0f, loadedIK->swingLimits[0].center.y);
	EXPECT_FLOAT_EQ(0.75f, loadedIK->swingLimits[0].radius);
}

} // namespace voxelformat
