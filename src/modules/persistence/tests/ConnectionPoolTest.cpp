/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "persistence/ConnectionPool.h"

namespace persistence {

class ConnectionPoolTest : public core::AbstractTest {
public:
	void SetUp() override {
		core::AbstractTest::SetUp();
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
	}
};

TEST_F(ConnectionPoolTest, testConnectionPoolSize) {
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_EQ(2, pool.init());
	pool.shutdown();
}

TEST_F(ConnectionPoolTest, testConnectionPoolGetConnection) {
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_EQ(2, pool.init());
	Connection* c = pool.connection();
	ASSERT_NE(nullptr, c);
	pool.shutdown();
}

TEST_F(ConnectionPoolTest, testConnectionPoolInvalidData) {
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_EQ(2, pool.init("invalid", "invalid", "invalid", "invalid"));
	ASSERT_EQ("invalid", core::Var::get(cfg::DatabaseName)->strVal());
	Connection* c = pool.connection();
	ASSERT_EQ(nullptr, c);
	pool.shutdown();
}

}
