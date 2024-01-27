/**
 * @file
 */

#include "Http.h"
#include "Request.h"
#include "app/App.h"
#include "engine-config.h"

namespace http {

bool isValidStatusCode(int statusCode) {
	return statusCode >= 200 && statusCode < 300;
}

bool download(const core::String &url, io::WriteStream &stream, int *statusCode) {
	if (url.empty()) {
		return false;
	}
	Request request(url, RequestType::GET);
	const core::String userAgent = app::App::getInstance()->fullAppname() + "/" PROJECT_VERSION;
	request.setUserAgent(userAgent);
	request.noCache();
	return request.execute(stream, statusCode);
}

} // namespace http
