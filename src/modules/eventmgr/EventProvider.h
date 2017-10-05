/**
 * @file
 */
#pragma once

#include "EventConfigurationData.h"
#include "EventModel.h"
#include "core/Common.h"
#include "EventId.h"
#include "EventType.h"
#include "persistence/DBHandler.h"
#include <memory>
#include <unordered_map>

namespace eventmgr {

/**
 * @brief Provides configured events via EventData
 * @ingroup Events
 */
class EventProvider {
public:
	typedef std::unordered_map<EventId, db::EventModelPtr> EventData;
private:
	persistence::DBHandlerPtr _dbHandler;
	EventData _eventData;
public:
	EventProvider(const persistence::DBHandlerPtr& dbHandler);

	const EventData& eventData() const;

	bool init();
	void shutdown();

	db::EventModelPtr get(EventId id) const;
};

inline const EventProvider::EventData& EventProvider::eventData() const {
	return _eventData;
}

typedef std::shared_ptr<EventProvider> EventProviderPtr;

}
