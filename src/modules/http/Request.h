/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"
#include "io/Stream.h"

namespace http {

enum class RequestType {
	GET, POST
};

class Request {
private:
	int _timeoutSecond = 5;
	int _connectTimeoutSecond = 5;
	const RequestType _type;
	const core::String _url;
	core::String _userAgent;
	core::String _body;
	core::StringMap<core::String> _headers;

public:
	Request(const core::String &url, RequestType type);
	void setUserAgent(const core::String &userAgent);
	void setConnectTimeoutSecond(int timeoutSecond);
	void setTimeoutSecond(int timeoutSecond);
	bool setBody(const core::String &body);
	void addHeader(const core::String &key, const core::String &value);
	bool execute(io::WriteStream &stream, int *statusCode = nullptr, core::StringMap<core::String> *headers = nullptr);
	void noCache();

	const core::StringMap<core::String> &headers() const;
	const core::String &url() const;
	const core::String &body() const;
	RequestType type() const;

	static bool supported();
};

inline const core::StringMap<core::String> &Request::headers() const {
	return _headers;
}

inline RequestType Request::type() const {
	return _type;
}

inline const core::String &Request::url() const {
	return _url;
}

inline const core::String &Request::body() const {
	return _body;
}

inline void Request::setUserAgent(const core::String &userAgent) {
	_userAgent = userAgent;
}

inline void Request::setConnectTimeoutSecond(int timeoutSecond) {
	_connectTimeoutSecond = timeoutSecond;
}

inline void Request::setTimeoutSecond(int timeoutSecond) {
	_timeoutSecond = timeoutSecond;
}

} // namespace http
