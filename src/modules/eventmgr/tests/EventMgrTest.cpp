/**
 * @file
 */

#include "persistence/tests/AbstractDatabaseTest.h"
#include "EventMgrModels.h"
#include "eventmgr/EventMgr.h"

namespace eventmgr {

class EventMgrTest : public persistence::AbstractDatabaseTest {
private:
	using Super = persistence::AbstractDatabaseTest;
protected:
	persistence::DBHandlerPtr _dbHandler;
	eventmgr::EventProviderPtr _eventProvider;
public:
	void SetUp() override {
		Super::SetUp();
		_dbHandler = std::make_shared<persistence::DBHandler>();
		ASSERT_TRUE(_dbHandler->init()) << "Could not initialize dbhandler";
		_eventProvider = std::make_shared<eventmgr::EventProvider>(_dbHandler);
		_dbHandler->truncate(db::EventPointModel());
		_dbHandler->truncate(db::EventModel());
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler->shutdown();
		_eventProvider->shutdown();
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
	EventMgr mgr(_eventProvider, _testApp->timeProvider());
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

TEST_F(EventMgrTest, testEventModelTimestamps) {
	const core::TimeProviderPtr& timeProvider = _testApp->timeProvider();
	const auto now = timeProvider->tickMillis();
	const uint64_t nowSeconds = now / 1000UL;

	db::EventModel model;
	model.setStartdate(nowSeconds);
	model.setType(Type::GENERIC);

	const int secondsRuntime = 50;

	model.setEnddate(nowSeconds + secondsRuntime);

	ASSERT_EQ(nowSeconds, model.startdate().seconds());
	ASSERT_EQ(nowSeconds + secondsRuntime, model.enddate().seconds());

	ASSERT_TRUE(_dbHandler->insert(model)) << "Could not add event entry";

	ASSERT_TRUE(_dbHandler->select(model, db::DBConditionEventId(model.id())));
	ASSERT_EQ(nowSeconds, model.startdate().seconds());
	ASSERT_EQ(nowSeconds + secondsRuntime, model.enddate().seconds());
}

TEST_F(EventMgrTest, testEventMgrUpdateStartStop) {
	const core::TimeProviderPtr& timeProvider = _testApp->timeProvider();
	const auto now = timeProvider->tickMillis();
	const uint64_t nowSeconds = now / 1000UL;

	db::EventModel model;
	model.setStartdate(nowSeconds);
	model.setType(Type::GENERIC);

	const int secondsRuntime = 50;

	const int secondsDelta = 1;
	const long millisDelta = secondsDelta * 1000L;
	model.setEnddate(nowSeconds + secondsRuntime);

	ASSERT_TRUE(_dbHandler->insert(model)) << "Could not add event entry";

	EventMgr mgr(_eventProvider, timeProvider);
	ASSERT_TRUE(mgr.init()) << "Could not initialize eventmgr";
	ASSERT_EQ(0, mgr.runningEvents());

	timeProvider->update(now + millisDelta);
	mgr.update(0L);
	ASSERT_EQ(1, mgr.runningEvents()) << "At " << timeProvider->toString(timeProvider->tickMillis()) << " should be a running event " << model.startdate().toString();

	const unsigned long millisRuntime = secondsRuntime * 1000UL;
	timeProvider->update(now + millisDelta + millisRuntime);
	mgr.update(0L);
	ASSERT_EQ(0, mgr.runningEvents()) << "At " << timeProvider->toString(timeProvider->tickMillis()) << " should be no running event " << model.enddate().toString();

	mgr.shutdown();
}

}
