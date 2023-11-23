/**
 * @file
 */

#include "HTTPMetricSender.h"
#include "core/Log.h"
#include "io/Stream.h"

namespace metric {

class NOPWriteStream : public io::WriteStream {
public:
	int write(const void *buf, size_t size) override {
		return size;
	}
};

HTTPMetricSender::HTTPMetricSender(const core::String &url) : _request(url, http::RequestType::POST) {
	_request.addHeader("Content-Type", "application/json");
}

bool HTTPMetricSender::send(const char *buffer) const {
	if (!_request.setBody(buffer)) {
		Log::debug("Failed to set body");
		return false;
	}
	NOPWriteStream stream;
	if (!_request.execute(stream)) {
		Log::debug("Failed to send metric %s", buffer);
		return false;
	}
	Log::debug("Sent metric %s", buffer);
	return true;
}

bool HTTPMetricSender::init() {
	_request.noCache();
	return true;
}

void HTTPMetricSender::shutdown() {
}

} // namespace metric
