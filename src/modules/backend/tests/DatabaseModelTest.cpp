/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "UserModel.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
#include "core/GameConfig.h"
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

		core::Singleton<persistence::ConnectionPool>::getInstance().init();
		ASSERT_TRUE(db::UserModel::createTable()) << "Could not create table";
		persistence::DBHandler::truncate(db::UserModel());
	}

	void TearDown() override {
		Super::TearDown();
		core::Singleton<persistence::ConnectionPool>::getInstance().shutdown();
	}

	void createUser(const std::string& email, const std::string& password) {
		const persistence::Timestamp ts = persistence::Timestamp::now();
		db::UserModel u;
		EXPECT_EQ(0, u.id());
		ASSERT_TRUE(u.insert(email, password, ts));
		EXPECT_NE(0, u.id());

		db::UserModel u2nd;
		ASSERT_TRUE(u2nd.select(email.c_str(), password.c_str(), nullptr));
		EXPECT_GT(u2nd.registrationdate().time(), uint64_t(0));
		EXPECT_EQ(u2nd.email(), email);
		ASSERT_EQ(u2nd.id(), u.id());

		db::UserModel u3nd;
		ASSERT_TRUE(u3nd.selectById(u.id()));
		EXPECT_GT(u3nd.registrationdate().time(), uint64_t(0));
		EXPECT_EQ(u3nd.email(), email);
		ASSERT_EQ(u3nd.id(), u.id());
	}
};

TEST_F(DatabaseModelTest, testCreateUser) {
	createUser("a@b.c.d", "secret");
}

TEST_F(DatabaseModelTest, testCreateUsers) {
	for (int i = 0; i < 5; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret");
	}
}

}
