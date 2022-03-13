/**
 * @file
 */

#include "voxelformat/GLTFFormat.h"
#include "AbstractVoxFormatTest.h"
#include "io/File.h"
#include "voxelformat/QBFormat.h"

namespace voxel {

class GLTFFormatTest : public AbstractVoxFormatTest {};

TEST_F(GLTFFormatTest, testExportMesh) {
	SceneGraph sceneGraph;
	{
		QBFormat sourceFormat;
		const core::String filename = "rgb.qb";
		const io::FilePtr &file = open(filename);
		io::FileStream stream(file);
		EXPECT_TRUE(sourceFormat.loadGroups(filename, stream, sceneGraph));
	}
	ASSERT_TRUE(sceneGraph.size() > 0);
	GLTFFormat f;
	const core::String outFilename = "exportrgb.gltf";
	const io::FilePtr &outFile = open(outFilename, io::FileMode::SysWrite);
	io::FileStream outStream(outFile);
	EXPECT_TRUE(f.saveGroups(sceneGraph, outFilename, outStream));
}

} // namespace voxel
