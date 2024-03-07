/**
 * @file
 */

#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class AnimaToonFormatTest: public AbstractVoxFormatTest {
};

TEST_F(AnimaToonFormatTest, testLoad) {
	canLoad("save_2022_Mar_09_13_43_20_Anima Toon.scn", 22);
}

TEST_F(AnimaToonFormatTest, testLoad_30) {
	canLoad("Anima Toon anim.scn", 22);
	canLoad("Anima Toon.scn", 22);
}

}
