/**
 * @file
 */

#pragma once

#include <memory>
#include "EventType.h"

namespace eventmgr {

/**
 * Event configuration data that is defined in lua scripts
 * @ingroup Events
 */
struct EventConfigurationData {
	const core::String eventNameId;
	const Type type = Type::NONE;
	// TODO: spawn data
	// TODO: poi data (automatic via spawning?)

	EventConfigurationData(const core::String& _eventNameId, Type _type) :
			eventNameId(_eventNameId), type(_type) {
	}
};

typedef std::shared_ptr<EventConfigurationData> EventConfigurationDataPtr;

}
