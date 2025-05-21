/**
 * @file
 */

#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"

namespace voxelformat {

class BinVoxFormatTest : public AbstractFormatTest {
protected:
	void saveLoad(int version) {
		util::ScopedVarChange versionVar(cfg::VoxformatBinvoxVersion, version);
		BinVoxFormat f;
		// binvox doesn't store palette data, only indices without the color information (since version >= 2).
		const voxel::ValidateFlags flags =
			voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Color | voxel::ValidateFlags::Palette);
		testSaveLoadVoxel("bv-smallvolumesavetest.binvox", &f, 0, 1, flags);
	}
};

TEST_F(BinVoxFormatTest, testLoad) {
	testLoad("test.binvox");
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxelV1) {
	saveLoad(1);
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxelV2) {
	saveLoad(2);
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxelV3) {
	saveLoad(3);
}

} // namespace voxelformat
