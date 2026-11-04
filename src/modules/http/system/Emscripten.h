/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "http/RequestContext.h"
#include "io/Stream.h"
#include <emscripten/fetch.h>

namespace http {

void configureFetch(emscripten_fetch_attr_t &attr, const RequestContext &ctx, core::Buffer<const char *> &headers);
void readFetchHeaders(emscripten_fetch_t *fetch, Headers &headers);

bool http_request(io::WriteStream &stream, int *statusCode, Headers *outheaders,
				  RequestContext &ctx);

inline bool http_supported() {
	return true;
}

} // namespace http
