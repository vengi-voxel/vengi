/**
 * @file
 */

#include "Http.h"
#include "Request.h"

namespace http {

bool download(const core::String &url, io::WriteStream &stream, int *statusCode) {
	Request request(url, RequestType::GET);
	return request.execute(stream, statusCode);
}

} // namespace http
