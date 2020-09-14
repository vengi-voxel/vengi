/**
 * @file
 */

#pragma once

#include "EntityTest.h"
#include "backend/entity/User.h"
#include "stock/StockDataProvider.h"
#include "persistence/DBHandler.h"
#include "persistence/PersistenceMgr.h"
#include "BackendModels.h"

namespace backend {

namespace {
const char *INV = R"(function init()
	local i = stock.createItem(1, 'WEAPON', 'some-id')
	local s = i:shape()
	s:addRect(0, 0, 1, 1)

	local invMain = stock.createContainer(1, 'main')
	local invMainShape = invMain:shape()
	invMainShape:addRect(0, 0, 1, 1)
end
)";

const char *COOLDOWNS = R"(
return {
	INCREASE = 15000,
	HUNT = 10000,
	LOGOUT = 10
}
)";

}

class UserTest: public EntityTest {
private:
	using Super = EntityTest;
protected:
	bool _dbSupported = false;
	persistence::DBHandlerPtr dbHandler;
	persistence::PersistenceMgrPtr persistenceMgr;
	stock::StockDataProviderPtr stockDataProvider;
	cooldown::CooldownProviderPtr cooldownProvider;

	void SetUp() override {
		Super::SetUp();
		stockDataProvider = std::make_shared<stock::StockDataProvider>();
		cooldownProvider = std::make_shared<cooldown::CooldownProvider>();

		core::Var::get(cfg::ServerUserTimeout, "60000");
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "enginetest");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabasePort, "5432");
		core::Var::get(cfg::DatabaseUser, "vengi");
		core::Var::get(cfg::DatabasePassword, "engine");

		dbHandler = std::make_shared<persistence::DBHandler>();
		_dbSupported = dbHandler->init();
		persistenceMgr = std::make_shared<persistence::PersistenceMgr>(dbHandler);
		if (_dbSupported) {
			dbHandler->createOrUpdateTable(db::UserModel());
			ASSERT_TRUE(persistenceMgr->init());
		}
		ASSERT_TRUE(stockDataProvider->init(INV)) << stockDataProvider->error();
		ASSERT_TRUE(cooldownProvider->init(COOLDOWNS)) << cooldownProvider->error();
	}

	inline UserPtr create(EntityId id, const char* name = "noname") {
		const UserPtr& u = std::make_shared<User>(nullptr, id, name, map, messageSender, timeProvider,
				containerProvider, cooldownProvider, dbHandler, persistenceMgr, stockDataProvider);
		u->init();
		map->addUser(u);
		entityStorage->addUser(u);
		return u;
	}

	void shutdown(EntityId id) {
		entityStorage->removeUser(id);
	}
};

}
