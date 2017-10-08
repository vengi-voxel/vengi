/**
 * @file
 */

#include "persistence/tests/AbstractDatabaseTest.h"
#include "UserModel.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
#include "engine-config.h"

namespace backend {

class DatabaseModelTest: public persistence::AbstractDatabaseTest {
private:
	using Super = persistence::AbstractDatabaseTest;
protected:
	persistence::DBHandler _dbHandler;
public:
	void SetUp() override {
		Super::SetUp();
		ASSERT_TRUE(_dbHandler.init());
		ASSERT_TRUE(_dbHandler.createTable(db::UserModel())) << "Could not create table";
		_dbHandler.truncate(db::UserModel());
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler.shutdown();
	}

	void createUser(const std::string& email, const std::string& password, int64_t& id) {
		const persistence::Timestamp ts = persistence::Timestamp::now();
		db::UserModel u;
		EXPECT_EQ(0, u.id());
		u.setEmail(email);
		u.setPassword(password);
		u.setRegistrationdate(ts);
		ASSERT_TRUE(_dbHandler.insert(u));
		EXPECT_NE(0, u.id());

		db::UserModel u2nd;
		const db::DBConditionUserEmail emailCond(email);
		const db::DBConditionUserPassword passwordCond(password);
		ASSERT_TRUE(_dbHandler.select(u2nd, persistence::DBConditionMultiple(true, {&emailCond, &passwordCond})));
		EXPECT_GT(u2nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(u2nd.email(), email);
		ASSERT_EQ(u2nd.id(), u.id());

		db::UserModel u3nd;
		ASSERT_TRUE(_dbHandler.select(u3nd, db::DBConditionUserId(u.id())));
		EXPECT_GT(u3nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(u3nd.email(), email);
		ASSERT_EQ(u3nd.id(), u.id());

		id = u.id();
	}
};

TEST_F(DatabaseModelTest, testCreateUser) {
	int64_t id = -1L;
	createUser("a@b.c.d", "secret", id);
}

TEST_F(DatabaseModelTest, testCreateUsers) {
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
}

TEST_F(DatabaseModelTest, testSelectAll) {
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	EXPECT_TRUE(_dbHandler.select(db::UserModel(), persistence::DBConditionOne(), [&] (db::UserModel&& model) {
		++count;
	}));
	EXPECT_EQ(count, expected);
}

TEST_F(DatabaseModelTest, testSelectByEmail) {
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionUserEmail condition("a1@b.c.d");
	EXPECT_TRUE(_dbHandler.select(db::UserModel(), condition, [&] (db::UserModel&& model) {
		++count;
		ASSERT_EQ(std::string(condition.value(0)), model.email());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testSelectById) {
	const int expected = 5;
	int64_t id = -1L;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionUserId condition(id);
	EXPECT_TRUE(_dbHandler.select(db::UserModel(), condition, [&] (db::UserModel&& model) {
		++count;
		ASSERT_EQ(id, model.id());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testTruncate) {
	int64_t id = -1L;
	createUser("a@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.truncate(db::UserModel()));
	int count = 0;
	_dbHandler.select(db::UserModel(), persistence::DBConditionOne(), [&] (db::UserModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testDelete) {
	int64_t id = -1L;
	createUser("a@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.deleteModel(db::UserModel(), db::DBConditionUserId(id)));
	int count = 0;
	_dbHandler.select(db::UserModel(), persistence::DBConditionOne(), [&] (db::UserModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testUpdate) {
	int64_t id = -1L;
	createUser("a@b.c.d", "secret", id);
	int count = 0;
	db::UserModel copy;
	_dbHandler.select(db::UserModel(), db::DBConditionUserId(id), [&] (db::UserModel&& model) {
		++count;
		copy = std::move(model);
	});
	ASSERT_EQ(count, 1);
	ASSERT_EQ("a@b.c.d", copy.email());
	copy.setEmail("no@mail.com");
	_dbHandler.update(copy);
	_dbHandler.select(db::UserModel(), db::DBConditionUserId(id), [&] (db::UserModel&& model) {
		++count;
		ASSERT_EQ(copy.email(), model.email());
	});
}

TEST_F(DatabaseModelTest, testTimestamp) {
	db::UserModel u;
	EXPECT_EQ(0, u.id());
	u.setEmail("timestamp@now.de");
	u.setPassword("now");
	const auto now = _testApp->timeProvider()->tickMillis();
	u.setRegistrationdate(now / 1000UL);
	ASSERT_TRUE(_dbHandler.insert(u));

	_dbHandler.select(db::UserModel(), db::DBConditionUserId(u.id()), [=] (db::UserModel&& model) {
		const persistence::Timestamp& ts = model.registrationdate();
		const persistence::Timestamp tsNow(now / 1000);
		ASSERT_NEAR(ts.millis(), now, 999) << "db: " << ts.toString() << " now: " << tsNow.toString();
	});
}

TEST_F(DatabaseModelTest, testLimitOrderBy) {
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
	const int limit = 2;
	int count = 0;
	const persistence::OrderBy orderBy(db::UserModel::f_id(), persistence::Order::DESC, limit);
	_dbHandler.select(db::UserModel(), orderBy, [&] (db::UserModel&& model) {
		++count;
	});
	ASSERT_EQ(limit, count);
}

TEST_F(DatabaseModelTest, testOffsetOrderBy) {
	int64_t id = -1L;
	const int n = 5;
	for (int i = 0; i < n; ++i) {
		createUser(core::string::format("a%i@b.c.d", i), "secret", id);
	}
	const int limit = -1;
	const int offset = 3;
	int count = 0;
	const persistence::OrderBy orderBy(db::UserModel::f_id(), persistence::Order::DESC, limit, offset);
	_dbHandler.select(db::UserModel(), orderBy, [&] (db::UserModel&& model) {
		++count;
	});
	ASSERT_EQ(n - offset, count);
}

}
