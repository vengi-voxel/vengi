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

TEST(EventBusTest, testSubscribeAndPublish_1) {
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

TEST(EventBusTest, testMassSubscribeAndPublish_10000000) {
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

TEST(EventBusTest, testSubscribeAndUnsubscribe_1000) {
	EventBus eventBus;
	HandlerTest handler;

	const int n = 1000;
	for (int i = 0; i < n; ++i) {
		eventBus.subscribe(handler);
	}
	ASSERT_EQ(n, eventBus.unsubscribe(handler));
}

TEST(EventBusTest, testMassPublish_10000000) {
	EventBus eventBus;
	HandlerTest handler;
	TestEvent event;

	eventBus.subscribe(handler);
	const int n = 10000000;
	for (int i = 0; i < n; ++i) {
		ASSERT_EQ(1, eventBus.publish(event)) << "Unexpected amount of handlers notified";
		ASSERT_EQ(i + 1, handler.getCount()) << "Unexpected handler notification amount";
	}

	ASSERT_EQ(1, eventBus.unsubscribe(handler));
	ASSERT_EQ(0, eventBus.publish(event)) << "Expected no handler to be notified";
	ASSERT_EQ(n, handler.getCount()) << "Expected the handler not to be notified again because we unsubscribed it before we published the event";
}

#define TOPIC(topic) \
class topic: public IEventBusTopic {}; \
topic __##topic

#define EVENT(event, topic) \
class event: public IEventBusEvent { public: event(topic _##topic) : IEventBusEvent(&_##topic) {} event() : IEventBusEvent(nullptr) {}  }

#define EVENTTOPIC(event, topic) \
TOPIC(topic); \
EVENT(event, topic);

#define EVENTTOPICHANDLER(event, topic, handler) \
EVENTTOPIC(event, topic); \
class handler: public CountHandlerTest<event> {}

TEST(EventBusTest, testTopic_1) {
	EVENTTOPICHANDLER(Topic1Event, Topic1, Topic1EventHandler);
	EventBus eventBus;
	Topic1EventHandler handler;
	Topic1Event event(__Topic1);

	eventBus.subscribe(handler, &__Topic1);
	ASSERT_EQ(1, eventBus.publish(event)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(1, handler.getCount()) << "Unexpected handler notification amount";

	Topic1Event noTopicEvent;
	ASSERT_EQ(0, eventBus.publish(noTopicEvent)) << "Unexpected amount of handlers notified - topic filtering isn't working";
	ASSERT_EQ(1, handler.getCount()) << "Unexpected handler notification amount";
}

TEST(EventBusTest, testMultipleTopics_1) {
	EVENTTOPICHANDLER(Topic1Event, Topic1, Topic1EventHandler);
	EventBus eventBus;
	Topic1EventHandler handler;
	Topic1Event event(__Topic1);

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
