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
	ASSERT_EQ(1, stock.add(_item1)) << "Could not add item to stock";
	ASSERT_EQ(1, stock.count(_item1->type()));
	ASSERT_EQ(1, stock.remove(_item1)) << "Could not remove from stock";
	ASSERT_EQ(0, stock.count(_item1->type()));
}

}
