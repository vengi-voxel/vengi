/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Set.h"
#include <numeric>
#include <random>

namespace collection {

class SetTest: public testing::Test {
};

TEST_F(SetTest, testDiff) {
	std::unordered_set<int> setDiff1;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		setDiff1.insert(i);
	}
	std::unordered_set<int> setDiff2;
	for (int i = 0; i < n; ++i) {
		setDiff2.insert(i);
	}
	setDiff2.insert(n + 1);
	auto diff = core::setDifference(setDiff1, setDiff2);
	EXPECT_FALSE(diff.empty());
	EXPECT_EQ(1u, diff.size());
}

TEST_F(SetTest, testDiff2) {
	std::unordered_set<int> setDiff1;
	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		setDiff1.insert(i);
		setDiff1.insert(-n - i);
	}
	std::unordered_set<int> setDiff2;
	for (int i = 0; i < n; ++i) {
		setDiff2.insert(i);
		setDiff2.insert(n + i);
	}
	auto diff = core::setDifference(setDiff1, setDiff2);
	EXPECT_FALSE(diff.empty());
	EXPECT_EQ(2000u, diff.size());
}

// exactly what is done for calculating the visible entities
TEST_F(SetTest, testVisibleActions) {
	std::unordered_set<int> setVisible1;
	std::unordered_set<int> setVisible2;

	setVisible1.insert(1);
	setVisible1.insert(2);
	setVisible1.insert(3);

	setVisible2.insert(1);
	setVisible2.insert(4);
	setVisible2.insert(5);
	setVisible2.insert(6);

	const auto& inBoth = core::setIntersection(setVisible1, setVisible2);
	EXPECT_EQ(1u, inBoth.size());
	EXPECT_EQ(1, *inBoth.begin());
	const std::unordered_set<int>& removeFromSet2 = core::setDifference(inBoth, setVisible2);
	EXPECT_EQ(3u, removeFromSet2.size());
	const std::unordered_set<int>& addToSet2 = core::setDifference(setVisible1, inBoth);
	EXPECT_EQ(2u, addToSet2.size());
	setVisible2 = core::setUnion(inBoth, addToSet2);
	EXPECT_EQ(3u, setVisible2.size());
	EXPECT_EQ(inBoth.size() + addToSet2.size(), setVisible2.size());
}

class SetMassTest: public testing::Test {
public:
	static const int offset = 1000;
	static const int n = 5000000;
	static std::vector<int> v1;
	static std::vector<int> v2;
	static std::vector<int> v3;
	static std::vector<int> v4;
	static std::unordered_set<int> set1;
	static std::unordered_set<int> set2;
	static void SetUpTestCase() {
		v1.resize(n);
		v2.resize(n);
		v3.resize(n);
		v4.resize(n);
		set1.reserve(n);
		set2.reserve(n);

		std::iota(std::begin(v1), std::end(v1), 0);
		std::iota(std::begin(v2), std::end(v2), offset);

		std::iota(std::begin(v3), std::end(v3), 0);
		std::iota(std::begin(v4), std::end(v4), offset);

		std::default_random_engine engine;
		std::shuffle(v3.begin(), v3.end(), engine);
		std::shuffle(v4.begin(), v4.end(), engine);

		for (int i = 0; i < n; ++i) {
			set1.insert(i);
			set2.insert(i + offset);
		}
	}
};

std::vector<int> SetMassTest::v1;
std::vector<int> SetMassTest::v2;
std::vector<int> SetMassTest::v3;
std::vector<int> SetMassTest::v4;
std::unordered_set<int> SetMassTest::set1;
std::unordered_set<int> SetMassTest::set2;

TEST_F(SetMassTest, testVectorIntersectionSorted) {
	std::vector<int> out;
	core::vectorIntersection(v1, v2, out);
	EXPECT_EQ(n - offset, (int)out.size());
}

TEST_F(SetMassTest, testVisibleActionsPerformance) {
	const auto& inBoth = core::setIntersection(set1, set2);
	EXPECT_EQ(n - offset, (int)inBoth.size());
}

// exactly what is done for calculating the visible entities
TEST_F(SetMassTest, testMassVisibleActions) {
	const size_t n1 = 20110;
	const size_t n2 = 22031;
	const size_t overlap = 120;

	std::unordered_set<int> setMass1(n1);
	std::unordered_set<int> setMass2(n2);

	for (size_t i = 0; i < n1; ++i) {
		setMass1.insert(i);
	}
	for (size_t i = n1 - overlap; i < n1 - overlap + n2; ++i) {
		setMass2.insert(i);
	}

	const auto& inBoth = core::setIntersection(setMass1, setMass2);
	EXPECT_EQ(overlap, inBoth.size());
	const std::unordered_set<int>& removeFromSet2 = core::setDifference(inBoth, setMass2);
	EXPECT_EQ(n2 - overlap, removeFromSet2.size());
	const std::unordered_set<int>& addToSet2 = core::setDifference(setMass1, inBoth);
	EXPECT_EQ(n1 - overlap, addToSet2.size());
	setMass2 = core::setUnion(inBoth, addToSet2);
	EXPECT_EQ(n1, setMass2.size());
	EXPECT_EQ(inBoth.size() + addToSet2.size(), setMass2.size());
}

}
