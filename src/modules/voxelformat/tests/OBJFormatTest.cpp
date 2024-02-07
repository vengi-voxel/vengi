/**
 * @file
 */

#include "voxelformat/private/mesh/OBJFormat.h"
#include "AbstractVoxFormatTest.h"
#include "core/GameConfig.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VoxelUtil.h"

namespace voxelformat {

class OBJFormatTest : public AbstractVoxFormatTest {};

TEST_F(OBJFormatTest, testVoxelize) {
	OBJFormat f;
	const core::String filename = "cube.obj";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	EXPECT_TRUE(sceneGraph.size() > 0);
}

// https://github.com/vengi-voxel/vengi/issues/393
TEST_F(OBJFormatTest, testVoxelizeUVSphereObj) {
	util::ScopedVarChange scoped1(cfg::VoxformatScale, "4");
	util::ScopedVarChange scoped2(cfg::VoxformatFillHollow, "false");
	OBJFormat f;
	const core::String filename = "bug393.obj";
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file);
	scenegraph::SceneGraph sceneGraph;
	EXPECT_TRUE(f.loadGroups(filename, stream, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	// tris at index: 2, 3, 4, 5, 15, 16, 17, 18 are problematic, because
	// they are all using the 7th vertex of the mesh and this vertex is
	// showing the bug.
	EXPECT_FALSE(voxel::isAir(node->volume()->voxel(1, 1, 2).getMaterial()));
	const int cntVoxels = voxel::countVoxels(node->volume());
	ASSERT_EQ(cntVoxels, 24);
}

TEST_F(OBJFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	{
		QBFormat sourceFormat;
		const core::String filename = "rgb.qb";
		const io::FilePtr &file = open(filename);
		io::FileStream stream(file);
		EXPECT_TRUE(sourceFormat.load(filename, stream, sceneGraph, testLoadCtx));
	}
	ASSERT_TRUE(sceneGraph.size() > 0);
	OBJFormat f;
	const core::String outFilename = "exportrgb.obj";
	const io::FilePtr &outFile = open(outFilename, io::FileMode::SysWrite);
	io::FileStream outStream(outFile);
	EXPECT_TRUE(f.saveGroups(sceneGraph, outFilename, outStream, testSaveCtx));
}

} // namespace voxelformat
