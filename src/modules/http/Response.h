/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "http/RequestContext.h"

namespace http {

/**
 * @brief Result of an HTTP request, delivered to async callbacks on the main thread.
 */
struct Response {
	/** @c true if the transport layer completed (status may still be non-2xx). */
	bool success = false;
	int statusCode = 0;
	core::String body;
	Headers headers;
	core::String url;
};

} // namespace http
