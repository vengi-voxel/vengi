/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/UUID.h"

namespace core {

class UUIDTest: public AbstractTest {
};

TEST_F(UUIDTest, test) {
	ASSERT_TRUE(core::generateUUID() != "") << "No uuid implementation was found";
}

}
