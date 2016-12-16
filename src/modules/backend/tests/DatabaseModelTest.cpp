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
	}

	void TearDown() override {
		Super::TearDown();
		core::Singleton<::persistence::ConnectionPool>::getInstance().shutdown();
	}
};

TEST_F(DatabaseModelTest, testCreate) {
	ASSERT_TRUE(persistence::UserStore::createTable()) << "Could not create table";
}

TEST_F(DatabaseModelTest, testWrite) {
	ASSERT_TRUE(persistence::UserStore::createTable()) << "Could not create table";
	const std::string email = "a@b.c.d";
	const std::string password = "secret";
	persistence::UserStore::truncate();
	const ::persistence::Timestamp ts = ::persistence::Timestamp::now();
	persistence::UserStore u(&email, &password, nullptr);
	ASSERT_EQ(0, u.userid());
	ASSERT_TRUE(u.insert(email, password, ts));
	ASSERT_NE(0, u.userid());

	persistence::UserStore u2nd(&email, &password, nullptr);
	ASSERT_EQ(u2nd.userid(), u.userid());
}

}
