/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/slab6/KV6Format.h"
#include "voxelformat/private/slab6/KVXFormat.h"

namespace voxelformat {

class KVXFormatTest: public AbstractVoxFormatTest {
};

TEST_F(KVXFormatTest, testLoad) {
	canLoad("test.kvx");
}

TEST_F(KVXFormatTest, testSaveSmallVoxel) {
	KVXFormat f;
	testSaveLoadVoxel("kvx-smallvolumesavetest.kvx", &f, -16, 15);
}

TEST_F(KVXFormatTest, DISABLED_testChrKnight) {
	KVXFormat f1;
	KV6Format f2;
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Pivot;
	testLoadSceneGraph("slab6_vox_test.kvx", f1, "slab6_vox_test.kv6", f2, flags);
}

}
