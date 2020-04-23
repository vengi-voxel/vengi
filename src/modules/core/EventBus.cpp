/**
 * @file
 */

#include "EventBus.h"
#include "Log.h"

namespace core {

EventBus::EventBus(const int initialHandlerSize) :
		_lock("EventBus") {
	_handlers.reserve(initialHandlerSize);
}

EventBus::~EventBus() {
	_handlers.clear();
}

void EventBus::subscribe(ClassTypeId index, void *handler, const IEventBusTopic* topic) {
	ScopedWriteLock lock(_lock);
	EventBusHandlerReferences& handlers = _handlers[index];
	const EventBusHandlerReference registration(handler, topic);
	handlers.push_back(registration);
}

int EventBus::unsubscribe(ClassTypeId index, void* handler, const IEventBusTopic* topic) {
	int unsubscribedHandlers = 0;
	ScopedWriteLock lock(_lock);
	EventBusHandlerReferences& handlers = _handlers[index];
	for (EventBusHandlerReferences::iterator i = handlers.begin(); i != handlers.end();) {
		EventBusHandlerReference& r = *i;
		if (r.getHandler() != reinterpret_cast<IEventBusHandler<IEventBusEvent>*>(handler)) {
			++i;
			continue;
		}
		if (topic != nullptr) {
			if (r.getTopic() == nullptr) {
				++i;
				continue;
			}
			if (!(*r.getTopic() == *topic)) {
				++i;
				continue;
			}
		}
		i = handlers.erase(i);
		++unsubscribedHandlers;
	}
	return unsubscribedHandlers;
}

int EventBus::update(int limit) {
	core_trace_scoped(EventBusUpdate);
	int i = 0;
	IEventBusEventPtr event;
	while (_queue.pop(event)) {
		publish(*event);
		if (limit > 0 && ++i >= limit) {
			break;
		}
	}
	return _queue.size();
}

int EventBus::size() const {
	return  _queue.size();
}

void EventBus::enqueue(const IEventBusEventPtr& e) {
	_queue.push(e);
}

int EventBus::publish(const IEventBusEvent& e) {
	const ClassTypeId index = e.typeId();
	// must be locked until the execution is done, because we are dealing with raw pointers here.
	// that means nobody may unsubscribe/subscribe during a publish as we are iterating the list.
	// if someone would unsubscribe he would maybe still get notified otherwise - or even worse,
	// the pointer we are iterating over is already freed.
	ScopedReadLock lock(_lock);
	EventBusHandlerReferenceMap::iterator i = _handlers.find(index);
	if (i == _handlers.end()) {
		return 0;
	}

	int notifiedHandlers = 0;
	EventBusHandlerReferences& handlers = i->second;
	for (auto& r : handlers) {
		if (r.getTopic() != nullptr) {
			const IEventBusTopic* topic = e.getTopic();
			if (topic == nullptr) {
				continue;
			}
			if (!(*r.getTopic() == *topic)) {
				continue;
			}
		}
		IEventBusHandler<IEventBusEvent>* handler = r.getHandler();
		handler->dispatch(e);
		++notifiedHandlers;
	}
	return notifiedHandlers;
}

}
