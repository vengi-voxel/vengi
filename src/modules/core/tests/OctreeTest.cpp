/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Octree.h"

namespace core {

class Item {
private:
	AABB<int> _bounds;
	int _id;
public:
	Item(const AABB<int>& bounds, int id) :
			_bounds(bounds), _id(id) {
	}

	const AABB<int>& aabb() const {
		return _bounds;
	}

	bool operator==(const Item& rhs) const {
		return rhs._id == _id;
	}
};

TEST(OctreeTest, testAdd) {
	Octree<Item, int> octree({0, 0, 0, 100, 100, 100});
	EXPECT_EQ(0, octree.count())<< "Expected to have no entries in the octree";
	EXPECT_TRUE(octree.insert({{51, 51, 51, 53, 53, 53}, 1}));
	EXPECT_EQ(1, octree.count())<< "Expected to have 1 entry in the octree";
	EXPECT_TRUE(octree.insert({{15, 15, 15, 18, 18, 18}, 2}));
	EXPECT_EQ(2, octree.count())<< "Expected to have 2 entries in the octree";
}

TEST(OctreeTest, testRemove) {
	Octree<Item, int> octree({0, 0, 0, 100, 100, 100});
	EXPECT_EQ(0, octree.count())<< "Expected to have no entries in the octree";
	const Item item({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item));
	const Item item2({52, 52, 52, 54, 55, 55}, 2);
	EXPECT_TRUE(octree.insert(item2));
	EXPECT_EQ(2, octree.count())<< "Expected to have 2 entries in the octree";
	EXPECT_TRUE(octree.remove(item));
	EXPECT_EQ(1, octree.count())<<"Expected to have 0 entries in the octree";
}

TEST(OctreeTest, testQuery) {
	// TODO: not using Item* but Item here results in valgrind errors...
	Octree<Item*, int> octree({0, 0, 0, 100, 100, 100}, 3);
	{
		Octree<Item*, int>::Contents contents;
		octree.query({50, 50, 50, 60, 60, 60}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
	}
	{
		Octree<Item*, int>::Contents contents;
		octree.query({52, 52, 52, 54, 54, 54}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
	}
	Item* item1 = new Item({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item1));
	{
		Octree<Item*, int>::Contents contents;
		octree.query(item1->aabb(), contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the item aabb";
	}
	{
		Octree<Item*, int>::Contents contents;
		octree.query({52, 52, 52, 54, 54, 54}, contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
	}
	{
		Octree<Item*, int>::Contents contents;
		octree.query({50, 50, 50, 52, 52, 52}, contents);
		EXPECT_TRUE(intersects(item1->aabb(), {50, 50, 50, 52, 52, 52}));
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
	}
}

TEST(OctreeTest, testOctreeCache) {
	Octree<Item*, int> octree({0, 0, 0, 100, 100, 100});
	OctreeCache<Item*, int> cache(octree);
	{
		Octree<Item*, int>::Contents contents;
		octree.query({50, 50, 50, 60, 60, 60}, contents);
		EXPECT_EQ(0u, contents.size())<<"Expected to find nothing in an empty tree";
		contents.clear();
		EXPECT_FALSE(cache.query({50, 50, 50, 60, 60, 60}, contents));
		contents.clear();
		EXPECT_TRUE(cache.query({50, 50, 50, 60, 60, 60}, contents));
	}
	Item* item = new Item({51, 51, 51, 53, 53, 53}, 1);
	EXPECT_TRUE(octree.insert(item));
	{
		Octree<Item*, int>::Contents contents;
		EXPECT_FALSE(cache.query({50, 50, 50, 60, 60, 60}, contents)) << "Expected to have the cache cleared, the octree was in a dirty state";
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the enclosing aabb";
	}
	{
		Octree<Item*, int>::Contents contents;
		octree.query({50, 50, 50, 52, 52, 52}, contents);
		EXPECT_EQ(1u, contents.size())<<"Expected to find one entry for the overlapping aabb";
		contents.clear();
		EXPECT_FALSE(cache.query({50, 50, 50, 52, 52, 52}, contents));
		contents.clear();
		EXPECT_TRUE(cache.query({50, 50, 50, 52, 52, 52}, contents));
	}
	delete item;
}

}
