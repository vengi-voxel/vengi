/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/EMailValidator.h"

namespace core {

TEST(EMailValidatorTest, testValid) {
	ASSERT_TRUE(core::isValidEmail("a@do.de"));
	ASSERT_TRUE(core::isValidEmail("foo.bar@somedomain.org"));
}

TEST(EMailValidatorTest, testInValid) {
	ASSERT_FALSE(core::isValidEmail("a@b.c"));
	ASSERT_FALSE(core::isValidEmail("foo.bar@somedomain.waytoolong"));
}

}
