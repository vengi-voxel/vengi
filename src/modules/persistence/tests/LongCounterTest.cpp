/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "persistence/LongCounter.h"

namespace persistence {

class LongCounterTest : public app::AbstractTest {
};

TEST_F(LongCounterTest, testUpdate) {
	LongCounter c(0);
	EXPECT_EQ(0, c.update());
	c.change(100);
	EXPECT_EQ(100, c.update());
	c.change(1);
	EXPECT_EQ(1, c.update());
	c.change(1);
	c.change(1);
	EXPECT_EQ(2, c.update());
}

TEST_F(LongCounterTest, DISABLED_testSet) {
	LongCounter c(100);
	c.set(100);
	EXPECT_EQ(0, c.update());
	c.set(102);
	EXPECT_EQ(2, c.update());
	c.set(98);
	EXPECT_EQ(-2, c.update());
}

}
