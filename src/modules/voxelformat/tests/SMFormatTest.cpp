/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/SMFormat.h"

namespace voxelformat {

class SMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SMFormatTest, DISABLED_testLoad) {
	// https://starmadedock.net
	canLoad("test.sment", 14);
}

}
