/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/UUID.h"

namespace core {

class UUIDTest: public testing::Test {
};

TEST_F(UUIDTest, test) {
	ASSERT_TRUE(core::generateUUID() != "") << "No uuid implementation was found";
}

}
