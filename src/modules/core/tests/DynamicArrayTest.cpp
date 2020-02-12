/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/DynamicArray.h"
#include "core/String.h"

namespace core {

struct DynamicArrayStruct {
	DynamicArrayStruct(const core::String &foo, int bar) :
			_foo(foo), _bar(bar) {
	}
	core::String _foo;
	int _bar;
};

TEST(DynamicArrayTest, testEmplaceBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.emplace_back("", 0);
	EXPECT_EQ(1u, array.size());
	EXPECT_EQ(32u, array.capacity());
}

TEST(DynamicArrayTest, testPushBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size());
	EXPECT_EQ(32u, array.capacity());
}

TEST(DynamicArrayTest, testClear) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size());
	EXPECT_EQ(32u, array.capacity());
	array.clear();
	EXPECT_EQ(0u, array.size());
	EXPECT_EQ(32u, array.capacity());
}

TEST(DynamicArrayTest, testRelease) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size());
	EXPECT_EQ(32u, array.capacity());
	array.release();
	EXPECT_EQ(0u, array.size());
	EXPECT_EQ(0u, array.capacity());
}

TEST(DynamicArrayTest, testIterate) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size());
	EXPECT_EQ(32u, array.capacity());
	int i = 1;
	for (const DynamicArrayStruct& d : array) {
		EXPECT_EQ(i++, d._bar);
	}
}

TEST(DynamicArrayTest, testCopy) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size());
	EXPECT_EQ(32u, array.capacity());
	DynamicArray<DynamicArrayStruct> copy(array);
	EXPECT_EQ(3u, copy.size());
	EXPECT_EQ(32u, copy.capacity());
}

TEST(DynamicArrayTest, testTriggerResize) {
	DynamicArray<DynamicArrayStruct, 2> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	EXPECT_EQ(2u, array.size());
	EXPECT_EQ(2u, array.capacity());
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size());
	EXPECT_EQ(4u, array.capacity());
}

}
