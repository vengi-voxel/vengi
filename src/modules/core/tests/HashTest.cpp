/**
 * @file
 */

#include "core/Hash.h"
#include <gtest/gtest.h>

namespace core {

TEST(HasTest, testUUID) {
	ASSERT_EQ(36u, generateUUID().size());
}

} // namespace core
