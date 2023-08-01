/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "io/FileStream.h"
#include "core/Var.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class AoSVXLFormatTest: public AbstractVoxFormatTest {
protected:
	bool onInitApp() override {
		if (!AbstractVoxFormatTest::onInitApp()) {
			return false;
		}
		// just to speed up the test runs...
		//core::Var::getSafe(cfg::VoxformatRGBFlattenFactor)->setVal("8");
		return true;
	}
};

TEST_F(AoSVXLFormatTest, testLoad) {
	canLoad("aceofspades.vxl", 1);
}

TEST_F(AoSVXLFormatTest, testLoadPalette) {
	AoSVXLFormat f;
	voxel::Palette pal;
	EXPECT_GT(loadPalette("aceofspades.vxl", f, pal), 200);
}

TEST_F(AoSVXLFormatTest, testLoadSaveAndLoadSceneGraph) {
	AoSVXLFormat src;
	AoSVXLFormat target;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette | voxel::ValidateFlags::Color);
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
	scenegraph::SceneGraphNode node1;
	node1.setVolume(&model1, false);
	sceneGraph.emplace(core::move(node1));
	io::BufferedReadWriteStream bufferedStream((int64_t)(10 * 1024 * 1024));

	ASSERT_TRUE(f.save(sceneGraph, filename, bufferedStream, testSaveCtx));
	bufferedStream.seek(0);
	scenegraph::SceneGraph sceneGraphLoad;
	EXPECT_TRUE(f.load(filename, bufferedStream, sceneGraphLoad, testLoadCtx));
	EXPECT_EQ(sceneGraphLoad.size(), 4);
}

}
