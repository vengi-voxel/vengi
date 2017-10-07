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

	void createEvent(Type type, db::EventModel& eventModel, const persistence::Timestamp& startdate = persistence::Timestamp::now(), const persistence::Timestamp& enddate = persistence::Timestamp::now()) {
		db::EventModel model;
		model.setType(type);
		model.setStartdate(startdate);
		model.setEnddate(enddate);

		ASSERT_EQ(startdate.seconds(), model.startdate().seconds());
		ASSERT_EQ(enddate.seconds(), model.enddate().seconds());

		ASSERT_TRUE(_dbHandler->insert(model));
		EXPECT_NE(0, model.id());

		ASSERT_TRUE(_dbHandler->select(eventModel, db::DBConditionEventId(model.id())));
		ASSERT_EQ(eventModel.id(), model.id());
	}

	void createEvent(Type type, EventId& id, const persistence::Timestamp& startdate = persistence::Timestamp::now(), const persistence::Timestamp& enddate = persistence::Timestamp::now()) {
		db::EventModel model;
		createEvent(type, model, startdate, enddate);
		id = model.id();
	}
};

TEST_F(EventMgrTest, testEventMgrInit) {
	EventMgr mgr(_eventProvider, _testApp->timeProvider());
	ASSERT_TRUE(mgr.init()) << "Could not initialize eventmgr";
	mgr.shutdown();
}

TEST_F(EventMgrTest, testEventModelInsert) {
	EventId id;
	createEvent(Type::GENERIC, id);
}

TEST_F(EventMgrTest, testEventPointModelInsert) {
	EventId id;
	createEvent(Type::GENERIC, id);
	db::EventPointModel pointModel;
	pointModel.setEventid(id);
	pointModel.setKey("test");
	pointModel.setPoints(1);
	pointModel.setUserid(1337);
	ASSERT_TRUE(_dbHandler->insert(pointModel));
}

TEST_F(EventMgrTest, testEventPointModelInsertUniqueKeys) {
	EventId id;
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
	const int secondsRuntime = 50;
	const uint64_t nowSeconds = now / 1000UL;

	db::EventModel model;
	createEvent(Type::GENERIC, model, nowSeconds, nowSeconds + secondsRuntime);

	ASSERT_TRUE(_dbHandler->select(model, db::DBConditionEventId(model.id())));
	ASSERT_EQ(nowSeconds, model.startdate().seconds());
	ASSERT_EQ(nowSeconds + secondsRuntime, model.enddate().seconds());
}

TEST_F(EventMgrTest, testEventMgrUpdateStartStop) {
	const core::TimeProviderPtr& timeProvider = _testApp->timeProvider();
	// current tick time: 1000ms
	timeProvider->update(1000UL);
	const auto now = timeProvider->tickMillis();
	// event start tick time: 2s
	const uint64_t eventStartSeconds = now / 1000UL + 1;
	// event stop tick time: 52s
	const int secondsRuntime = 50;
	const uint64_t eventStopTime = eventStartSeconds + secondsRuntime;

	db::EventModel model;
	createEvent(Type::GENERIC, model, eventStartSeconds, eventStopTime);

	EventMgr mgr(_eventProvider, timeProvider);
	ASSERT_TRUE(mgr.init()) << "Could not initialize eventmgr";
	ASSERT_EQ(0, mgr.runningEvents());

	// current tick time is 1s, event starts at 2s
	mgr.update(0L);
	ASSERT_EQ(0, mgr.runningEvents()) << "At " << timeProvider->toString(timeProvider->tickMillis()) << " should be no running event " << model.startdate().toString();

	// current tick time: 2000ms
	timeProvider->update(eventStartSeconds * 1000UL);
	mgr.update(0L);
	ASSERT_EQ(1, mgr.runningEvents()) << "At " << timeProvider->toString(timeProvider->tickMillis()) << " should be a running event " << model.startdate().toString();

	// current tick time: 52000ms
	timeProvider->update(eventStopTime * 1000UL);
	mgr.update(0L);
	ASSERT_EQ(0, mgr.runningEvents()) << "At " << timeProvider->toString(timeProvider->tickMillis()) << " should be no running event " << model.enddate().toString();

	mgr.shutdown();
}

}
