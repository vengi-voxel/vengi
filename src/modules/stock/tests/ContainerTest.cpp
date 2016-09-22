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
	ASSERT_FALSE(c.add(_item1, 0, 0));
	ASSERT_TRUE(c.add(_item1, 0, 1));
	ASSERT_FALSE(c.add(_item2, 0, 0));
	ASSERT_FALSE(c.add(_item2, 0, 1));
	ASSERT_EQ(_item1, c.remove(0, 1));
	ASSERT_EQ(1, c.free());
	ASSERT_TRUE(c.add(_item2, 0, 1));
	ASSERT_EQ(1, c.size());
	ASSERT_EQ(0, c.free());
}

}
