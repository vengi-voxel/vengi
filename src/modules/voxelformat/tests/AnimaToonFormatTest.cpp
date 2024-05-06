/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class AnimaToonFormatTest : public AbstractFormatTest {};

TEST_F(AnimaToonFormatTest, testLoad) {
	testLoad("save_2022_Mar_09_13_43_20_Anima Toon.scn", 22);
}

TEST_F(AnimaToonFormatTest, testLoad_30) {
	testLoad("Anima Toon anim.scn", 22);
	testLoad("Anima Toon.scn", 22);
}

} // namespace voxelformat
