/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/AoSVXLFormat.h"
#include "io/FileStream.h"
#include "core/Var.h"

namespace voxelformat {

class AoSVXLFormatTest: public AbstractVoxFormatTest {
protected:
	bool onInitApp() override {
		if (!AbstractVoxFormatTest::onInitApp()) {
			return false;
		}
		// just to speed up the test runs...
		core::Var::getSafe(cfg::VoxformatRGBFlattenFactor)->setVal("8");
		return true;
	}
};

TEST_F(AoSVXLFormatTest, testLoad) {
	canLoad("aceofspades.vxl", 4);
}

TEST_F(AoSVXLFormatTest, testLoadPalette) {
	AoSVXLFormat f;
	voxel::Palette pal;
	EXPECT_GT(loadPalette("aceofspades.vxl", f, pal), 200);
}

TEST_F(AoSVXLFormatTest, testSave) {
	AoSVXLFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(255, 63, 255));
	voxel::RawVolume layer1(region);
	const char *filename = "tests-aos.vxl";
	for (int x = 0; x < region.getWidthInVoxels(); ++x) {
		for (int z = 0; z < region.getDepthInVoxels(); ++z) {
			EXPECT_TRUE(layer1.setVoxel(x, 0, z, voxel::createVoxel(1)));
			EXPECT_TRUE(layer1.setVoxel(x, 1, z, voxel::createVoxel(1)));
		}
	}
	SceneGraph sceneGraph;
	SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	sceneGraph.emplace(core::move(node1));
	const io::FilePtr &sfile = open(filename, io::FileMode::SysWrite);
	io::FileStream sstream(sfile);
	ASSERT_TRUE(f.save(sceneGraph, sfile->name(), sstream, testThumbnailCreator));
	SceneGraph sceneGraphLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	EXPECT_TRUE(f.load(file->name(), stream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

}
