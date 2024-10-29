/**
 * @file
 */

#include "core/Alphanumeric.h"
#include <gtest/gtest.h>

namespace core {

inline std::ostream &operator<<(::std::ostream &os, const Alphanumeric &dt) {
	return os << dt.c_str();
}

TEST(AlphanumericTest, testSort) {
	Alphanumeric a1("1");
	Alphanumeric a10("10");
	Alphanumeric a11("11");
	Alphanumeric a100("100");
	Alphanumeric a2("2");
	Alphanumeric a21("21");
	Alphanumeric a3("3");
	Alphanumeric str21("str21");
	Alphanumeric Str20("Str20");

	EXPECT_LT(a1, a2);
	EXPECT_LT(a2, a3);
	EXPECT_LT(a3, a10);
	EXPECT_LT(a10, a11);
	EXPECT_LT(a11, a21);
	EXPECT_LT(a21, a100);
	EXPECT_LT(a100, str21);
	EXPECT_LT(Str20, str21);

	EXPECT_GT(a2, a1);
	EXPECT_GT(a3, a2);
	EXPECT_GT(a10, a3);
	EXPECT_GT(a11, a10);
	EXPECT_GT(a21, a11);
	EXPECT_GT(a100, a21);
	EXPECT_GT(str21, a100);
	EXPECT_GT(str21, Str20);
}

TEST(AlphanumericTest, testSort2) {
	Alphanumeric a2("1abc2");
	Alphanumeric a3("1abc3");
	Alphanumeric a1("1abc1");

	EXPECT_LT(a1, a2);
	EXPECT_LT(a2, a3);
	EXPECT_LT(a1, a3);
}

} // namespace core
