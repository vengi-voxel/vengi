/**
 * @file
 */

#pragma once

#include "IMetricSender.h"
#include "core/String.h"
#include "http/Client.h"

namespace metric {

class HTTPMetricSender : public IMetricSender {
private:
	http::Request _request;
	mutable http::Client _client;

public:
	HTTPMetricSender(const core::String &url, const core::String &userAgent);
	bool send(const char *buffer) const override;

	bool init() override;
	void shutdown() override;
};

} // namespace metric
