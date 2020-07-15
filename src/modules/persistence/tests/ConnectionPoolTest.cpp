/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "core/Singleton.h"
#include "persistence/ConnectionPool.h"
#include "persistence/ScopedConnection.h"
#include "persistence/postgres/PQSymbol.h"
#include <gtest/gtest.h>

namespace persistence {

class ConnectionPoolTest : public AbstractDatabaseTest {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	ConnectionPool _connectionPool;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = postgresInit() && _connectionPool.init();
		if (!_supported) {
			Log::warn("ConnectionPoolTest is skipped");
		}
	}

	void TearDown() override {
		Super::TearDown();
		_connectionPool.shutdown();
		postgresShutdown();
	}
};

TEST_F(ConnectionPoolTest, testConnectionPoolGetConnection) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_connectionPool.init());
	EXPECT_EQ(core::Var::get(cfg::DatabaseMinConnections)->intVal(), _connectionPool.connections()) << "Unexpected connection amount";
	const ScopedConnection scoped(_connectionPool, _connectionPool.connection());
	EXPECT_EQ(core::Var::get(cfg::DatabaseMinConnections)->intVal(), _connectionPool.connections()) << "Connection amount should not change";
	EXPECT_TRUE((bool)scoped) << "ScopedConnection should hold a valid connection";
}

}
