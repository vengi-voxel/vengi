/**
 * @file
 */
#include "EventProvider.h"
#include "commonlua/LUA.h"
#include "core/App.h"
#include "core/Array.h"
#include "core/Common.h"
#include "io/Filesystem.h"

namespace eventmgr {

EventProvider::EventProvider(const persistence::DBHandlerPtr& dbHandler) :
		_dbHandler(dbHandler) {
}

bool EventProvider::init() {
	return _dbHandler->select(db::EventModel(), persistence::DBConditionOne(), [this] (db::EventModel&& model) {
		_eventData.insert(std::make_pair((EventId)model.id(), std::forward<db::EventModel>(model)));
	});
}

void EventProvider::shutdown() {
	_eventData.clear();
}

const db::EventModel* EventProvider::get(EventId id) const {
	auto i = _eventData.find(id);
	if (i == _eventData.end()) {
		return nullptr;
	}
	// TODO: this doesn't work because the pointer changes if new event data was inserted
	return &i->second;
}

}
