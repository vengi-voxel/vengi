/**
 * @file
 */

#include <gtest/gtest.h>
#include "math/QuadTree.h"

namespace math {

namespace quad {
class Item {
private:
	RectFloat _bounds;
	int _id;
public:
	Item(const RectFloat& rect, int id) :
			_bounds(rect), _id(id) {
	}

	RectFloat getRect() const {
		return _bounds;
	}

	bool operator==(const Item& rhs) const {
		return rhs._id == _id;
	}
};
}

TEST(QuadTreeTest, testAdd) {
	QuadTree<quad::Item, float> quadTree(RectFloat(0, 0, 100, 100));
	EXPECT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const quad::Item item1(RectFloat(51, 51, 53, 53), 1);
	EXPECT_TRUE(quadTree.insert(item1));
	EXPECT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
	const quad::Item item2(RectFloat(15, 15, 18, 18), 2);
	EXPECT_TRUE(quadTree.insert(item2));
	EXPECT_EQ(2, quadTree.count())<< "Expected to have 2 entries in the quad tree";
}

TEST(QuadTreeTest, testRemove) {
	QuadTree<quad::Item, float> quadTree(RectFloat(0, 0, 100, 100));
	EXPECT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const quad::Item item(RectFloat(51, 51, 53, 53), 1);
	EXPECT_TRUE(quadTree.insert(item));
	EXPECT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
	EXPECT_TRUE(quadTree.remove(item));
	EXPECT_EQ(0, quadTree.count())<<"Expected to have 0 entries in the quad tree";
}

TEST(QuadTreeTest, testMax) {
	QuadTree<quad::Item, float> quadTree(RectFloat::getMaxRect());
	EXPECT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const quad::Item item1(RectFloat(51.0f, 51.0f, 53.0f, 53.0f), 1);
	EXPECT_TRUE(quadTree.insert(item1)) << "Could not enter the item";
	EXPECT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
}

TEST(QuadTreeTest, testQuery) {
	QuadTree<quad::Item, float> quadTree(RectFloat(0.0f, 0.0f, 100.0f, 100.0f));
	{
		QuadTree<quad::Item, float>::Contents contents;
		quadTree.query(RectFloat(50.0f, 50.0f, 60.0f, 60.0f), contents);
		EXPECT_EQ(0u, contents.size())<<"expected to find nothing in an empty tree";
	}
	const quad::Item item1(RectFloat(51.0f, 51.0f, 53.0f, 53.0f), 1);
	EXPECT_TRUE(quadTree.insert(item1));
	{
		QuadTree<quad::Item, float>::Contents contents;
		quadTree.query(RectFloat::getMaxRect(), contents);
		EXPECT_EQ(1u, contents.size())<<"expected to find one entry for the max rect";
	}
	{
		QuadTree<quad::Item, float>::Contents contents;
		quadTree.query(item1.getRect(), contents);
		EXPECT_EQ(1u, contents.size())<<"expected to find one entry for the item rect";
	}
}

}
