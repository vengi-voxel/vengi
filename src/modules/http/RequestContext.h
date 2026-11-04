/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "core/collection/DynamicStringMap.h"
#include "core/concurrent/Atomic.h"

namespace http {

using Headers = core::DynamicStringMap<core::String>;

enum class RequestType { GET, POST, PATCH };

struct RequestContext {
	int _timeoutSecond = 5;
	int _connectTimeoutSecond = 5;
	bool _followRedirects = true;
	RequestType _type;
	core::String _url;
	core::String _userAgent;
	core::Buffer<uint8_t> _body;
	Headers _headers;
	/** Optional cooperative cancellation, owned by the asynchronous HTTP service. */
	const core::AtomicBool *_cancelled = nullptr;
};

} // namespace http
