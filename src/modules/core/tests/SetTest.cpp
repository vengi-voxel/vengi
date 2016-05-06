/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Set.h"

namespace core {

TEST(SetTest, testDiff) {
	std::unordered_set<int> set1;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		set1.insert(i);
	}
	std::unordered_set<int> set2;
	for (int i = 0; i < n; ++i) {
		set2.insert(i);
	}
	set2.insert(n + 1);
	auto diff = core::setDifference(set1, set2);
	ASSERT_FALSE(diff.empty());
	ASSERT_EQ(1u, diff.size());
}

TEST(SetTest, testDiff2) {
	std::unordered_set<int> set1;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		set1.insert(i);
		set1.insert(-n - i);
	}
	std::unordered_set<int> set2;
	for (int i = 0; i < n; ++i) {
		set2.insert(i);
		set2.insert(n + i);
	}
	auto diff = core::setDifference(set1, set2);
	ASSERT_FALSE(diff.empty());
	ASSERT_EQ(2000u, diff.size());
}

}
