/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/AnimaToonFormat.h"
#include "io/FileStream.h"
#include "core/Var.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class AnimaToonFormatTest: public AbstractVoxFormatTest {
};

TEST_F(AnimaToonFormatTest, DISABLED_testLoad) {
	canLoad("save_2022_Mar_09_13_43_20_Anima Toon.scn", 22);
}

}
