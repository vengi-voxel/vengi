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
	if (!_request.setBody(buffer)) {
		Log::debug("Failed to set body");
		return false;
	}
	io::NOPWriteStream stream;
	int statusCode = -1;
	if (!_request.execute(stream, &statusCode)) {
		Log::debug("Failed to send metric %s - got status %i", buffer, statusCode);
		return false;
	}
	Log::debug("Sent metric %s - got status: %i", buffer, statusCode);
	return http::isValidStatusCode(statusCode);
}

bool HTTPMetricSender::init() {
	_request.noCache();
	return true;
}

void HTTPMetricSender::shutdown() {
}

} // namespace metric
