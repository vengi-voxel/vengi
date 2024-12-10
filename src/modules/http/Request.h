/**
 * @file
 */

#pragma once

#include "http/RequestContext.h"
#include "io/Stream.h"

namespace http {

class Request {
private:
	RequestContext _ctx;

public:
	Request(const core::String &url, RequestType type);
	void setUserAgent(const core::String &userAgent);
	void setConnectTimeoutSecond(int timeoutSecond);
	void setTimeoutSecond(int timeoutSecond);
	bool setBody(const core::String &body);
	void addHeader(const core::String &key, const core::String &value);
	bool execute(io::WriteStream &stream, int *statusCode = nullptr,
				 core::StringMap<core::String> *outheaders = nullptr);
	void noCache();

	const core::StringMap<core::String> &headers() const;
	const core::String &url() const;
	const core::String &body() const;
	RequestType type() const;

	static bool supported();
};

inline const core::StringMap<core::String> &Request::headers() const {
	return _ctx._headers;
}

inline RequestType Request::type() const {
	return _ctx._type;
}

inline const core::String &Request::url() const {
	return _ctx._url;
}

inline const core::String &Request::body() const {
	return _ctx._body;
}

inline void Request::setUserAgent(const core::String &userAgent) {
	_ctx._userAgent = userAgent;
}

inline void Request::setConnectTimeoutSecond(int timeoutSecond) {
	_ctx._connectTimeoutSecond = timeoutSecond;
}

inline void Request::setTimeoutSecond(int timeoutSecond) {
	_ctx._timeoutSecond = timeoutSecond;
}

} // namespace http
