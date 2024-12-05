/**
 * @file
 */

#include "Http.h"
#include "Request.h"

namespace http {

bool isValidStatusCode(int statusCode) {
	return statusCode >= 200 && statusCode < 300;
}

bool download(const core::String &url, io::WriteStream &stream, int *statusCode,
			  core::StringMap<core::String> *outheaders) {
	if (url.empty()) {
		return false;
	}
	Request request(url, RequestType::GET);
	request.noCache();
	return request.execute(stream, statusCode, outheaders);
}

} // namespace http
