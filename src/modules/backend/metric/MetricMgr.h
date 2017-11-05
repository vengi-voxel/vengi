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
	public core::IEventBusHandler<EntityRemoveEvent>,
	public core::IEventBusHandler<metric::MetricEvent> {
private:
	metric::Metric _metric;
	metric::IMetricSenderPtr _metricSender;
public:
	MetricMgr(const metric::IMetricSenderPtr& metricSender, const core::EventBusPtr& eventBus);

	void onEvent(const metric::MetricEvent& event);
	void onEvent(const network::NewConnectionEvent& event);
	void onEvent(const EntityRemoveEvent& event);

	metric::Metric& metric();

	bool init();
	void shutdown();
};

inline metric::Metric& MetricMgr::metric() {
	return _metric;
}

typedef std::shared_ptr<MetricMgr> MetricMgrPtr;

}
