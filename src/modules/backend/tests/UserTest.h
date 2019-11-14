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

const char *COOLDOWNS = R"(addCooldown("INCREASE", 15000)
addCooldown("HUNT", 10000)
addCooldown("LOGOUT", 100)
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
		ASSERT_TRUE(stockDataProvider->init(INV)) << stockDataProvider->error();
		cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		ASSERT_TRUE(cooldownProvider->init(COOLDOWNS)) << cooldownProvider->error();

		core::Var::get(cfg::ServerUserTimeout, "60000");
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "enginetest");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabaseUser, "engine");
		core::Var::get(cfg::DatabasePassword, "engine");

		dbHandler = std::make_shared<persistence::DBHandler>();
		_dbSupported = dbHandler->init();
		persistenceMgr = std::make_shared<persistence::PersistenceMgr>(dbHandler);
		if (_dbSupported) {
			dbHandler->createOrUpdateTable(db::UserModel());
			ASSERT_TRUE(persistenceMgr->init());
		}
	}

	inline UserPtr create(EntityId id, const char* name = "noname") {
		const UserPtr& u = std::make_shared<User>(nullptr, id, name, map, messageSender, timeProvider,
				containerProvider, cooldownProvider, dbHandler, persistenceMgr, stockDataProvider);
		u->init();
		map->addUser(u);
		entityStorage->addUser(u);
		return u;
	}

	void shutdown(const UserPtr& user) {
		entityStorage->removeUser(user->id());
	}
};

}
