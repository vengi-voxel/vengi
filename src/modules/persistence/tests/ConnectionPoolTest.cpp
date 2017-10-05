/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "core/Singleton.h"
#include "persistence/ConnectionPool.h"

namespace persistence {

class ConnectionPoolTest : public AbstractDatabaseTest {
};

TEST_F(ConnectionPoolTest, testConnectionPoolSize) {
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_EQ(1, pool.init());
	pool.shutdown();
}

TEST_F(ConnectionPoolTest, testConnectionPoolGetConnection) {
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_EQ(1, pool.init());
	Connection* c = pool.connection();
	ASSERT_NE(nullptr, c);
	pool.shutdown();
}

}
