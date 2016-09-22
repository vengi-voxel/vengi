/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "stock/ItemProvider.h"

namespace stock {

class AbstractStockTest: public core::AbstractTest {
public:
	ItemProvider _provider;
	ItemData *_itemData1;
	ItemData *_itemData2;

	virtual void SetUp() override {
		core::AbstractTest::SetUp();
		_itemData1 = new ItemData(1, ItemType::WEAPON);
		_itemData1->setSize(1, 1);
		ASSERT_TRUE(_provider.addItemData(_itemData1)) << "Could not add item to provider for id 1";
		ASSERT_EQ(_itemData1, _provider.getItemData(1)) << "Could not get item data for id 1";
		_itemData2 = new ItemData(2, ItemType::WEAPON);
		_itemData2->setSize(1, 1);
		ASSERT_TRUE(_provider.addItemData(_itemData2)) << "Could not add item to provider for id 2";
		ASSERT_EQ(_itemData1, _provider.getItemData(1)) << "Could not get item data for id 1";
		ASSERT_EQ(_itemData2, _provider.getItemData(2)) << "Could not get item data for id 2";
		ItemData* itemDataDup = new ItemData(1, ItemType::WEAPON);
		ASSERT_FALSE(_provider.addItemData(itemDataDup)) << "Added duplicated item id 1 to item provider";
		delete itemDataDup;
	}

	virtual void TearDown() override {
		core::AbstractTest::TearDown();
		_provider.shutdown();
		_itemData1 = nullptr;
		_itemData2 = nullptr;
	}
};

}
