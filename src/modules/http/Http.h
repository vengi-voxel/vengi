/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "http/RequestContext.h"
#include "io/Stream.h"

namespace http {

bool isValidStatusCode(int statusCode);
bool download(const core::String &url, io::WriteStream &stream, int *statusCode = nullptr,
			  Headers *outheaders = nullptr);

} // namespace http
