/**
 * @file
 */

#include "HTTPMetricSender.h"
#include "core/Log.h"
#include "http/Http.h"
#include "io/Stream.h"

namespace metric {

HTTPMetricSender::HTTPMetricSender(const core::String &url, const core::String &userAgent) : _request(url, http::RequestType::POST) {
	_request.addHeader("Content-Type", "application/json");
	_request.setUserAgent(userAgent);
}

bool HTTPMetricSender::send(const char *buffer) const {
	http::Request request = _request;
	if (!request.setBody(buffer)) {
		return false;
	}
	return _client.execute(request) != 0;
}

bool HTTPMetricSender::init() {
	_request.noCache();
	return true;
}

void HTTPMetricSender::shutdown() {
	_client.cancelAll();
}

} // namespace metric
