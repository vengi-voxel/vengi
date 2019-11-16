/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "core/Singleton.h"
#include "persistence/ConnectionPool.h"

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
		_supported = _connectionPool.init();
		if (!_supported) {
			Log::warn("ConnectionPoolTest is skipped");
		}
	}

	void TearDown() override {
		Super::TearDown();
		_connectionPool.shutdown();
	}
};

TEST_F(ConnectionPoolTest, testConnectionPoolGetConnection) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_connectionPool.init());
	Connection* c = _connectionPool.connection();
	ASSERT_NE(nullptr, c);
	_connectionPool.shutdown();
}

}
