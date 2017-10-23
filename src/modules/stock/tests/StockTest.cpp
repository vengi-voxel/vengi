/**
 * @file
 */

#include "stock/tests/AbstractStockTest.h"
#include "stock/Stock.h"

namespace stock {

class StockTest: public AbstractStockTest {
};

TEST_F(StockTest, testAddAndRemove) {
	Stock stock;
	ASSERT_EQ(1, _item1->amount());
	ASSERT_EQ(_item1, stock.add(_item1)) << "Could not add item to stock";
	const ItemPtr& item3 = _provider.createItem(_itemData1->id());
	item3->changeAmount(1);
	ASSERT_EQ(_item1, stock.add(item3)) << "Got wrong item from stock - item1 and item3 are the same";
	ASSERT_EQ(2, stock.count(_item1->type()));
	ASSERT_EQ(2, _item1->amount());
	ASSERT_EQ(0, stock.remove(_item1)) << "Could not remove from stock";
	ASSERT_EQ(0, stock.count(_item1->type()));
}

}
