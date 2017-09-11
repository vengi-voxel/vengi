/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "DatabaseModels.h"
#include "persistence/ConnectionPool.h"
#include "config.h"

namespace backend {

class DatabaseModelTest: public core::AbstractTest {
	using Super = core::AbstractTest;
public:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "engine");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabaseUser, "engine");
		core::Var::get(cfg::DatabasePassword, "engine");

		core::Singleton<::persistence::ConnectionPool>::getInstance().init();
		ASSERT_TRUE(persistence::UserStore::createTable()) << "Could not create table";
		persistence::UserStore::truncate();
	}

	void TearDown() override {
		Super::TearDown();
		core::Singleton<::persistence::ConnectionPool>::getInstance().shutdown();
	}

	void createUser(const std::string& email, const std::string& password) {
		const ::persistence::Timestamp ts = ::persistence::Timestamp::now();
		persistence::UserStore u;
		ASSERT_EQ(0, u.userid());
		ASSERT_TRUE(u.insert(email, password, ts));
		ASSERT_NE(0, u.userid());

		persistence::UserStore u2nd(&email, &password, nullptr);
		ASSERT_EQ(u2nd.userid(), u.userid());
	}
};

TEST_F(DatabaseModelTest, testCreateUser) {
	createUser("a@b.c.d", "secret");
}

TEST_F(DatabaseModelTest, testCreateUsers) {
	for (int i = 0; i < 100; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret");
	}
}

}
