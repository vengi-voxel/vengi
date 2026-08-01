/**
 * @file
 */

#include "voxelformat/private/goxel/GoxFormat.h"
#include "AbstractFormatTest.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class GoxFormatTest : public AbstractFormatTest {};

TEST_F(GoxFormatTest, testLoad) {
	testLoad("test.gox");
}

TEST_F(GoxFormatTest, testSaveSmallVoxel) {
	GoxFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.gox", &f, -16, 15, voxel::ValidateFlags::None);
}

TEST_F(GoxFormatTest, testLoadRGB) {
	testRGB("rgb.gox");
}

TEST_F(GoxFormatTest, testLoadScreenshot) {
	testLoadScreenshot("chr_knight.gox", 128, 128, color::RGBA(158, 59, 59), 65, 27);
}

TEST_F(GoxFormatTest, testLoadShapeLayers) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "gox-shape-3-layers.gox", 4);
	int shapes = 0;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *volume = node.volume();
		ASSERT_NE(nullptr, volume);
		if (voxelutil::countVoxels(*volume) > 0) {
			shapes++;
		}
	}
	EXPECT_EQ(3, shapes);
}

TEST_F(GoxFormatTest, testForkLayerAndCameraRoundtrip) {
	GoxFormat f;
	palette::Palette pal;
	pal.magicaVoxel();
	const voxel::Region region = voxel::Region::fromSize(2);
	voxel::RawVolume volume(region);
	ASSERT_TRUE(volume.setVoxel(0, 0, 0, voxel::createVoxel(pal, 1)));

	scenegraph::SceneGraph sceneGraphSave;
	int parentId = InvalidNodeId;
	{
		scenegraph::SceneGraphNode parent(scenegraph::SceneGraphNodeType::Model);
		parent.setName("parent-layer");
		parent.setUnownedVolume(&volume);
		parent.setPalette(pal);
		parent.setOpacity(0.5f);
		parent.setProperty("vol_snap", "false");
		parent.setProperty("collapsed", "true");
		parent.setLocked(true);
		parentId = sceneGraphSave.emplace(core::move(parent));
		ASSERT_NE(InvalidNodeId, parentId);
	}
	{
		scenegraph::SceneGraphNode child(scenegraph::SceneGraphNodeType::Model);
		child.setName("child-layer");
		child.setUnownedVolume(&volume);
		child.setPalette(pal);
		child.setOpacity(0.25f);
		const int childId = sceneGraphSave.emplace(core::move(child), parentId);
		ASSERT_NE(InvalidNodeId, childId);
	}
	{
		scenegraph::SceneGraphNodeCamera cam;
		cam.setName("fork-cam");
		cam.setPerspective();
		cam.setFarPlane(42.0f);
		cam.setProperty("mode", "2"); // player
		cam.setProperty("standing_h", "3.5");
		cam.setProperty("crouch_h", "1.25");
		ASSERT_NE(InvalidNodeId, sceneGraphSave.emplace(core::move(cam)));
	}

	const io::ArchivePtr &archive = helper_archive();
	ASSERT_TRUE(f.save(sceneGraphSave, "gox-fork-roundtrip.gox", archive, testSaveCtx));

	scenegraph::SceneGraph sceneGraphLoad;
	ASSERT_TRUE(f.load("gox-fork-roundtrip.gox", archive, sceneGraphLoad, testLoadCtx));

	const scenegraph::SceneGraphNode *parent = nullptr;
	const scenegraph::SceneGraphNode *child = nullptr;
	for (auto iter = sceneGraphLoad.beginModel(); iter != sceneGraphLoad.end(); ++iter) {
		if ((*iter).name() == "parent-layer") {
			parent = &*iter;
		} else if ((*iter).name() == "child-layer") {
			child = &*iter;
		}
	}
	ASSERT_NE(nullptr, parent);
	ASSERT_NE(nullptr, child);
	EXPECT_TRUE(parent->locked());
	EXPECT_FALSE(child->locked());
	EXPECT_FLOAT_EQ(0.5f, parent->opacity());
	EXPECT_FLOAT_EQ(0.25f, child->opacity());
	EXPECT_EQ("false", parent->property("vol_snap"));
	EXPECT_EQ("true", parent->property("collapsed"));
	EXPECT_EQ(parent->id(), sceneGraphLoad.parentId(*child));

	const scenegraph::SceneGraphNode *camNode = nullptr;
	for (auto iter = sceneGraphLoad.begin(scenegraph::SceneGraphNodeType::Camera); iter != sceneGraphLoad.end();
		 ++iter) {
		if ((*iter).name() == "fork-cam") {
			camNode = &*iter;
			break;
		}
	}
	ASSERT_NE(nullptr, camNode);
	const scenegraph::SceneGraphNodeCamera &cam = scenegraph::toCameraNode(*camNode);
	EXPECT_EQ("2", cam.property("mode"));
	EXPECT_FLOAT_EQ(3.5f, cam.propertyf("standing_h"));
	EXPECT_FLOAT_EQ(1.25f, cam.propertyf("crouch_h"));
}

} // namespace voxelformat
