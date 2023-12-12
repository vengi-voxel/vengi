/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/vengi/VENGIFormat.h"

namespace voxelformat {

class VENGIFormatTest : public AbstractVoxFormatTest {};

TEST_F(VENGIFormatTest, testSaveSmallVolume) {
	VENGIFormat f;
	testSaveSmallVolume("testSaveSmallVolume.vengi", &f);
}

TEST_F(VENGIFormatTest, DISABLED_testAO) {
	VENGIFormat f;
	const scenegraph::SceneGraph::MergedVolumePalette &merged = load("ambient-occlusion.vengi", f);
	ASSERT_TRUE(merged.first != nullptr);
	dump("ambientocclusion", merged.first, "ambient-occlusion.h");
	delete merged.first;
}

TEST_F(VENGIFormatTest, testSaveLoadVoxel) {
	VENGIFormat f;
	testSaveLoadVoxel("testSaveLoadVoxel.vengi", &f);
}

} // namespace voxelformat
