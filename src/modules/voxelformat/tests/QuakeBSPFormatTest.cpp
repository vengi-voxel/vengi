/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"

namespace voxelformat {

class QuakeBSPFormatTest : public AbstractFormatTest {};

TEST_F(QuakeBSPFormatTest, testLoad) {
	util::ScopedVarChange scoped(cfg::VoxformatScale, "0.001");
	testLoad("ufoai.bsp", 3); // hospital bsp from https://ufoai.org/maps/2.6/base/maps/b/
}

TEST_F(QuakeBSPFormatTest, testLoadGoldSrc) {
	util::ScopedVarChange scoped(cfg::VoxformatScale, "0.001");
	testLoad("goldsrc.bsp", 1);
}

} // namespace voxelformat
