/**
 * @file
 */

#include "MetricFacade.h"
#include "UDPMetricSender.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Singleton.h"
#include "core/Var.h"

namespace metric {

struct MetricState {
	metric::IMetricSenderPtr _sender;
	metric::Metric _metric;

	bool init(const core::String &appname);
	void shutdown();
};

bool MetricState::init(const core::String &appname) {
	const core::VarPtr &flavor = core::Var::get(cfg::MetricFlavor, "");
	if (flavor->strVal().empty()) {
		Log::debug("No metrics activated - skip init");
		return false;
	}
	const core::String &host = core::Var::get(cfg::MetricHost, "127.0.0.1")->strVal();
	const int port = core::Var::get(cfg::MetricPort, "8125")->intVal();
	_sender = core::make_shared<metric::UDPMetricSender>(host, port);
	if (!_sender->init()) {
		Log::warn("Failed to init metric sender");
		return false;
	}
	if (!_metric.init(appname.c_str(), _sender)) {
		Log::warn("Failed to init metrics");
		return false;
	}
	Log::info("Initialized metrics");
	return true;
}

void MetricState::shutdown() {
	if (_sender) {
		_sender->shutdown();
		_sender = metric::IMetricSenderPtr();
	}
	_metric.shutdown();
}

bool count(const char *key, int delta, const TagMap &tags) {
	MetricState &s = core::Singleton<MetricState>::getInstance();
	if (!s._sender) {
		return false;
	}
	return s._metric.count(key, delta, tags);
}

bool init(const core::String &appname) {
	return core::Singleton<MetricState>::getInstance().init(appname);
}

void shutdown() {
	core::Singleton<MetricState>::getInstance().shutdown();
}

} // namespace metric
