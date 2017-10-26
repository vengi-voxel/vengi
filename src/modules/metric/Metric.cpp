/**
 * @file
 */

#include "Metric.h"
#include "core/Log.h"
#include <cstdio>
#include <cstring>

namespace metric {

Metric::Metric(const IMetricSenderPtr& messageSender, const std::string& prefix, Flavor flavor) :
		_prefix(prefix), _flavor(flavor), _messageSender(messageSender) {
}

Metric::~Metric() {
	shutdown();
}

bool Metric::init() {
	return _messageSender->init();
}

void Metric::shutdown() {
	_messageSender->shutdown();
}

bool Metric::createTags(char* buffer, size_t len, const TagMap& tags, const char* sep, const char* preamble, const char *split) const {
	if (tags.empty()) {
		return true;
	}
	const size_t preambleLen = strlen(preamble);
	if (preambleLen >= len) {
		return false;
	}
	strncpy(buffer, preamble, preambleLen);
	buffer += preambleLen;
	int remainingLen = len - preambleLen;
	bool first = true;
	const size_t splitLen = strlen(split);
	for (const auto& e : tags) {
		if (remainingLen <= 0) {
			return false;
		}
		size_t keyValueLen = e.first.size() + strlen(sep) + e.second.size();
		int written;
		if (first) {
			written = snprintf(buffer, remainingLen, "%s%s%s", e.first.c_str(), sep, e.second.c_str());
		} else {
			keyValueLen += splitLen;
			written = snprintf(buffer, remainingLen, "%s%s%s%s", split, e.first.c_str(), sep, e.second.c_str());
		}
		if (written >= remainingLen) {
			return false;
		}
		buffer += keyValueLen;
		remainingLen -= keyValueLen;
		first = false;
	}
	return true;
}

bool Metric::assemble(const char* key, int value, const char* type, const TagMap& tags) const {
	constexpr int metricSize = 256;
	char buffer[metricSize];
	constexpr int tagsSize = 256;
	char tagsBuffer[tagsSize];
	int written;
	switch (_flavor) {
	case Flavor::Etsy:
		written = snprintf(buffer, sizeof(buffer), "%s%s:%i|%s", _prefix.c_str(), key, value, type);
		break;
	case Flavor::Datadog:
		if (!createTags(tagsBuffer, sizeof(tagsBuffer), tags, ":", "|#", ",")) {
			return false;
		}
		written = snprintf(buffer, sizeof(buffer), "%s%s:%i|%s%s", _prefix.c_str(), key, value, type, tagsBuffer);
		break;
	case Flavor::Telegraf:
	default:
		if (!createTags(tagsBuffer, sizeof(tagsBuffer), tags, "=", ",", ",")) {
			return false;
		}
		written = snprintf(buffer, sizeof(buffer), "%s%s%s:%i|%s", _prefix.c_str(), key, tagsBuffer, value, type);
		break;
	}
	if (written >= metricSize) {
		return false;
	}
	return _messageSender->send(buffer);
}

bool Metric::increment(const char* key, const TagMap& tags) const {
	return count(key, 1, tags);
}

bool Metric::decrement(const char* key, const TagMap& tags) const {
	return count(key, -1, tags);
}

bool Metric::count(const char* key, int delta, const TagMap& tags, float sampleRate) const {
	return assemble(key, delta, "c", tags); // TODO:"|@%f", sampleRate
}

bool Metric::gauge(const char* key, uint32_t value, const TagMap& tags) const {
	return assemble(key, value, "g", tags);
}

bool Metric::timing(const char* key, uint32_t millis, const TagMap& tags) const {
	return assemble(key, millis, "ms", tags);
}

bool Metric::histogram(const char* key, uint32_t millis, const TagMap& tags) const {
	return assemble(key, millis, "h", tags);
}

bool Metric::meter(const char* key, int value, const TagMap& tags) const {
	return assemble(key, value, "m", tags);
}

}
