/**
 * @file
 */

#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class KV6FormatTest: public AbstractVoxFormatTest {
};

TEST_F(KV6FormatTest, testLoad) {
	canLoad("test.kv6");
}

}
