/**
 * @file
 */

#include "EventMgr.h"
#include "EventMgrModels.h"
#include "core/Log.h"

namespace eventmgr {

EventMgr::EventMgr() {
}

bool EventMgr::init() {
	if (!db::EventModel::createTable()) {
		Log::error("Failed to create event table");
		return false;
	}
	if (!db::EventPointModel::createTable()) {
		Log::error("Failed to create event point table");
		return false;
	}

	return true;
}

void EventMgr::update(long dt) {
}

void EventMgr::shutdown() {
}

}
