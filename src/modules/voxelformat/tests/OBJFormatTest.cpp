/**
 * @file
 */

#include "voxelformat/private/mesh/OBJFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "io/MemoryArchive.h"
#include "palette/Material.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class OBJFormatTest : public AbstractFormatTest {};

TEST_F(OBJFormatTest, testSceneVoxelSize) {
	util::ScopedVarChange voxelSize(cfg::VoxformatVoxelSize, "28");
	util::ScopedVarChange voxelMode(cfg::VoxformatVoxelizeMode, "1");
	util::ScopedVarChange createPalette(cfg::VoxelCreatePalette, "true");
	util::ScopedVarChange fillHollow(cfg::VoxformatFillHollow, "false");
	const char obj[] = "o small\nv 0 0 0\nv 2 0 0\nv 0 2 0\nf 1 2 3\n"
		"o large\nv 10 0 0\nv 14 0 0\nv 10 4 0\nf 4 5 6\n";
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(archive->add("scene.obj", (const uint8_t *)obj, sizeof(obj) - 1));
	OBJFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("scene.obj", archive, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *small = graph.findNodeByName("small");
	const scenegraph::SceneGraphNode *large = graph.findNodeByName("large");
	ASSERT_NE(nullptr, small);
	ASSERT_NE(nullptr, large);
	EXPECT_LE(small->region().getDimensionsInVoxels().x, 5);
	EXPECT_GE(small->region().getDimensionsInVoxels().x, 4);
	EXPECT_LE(large->region().getDimensionsInVoxels().x, 9);
	EXPECT_GE(large->region().getDimensionsInVoxels().x, 8);
	EXPECT_FLOAT_EQ(20.0f, large->transform(0).worldTranslation().x);
}

TEST_F(OBJFormatTest, testVoxelize) {
	testLoad("cube.obj", 6);
}

TEST_F(OBJFormatTest, testSaveChrKnight) {
	OBJFormat format;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::IgnoreHollow;
	testSaveMesh("chr_knight.qbcl", "chr_knight.obj", &format, flags);
}

TEST_F(OBJFormatTest, testSaveCC) {
	OBJFormat format;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::IgnoreHollow;
	testSaveMesh("cc.vxl", "cc.obj", &format, flags);
}

TEST_F(OBJFormatTest, testSaveLoadPointCloud) {
	OBJFormat format;
	testSaveLoadPointCloud("pointcloud-saveload.obj", &format);
}

// https://github.com/vengi-voxel/vengi/issues/393
TEST_F(OBJFormatTest, testVoxelizeUVSphereObj) {
	util::ScopedVarChange scoped1(cfg::VoxformatScale, "4");
	util::ScopedVarChange scoped2(cfg::VoxformatFillHollow, "false");
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "bug393.obj");
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	// tris at index: 2, 3, 4, 5, 15, 16, 17, 18 are problematic, because
	// they are all using the 7th vertex of the mesh and this vertex is
	// showing the bug.
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(1, 1, 2).getMaterial()));
	const int cntVoxels = voxelutil::countVoxels(*node->volume());
	ASSERT_EQ(cntVoxels, 24);
}

TEST_F(OBJFormatTest, testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	// Some material properties are not representable in MTL.
	core::Buffer<palette::MaterialProperty> ignoredMaterials;
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialLowDynamicRange);
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialFlux);
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialSp);
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialMedia);
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialDensity);
	ignoredMaterials.push_back(palette::MaterialProperty::MaterialPhase);
	testMaterial(sceneGraph, "test_material.obj", ignoredMaterials, true);
}

} // namespace voxelformat
