/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "EventMgrModels.h"
#include "../EventMgr.h"
#include "engine-config.h"

namespace eventmgr {

class EventMgrTest : public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	persistence::DBHandlerPtr _dbHandler;
public:
	void SetUp() override {
		core::AbstractTest::SetUp();
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "engine");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabaseUser, "engine");
		core::Var::get(cfg::DatabasePassword, "engine");

		_dbHandler = std::make_shared<persistence::DBHandler>();
		ASSERT_TRUE(_dbHandler->init()) << "Could not initialize dbhandler";
		_dbHandler->truncate(db::EventPointModel());
		_dbHandler->truncate(db::EventModel());
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler->shutdown();
	}

	void createEvent(Type type, int64_t& id) {
		db::EventModel model;
		model.setType(std::enum_value(type));
		model.setStartdate(persistence::Timestamp::now());
		model.setEnddate(persistence::Timestamp::now());
		ASSERT_TRUE(_dbHandler->insert(model));
		EXPECT_NE(0, model.id());
		id = model.id();

		db::EventModel eventModel;
		ASSERT_TRUE(_dbHandler->select(eventModel, db::DBConditionEventId(model.id())));
		ASSERT_EQ(eventModel.id(), model.id());
	}
};

TEST_F(EventMgrTest, testEventMgrInit) {
	EventMgr mgr(_dbHandler);
	ASSERT_TRUE(mgr.init()) << "Could not initialize eventmgr";
	mgr.shutdown();
}

TEST_F(EventMgrTest, testEventModelInsert) {
	int64_t id;
	createEvent(Type::GENERIC, id);
}

TEST_F(EventMgrTest, testEventPointModelInsert) {
	int64_t id;
	createEvent(Type::GENERIC, id);
	db::EventPointModel pointModel;
	pointModel.setEventid(id);
	pointModel.setKey("test");
	pointModel.setPoints(1);
	pointModel.setUserid(1337);
	ASSERT_TRUE(_dbHandler->insert(pointModel));
}

TEST_F(EventMgrTest, testEventPointModelInsertUniqueKeys) {
	int64_t id;
	createEvent(Type::GENERIC, id);
	db::EventPointModel pointModel;
	pointModel.setEventid(id);
	pointModel.setKey("test");
	pointModel.setPoints(1);
	pointModel.setUserid(1337);
	ASSERT_TRUE(_dbHandler->insert(pointModel));
	ASSERT_TRUE(_dbHandler->insert(pointModel));

	{
		db::EventPointModel eventPointModel;
		db::DBConditionEventPointEventid c1(id);
		db::DBConditionEventPointUserid c2(pointModel.userid());
		db::DBConditionEventPointKey c3("test");
		persistence::DBConditionMultiple m(true, {&c1, &c2, &c3});
		ASSERT_TRUE(_dbHandler->select(eventPointModel, m));
		ASSERT_EQ(eventPointModel.points(), 2L);
	}

	{
		db::EventPointModel eventPointModel;
		db::DBConditionEventPointEventid c1(id);
		db::DBConditionEventPointUserid c2(pointModel.userid());
		db::DBConditionEventPointKey c3("test2");
		persistence::DBConditionMultiple m(true, {&c1, &c2, &c3});
		ASSERT_TRUE(_dbHandler->select(eventPointModel, m));
		ASSERT_EQ(eventPointModel.points(), 0L);
	}
}

}
