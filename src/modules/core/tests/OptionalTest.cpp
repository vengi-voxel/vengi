/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Optional.h"

namespace core {

TEST(OptionalTest, testDefault) {
	Optional<int> optional;
	EXPECT_FALSE(optional.hasValue());
	EXPECT_EQ(nullptr, optional.value());
}

TEST(OptionalTest, testHasValue) {
	Optional<int> optional;
	int value = 0;
	optional.setValue(&value);
	EXPECT_TRUE(optional.hasValue());
	optional.setValue(nullptr);
	EXPECT_FALSE(optional.hasValue());
	optional.setValue(value);
	EXPECT_TRUE(optional.hasValue());
	optional.setValue(nullptr);
	EXPECT_FALSE(optional.hasValue());
}

TEST(OptionalTest, testAssignment) {
	Optional<int> optional;
	int value = 0;
	optional.setValue(value);
	Optional<int> optional2;
	optional2.setValue(value);
	optional2 = optional;
	EXPECT_NE(optional.value(), optional2.value());
}

}
