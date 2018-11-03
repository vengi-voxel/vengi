/**
 * @file
 */

#include "MetricMgr.h"
#include "core/Log.h"
#include "core/EventBus.h"
#include "backend/entity/Entity.h"
#include "metric/UDPMetricSender.h"
#include "network/ProtocolEnum.h"
#include "backend/world/Map.h"

namespace backend {

MetricMgr::MetricMgr(
		const metric::MetricPtr& metric,
		const core::EventBusPtr& eventBus) :
		_metric(metric) {
	eventBus->subscribe<EntityAddToMapEvent>(*this);
	eventBus->subscribe<EntityRemoveFromMapEvent>(*this);
	eventBus->subscribe<EntityAddEvent>(*this);
	eventBus->subscribe<EntityDeleteEvent>(*this);
	eventBus->subscribe<metric::MetricEvent>(*this);
	eventBus->subscribe<network::NewConnectionEvent>(*this);
}

bool MetricMgr::init() {
	return true;
}

void MetricMgr::shutdown() {
}

void MetricMgr::onEvent(const metric::MetricEvent& event) {
	metric::MetricEventType type = event.type();
	switch (type) {
	case metric::MetricEventType::Count:
		_metric->count(event.key().c_str(), event.value(), event.tags());
		break;
	case metric::MetricEventType::Gauge:
		_metric->gauge(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Timing:
		_metric->timing(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Histogram:
		_metric->histogram(event.key().c_str(), (uint32_t)event.value(), event.tags());
		break;
	case metric::MetricEventType::Meter:
		_metric->meter(event.key().c_str(), event.value(), event.tags());
		break;
	}
}

void MetricMgr::onEvent(const network::NewConnectionEvent& event) {
	Log::info("new connection - waiting for login request from %u", event.get()->connectID);
	_metric->increment("count.user");
}

void MetricMgr::onEvent(const EntityAddEvent& event) {
	const EntityPtr& entity = event.get();
	const network::EntityType type = entity->entityType();
	const char *typeName = network::EnumNameEntityType(type);
	_metric->increment("count.entity", {{"type", typeName}});
}

void MetricMgr::onEvent(const EntityDeleteEvent& event) {
	const network::EntityType type = event.entityType();
	const char *typeName = network::EnumNameEntityType(type);
	_metric->decrement("count.entity", {{"type", typeName}});
}

void MetricMgr::onEvent(const EntityAddToMapEvent& event) {
	const EntityPtr& entity = event.get();
	const MapPtr& map = entity->map();
	const network::EntityType type = entity->entityType();
	const char *typeName = network::EnumNameEntityType(type);
	_metric->increment("count.map.entity", {{"map", map->idStr()}, {"type", typeName}});
}

void MetricMgr::onEvent(const EntityRemoveFromMapEvent& event) {
	const EntityPtr& entity = event.get();
	const MapPtr& map = entity->map();
	const network::EntityType type = entity->entityType();
	const char *typeName = network::EnumNameEntityType(type);
	_metric->decrement("count.map.entity", {{"map", map->idStr()}, {"type", typeName}});
}

}
