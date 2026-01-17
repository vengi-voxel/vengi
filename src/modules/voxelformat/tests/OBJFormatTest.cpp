/**
 * @file
 */

#include "voxelformat/private/mesh/OBJFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/Voxel.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class OBJFormatTest : public AbstractFormatTest {};

TEST_F(OBJFormatTest, testVoxelize) {
	testLoad("cube.obj", 6);
}

TEST_F(OBJFormatTest, testSaveChrKnight) {
	OBJFormat format;
	testSaveMesh("chr_knight.qbcl", "chr_knight.obj", &format);
}

TEST_F(OBJFormatTest, testSaveCC) {
	OBJFormat format;
	testSaveMesh("cc.vxl", "cc.obj", &format);
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

// TODO: MATERIAL: materials are not yet written for obj
TEST_F(OBJFormatTest, DISABLED_testMaterial) {
	scenegraph::SceneGraph sceneGraph;
	testMaterial(sceneGraph, "test_material.obj");
}

} // namespace voxelformat
