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
	Item* item = _provider.createItem(1);
	ASSERT_NE(nullptr, item);
	ASSERT_EQ(1, stock.add(item)) << "Could not add item to stock";
	ASSERT_EQ(1, stock.count(item->type()));
	ASSERT_EQ(1, stock.remove(item)) << "Could not remove from stock";
	ASSERT_EQ(0, stock.count(item->type()));
	delete item;
}

}
