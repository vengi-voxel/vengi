/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Singleton.h"
#include "persistence/ConnectionPool.h"

namespace persistence {

class ConnectionPoolTest : public core::AbstractTest {
public:
	void SetUp() override {
		core::AbstractTest::SetUp();
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "engine");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabaseUser, "engine");
		core::Var::get(cfg::DatabasePassword, "engine");
	}
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
