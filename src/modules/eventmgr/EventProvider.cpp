/**
 * @file
 */
#include "EventProvider.h"
#include "commonlua/LUA.h"
#include "core/App.h"
#include "core/Array.h"
#include "core/Common.h"
#include "persistence/DBHandler.h"
#include "core/io/Filesystem.h"
#include "EventMgrModels.h"

namespace eventmgr {

EventProvider::EventProvider(const persistence::DBHandlerPtr& dbHandler) :
		_dbHandler(dbHandler) {
}

bool EventProvider::init() {
	if (!_dbHandler->createTable(db::EventModel())) {
		Log::error("Failed to create event table");
		return false;
	}
	if (!_dbHandler->createTable(db::EventPointModel())) {
		Log::error("Failed to create event point table");
		return false;
	}

	return _dbHandler->select(db::EventModel(), persistence::DBConditionOne(), [this] (db::EventModel&& model) {
		const db::EventModelPtr& modelPtr = std::make_shared<db::EventModel>(std::forward<db::EventModel>(model));
		_eventData.insert(std::make_pair((EventId)model.id(), std::move(modelPtr)));
	});
}

void EventProvider::shutdown() {
	_eventData.clear();
}

db::EventModelPtr EventProvider::get(EventId id) const {
	auto i = _eventData.find(id);
	if (i == _eventData.end()) {
		return db::EventModelPtr();
	}
	return i->second;
}

}
