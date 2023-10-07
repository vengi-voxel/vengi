/**
 * @file
 */

#include "voxelformat/private/slab6/SLAB6VoxFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/slab6/KV6Format.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class SLAB6VoxFormatTest : public AbstractVoxFormatTest {};

TEST_F(SLAB6VoxFormatTest, testSaveSmallVoxel) {
	SLAB6VoxFormat f;
	testSaveLoadVoxel("loadvoxel.vox", &f);
}

TEST_F(SLAB6VoxFormatTest, testChrKnight) {
	SLAB6VoxFormat f1;
	KV6Format f2;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::IgnoreHollow);
	testLoadSceneGraph("slab6_vox_test.vox", f1, "slab6_vox_test.kv6", f2, flags);
}

} // namespace voxelformat
