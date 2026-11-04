/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "http/RequestContext.h"

namespace http {

struct Response {
	/** Transport completed. Check statusCode separately for HTTP errors. */
	bool success = false;
	int statusCode = 0;
	/** Raw response bytes; may contain embedded zeroes. */
	core::Buffer<uint8_t> body;
	Headers headers;
	core::String url;

	/** @brief Convert a text response to a string. Use body for binary data. */
	core::String text() const {
		return body.empty() ? core::String() : core::String((const char *)body.data(), body.size());
	}
};

} // namespace http
