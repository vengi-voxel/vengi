/**
 * @file
 */

#include "persistence/tests/AbstractDatabaseTest.h"
#include "TestModel.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
#include "engine-config.h"

namespace backend {

class DatabaseModelTest: public persistence::AbstractDatabaseTest {
private:
	using Super = persistence::AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandler _dbHandler;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = _dbHandler.init();
		if (_supported) {
			ASSERT_TRUE(_dbHandler.createTable(db::TestModel())) << "Could not create table";
			ASSERT_TRUE(_dbHandler.dropTable(db::TestModel()));
			ASSERT_TRUE(_dbHandler.createTable(db::TestModel())) << "Could not create table";
		}
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler.shutdown();
	}

	void createUser(const std::string& email, const std::string& password, int64_t& id) {
		ASSERT_TRUE(_supported);
		const persistence::Timestamp ts = persistence::Timestamp::now();
		db::TestModel u;
		EXPECT_EQ(0, u.id());
		u.setEmail(email);
		u.setPassword(password);
		u.setRegistrationdate(ts);
		ASSERT_TRUE(_dbHandler.insert(u));
		EXPECT_NE(0, u.id());

		db::TestModel u2nd;
		const db::DBConditionTestEmail emailCond(email);
		const db::DBConditionTestPassword passwordCond(password);
		ASSERT_TRUE(_dbHandler.select(u2nd, persistence::DBConditionMultiple(true, {&emailCond, &passwordCond})));
		EXPECT_GT(u2nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(u2nd.email(), email);
		ASSERT_EQ(u2nd.id(), u.id());

		db::TestModel u3nd;
		ASSERT_TRUE(_dbHandler.select(u3nd, db::DBConditionTestId(u.id())));
		EXPECT_GT(u3nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(u3nd.email(), email);
		ASSERT_EQ(u3nd.id(), u.id());

		id = u.id();
	}
};

TEST_F(DatabaseModelTest, testCreateUser) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createUser("testCreateUser@b.c.d", "secret", id);
}

TEST_F(DatabaseModelTest, testCreateUsers) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createUser(core::string::format("testCreateUsers%i@b.c.d", i), "secret", id);
	}
}

TEST_F(DatabaseModelTest, testSelectAll) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("testSelectAll%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	}));
	EXPECT_EQ(count, expected);
}

TEST_F(DatabaseModelTest, testSelectByEmail) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("testSelectByEmail%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionTestEmail condition("testSelectByEmail1@b.c.d");
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), condition, [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(std::string(condition.value(0)), model.email());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testSelectById) {
	if (!_supported) {
		return;
	}
	const int expected = 5;
	int64_t id = -1L;
	for (int i = 0; i < expected; ++i) {
		createUser(core::string::format("testSelectById%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionTestId condition(id);
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), condition, [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(id, model.id());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testTruncate) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createUser("testTruncate@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.truncate(db::TestModel()));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testDelete) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createUser("testDelete@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.deleteModel(db::TestModel(), db::DBConditionTestId(id)));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testUpdate) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createUser("testUpdate@b.c.d", "secret", id);
	int count = 0;
	db::TestModel copy;
	_dbHandler.select(db::TestModel(), db::DBConditionTestId(id), [&] (db::TestModel&& model) {
		++count;
		copy = std::move(model);
	});
	ASSERT_EQ(count, 1);
	ASSERT_EQ("testUpdate@b.c.d", copy.email());
	copy.setEmail("no@mail.com");
	_dbHandler.update(copy);
	_dbHandler.select(db::TestModel(), db::DBConditionTestId(id), [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(copy.email(), model.email());
	});
}

TEST_F(DatabaseModelTest, testTimestamp) {
	if (!_supported) {
		return;
	}
	db::TestModel u;
	EXPECT_EQ(0, u.id());
	u.setEmail("testTimestamp@now.de");
	u.setPassword("now");
	const auto now = _testApp->timeProvider()->tickMillis();
	u.setRegistrationdate(now / 1000UL);
	ASSERT_TRUE(_dbHandler.insert(u));

	_dbHandler.select(db::TestModel(), db::DBConditionTestId(u.id()), [=] (db::TestModel&& model) {
		const persistence::Timestamp& ts = model.registrationdate();
		const persistence::Timestamp tsNow(now / 1000);
		ASSERT_NEAR(ts.millis(), now, 999) << "db: " << ts.toString() << " now: " << tsNow.toString();
	});
}

TEST_F(DatabaseModelTest, testLimitOrderBy) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createUser(core::string::format("testLimitOrderBy%i@b.c.d", i), "secret", id);
	}
	const int limit = 2;
	int count = 0;
	const persistence::OrderBy orderBy(db::TestModel::f_id(), persistence::Order::DESC, limit);
	_dbHandler.select(db::TestModel(), orderBy, [&] (db::TestModel&& model) {
		++count;
	});
	ASSERT_EQ(limit, count);
}

TEST_F(DatabaseModelTest, testOffsetOrderBy) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int n = 5;
	for (int i = 0; i < n; ++i) {
		createUser(core::string::format("testOffsetOrderBy%i@b.c.d", i), "secret", id);
	}
	const int limit = -1;
	const int offset = 3;
	int count = 0;
	const persistence::OrderBy orderBy(db::TestModel::f_id(), persistence::Order::DESC, limit, offset);
	_dbHandler.select(db::TestModel(), orderBy, [&] (db::TestModel&& model) {
		++count;
	});
	ASSERT_EQ(n - offset, count);
}

}
