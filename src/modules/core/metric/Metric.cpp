/**
 * @file
 */

#include "Metric.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include <stdio.h>
#include <string.h>
#include <SDL_stdinc.h>

namespace metric {

Metric::~Metric() {
	shutdown();
}

bool Metric::init(const char *prefix, const IMetricSenderPtr& messageSender) {
	_prefix = prefix;
	const core::String& flavor = core::Var::getSafe(cfg::MetricFlavor)->strVal();
	if (flavor == "telegraf") {
		_flavor = Flavor::Telegraf;
		Log::debug("Using metric flavor 'telegraf'");
	} else if (flavor == "etsy") {
		_flavor = Flavor::Etsy;
		Log::debug("Using metric flavor 'etsy'");
	} else if (flavor == "datadog") {
		_flavor = Flavor::Datadog;
		Log::debug("Using metric flavor 'datadog'");
	} else if (flavor == "influx") {
		_flavor = Flavor::Influx;
		Log::debug("Using metric flavor 'influx'");
	} else {
		Log::warn("Invalid %s given - using telegraf", cfg::MetricFlavor);
	}
	_messageSender = messageSender;
	return true;
}

void Metric::shutdown() {
	_messageSender = IMetricSenderPtr();
}

bool Metric::createTags(char* buffer, size_t len, const TagMap& tags, const char* sep, const char* preamble, const char *split) const {
	if (tags.empty()) {
		return true;
	}
	const size_t preambleLen = SDL_strlen(preamble);
	if (preambleLen >= len) {
		return false;
	}
	strncpy(buffer, preamble, preambleLen);
	buffer += preambleLen;
	int remainingLen = (int)(len - preambleLen);
	bool first = true;
	const size_t splitLen = SDL_strlen(split);
	for (const auto& e : tags) {
		if (remainingLen <= 0) {
			return false;
		}
		size_t keyValueLen = e->key.size() + strlen(sep) + e->value.size();
		int written;
		if (first) {
			written = SDL_snprintf(buffer, remainingLen, "%s%s%s", e->key.c_str(), sep, e->value.c_str());
		} else {
			keyValueLen += splitLen;
			written = SDL_snprintf(buffer, remainingLen, "%s%s%s%s", split, e->key.c_str(), sep, e->value.c_str());
		}
		if (written >= remainingLen) {
			return false;
		}
		buffer += keyValueLen;
		remainingLen -= (int)keyValueLen;
		first = false;
	}
	return true;
}

bool Metric::assemble(const char* key, int value, const char* type, const TagMap& tags) const {
	if (!_messageSender) {
		return false;
	}
	constexpr int metricSize = 256;
	char buffer[metricSize];
	constexpr int tagsSize = 256;
	char tagsBuffer[tagsSize] = "";
	int written;
	switch (_flavor) {
	case Flavor::Etsy:
		written = SDL_snprintf(buffer, sizeof(buffer), "%s.%s:%i|%s", _prefix.c_str(), key, value, type);
		break;
	case Flavor::Datadog:
		if (!createTags(tagsBuffer, sizeof(tagsBuffer), tags, ":", "|#", ",")) {
			return false;
		}
		written = SDL_snprintf(buffer, sizeof(buffer), "%s.%s:%i|%s%s", _prefix.c_str(), key, value, type, tagsBuffer);
		break;
	case Flavor::Influx:
		if (!createTags(tagsBuffer, sizeof(tagsBuffer), tags, "=", ",", ",")) {
			return false;
		}
		written = SDL_snprintf(buffer, sizeof(buffer), "%s_%s,type=%s%s value=%i", _prefix.c_str(), key, type, tagsBuffer, value);
		break;
	case Flavor::Telegraf:
	default:
		if (!createTags(tagsBuffer, sizeof(tagsBuffer), tags, "=", ",", ",")) {
			return false;
		}
		written = SDL_snprintf(buffer, sizeof(buffer), "%s.%s%s:%i|%s", _prefix.c_str(), key, tagsBuffer, value, type);
		break;
	}
	if (written >= metricSize) {
		return false;
	}
	return _messageSender->send(buffer);
}

}
