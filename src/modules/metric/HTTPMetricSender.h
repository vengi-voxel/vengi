/**
 * @file
 */

#pragma once

#include "IMetricSender.h"
#include "core/String.h"

namespace metric {

class HTTPMetricSender : public IMetricSender {
private:
	core::String _url;
	core::String _userAgent;

public:
	HTTPMetricSender(const core::String &url, const core::String &userAgent);
	bool send(const char *buffer) const override;

	bool init() override;
	void shutdown() override;
};

} // namespace metric
