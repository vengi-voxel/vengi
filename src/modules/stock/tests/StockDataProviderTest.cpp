/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "stock/StockDataProvider.h"

namespace stock {

class StockDataProviderTest: public core::AbstractTest {
};

TEST_F(StockDataProviderTest, testResetAndDuplicate) {
	StockDataProvider provider;
	ASSERT_TRUE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	ASSERT_TRUE(provider.addItemData(new ItemData(2, ItemType::WEAPON)));
	ASSERT_FALSE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	provider.reset();
	ASSERT_TRUE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	provider.shutdown();
}

TEST_F(StockDataProviderTest, testInit) {
	const char* lua = ""
		"function init()\n"
		"  local i = stock.createItem(1, 'WEAPON', 'some-id')\n"
		//"  print(i)\n"
		"  local s = i:shape()\n"
		"  s:addRect(0, 0, 1, 1)\n"
		"\n"
		"  local invMain = stock.createContainer(\"main\")\n"
		"  local invMainShape = invMain:shape()\n"
		"  invMainShape:addRect(0, 0, 1, 1)\n"
		"end\n";
	StockDataProvider provider;
	ASSERT_TRUE(provider.init(lua)) << provider.error();
	const ItemData* itemData = provider.itemData(1);
	ASSERT_NE(nullptr, itemData);
	ASSERT_EQ(ItemType::WEAPON, itemData->type());
	ASSERT_EQ((ItemId)1, itemData->id());
	ASSERT_TRUE(itemData->shape().isInShape(0, 0));
	ASSERT_FALSE(itemData->shape().isInShape(1, 0));
	ASSERT_FALSE(itemData->shape().isInShape(1, 1));
	ASSERT_FALSE(itemData->shape().isInShape(0, 1));
}

}
