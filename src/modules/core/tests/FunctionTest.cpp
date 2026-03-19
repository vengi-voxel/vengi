/**
 * @file
 */

#include "core/Function.h"
#include "core/Common.h"
#include <gtest/gtest.h>

namespace core {

class FunctionTest : public testing::Test {};

TEST_F(FunctionTest, testEmpty) {
	core::Function<void()> f;
	EXPECT_FALSE(f);
}

TEST_F(FunctionTest, testNullptr) {
	core::Function<void()> f(nullptr);
	EXPECT_FALSE(f);
}

TEST_F(FunctionTest, testLambdaVoid) {
	bool called = false;
	core::Function<void()> f([&called]() { called = true; });
	EXPECT_TRUE(f);
	f();
	EXPECT_TRUE(called);
}

TEST_F(FunctionTest, testLambdaReturnValue) {
	core::Function<int()> f([]() { return 42; });
	EXPECT_EQ(42, f());
}

TEST_F(FunctionTest, testLambdaWithArgs) {
	core::Function<int(int, int)> f([](int a, int b) { return a + b; });
	EXPECT_EQ(7, f(3, 4));
}

TEST_F(FunctionTest, testMoveConstruct) {
	int value = 0;
	core::Function<void()> f([&value]() { value = 1; });
	core::Function<void()> f2(core::move(f));
	EXPECT_FALSE(f);
	EXPECT_TRUE(f2);
	f2();
	EXPECT_EQ(1, value);
}

TEST_F(FunctionTest, testMoveAssign) {
	int value = 0;
	core::Function<void()> f([&value]() { value = 1; });
	core::Function<void()> f2;
	f2 = core::move(f);
	EXPECT_FALSE(f);
	EXPECT_TRUE(f2);
	f2();
	EXPECT_EQ(1, value);
}

TEST_F(FunctionTest, testCopyConstruct) {
	int counter = 0;
	core::Function<void()> f([&counter]() { counter++; });
	core::Function<void()> f2(f);
	EXPECT_TRUE(f);
	EXPECT_TRUE(f2);
	f();
	f2();
	EXPECT_EQ(2, counter);
}

TEST_F(FunctionTest, testCopyAssign) {
	int counter = 0;
	core::Function<void()> f([&counter]() { counter++; });
	core::Function<void()> f2;
	f2 = f;
	EXPECT_TRUE(f);
	EXPECT_TRUE(f2);
	f();
	f2();
	EXPECT_EQ(2, counter);
}

TEST_F(FunctionTest, testAssignNullptr) {
	core::Function<void()> f([]() {});
	EXPECT_TRUE(f);
	f = nullptr;
	EXPECT_FALSE(f);
}

TEST_F(FunctionTest, testSelfMoveAssign) {
	int value = 0;
	core::Function<void()> f([&value]() { value = 1; });
	auto *p = &f;
	*p = core::move(f);
	EXPECT_TRUE(f);
	f();
	EXPECT_EQ(1, value);
}

TEST_F(FunctionTest, testLargeCapture) {
	// Force heap allocation by capturing a large object
	char buf[256];
	for (int i = 0; i < 256; i++) {
		buf[i] = (char)i;
	}
	core::Function<int()> f([buf]() { return (int)buf[42]; });
	EXPECT_EQ(42, f());

	// Move the large function
	core::Function<int()> f2(core::move(f));
	EXPECT_FALSE(f);
	EXPECT_EQ(42, f2());

	// Copy the large function
	core::Function<int()> f3(f2);
	EXPECT_EQ(42, f2());
	EXPECT_EQ(42, f3());
}

TEST_F(FunctionTest, testReassign) {
	core::Function<int()> f([]() { return 1; });
	EXPECT_EQ(1, f());
	f = []() { return 2; };
	EXPECT_EQ(2, f());
}

} // namespace core
