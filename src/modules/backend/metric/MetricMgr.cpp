/**
 * @file
 */

#include "MetricMgr.h"
#include "core/Log.h"
#include "core/EventBus.h"
#include "backend/entity/Entity.h"
#include "metric/UDPMetricSender.h"

namespace backend {

MetricMgr::MetricMgr(const metric::IMetricSenderPtr& metricSender, const core::EventBusPtr& eventBus) :
		_metric("server."), _metricSender(metricSender) {
	eventBus->subscribe<EntityRemoveEvent>(*this);
	eventBus->subscribe<metric::MetricEvent>(*this);
	eventBus->subscribe<network::NewConnectionEvent>(*this);
}

bool MetricMgr::init() {
	if (!_metricSender->init()) {
		Log::warn("Failed to init metrics");
		return false;
	}
	if (!_metric.init(_metricSender)) {
		Log::warn("Failed to init metrics");
		// no hard error...
	}
	return true;
}

void MetricMgr::shutdown() {
	_metricSender->shutdown();
	_metric.shutdown();
}

void MetricMgr::onEvent(const metric::MetricEvent& event) {
	metric::MetricEventType type = event.type();
	switch (type) {
	case metric::MetricEventType::Count:
		_metric.count(event.key().c_str(), event.value(), event.tags());
		break;
	case metric::MetricEventType::Gauge:
		_metric.gauge(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Timing:
		_metric.timing(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Histogram:
		_metric.histogram(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Meter:
		_metric.meter(event.key().c_str(), event.value(), event.tags());
		break;
	}
}

void MetricMgr::onEvent(const network::NewConnectionEvent& event) {
	Log::info("new connection - waiting for login request from %u", event.peer()->connectID);
	_metric.increment("count.user");
}

void MetricMgr::onEvent(const EntityRemoveEvent& event) {
	if (event.entity()->entityType() == network::EntityType::PLAYER) {
		_metric.decrement("count.user");
	} else {
		_metric.decrement("count.npc");
	}
}

}
