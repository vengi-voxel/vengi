/**
 * @file
 *
 * https://github.com/b/statsd_spec
 */

#pragma once

#include "core/EventBus.h"
#include "core/collection/Map.h"
#include <stdint.h>
#include <string>

namespace metric {

enum class MetricEventType {
	Count,
	Gauge,
	Timing,
	Histogram,
	Meter
};

using TagMap = core::Map<std::string, std::string, 4, std::hash<std::string>>;

class MetricEvent: public core::IEventBusEvent {
private:
	const std::string _key;
	const int32_t _value;
	const MetricEventType _type;
	TagMap _tags;
	MetricEvent() : _value(0), _type(MetricEventType::Count) {}
public:
	EVENTBUSTYPEID(MetricEvent)

	MetricEvent(const std::string& key, int32_t value, MetricEventType type, const TagMap& tags) :
			_key(key), _value(value), _type(type), _tags(tags) {
	}

	inline const TagMap& tags() const {
		return _tags;
	}

	inline MetricEventType type() const {
		return _type;
	}

	inline const std::string& key() const {
		return _key;
	}

	inline int32_t value() const {
		return _value;
	}
};

/**
 * @brief Add the specified delta to the given key
 */
static inline MetricEvent count(const std::string& key, int delta, const TagMap& tags = {}) {
	return MetricEvent(key, delta, MetricEventType::Count, tags);
}

/**
 * @brief Records a meter
 */
static inline MetricEvent meter(const std::string& key, int delta, const TagMap& tags = {}) {
	return MetricEvent(key, delta, MetricEventType::Meter, tags);
}

/**
 * @brief Records a gauge with the give value for the key
 */
static inline MetricEvent gauge(const std::string& key, uint32_t value, const TagMap& tags = {}) {
	return MetricEvent(key, value, MetricEventType::Gauge, tags);
}

/**
 * @brief Records a timing in millis for a key
 */
static inline MetricEvent timing(const std::string& key, uint32_t millis, const TagMap& tags = {}) {
	return MetricEvent(key, millis, MetricEventType::Timing, tags);
}

/**
 * @brief Records a histogram
 */
static inline MetricEvent histogram(const std::string& key, uint32_t millis, const TagMap& tags = {}) {
	return MetricEvent(key, millis, MetricEventType::Histogram, tags);
}

/**
 * @brief Increments the key
 */
static inline MetricEvent increment(const std::string& key, const TagMap& tags = {}) {
	return count(key, 1, tags);
}

/**
 * @brief Decrements the key
 */
static inline MetricEvent decrement(const std::string& key, const TagMap& tags = {}) {
	return count(key, -1, tags);
}

}
