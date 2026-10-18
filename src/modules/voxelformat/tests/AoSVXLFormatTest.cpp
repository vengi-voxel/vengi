/**
 * @file
 */

#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "AbstractFormatTest.h"
#include "io/Archive.h"
#include "io/MemoryArchive.h"
#include "io/MemoryReadStream.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxelformat/private/aceofspades/AoSFormat.h"
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
		// core::getVar(cfg::VoxformatRGBFlattenFactor)->setVal("8");
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
	const voxel::Region &region = voxel::Region::fromSize(glm::ivec3(256, 64, 256));
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
	node1.setUnownedVolume(&model1);
	sceneGraph.emplace(core::move(node1));

	io::ArchivePtr archive = helper_archive();
	ASSERT_TRUE(f.save(sceneGraph, filename, archive, testSaveCtx));
	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(f.load(filename, archive, sceneGraphLoad, testLoadCtx));
	EXPECT_EQ(sceneGraphLoad.size(), 1u);
}

TEST_F(AoSVXLFormatTest, testCrop) {
	AoSVXLFormat vxl;
	AoSFormat aos;
	for (Format *format : {(Format *)&vxl, (Format *)&aos}) {
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		scenegraph::SceneGraph graph;
		graph.node(0).setProperty(scenegraph::PropTitle, "Cropped map");
		const glm::ivec3 mins(-3, 7, -9);
		const glm::vec3 translation(10, 20, 30);
		const color::RGBA red(255, 0, 0, 255);
		const color::RGBA blue(0, 0, 255, 255);
		scenegraph::SceneGraphNode terrain(scenegraph::SceneGraphNodeType::Model);
		terrain.setVolume(new voxel::RawVolume(voxel::Region(mins, mins + glm::ivec3(512, 0, 0))));
		palette::Palette palette;
		palette.setColor(0, red);
		terrain.setPalette(palette);
		terrain.transform().setLocalTranslation(translation);
		for (int x : {0, 511, 512}) {
			terrain.volume()->setVoxel(mins + glm::ivec3(x, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 0));
		}
		const int terrainId = graph.emplace(core::move(terrain));
		palette.setColor(0, blue);
		// Y crop is 255 (libvxl depth-256 encode limitation); keep 254, drop 255+.
		for (const glm::ivec3 &offset : {glm::ivec3(1, 254, 511), glm::ivec3(2, 255, 2), glm::ivec3(3, 3, 512)}) {
			scenegraph::SceneGraphNode chunk(scenegraph::SceneGraphNodeType::Model);
			const glm::ivec3 pos = mins + offset;
			chunk.setVolume(new voxel::RawVolume(voxel::Region(pos, pos)));
			chunk.volume()->setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, 0));
			chunk.setPalette(palette);
			chunk.transform().setLocalTranslation(translation);
			graph.emplace(core::move(chunk));
		}
		const char *filename = format == &vxl ? "cropped.vxl" : "cropped.aos";
		SCOPED_TRACE(filename);
		ASSERT_TRUE(format->save(graph, filename, archive, testSaveCtx));
		scenegraph::SceneGraph loaded;
		ASSERT_TRUE(format->load(filename, archive, loaded, testLoadCtx));
		ASSERT_EQ(loaded.size(), 1u);
		EXPECT_EQ(loaded.root().property(scenegraph::PropTitle), "Cropped map");
		const scenegraph::SceneGraphNode &node = *loaded.firstModelNode();
		ASSERT_EQ(node.region().getDimensionsInVoxels(), glm::ivec3(512, 256, 512));
		for (const glm::ivec3 &pos : {glm::ivec3(0), glm::ivec3(511, 0, 0), glm::ivec3(1, 254, 511)}) {
			const voxel::Voxel voxel = node.volume()->voxel(pos);
			EXPECT_FALSE(voxel::isAir(voxel.getMaterial()));
			EXPECT_EQ(node.palette().color(voxel.getColor()), pos.y == 254 ? blue : red);
		}
		EXPECT_TRUE(voxel::isAir(node.volume()->voxel(3, 3, 0).getMaterial()));
		EXPECT_EQ(graph.node(terrainId).region().getWidthInVoxels(), 513);
		EXPECT_FALSE(voxel::isAir(graph.node(terrainId).volume()->voxel(mins + glm::ivec3(512, 0, 0)).getMaterial()));
	}
}

} // namespace voxelformat
