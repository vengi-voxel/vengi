/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "backend/eventbus/Event.h"
#include "metric/Metric.h"
#include "metric/MetricEvent.h"
#include "metric/IMetricSender.h"
#include "network/NetworkEvents.h"
#include <memory>

namespace backend {

class MetricMgr :
	public core::IEventBusHandler<network::NewConnectionEvent>,
	public core::IEventBusHandler<metric::MetricEvent> ,
	public core::IEventBusHandler<EntityRemoveFromMapEvent>,
	public core::IEventBusHandler<EntityAddToMapEvent>,
	public core::IEventBusHandler<EntityDeleteEvent>,
	public core::IEventBusHandler<EntityAddEvent>{
private:
	metric::MetricPtr _metric;
public:
	MetricMgr(const metric::MetricPtr& metric, const core::EventBusPtr& eventBus);

	void onEvent(const metric::MetricEvent& event);
	void onEvent(const network::NewConnectionEvent& event);
	void onEvent(const EntityRemoveFromMapEvent& event);
	void onEvent(const EntityAddToMapEvent& event);
	void onEvent(const EntityDeleteEvent& event);
	void onEvent(const EntityAddEvent& event);

	metric::MetricPtr& metric();

	bool init();
	void shutdown();
};

inline metric::MetricPtr& MetricMgr::metric() {
	return _metric;
}

typedef std::shared_ptr<MetricMgr> MetricMgrPtr;

}
