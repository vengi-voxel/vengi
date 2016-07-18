/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "persistence/ConnectionPool.h"

namespace persistence {

class ConnectionPoolTest : public core::AbstractTest {
};

TEST(ConnectionPoolTest, testConnectionPoolSize) {
	ConnectionPool& pool = ConnectionPool::get();
	ASSERT_EQ(2, pool.init());
	pool.shutdown();
}

TEST(ConnectionPoolTest, testConnectionPoolGetConnection) {
	ConnectionPool& pool = ConnectionPool::get();
	ASSERT_EQ(2, pool.init());
	Connection* c = pool.connection();
	ASSERT_NE(nullptr, c);
	pool.shutdown();
}

}
