/**
 * @file
 */

#pragma once

#include <unordered_map>
#include <list>
#include <typeindex>
#include <type_traits>
#include <vector>
#include <memory>
#include "core/ReadWriteLock.h"
#include "collection/ConcurrentQueue.h"

namespace core {

class IEventBusEvent;

/**
 * @brief The handler will get notified for every published IEventBusEvent that it is registered for.
 */
template<class T>
class IEventBusHandler {
private:
	friend class EventBus;
	void dispatch(const IEventBusEvent& e) {
		onEvent(reinterpret_cast<const T &>(e));
	}
public:
	IEventBusHandler() {
		static_assert(std::is_base_of<IEventBusEvent, T>::value, "Wrong type given, must extend IEventBusEvent");
	}

	virtual ~IEventBusHandler() = default;

	/**
	 * @brief Callback for the event. Keep in mind that this can be called from different threads
	 */
	virtual void onEvent(const T&) = 0;
};

/**
 * @brief With a topic you can filter your event dispatching to those IEventBusHandler instances that were subscribed with the same topic.
 * @see EventBus::subscribe() for more details.
 */
class IEventBusTopic {
protected:
	const std::type_index _index;

	IEventBusTopic() :
			_index(typeid(*this)) {
	}
public:
	virtual ~IEventBusTopic() = default;

	inline bool operator==(const IEventBusTopic& other) const {
		return _index == other._index;
	}
};

/**
 * @brief Base class for all the events that EventBus::publish() is publishing
 */
class IEventBusEvent {
protected:
	const IEventBusTopic *_topic;

	IEventBusEvent(const IEventBusTopic* const topic = nullptr) :
			_topic(topic) {
	}
private:
	friend class EventBus;
	inline const IEventBusTopic* getTopic() const {
		return _topic;
	}
public:

	virtual ~IEventBusEvent() = default;
};

typedef std::shared_ptr<IEventBusEvent> IEventBusEventPtr;

/**
 * @brief Generates a new default event without a topic and any state attached
 */
#define EVENTBUSEVENT(name) class name: public IEventBusEvent { public: name() : IEventBusEvent(nullptr) { } }

/**
 * @brief EventBus with topic (IEventBusTopic) support
 *
 * Use subscribe() and unsubscribe() to manager your IEventBusHandler instances.
 */
class EventBus {
private:
	class EventBusHandlerReference;
	typedef std::list<EventBusHandlerReference> EventBusHandlerReferences;
	typedef std::unordered_map<std::type_index, EventBusHandlerReferences> EventBusHandlerReferenceMap;
	core::ReadWriteLock _lock;

	core::ConcurrentQueue<IEventBusEventPtr> _queue;

	class EventBusHandlerReference {
	private:
		void* const _handler;
		const IEventBusTopic *_topic;

	public:
		EventBusHandlerReference(void* handler, const IEventBusTopic* topic) :
				_handler(handler), _topic(topic) {
		}

		inline IEventBusHandler<IEventBusEvent>* getHandler() const {
			return static_cast<IEventBusHandler<IEventBusEvent>*>(_handler);
		}

		inline const IEventBusTopic* getTopic() const {
			return _topic;
		}
	};

	EventBusHandlerReferenceMap _handlers;

	int unsubscribe(const std::type_index& index, void* handler, const IEventBusTopic* topic);
	void subscribe(const std::type_index& index, void *handler, const IEventBusTopic* topic);

public:
	/**
	 * @param[in] initialHandlerSize Used to calculate the amount of memory that is reserved in the
	 * handler map to reduce memory allocations.
	 */
	EventBus(const int initialHandlerSize = 64);
	~EventBus();

	/**
	 * @brief Subscribe a handler with the ability to filter by the given IEventBusTopic
	 * @param[in,out] handler The IEventBusHandler to subscribe
	 * @param[in] topic The specific topic to subscribe the IEventBusHandler for. If this is @c nullptr
	 * the handler is notified no matter which IEventBusTopic the IEventBusEvent is published with.
	 * @sa unsubscribe()
	 */
	template<class T>
	void subscribe(IEventBusHandler<T>& handler, const IEventBusTopic* topic = nullptr) {
		const std::type_index& index = typeid(T);
		subscribe(index, (void*)&handler, topic);
	}

	/**
	 * @brief Unsubscribe the given handler for the specified topic
	 * @param[in,out] handler The IEventBusHandler to unsubscribe
	 * @param[in] topic The specific topic to unsubscribe the IEventBusHandler for. If this is
	 * @c nullptr the given handler is unsubscribed no matter which topic it was subscribed with.
	 * @sa subscribe()
	 * @return The amount of unsubscribed IEventBusHandler instances
	 */
	template<class T>
	int unsubscribe(IEventBusHandler<T>& handler, const IEventBusTopic* topic = nullptr) {
		const std::type_index& index = typeid(T);
		return unsubscribe(index, (void*)&handler, topic);
	}

	/**
	 * @brief Publishes the given IEventBusEvent and notifies the IEventBusHandler
	 * @param[in] e The event that should be published
	 * @note Only those IEventBusHandler are notified that have registered with the same topic
	 * that the event is publishing with (or if the handler was not registered with a topic at
	 * all).
	 * @return The amount of notified IEventBusHandler instances
	 */
	int publish(const IEventBusEvent& e);

	/**
	 * @brief Execute all queued events
	 * @param[in] limit Limit the amount of executed events - if there are too many. If -1 is given here,
	 * all events are handled.
	 * @return the amount of events that are still in the queue (due to the limit)
	 */
	int update(int limit = -1);

	/**
	 * @brief Execute in the main thread in the next tick
	 */
	void enqueue(const IEventBusEventPtr& e);
};

typedef std::shared_ptr<EventBus> EventBusPtr;

}
