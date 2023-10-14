/**
 * @file
 */

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
	void setConnectTimeoutSecond(int timeoutSecond);
	void setTimeoutSecond(int timeoutSecond);
	bool setBody(const core::String &body);
	void addHeader(const core::String &key, const core::String &value);
	bool execute(io::WriteStream &stream);
	void noCache();

	static bool supported();
};

inline void Request::setConnectTimeoutSecond(int timeoutSecond) {
	_connectTimeoutSecond = timeoutSecond;
}

inline void Request::setTimeoutSecond(int timeoutSecond) {
	_timeoutSecond = timeoutSecond;
}

} // namespace http
