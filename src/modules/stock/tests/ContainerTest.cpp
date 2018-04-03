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
	EXPECT_TRUE(shape.addRect(0, 1, 1, 1));
	c.init(shape);
	EXPECT_FALSE(c.add(_item1, 0, 0));
	EXPECT_TRUE(c.add(_item1, 0, 1));
	EXPECT_FALSE(c.add(_item2, 0, 0));
	EXPECT_FALSE(c.add(_item2, 0, 1));
	EXPECT_EQ(_item1, c.remove(0, 1));
	EXPECT_EQ(1, c.free());
	EXPECT_TRUE(c.add(_item2, 0, 1));
	EXPECT_EQ(1, c.size());
	EXPECT_EQ(0, c.free());
}

TEST_F(ContainerTest, testNotUnique) {
	ContainerShape shape;
	EXPECT_TRUE(shape.addRect(0, 0, 2, 2));
	Container c;
	c.init(shape);
	EXPECT_TRUE(c.add(_item2, 0, 0));
	EXPECT_TRUE(c.add(_item2, 0, 1));
}

TEST_F(ContainerTest, testSingle) {
	ContainerShape shape;
	EXPECT_TRUE(shape.addRect(0, 0, 30, 30));
	Container c;
	c.init(shape, Container::Single);
	EXPECT_TRUE(c.add(_item2, 0, 0));
	EXPECT_FALSE(c.add(_item1, 0, 1));
}

TEST_F(ContainerTest, testUnique) {
	ContainerShape shape;
	EXPECT_TRUE(shape.addRect(0, 0, 2, 2));
	Container c;
	c.init(shape, Container::Unique);
	EXPECT_TRUE(c.add(_item2, 0, 0));
	EXPECT_FALSE(c.add(_item2, 0, 1));
}

}
