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
private:
	persistence::DBHandlerPtr _dbHandler;
	std::unordered_map<EventId, db::EventModelPtr> _eventData;
public:
	EventProvider(const persistence::DBHandlerPtr& dbHandler);

	bool init();
	void shutdown();

	db::EventModelPtr get(EventId id) const;
};

typedef std::shared_ptr<EventProvider> EventProviderPtr;

}
