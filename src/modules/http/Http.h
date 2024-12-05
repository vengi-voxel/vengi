/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/StringMap.h"
#include "io/Stream.h"

namespace http {

bool isValidStatusCode(int statusCode);
bool download(const core::String &url, io::WriteStream &stream, int *statusCode = nullptr,
			  core::StringMap<core::String> *outheaders = nullptr);

} // namespace http
