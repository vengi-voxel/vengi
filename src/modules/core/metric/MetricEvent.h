/**
 * @file
 *
 * https://github.com/b/statsd_spec
 */

#pragma once

#include "core/EventBus.h"
#include "core/collection/Map.h"
#include <stdint.h>
#include "core/String.h"

namespace metric {

enum class MetricEventType {
	Count,
	Gauge,
	Timing,
	Histogram,
	Meter
};

using TagMap = core::Map<core::String, core::String, 4, core::StringHash>;

class MetricEvent: public core::IEventBusEvent {
private:
	const core::String _key;
	const int32_t _value;
	const MetricEventType _type;
	TagMap _tags;
	MetricEvent() : _value(0), _type(MetricEventType::Count) {}
public:
	EVENTBUSTYPEID(MetricEvent)

	MetricEvent(const core::String& key, int32_t value, MetricEventType type, const TagMap& tags) :
			_key(key), _value(value), _type(type), _tags(tags) {
	}

	inline const TagMap& tags() const {
		return _tags;
	}

	inline MetricEventType type() const {
		return _type;
	}

	inline const core::String& key() const {
		return _key;
	}

	inline int32_t value() const {
		return _value;
	}
};

/**
 * @brief Add the specified delta to the given key
 */
static inline MetricEvent count(const core::String& key, int delta, const TagMap& tags = {}) {
	return MetricEvent(key, delta, MetricEventType::Count, tags);
}

/**
 * @brief Records a meter
 */
static inline MetricEvent meter(const core::String& key, int delta, const TagMap& tags = {}) {
	return MetricEvent(key, delta, MetricEventType::Meter, tags);
}

/**
 * @brief Records a gauge with the give value for the key
 */
static inline MetricEvent gauge(const core::String& key, uint32_t value, const TagMap& tags = {}) {
	return MetricEvent(key, value, MetricEventType::Gauge, tags);
}

/**
 * @brief Records a timing in millis for a key
 */
static inline MetricEvent timing(const core::String& key, uint32_t millis, const TagMap& tags = {}) {
	return MetricEvent(key, millis, MetricEventType::Timing, tags);
}

/**
 * @brief Records a histogram
 */
static inline MetricEvent histogram(const core::String& key, uint32_t millis, const TagMap& tags = {}) {
	return MetricEvent(key, millis, MetricEventType::Histogram, tags);
}

/**
 * @brief Increments the key
 */
static inline MetricEvent increment(const core::String& key, const TagMap& tags = {}) {
	return count(key, 1, tags);
}

/**
 * @brief Decrements the key
 */
static inline MetricEvent decrement(const core::String& key, const TagMap& tags = {}) {
	return count(key, -1, tags);
}

}
