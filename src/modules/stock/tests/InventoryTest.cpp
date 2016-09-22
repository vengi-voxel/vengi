/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Inventory.h"

namespace stock {

class InventoryTest: public AbstractStockTest {
protected:
	Inventory _inv;
	const uint8_t _containerId = 0u;
	const Container* _container;
	Item* _item1;
	Item* _item2;

public:
	virtual void SetUp() override {
		AbstractStockTest::SetUp();
		_itemData1->setSize(1, 2);
		_itemData2->setSize(1, 1);

		ContainerShape shape;
		shape.addRect(0, 1, 1, 1);
		shape.addRect(1, 1, 4, 4);
		ASSERT_TRUE(_inv.initContainer(_containerId, shape));

		_container = _inv.container(_containerId);

		_item1 = _provider.createItem(_itemData1->id());
		_item2 = _provider.createItem(_itemData2->id());
		ASSERT_EQ(2, _item1->shape().size());
		ASSERT_EQ(1, _item2->shape().size());
	}

	virtual void TearDown() override {
		AbstractStockTest::TearDown();
		delete _item1;
		delete _item2;
		_item1 = _item2 = nullptr;
	}
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
