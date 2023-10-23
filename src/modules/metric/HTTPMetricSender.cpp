/**
 * @file
 */

#include "HTTPMetricSender.h"
#include "io/Stream.h"

namespace metric {

class NOPWriteStream : public io::WriteStream {
public:
	int write(const void *buf, size_t size) override {
		return size;
	}
};

HTTPMetricSender::HTTPMetricSender(const core::String &url) : _request(url, http::RequestType::POST) {
}

bool HTTPMetricSender::send(const char *buffer) const {
	_request.setBody(buffer);
	NOPWriteStream stream;
	return _request.execute(stream);
}

bool HTTPMetricSender::init() {
	return true;
}

void HTTPMetricSender::shutdown() {
}

} // namespace metric
