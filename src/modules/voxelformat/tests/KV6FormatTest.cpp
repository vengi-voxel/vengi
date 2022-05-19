/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KV6Format.h"

namespace voxelformat {

class KV6FormatTest: public AbstractVoxFormatTest {
};

TEST_F(KV6FormatTest, testLoad) {
	canLoad("test.kv6");
}

}
