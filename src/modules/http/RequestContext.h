/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"

namespace http {
	
using Headers = core::DynamicStringMap<core::String>;

enum class RequestType { GET, POST };

struct RequestContext {
	int _timeoutSecond = 5;
	int _connectTimeoutSecond = 5;
	RequestType _type;
	core::String _url;
	core::String _userAgent;
	core::String _body;
	Headers _headers;
};

} // namespace http
