/**
 * @file
 */

#pragma once

#include "ResponseParser.h"
#include "core/Common.h"
#include "core/String.h"

namespace http {

class HttpClient {
private:
	core::String _baseUrl;
	int _requestTimeOut = 0;
public:
	HttpClient(const core::String &baseUrl = "");

	/**
	 * @brief Change the base url for the http client that is put in front of every request
	 * @return @c false if the given base url is not a valid url
	 */
	bool setBaseUrl(const core::String &baseUrl);
	void setRequestTimeout(int seconds);

	ResponseParser get(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(2);
};

inline void HttpClient::setRequestTimeout(int seconds) {
	_requestTimeOut = seconds;
}

}
