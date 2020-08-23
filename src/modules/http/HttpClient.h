/**
 * @file
 */

#pragma once

#include "ResponseParser.h"
#include "core/Common.h"
#include "core/String.h"
#include "http/HttpHeader.h"
#include "http/HttpResponse.h"

namespace http {

class HttpClient {
private:
	core::String _baseUrl;
	int _requestTimeOut = 0;
	http::HeaderMap _headers;

public:
	HttpClient(const core::String &baseUrl = "");

	/**
	 * @brief Change the base url for the http client that is put in front of every request
	 * @return @c false if the given base url is not a valid url
	 */
	bool setBaseUrl(const core::String &baseUrl);
	const core::String& baseUrl() const;
	void setRequestTimeout(int seconds);
	int requestTimeout() const;

	HttpClient& charset(const char* charset);
	HttpClient& contentType(const char* mimeType);
	HttpClient& accept(const char* mimeType);
	HttpClient& header(const char* key, const char *value);
	http::HeaderMap& headers();

	ResponseParser get(CORE_FORMAT_STRING const char *msg, ...) CORE_PRINTF_VARARG_FUNC(2);
};

inline http::HeaderMap& HttpClient::headers() {
	return _headers;
}

inline void HttpClient::setRequestTimeout(int seconds) {
	_requestTimeOut = seconds;
}

inline int HttpClient::requestTimeout() const {
	return _requestTimeOut;
}

inline const core::String& HttpClient::baseUrl() const {
	return _baseUrl;
}

inline HttpClient& HttpClient::contentType(const char* mimeType) {
	header(header::CONTENT_TYPE, mimeType);
	return *this;
}

inline HttpClient& HttpClient::charset(const char* charset) {
	header(header::CHARSET, charset);
	return *this;
}

inline HttpClient& HttpClient::accept(const char* mimeType) {
	header(header::ACCEPT, mimeType);
	return *this;
}

inline HttpClient& HttpClient::header(const char* key, const char *value) {
	_headers.put(key, value);
	return *this;
}

}
