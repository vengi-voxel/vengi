/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/EventBus.h"

namespace core {

EVENTBUSEVENT(TestEvent);

template<class T>
class CountHandlerTest: public IEventBusHandler<T> {
private:
	int _count;
public:
	CountHandlerTest() : _count(0) {}

	void onEvent(const T&) override {
		++_count;
	}

	inline int getCount() const {
		return _count;
	}
};

class HandlerTest: public CountHandlerTest<TestEvent> {
};

class EventBusTest : public testing::Test {
};

TEST_F(EventBusTest, testSubscribeAndPublish_1) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	eventBus.subscribe(handler);
	ASSERT_EQ(1, eventBus.publish(event)) << "Expected one handler to be notified";
	ASSERT_EQ(1, handler.getCount()) << "Expected the handler to be notified once";

	ASSERT_EQ(1, eventBus.unsubscribe(handler));
	ASSERT_EQ(0, eventBus.publish(event)) << "Expected no handler to be notified";
	ASSERT_EQ(1, handler.getCount()) << "Expected the handler not to be notified twice because we unsubscribed it before we published the event";
}

TEST_F(EventBusTest, testSubscribeAndQueue_1) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	eventBus.subscribe(handler);
	eventBus.enqueue(std::make_shared<TestEvent>());
	ASSERT_EQ(0, handler.getCount()) << "Expected the handler to be not yet notified";

	ASSERT_EQ(0, eventBus.update());
	ASSERT_EQ(1, handler.getCount()) << "Expected the handler to be notified once";
}

TEST_F(EventBusTest, testSubscribeAndQueuePendingLeft) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	eventBus.subscribe(handler);
	eventBus.enqueue(std::make_shared<TestEvent>());
	eventBus.enqueue(std::make_shared<TestEvent>());
	ASSERT_EQ(0, handler.getCount()) << "Expected the handler to be not yet notified";

	ASSERT_EQ(1, eventBus.update(1)) << "Expected to still have one pending event left in the queue";
	ASSERT_EQ(1, handler.getCount()) << "Expected the handler to be notified once";
}

TEST_F(EventBusTest, DISABLED_testMassSubscribeAndPublish_10000000) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	const int n = 10000000;
	for (int i = 0; i < n; ++i) {
		eventBus.subscribe(handler);
	}
	ASSERT_EQ(n, eventBus.publish(event)) << "Unexpected amount of handlers notified";
	ASSERT_EQ(n, handler.getCount()) << "Unexpected handler notification amount";

	ASSERT_EQ(n, eventBus.unsubscribe(handler));
	ASSERT_EQ(0, eventBus.publish(event)) << "Expected no handler to be notified";
	ASSERT_EQ(n, handler.getCount()) << "Expected the handler not to be notified again because we unsubscribed it before we published the event";
}

TEST_F(EventBusTest, testSubscribeAndUnsubscribe_1000) {
	EventBus eventBus;
	HandlerTest handler;

	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		eventBus.subscribe(handler);
	}
	ASSERT_EQ(n, eventBus.unsubscribe(handler));
}

TEST_F(EventBusTest, testMassPublish_10000) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	eventBus.subscribe(handler);
	const int n = 10000;
	for (int i = 0; i < n; ++i) {
		ASSERT_EQ(1, eventBus.publish(event)) << "Unexpected amount of handlers notified";
		ASSERT_EQ(i + 1, handler.getCount()) << "Unexpected handler notification amount";
	}

	ASSERT_EQ(1, eventBus.unsubscribe(handler));
	ASSERT_EQ(0, eventBus.publish(event)) << "Expected no handler to be notified";
	ASSERT_EQ(n, handler.getCount()) << "Expected the handler not to be notified again because we unsubscribed it before we published the event";
}

#define EVENTTOPICHANDLER(event, topic, handler) \
EVENTBUSTOPIC(topic); \
topic __##topic; \
EVENTBUSEVENT(event); \
class handler: public CountHandlerTest<event> {}

TEST_F(EventBusTest, testTopic_1) {
	EVENTTOPICHANDLER(Topic1Event, Topic1, Topic1EventHandler);
	EventBus eventBus;
	Topic1EventHandler handler;
	Topic1Event event(&__Topic1);

	eventBus.subscribe(handler, &__Topic1);
	ASSERT_EQ(1, eventBus.publish(event)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(1, handler.getCount()) << "Unexpected handler notification amount";

	Topic1Event noTopicEvent;
	ASSERT_EQ(0, eventBus.publish(noTopicEvent)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(1, handler.getCount()) << "Unexpected handler notification amount";
}

TEST_F(EventBusTest, testMultipleTopics_1) {
	EVENTTOPICHANDLER(Topic1Event, Topic1, Topic1EventHandler);
	EventBus eventBus;
	Topic1EventHandler handler;
	Topic1Event event(&__Topic1);

	eventBus.subscribe(handler, &__Topic1);
	eventBus.subscribe(handler);
	ASSERT_EQ(2, eventBus.publish(event)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(2, handler.getCount()) << "Unexpected handler notification amount";

	eventBus.unsubscribe(handler, &__Topic1);
	Topic1Event noTopicEvent;
	ASSERT_EQ(1, eventBus.publish(noTopicEvent)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(3, handler.getCount()) << "Unexpected handler notification amount";
}

}
