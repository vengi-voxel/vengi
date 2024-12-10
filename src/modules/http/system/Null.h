/**
 * @file
 */

#pragma once

#include "http/RequestContext.h"
#include "io/Stream.h"

namespace http {

inline bool http_request(io::WriteStream &stream, int *statusCode, core::StringMap<core::String> *outheaders,
						 RequestContext &ctx) {
	return false;
}

inline bool http_supported() {
	return false;
}

} // namespace http
