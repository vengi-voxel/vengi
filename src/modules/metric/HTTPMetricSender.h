/**
 * @file
 */

#pragma once

#include "IMetricSender.h"
#include "core/String.h"
#include "http/Request.h"

namespace metric {

class HTTPMetricSender : public IMetricSender {
private:
	mutable http::Request _request;

public:
	HTTPMetricSender(const core::String &url);
	bool send(const char *buffer) const override;

	bool init() override;
	void shutdown() override;
};

} // namespace metric
