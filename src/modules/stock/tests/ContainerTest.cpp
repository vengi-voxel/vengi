/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Container.h"

namespace stock {

class ContainerTest: public AbstractStockTest {
};

TEST_F(ContainerTest, testAddAndRemove) {
	Container c;
	ContainerShape shape;
	shape.addRect(0, 1, 1, 1);
	c.init(shape);
	Item* item1 = _provider.createItem(1);
	ASSERT_FALSE(c.add(item1, 0, 0));
	ASSERT_TRUE(c.add(item1, 0, 1));
	Item* item2 = _provider.createItem(2);
	ASSERT_FALSE(c.add(item2, 0, 0));
	ASSERT_FALSE(c.add(item2, 0, 1));
	ASSERT_EQ(item1, c.remove(0, 1));
	ASSERT_EQ(1, c.free());
	ASSERT_TRUE(c.add(item2, 0, 1));
	ASSERT_EQ(1, c.size());
	ASSERT_EQ(0, c.free());
	delete item1;
	delete item2;
}

}
