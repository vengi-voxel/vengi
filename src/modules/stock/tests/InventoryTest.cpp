/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"

namespace stock {

class InventoryTest: public AbstractStockTest {
};

TEST_F(InventoryTest, testAddValidLocation) {
	ASSERT_TRUE(_inv.add(_containerId, _item1, 1, 1)) << "Could not place item to valid container position";
	ASSERT_TRUE(_container->hasItemOfType(_itemData1->type()));
	ASSERT_EQ(15, _container->free());
}

TEST_F(InventoryTest, testAddInvalidLocation) {
	ASSERT_FALSE(_inv.add(_containerId, _item1, 0, 0)) << "Could place item to invalid container position";
	ASSERT_FALSE(_container->hasItemOfType(_itemData1->type()));
	ASSERT_EQ(17, _container->free());
}

TEST_F(InventoryTest, testAddAndRemove) {
	ASSERT_TRUE(_inv.add(_containerId, _item1, 1, 1)) << "Could not place item to valid container position";
	ASSERT_TRUE(_container->hasItemOfType(_itemData1->type()));
	ASSERT_EQ(15, _container->free());
	ASSERT_EQ(_item1, _inv.remove(_containerId, 1, 2)) << "Could not remove item from container 1, 2 - even though the item with a size of two should also occupy this field";
	ASSERT_EQ(17, _container->free());
}

TEST_F(InventoryTest, testRemoveFromInvalidLocation) {
	ASSERT_TRUE(_inv.add(_containerId, _item1, 1, 1)) << "Could not place item to valid container position";
	ASSERT_EQ(nullptr, _inv.remove(_containerId, 3, 3)) << "Removed item from invalid position 3, 3";
	ASSERT_EQ(nullptr, _inv.remove(_containerId, 3, 1)) << "Removed item from invalid position 3, 1";
	ASSERT_EQ(nullptr, _inv.remove(_containerId, 2, 1)) << "Removed item from invalid position 2, 1";
	ASSERT_EQ(15, _container->free());
}

}
