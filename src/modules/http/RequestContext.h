/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"

namespace http {

enum class RequestType { GET, POST };

struct RequestContext {
	int _timeoutSecond = 5;
	int _connectTimeoutSecond = 5;
	RequestType _type;
	core::String _url;
	core::String _userAgent;
	core::String _body;
	core::StringMap<core::String> _headers;
};

} // namespace http
