/**
 * @file
 */

#include <gtest/gtest.h>
#include "util/EMailValidator.h"

namespace util {

TEST(EMailValidatorTest, testValid) {
	ASSERT_TRUE(util::isValidEmail("a@do.de"));
	ASSERT_TRUE(util::isValidEmail("foo.bar@somedomain.org"));
}

TEST(EMailValidatorTest, testInValid) {
	ASSERT_FALSE(util::isValidEmail("a@b.c"));
	ASSERT_FALSE(util::isValidEmail("foo.bar@somedomain.waytoolong"));
}

}
