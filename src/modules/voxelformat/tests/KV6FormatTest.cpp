/**
 * @file
 */

#include "voxelformat/private/slab6/KV6Format.h"
#include "AbstractFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class KV6FormatTest : public AbstractFormatTest {};

TEST_F(KV6FormatTest, testLoad) {
	testLoad("test.kv6");
}

TEST_F(KV6FormatTest, testSaveCubeModel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadCube("kv6-savecubemodel.kv6", &f, flags);
}

TEST_F(KV6FormatTest, testSaveSmallVoxel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadVoxel("kv6-smallvolumesavetest.kv6", &f, -16, 15, flags);
}

TEST_F(KV6FormatTest, testLoadSave) {
	KV6Format f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testConvert("voxlap5.kv6", f, "kv6-voxlap5.kv6", f, flags);
}

} // namespace voxelformat
