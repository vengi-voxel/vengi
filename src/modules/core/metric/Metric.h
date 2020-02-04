/**
 * @file
 * @defgroup Compute
 * @{
 *
 * The metric module can generate multiple metric flavors for statsd and influx.
 *
 * @}
 */

#pragma once

#include "IMetricSender.h"
#include "core/NonCopyable.h"
#include "core/collection/Map.h"
#include <memory>
#include "core/String.h"
#include <stdint.h>

namespace metric {

/**
 * @brief The metric flavors
 */
enum class Flavor {
	Etsy,		/**< https://github.com/etsy/statsd/blob/master/docs/metric_types.md */
	Datadog,	/**< https://docs.datadoghq.com/guides/dogstatsd/#datagram-format */
	Telegraf,	/**< https://www.influxdata.com/blog/getting-started-with-sending-statsd-metrics-to-telegraf-influxdb */
	Influx		/**< https://docs.influxdata.com/influxdb/v1.4/guides/writing_data */
};

/**
 * @brief If the configured Flavor supports tags, they are just a key-value pair of strings
 */
using TagMap = core::Map<core::String, core::String, 4, std::hash<core::String>>;

/**
 * @brief The Metric class generates and publishes metrics
 */
class Metric : public core::NonCopyable {
private:
	core::String _prefix;
	Flavor _flavor = Flavor::Telegraf;
	IMetricSenderPtr _messageSender;

	/**
	 * @brief Create the needed tag list if it is supported by the specified flavor
	 * @param[out] buffer The buffer to write the tag list into
	 * @param[in] len The size of the target buffer
	 * @param[in] sep The separator between the key and the value
	 * @param[in] preamble The preamble to add to the buffer if there are tags and to split from the remaining metric message
	 * @param[in] split The separator between key/value pairs
	 * @return @c false if not all tags could get written into the specified target buffer, @c true otherwise
	 */
	static bool createTags(char *buffer, size_t len, const TagMap& tags, const char* sep, const char* preamble, const char *split = ",");
	bool assemble(const char* key, int value, const char* type, const TagMap& tags = {}) const;
public:
	~Metric();

	/**
	 * @param[in] messageSender @c IMessageSender - must already be initialized
	 * @note Reads the @c metric_flavor cvar to configure the flavor.
	 */
	bool init(const char *prefix, const IMetricSenderPtr& messageSender);
	void shutdown();

	/**
	 * @brief Increments the key
	 */
	bool increment(const char* key, const TagMap& tags = {}) const;

	/**
	 * @brief Decrements the key
	 */
	bool decrement(const char* key, const TagMap& tags = {}) const;

	/**
	 * @brief Add the specified delta to the given key
	 * A counter is a gauge calculated at the server. Metrics sent by the client
	 * increment or decrement the value of the gauge rather than giving its current
	 * value. Counters may also have an associated sample rate, given as a decimal
	 * of the number of samples per event count. For example, a sample rate of 1/10
	 * would be exported as 0.1. Valid counter values are in the range (-2^63^, 2^63^).
	 * @code <metric name>:<value>|c[|@<sample rate>] @endcode
	 */
	bool count(const char* key, int delta, const TagMap& tags = {}, float sampleRate = 1.0f) const;

	/**
	 * @brief Records a gauge with the give value for the key
	 *
	 * A gauge is an instantaneous measurement of a value, like the gas
	 * gauge in a car. It differs from a counter by being calculated at the
	 * client rather than the server. Valid gauge values are in the range [0, 2^64^)
	 * @code <metric name>:<value>|g @endcode
	 */
	bool gauge(const char* key, uint32_t value, const TagMap& tags = {}) const;

	/**
	 * @brief Records a timing in millis for a key
	 * A timer is a measure of the number of milliseconds elapsed between a start
	 * and end time, for example the time to complete rendering of a web page for
	 * a user. Valid timer values are in the range [0, 2^64^).
	 * @code <metric name>:<value>|ms @endcode
	 */
	bool timing(const char* key, uint32_t millis, const TagMap& tags = {}) const;

	/**
	 * @brief Records a histogram
	 * A histogram is a measure of the distribution of timer values over time,
	 * calculated at the server. As the data exported for timers and histograms is
	 * the same, this is currently an alias for a timer. Valid histogram values
	 * are in the range [0, 2^64^).
	 * @code <metric name>:<value>|h @endcode
	 */
	bool histogram(const char* key, uint32_t millis, const TagMap& tags = {}) const;

	/**
	 * @brief Records a meter
	 * A meter measures the rate of events over time, calculated at the server.
	 * They may also be thought of as increment-only counters. Valid meter values
	 * are in the range [0, 2^64^).
	 * @code <metric name>:<value>|m @endcode
	 * In at least one implementation, this is abbreviated for the common case
	 * of incrementing the meter by 1.
	 * @code <metric name> @endcode
	 * While this is convenient, the full, explicit metric form should be used.
	 * The shortened form is documented here for completeness.
	 */
	bool meter(const char* key, int value, const TagMap& tags = {}) const;
};

inline bool Metric::increment(const char* key, const TagMap& tags) const {
	return count(key, 1, tags);
}

inline bool Metric::decrement(const char* key, const TagMap& tags) const {
	return count(key, -1, tags);
}

inline bool Metric::count(const char* key, int delta, const TagMap& tags, float sampleRate) const {
	return assemble(key, delta, "c", tags); // TODO:"|@%f", sampleRate
}

inline bool Metric::gauge(const char* key, uint32_t value, const TagMap& tags) const {
	return assemble(key, value, "g", tags);
}

inline bool Metric::timing(const char* key, uint32_t millis, const TagMap& tags) const {
	return assemble(key, millis, "ms", tags);
}

inline bool Metric::histogram(const char* key, uint32_t millis, const TagMap& tags) const {
	return assemble(key, millis, "h", tags);
}

inline bool Metric::meter(const char* key, int value, const TagMap& tags) const {
	return assemble(key, value, "m", tags);
}

using MetricPtr = std::shared_ptr<Metric>;

}
