/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../EventMgr.h"

namespace eventmgr {

class EventMgrTest : public core::AbstractTest {
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

TEST_F(EventMgrTest, testEventMgrInit) {
	const persistence::DBHandlerPtr& dbHandler = std::make_shared<persistence::DBHandler>();
	ASSERT_TRUE(dbHandler->init()) << "Could not initialize dbhandler";
	EventMgr mgr(dbHandler);
	ASSERT_TRUE(mgr.init()) << "Could not initialize eventmgr";
	mgr.shutdown();
	dbHandler->shutdown();
}

}
