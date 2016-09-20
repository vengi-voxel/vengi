/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/ItemProvider.h"

namespace stock {

class ItemProviderTest: public AbstractStockTest {
};

TEST_F(ItemProviderTest, testResetAndDuplicate) {
	ItemProvider provider;
	ASSERT_TRUE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	ASSERT_TRUE(provider.addItemData(new ItemData(2, ItemType::WEAPON)));
	ASSERT_FALSE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	provider.reset();
	ASSERT_TRUE(provider.addItemData(new ItemData(1, ItemType::WEAPON)));
	provider.shutdown();
}

TEST_F(ItemProviderTest, testInit) {
	const char* lua = ""
		"function init()\n"
		"  local i = item.createItem(1, 'WEAPON')\n"
		//"  print(i)\n"
		"  local s = i:getShape()\n"
		"  s:addRect(0, 0, 1, 1)\n"
		"end\n";
	ItemProvider provider;
	ASSERT_TRUE(provider.init(lua)) << provider.error();
	const ItemData* itemData = provider.getItemData(1);
	ASSERT_NE(nullptr, itemData);
	ASSERT_EQ(ItemType::WEAPON, itemData->type());
	ASSERT_EQ((ItemId)1, itemData->id());
	ASSERT_TRUE(itemData->shape().isInShape(0, 0));
	ASSERT_FALSE(itemData->shape().isInShape(1, 0));
	ASSERT_FALSE(itemData->shape().isInShape(1, 1));
	ASSERT_FALSE(itemData->shape().isInShape(0, 1));
}

}
