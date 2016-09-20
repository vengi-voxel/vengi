/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Shape.h"

namespace stock {

class ShapeTest: public AbstractStockTest {
};

TEST_F(ShapeTest, testContainerShapeRect) {
	ContainerShape shape;
	shape.addRect(0, 1, 1, 1);
	shape.addRect(1, 1, 4, 4);
	ASSERT_FALSE(shape.isInShape(0, 0));
	ASSERT_TRUE(shape.isInShape(1, 1));
	ASSERT_TRUE(shape.isFree(1, 1));
	shape.addShape((ItemShapeType)1, 1, 1);
	ASSERT_FALSE(shape.isFree(1, 1));
	shape.removeShape((ItemShapeType)1, 1, 1);
	ASSERT_TRUE(shape.isFree(1, 1));
	ASSERT_EQ(17, shape.size());
	ASSERT_EQ(17, shape.free());
}

TEST_F(ShapeTest, testContainerShapeRectOutside32Bits) {
	ContainerShape shape;
	shape.addRect(33, 0, 1, 1);
	ASSERT_TRUE(shape.isInShape(33, 0));
}

TEST_F(ShapeTest, testItemShapeSingleSetAndTest) {
	ItemShape shape;
	ASSERT_EQ(0, shape.size());
	ASSERT_EQ(0, shape.height());
	ASSERT_EQ(0, shape.width());
	shape.set(1, 1);
	ASSERT_FALSE(shape.isInShape(0, 0));
	ASSERT_FALSE(shape.isInShape(0, 1));
	ASSERT_FALSE(shape.isInShape(0, 2));
	ASSERT_FALSE(shape.isInShape(1, 0));
	ASSERT_TRUE(shape.isInShape(1, 1));
	ASSERT_FALSE(shape.isInShape(1, 2));
	ASSERT_FALSE(shape.isInShape(2, 0));
	ASSERT_FALSE(shape.isInShape(2, 1));
	ASSERT_FALSE(shape.isInShape(2, 2));
	ASSERT_EQ(1, shape.size());
	ASSERT_EQ(2, shape.height());
	ASSERT_EQ(2, shape.width());
}

TEST_F(ShapeTest, testItemShapeRect) {
	ItemShape shape;
	ASSERT_EQ(0, shape.size());
	ASSERT_EQ(0, shape.height());
	ASSERT_EQ(0, shape.width());
	shape.addRect(1, 1, 1, 1);
	ASSERT_FALSE(shape.isInShape(0, 0));
	ASSERT_FALSE(shape.isInShape(0, 1));
	ASSERT_FALSE(shape.isInShape(0, 2));
	ASSERT_FALSE(shape.isInShape(1, 0));
	ASSERT_TRUE(shape.isInShape(1, 1));
	ASSERT_FALSE(shape.isInShape(1, 2));
	ASSERT_FALSE(shape.isInShape(2, 0));
	ASSERT_FALSE(shape.isInShape(2, 1));
	ASSERT_FALSE(shape.isInShape(2, 2));
	ASSERT_EQ(1, shape.size());
	ASSERT_EQ(2, shape.height());
	ASSERT_EQ(2, shape.width());
}

TEST_F(ShapeTest, testContainerShape) {
	ContainerShape containerShape;
	containerShape.addRect(0, 0, 2, 2);
	containerShape.addRect(2, 0, 2, 1);
	ASSERT_EQ(6, containerShape.size());
	ItemShape itemShape;
	itemShape.set(0, 0);
	const ItemShapeType itemShapeType = static_cast<ItemShapeType>(itemShape);
	ASSERT_EQ((ItemShapeType)1, itemShapeType);
	ASSERT_TRUE(containerShape.isFree(itemShape, 0, 0));
	containerShape.addShape(itemShapeType, 0, 0);
	ASSERT_EQ(5, containerShape.free());
	ASSERT_FALSE(containerShape.isFree(itemShape, 0, 0));
	containerShape.removeShape(itemShapeType, 0, 0);
	ASSERT_EQ(6, containerShape.size());
	ASSERT_EQ(6, containerShape.free());
	ASSERT_TRUE(containerShape.isFree(itemShape, 0, 0));
}

}
