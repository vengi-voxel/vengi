/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Shape.h"
#include "core/Common.h"

// recursive macro helpers to represent binary masks
template<uint64_t N>
struct Binary { static const uint64_t value = Binary<N / 10>::value << 1 | (N % 10); };
template<uint64_t N>
const uint64_t Binary<N>::value;
template<>
struct Binary<0> { static const uint64_t value = uint64_t(0); };

namespace stock {

class ShapeTest: public AbstractStockTest {
};

TEST_F(ShapeTest, testContainerShapeRect) {
	ContainerShape shape;
	EXPECT_TRUE(shape.addRect(0, 1, 1, 1));
	EXPECT_TRUE(shape.addRect(1, 1, 4, 4));
	EXPECT_FALSE(shape.isInShape(0, 0));
	EXPECT_TRUE(shape.isInShape(1, 1));
	EXPECT_TRUE(shape.isFree(1, 1));
	shape.addShape((ItemShapeType)Binary<1>::value, 1, 1);
	EXPECT_FALSE(shape.isFree(1, 1));
	shape.removeShape((ItemShapeType)Binary<1>::value, 1, 1);
	EXPECT_TRUE(shape.isFree(1, 1));
	EXPECT_EQ(17, shape.size());
	EXPECT_EQ(17, shape.free());
}

TEST_F(ShapeTest, testContainerShapeRectOutside32Bits) {
	ContainerShape shape;
	EXPECT_TRUE(shape.addRect(33, 0, 1, 1));
	EXPECT_TRUE(shape.isInShape(33, 0));
}

TEST_F(ShapeTest, testItemShapeSingleSetAndTest) {
	ItemShape shape;
	EXPECT_EQ(0, shape.size());
	EXPECT_EQ(0, shape.height());
	EXPECT_EQ(0, shape.width());
	shape.set(1, 1);
	EXPECT_FALSE(shape.isInShape(0, 0));
	EXPECT_FALSE(shape.isInShape(0, 1));
	EXPECT_FALSE(shape.isInShape(0, 2));
	EXPECT_FALSE(shape.isInShape(1, 0));
	EXPECT_TRUE(shape.isInShape(1, 1));
	EXPECT_FALSE(shape.isInShape(1, 2));
	EXPECT_FALSE(shape.isInShape(2, 0));
	EXPECT_FALSE(shape.isInShape(2, 1));
	EXPECT_FALSE(shape.isInShape(2, 2));
	EXPECT_EQ(1, shape.size());
	EXPECT_EQ(2, shape.height());
	EXPECT_EQ(2, shape.width());
}

TEST_F(ShapeTest, testItemShapeRect) {
	ItemShape shape;
	EXPECT_EQ(0, shape.size());
	EXPECT_EQ(0, shape.height());
	EXPECT_EQ(0, shape.width());
	EXPECT_TRUE(shape.addRect(1, 1, 1, 1));
	EXPECT_FALSE(shape.isInShape(0, 0));
	EXPECT_FALSE(shape.isInShape(0, 1));
	EXPECT_FALSE(shape.isInShape(0, 2));
	EXPECT_FALSE(shape.isInShape(1, 0));
	EXPECT_TRUE(shape.isInShape(1, 1));
	EXPECT_FALSE(shape.isInShape(1, 2));
	EXPECT_FALSE(shape.isInShape(2, 0));
	EXPECT_FALSE(shape.isInShape(2, 1));
	EXPECT_FALSE(shape.isInShape(2, 2));
	EXPECT_EQ(1, shape.size());
	EXPECT_EQ(2, shape.height());
	EXPECT_EQ(2, shape.width());
}

TEST_F(ShapeTest, testContainerShape) {
	ContainerShape containerShape;
	EXPECT_TRUE(containerShape.addRect(0, 0, 2, 2));
	EXPECT_TRUE(containerShape.addRect(2, 0, 2, 1));
	EXPECT_EQ(6, containerShape.size());
	ItemShape itemShape;
	itemShape.set(0, 0);
	const ItemShapeType itemShapeType = static_cast<ItemShapeType>(itemShape);
	EXPECT_EQ((ItemShapeType)Binary<1>::value, itemShapeType);
	EXPECT_TRUE(containerShape.isFree(itemShape, 0, 0));
	containerShape.addShape(itemShapeType, 0, 0);
	EXPECT_EQ(5, containerShape.free());
	EXPECT_FALSE(containerShape.isFree(itemShape, 0, 0));
	containerShape.removeShape(itemShapeType, 0, 0);
	EXPECT_EQ(6, containerShape.size());
	EXPECT_EQ(6, containerShape.free());
	EXPECT_TRUE(containerShape.isFree(itemShape, 0, 0));
}

}
