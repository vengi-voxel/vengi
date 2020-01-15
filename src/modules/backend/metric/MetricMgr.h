/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "core/IComponent.h"
#include "backend/eventbus/Event.h"
#include "core/metric/Metric.h"
#include "core/metric/MetricEvent.h"
#include "core/metric/IMetricSender.h"
#include "network/NetworkEvents.h"
#include <memory>

namespace backend {

class MetricMgr :
	public core::IComponent,
	public core::IEventBusHandler<network::NewConnectionEvent>,
	public core::IEventBusHandler<metric::MetricEvent> ,
	public core::IEventBusHandler<EntityRemoveFromMapEvent>,
	public core::IEventBusHandler<EntityAddToMapEvent>,
	public core::IEventBusHandler<EntityDeleteEvent>,
	public core::IEventBusHandler<EntityAddEvent> {
private:
	metric::MetricPtr _metric;
public:
	MetricMgr(const metric::MetricPtr& metric, const core::EventBusPtr& eventBus);

	void onEvent(const metric::MetricEvent& event) override;
	void onEvent(const network::NewConnectionEvent& event) override;
	void onEvent(const EntityRemoveFromMapEvent& event) override;
	void onEvent(const EntityAddToMapEvent& event) override;
	void onEvent(const EntityDeleteEvent& event) override;
	void onEvent(const EntityAddEvent& event) override;

	metric::MetricPtr& metric();

	bool init() override;
	void shutdown() override;
};

inline metric::MetricPtr& MetricMgr::metric() {
	return _metric;
}

typedef std::shared_ptr<MetricMgr> MetricMgrPtr;

}
