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
public:
	void SetUp() override {
		Super::SetUp();
		ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
		_supported = pool.init();
		if (!_supported) {
			Log::warn("ConnectionPoolTest is skipped");
		}
	}
};

TEST_F(ConnectionPoolTest, testConnectionPoolGetConnection) {
	if (!_supported) {
		return;
	}
	ConnectionPool& pool = core::Singleton<ConnectionPool>::getInstance();
	ASSERT_TRUE(pool.init());
	Connection* c = pool.connection();
	ASSERT_NE(nullptr, c);
	pool.shutdown();
}

}
