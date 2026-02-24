/**
 * @file
 */

#include "MetricFacade.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/concurrent/ThreadPool.h"
#include "engine-config.h" // PROJECT_VERSION
#include "metric/HTTPMetricSender.h"
#include "metric/UDPMetricSender.h"

namespace metric {

struct MetricState {
	metric::IMetricSenderPtr _sender;
	metric::Metric _metric;
	core::ThreadPool _threadPool{1, "metric"};

	bool init(const core::String &appname);
	void shutdown();

	static MetricState &getInstance() {
		static MetricState theInstance;
		return theInstance;
	}
};

bool MetricState::init(const core::String &appname) {
	const core::VarPtr &flavor = core::getVar(cfg::MetricFlavor);
	if (flavor->strVal().empty()) {
		Log::debug("No metrics activated - skip init");
		return false;
	}

	if (flavor->strVal() == "json") {
		const core::VarDef metricJsonUrl(cfg::MetricJsonUrl, "https://vengi-voxel.de/api/metric", N_("Metric JSON URL"), N_("The URL to send JSON metrics to"));
		const core::String &url = core::Var::registerVar(metricJsonUrl)->strVal();
		const core::String userAgent = appname + "/" PROJECT_VERSION;
		_sender = core::make_shared<metric::HTTPMetricSender>(url, userAgent);
	} else {
		const core::VarDef metricHost(cfg::MetricHost, "127.0.0.1", N_("Metric Host"), N_("The host to send metrics to"));
		const core::String &host = core::Var::registerVar(metricHost)->strVal();
		const core::VarDef metricPort(cfg::MetricPort, 8125, N_("Metric Port"), N_("The port to send metrics to"));
		const int port = core::Var::registerVar(metricPort)->intVal();
		_sender = core::make_shared<metric::UDPMetricSender>(host, port);
	}
	if (!_sender->init()) {
		Log::warn("Failed to init metric sender");
		return false;
	}
	if (!_metric.init(appname.c_str(), _sender)) {
		Log::warn("Failed to init metrics");
		return false;
	}
	_threadPool.init();
	Log::info("Initialized metrics");
	return true;
}

void MetricState::shutdown() {
	_threadPool.shutdown(true);
	if (_sender) {
		_sender->shutdown();
		_sender = metric::IMetricSenderPtr();
	}
	_metric.shutdown();
}

bool count(const core::String &key, int delta, const TagMap &tags) {
	MetricState &s = MetricState::getInstance();
	s._threadPool.schedule([=, &s]() { s._metric.count(key.c_str(), delta, tags); });
	return true;
}

bool init(const core::String &appname) {
	return MetricState::getInstance().init(appname);
}

void shutdown() {
	MetricState::getInstance().shutdown();
}

} // namespace metric
