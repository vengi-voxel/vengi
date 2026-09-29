/**
 * @file
 */

#include "HTTPMetricSender.h"
#include "core/Log.h"
#include "http/Http.h"
#include "http/RequestAsync.h"

namespace metric {

HTTPMetricSender::HTTPMetricSender(const core::String &url, const core::String &userAgent)
	: _url(url), _userAgent(userAgent) {
}

bool HTTPMetricSender::send(const char *buffer) const {
	if (buffer == nullptr || buffer[0] == '\0') {
		Log::debug("Failed to set body");
		return false;
	}
	http::Request request(_url, http::RequestType::POST);
	request.addHeader("Content-Type", "application/json");
	request.setUserAgent(_userAgent);
	request.noCache();
	if (!request.setBody(buffer)) {
		Log::debug("Failed to set body");
		return false;
	}
	const core::String payload(buffer);
	return http::requestAsync(core::move(request), [payload](const http::Response &response) {
			   if (!response.success || !http::isValidStatusCode(response.statusCode)) {
				   Log::debug("Failed to send metric %s - got status %i", payload.c_str(), response.statusCode);
				   return;
			   }
			   Log::debug("Sent metric %s - got status: %i", payload.c_str(), response.statusCode);
		   }) != 0u;
}

bool HTTPMetricSender::init() {
	return true;
}

void HTTPMetricSender::shutdown() {
}

} // namespace metric
