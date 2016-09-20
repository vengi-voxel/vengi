/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Inventory.h"

namespace stock {

class InventoryTest: public AbstractStockTest {
};

TEST_F(InventoryTest, testAdd) {
	_provider.reset();
	ItemData* itemData = new ItemData(1, ItemType::WEAPON);
	itemData->setSize(1, 2);
	ASSERT_TRUE(_provider.addItemData(itemData));
	Inventory inv;
	ContainerShape shape;
	shape.addRect(0, 1, 1, 1);
	shape.addRect(1, 1, 4, 4);
	ASSERT_EQ(17, shape.size());
	ASSERT_EQ(17, shape.free());
	ASSERT_TRUE(shape.isInShape(1, 1));
	const uint8_t containerId = 0u;
	ASSERT_TRUE(inv.initContainer(containerId, shape));
	Item* item1 = _provider.createItem(itemData->id());
	ASSERT_FALSE(inv.add(containerId, item1, 0, 0)) << "Could place item to invalid container position";
	ASSERT_FALSE(inv.container(containerId)->hasItemOfType(itemData->type()));
	Item* item2 = _provider.createItem(itemData->id());
	ASSERT_EQ(2, item2->shape().size());
	ASSERT_TRUE(inv.add(containerId, item2, 1, 1)) << "Could not place item to valid container position";
	ASSERT_TRUE(inv.container(containerId)->hasItemOfType(itemData->type()));
	ASSERT_EQ(15, inv.container(containerId)->free());
	ASSERT_EQ(nullptr, inv.remove(containerId, 3, 3)) << "Removed item from invalid position 3, 3";
	ASSERT_EQ(nullptr, inv.remove(containerId, 3, 1)) << "Removed item from invalid position 3, 1";
	ASSERT_EQ(nullptr, inv.remove(containerId, 2, 1)) << "Removed item from invalid position 2, 1";
	ASSERT_NE(nullptr, inv.remove(containerId, 1, 2)) << "Could not remove item from container 1, 2 - even though the item with a size of two should also occupy this field";
	ASSERT_EQ(17, shape.free());
	delete item1;
	delete item2;
}

}
