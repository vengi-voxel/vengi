/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "UserModel.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
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
		ASSERT_TRUE(persistence::UserModel::createTable()) << "Could not create table";
		::persistence::DBHandler::truncate(persistence::UserModel());
	}

	void TearDown() override {
		Super::TearDown();
		core::Singleton<::persistence::ConnectionPool>::getInstance().shutdown();
	}

	void createUser(const std::string& email, const std::string& password) {
		const ::persistence::Timestamp ts = ::persistence::Timestamp::now();
		persistence::UserModel u;
		ASSERT_EQ(0, u.id());
		ASSERT_TRUE(u.insert(email, password, ts));
		ASSERT_NE(0, u.id());

		persistence::UserModel u2nd;
		ASSERT_TRUE(u2nd.select(email.c_str(), password.c_str(), nullptr));
		ASSERT_EQ(u2nd.id(), u.id());
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
