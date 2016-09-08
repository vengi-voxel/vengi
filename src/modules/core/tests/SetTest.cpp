/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Set.h"

namespace core {

class SetTest: public AbstractTest {
};


TEST_F(SetTest, testDiff) {
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

TEST_F(SetTest, testDiff2) {
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

// exactly what is done for calculating the visible entities
TEST_F(SetTest, testVisibleActions) {
	std::unordered_set<int> set1;
	std::unordered_set<int> set2;

	set1.insert(1);
	set1.insert(2);
	set1.insert(3);

	set2.insert(1);
	set2.insert(4);
	set2.insert(5);
	set2.insert(6);

	const auto& inBoth = core::setIntersection(set1, set2);
	ASSERT_EQ(1u, inBoth.size());
	ASSERT_EQ(1, *inBoth.begin());
	const std::unordered_set<int>& removeFromSet2 = core::setDifference(inBoth, set2);
	ASSERT_EQ(3u, removeFromSet2.size());
	const std::unordered_set<int>& addToSet2 = core::setDifference(set1, inBoth);
	ASSERT_EQ(2u, addToSet2.size());
	set2 = core::setUnion(inBoth, addToSet2);
	ASSERT_EQ(3u, set2.size());
	ASSERT_EQ(inBoth.size() + addToSet2.size(), set2.size());
}

}
