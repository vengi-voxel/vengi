#include <gtest/gtest.h>
#include "core/QuadTree.h"

namespace core {

class Item {
private:
	RectFloat _rect;
	int _id;
public:
	Item(const RectFloat& rect, int id) :
			_rect(rect), _id(id) {
	}

	RectFloat getRect() const {
		return _rect;
	}

	bool operator==(const Item& rhs) const {
		return rhs._id == _id;
	}
};

TEST(QuadTreeTest, testAdd) {
	core::QuadTree<Item, float> quadTree(RectFloat(0, 0, 100, 100));
	ASSERT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const Item item1(RectFloat(51, 51, 53, 53), 1);
	ASSERT_TRUE(quadTree.insert(item1));
	ASSERT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
	const Item item2(RectFloat(15, 15, 18, 18), 2);
	ASSERT_TRUE(quadTree.insert(item2));
	ASSERT_EQ(2, quadTree.count())<< "Expected to have 2 entries in the quad tree";
}

TEST(QuadTreeTest, testRemove) {
	core::QuadTree<Item, float> quadTree(RectFloat(0, 0, 100, 100));
	ASSERT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const Item item(RectFloat(51, 51, 53, 53), 1);
	ASSERT_TRUE(quadTree.insert(item));
	ASSERT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
	ASSERT_TRUE(quadTree.remove(item));
	ASSERT_EQ(0, quadTree.count())<<"Expected to have 0 entries in the quad tree";
}

TEST(QuadTreeTest, testMax) {
	core::QuadTree<Item, float> quadTree(RectFloat::getMaxRect());
	ASSERT_EQ(0, quadTree.count())<< "Expected to have no entries in the quad tree";
	const Item item1(RectFloat(51, 51, 53, 53), 1);
	ASSERT_TRUE(quadTree.insert(item1)) << "Could not enter the item";
	ASSERT_EQ(1, quadTree.count())<< "Expected to have 1 entry in the quad tree";
}

TEST(QuadTreeTest, testQuery) {
	core::QuadTree<Item, float> quadTree(RectFloat(0, 0, 100, 100));
	{
		const QuadTree<Item, float>::Contents& contents = quadTree.query(RectFloat(50, 50, 60, 60));
		ASSERT_EQ(0u, contents.size())<<"expected to find nothing in an empty tree";
	}
	const Item item1(RectFloat(51, 51, 53, 53), 1);
	ASSERT_TRUE(quadTree.insert(item1));
	{
		const QuadTree<Item, float>::Contents& contents = quadTree.query(RectFloat::getMaxRect());
		ASSERT_EQ(1u, contents.size())<<"expected to find one entry for the max rect";
	}
	{
		const QuadTree<Item, float>::Contents& contents = quadTree.query(item1.getRect());
		ASSERT_EQ(1u, contents.size())<<"expected to find one entry for the item rect";
	}
}

}
