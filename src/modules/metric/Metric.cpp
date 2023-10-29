/**
 * @file
 */

#include "Metric.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include <SDL_stdinc.h>
#include <stdio.h>
#include <string.h>

namespace metric {

Metric::~Metric() {
	shutdown();
}

bool Metric::init(const char *prefix, const IMetricSenderPtr &messageSender) {
	_prefix = prefix;
	core::VarPtr uuid = core::Var::get(cfg::MetricUUID, "");
	if (uuid->strVal().empty()) {
		uuid->setVal(core::generateUUID());
	}
	_uuid = uuid->strVal();
	Log::debug("Use uuid for metrics: %s", _uuid.c_str());

	const core::String &flavor = core::Var::getSafe(cfg::MetricFlavor)->strVal();
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
	} else if (flavor == "json") {
		_flavor = Flavor::JSON;
		Log::debug("Using metric flavor 'json'");
	} else {
		Log::warn("Invalid %s given - using telegraf", cfg::MetricFlavor);
	}
	_messageSender = messageSender;
	return true;
}

void Metric::shutdown() {
	_messageSender = IMetricSenderPtr();
}

bool Metric::createTags(char *buffer, size_t len, const TagMap &tags, const char *sep, const char *preamble,
						const char *split) const {
	const size_t preambleLen = SDL_strlen(preamble);
	if (preambleLen >= len) {
		return false;
	}
	SDL_strlcpy(buffer, preamble, len);
	buffer += preambleLen;
	int remainingLen = (int)(len - preambleLen);

	const int uuidWritten = SDL_snprintf(buffer, remainingLen, "uuid%s%s", sep, _uuid.c_str());
	if (uuidWritten >= remainingLen) {
		return false;
	}
	size_t uuidKeyValueLen = 4 + SDL_strlen(sep) + _uuid.size();
	buffer += uuidKeyValueLen;
	remainingLen -= uuidKeyValueLen;

	const char illegal[] = {
		' ', '#', ';', ',', ':', '=', '(', ')', '[', ']', '|'
	};

	const size_t splitLen = SDL_strlen(split);
	for (const auto &e : tags) {
		if (remainingLen <= 0) {
			return false;
		}
		if (e->second.empty()) {
			continue;
		}
		size_t keyValueLen = e->key.size() + SDL_strlen(sep) + e->value.size();
		keyValueLen += splitLen;
		core::String val = e->value;
		for (char c : illegal) {
			val.replaceAllChars(c, '_');
		}
		const int written = SDL_snprintf(buffer, remainingLen, "%s%s%s%s", split, e->key.c_str(), sep, val.c_str());
		if (written >= remainingLen) {
			return false;
		}
		buffer += keyValueLen;
		remainingLen -= (int)keyValueLen;
	}
	return true;
}

bool Metric::assemble(const char *key, int value, const char *type, const TagMap &tags) const {
	if (!_messageSender) {
		return false;
	}
	constexpr int metricSize = 256;
	char buffer[metricSize];
	constexpr int tagsSize = 256;
	char tagsBuffer[tagsSize] = "";
	int written;
	switch (_flavor) {
	case Flavor::JSON: {
		core::String json;
		json.append("{");
		json.append("\"name\": \"").append(_prefix).append(".").append(key).append("\",");
		json.append("\"value\": ").append(value).append(",");
		json.append("\"type\": \"").append(type).append("\",");
		json.append("\"tags\": {");
		json.append("\"uuid\": \"").append(_uuid).append("\"");
		for (const auto &e : tags) {
			json.append(",");
			json.append("\"").append(e->first).append("\": \"").append(e->second).append("\"");
		}
		json.append("}");
		json.append("}");
		written = json.size();
		return _messageSender->send(json.c_str());
	}
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
		written = SDL_snprintf(buffer, sizeof(buffer), "%s_%s,type=%s%s value=%i", _prefix.c_str(), key, type,
							   tagsBuffer, value);
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

} // namespace metric
