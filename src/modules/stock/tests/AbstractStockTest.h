/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "stock/StockDataProvider.h"
#include "stock/Inventory.h"
#include "stock/Container.h"

namespace stock {

class AbstractStockTest: public app::AbstractTest {
protected:
	StockDataProviderPtr _provider;
	ItemData *_itemData1;
	ItemData *_itemData2;
	Inventory _inv;
	const uint8_t _containerId = 0u;
	const Container* _container;
	ItemPtr _item1;
	ItemPtr _item2;
public:
	virtual void SetUp() override {
		app::AbstractTest::SetUp();
		_provider = std::make_shared<StockDataProvider>();
		_itemData1 = new ItemData(1, ItemType::WEAPON);
		_itemData1->setSize(1, 2);
		EXPECT_EQ(2, _itemData1->shape().size());
		EXPECT_TRUE(_provider->addItemData(_itemData1)) << "Could not add item to provider for id 1";
		EXPECT_EQ(_itemData1, _provider->itemData(1)) << "Could not get item data for id 1";
		_itemData2 = new ItemData(2, ItemType::WEAPON);
		_itemData2->setSize(1, 1);
		EXPECT_EQ(1, _itemData2->shape().size());
		EXPECT_TRUE(_provider->addItemData(_itemData2)) << "Could not add item to provider for id 2";
		EXPECT_EQ(_itemData1, _provider->itemData(1)) << "Could not get item data for id 1";
		EXPECT_EQ(_itemData2, _provider->itemData(2)) << "Could not get item data for id 2";
		ItemData* itemDataDup = new ItemData(1, ItemType::WEAPON);
		EXPECT_FALSE(_provider->addItemData(itemDataDup)) << "Added duplicated item id 1 to item provider";
		delete itemDataDup;

		ContainerShape shape;
		shape.addRect(0, 1, 1, 1);
		shape.addRect(1, 1, 4, 4);
		EXPECT_TRUE(_inv.initContainer(_containerId, shape));

		_container = _inv.container(_containerId);

		_item1 = _provider->createItem(_itemData1->id());
		_item1->changeAmount(1);

		_item2 = _provider->createItem(_itemData2->id());
		_item2->changeAmount(1);
	}

	virtual void TearDown() override {
		app::AbstractTest::TearDown();
		_provider->shutdown();
		_itemData1 = nullptr;
		_itemData2 = nullptr;
	}
};

}
