/**
 * @file
 */

#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "AbstractFormatTest.h"
#include "io/Archive.h"
#include "io/MemoryReadStream.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class AoSVXLFormatTest : public AbstractFormatTest {
protected:
	class AoSVXLFormatEx : public AoSVXLFormat {
	public:
		void loadMetadata(scenegraph::SceneGraphNode &node, const core::String &metadata) {
			io::MemoryReadStream stream(metadata.c_str(), metadata.size());
			AoSVXLFormat::loadMetadataTxt(node, "test", &stream);
		}
	};

	void loadMetadata(scenegraph::SceneGraphNode &node, const core::String &metadata) {
		AoSVXLFormatEx f;
		f.loadMetadata(node, metadata);
	}

	bool onInitApp() override {
		if (!AbstractFormatTest::onInitApp()) {
			return false;
		}
		// just to speed up the test runs...
		// core::Var::getVar(cfg::VoxformatRGBFlattenFactor)->setVal("8");
		return true;
	}
};

TEST_F(AoSVXLFormatTest, testLoad) {
	testLoad("aceofspades.vxl", 1);
}

TEST_F(AoSVXLFormatTest, testLoadMetadataMultiline) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	loadMetadata(node, R"(name = 'name with spaces'
version = '1.0'
author = 'test_author'
description = ("Multiline description "
               "with a dot at the end.")
)");
	EXPECT_EQ(node.property(scenegraph::PropTitle), "name with spaces");
	EXPECT_EQ(node.property(scenegraph::PropVersion), "1.0");
	EXPECT_EQ(node.property(scenegraph::PropAuthor), "test_author");
	EXPECT_EQ(node.property(scenegraph::PropDescription), "Multiline description with a dot at the end.");
}

TEST_F(AoSVXLFormatTest, testLoadMetadata) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	loadMetadata(node, R"(name = "name with spaces"
version = '2.0'
author = 'test author'
description = "description and spaces."
)");
	EXPECT_EQ(node.property(scenegraph::PropTitle), "name with spaces");
	EXPECT_EQ(node.property(scenegraph::PropVersion), "2.0");
	EXPECT_EQ(node.property(scenegraph::PropAuthor), "test author");
	EXPECT_EQ(node.property(scenegraph::PropDescription), "description and spaces.");
}

TEST_F(AoSVXLFormatTest, testLoadMetadataWithScript) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	loadMetadata(node, R"(name = 'name'

version = '1.0'

author = 'someone'

description = 'description!'

extensions = { 'water_damage' : 200}



# script

from pyspades.constants import *

import random

)");
	EXPECT_EQ(node.property(scenegraph::PropTitle), "name");
	EXPECT_EQ(node.property(scenegraph::PropVersion), "1.0");
	EXPECT_EQ(node.property(scenegraph::PropAuthor), "someone");
	EXPECT_EQ(node.property(scenegraph::PropDescription), "description!");
}

TEST_F(AoSVXLFormatTest, testLoadPalette) {
	AoSVXLFormat f;
	palette::Palette pal;
	EXPECT_GT(helper_loadPalette("aceofspades.vxl", helper_filesystemarchive(), f, pal), 200);
}

TEST_F(AoSVXLFormatTest, testLoadSaveAndLoadSceneGraph) {
	AoSVXLFormat src;
	AoSVXLFormat target;
	voxel::ValidateFlags flags =
		voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
	testLoadSaveAndLoadSceneGraph("aceofspades.vxl", src, "aceofspades-test.vxl", target, flags);
}

TEST_F(AoSVXLFormatTest, testSave) {
	AoSVXLFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(255, 63, 255));
	voxel::RawVolume model1(region);
	const char *filename = "tests-aos.vxl";
	for (int x = 0; x < region.getWidthInVoxels(); ++x) {
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			EXPECT_TRUE(model1.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
			EXPECT_TRUE(model1.setVoxel(x, 1, z, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		}
	}
	scenegraph::SceneGraph sceneGraph;
	scenegraph::SceneGraphNode node1(scenegraph::SceneGraphNodeType::Model);
	node1.setVolume(&model1, false);
	sceneGraph.emplace(core::move(node1));

	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(f.save(sceneGraph, filename, archive, testSaveCtx));
	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(f.load(filename, archive, sceneGraphLoad, testLoadCtx));
	EXPECT_EQ(sceneGraphLoad.size(), 1u);
}

} // namespace voxelformat
