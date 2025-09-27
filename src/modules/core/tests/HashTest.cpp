/**
 * @file
 */

#include "core/Hash.h"
#include "core/UUID.h"
#include <gtest/gtest.h>

namespace core {

TEST(HasTest, testUUID) {
	ASSERT_EQ(36u, core::UUID::generate().str().size());
}

} // namespace core
