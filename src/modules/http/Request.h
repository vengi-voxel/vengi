/**
 * @file
 */

#pragma once

#include "ResponseParser.h"
#include "HttpHeader.h"
#include "HttpMethod.h"
#include "Url.h"
#include "Network.h"
#include "core/String.h"

namespace http {

/**
 * @note The pointers that are given for the headers must stay valid until execute() was called.
 */
class Request {
private:
	const Url _url;
	SOCKET _socketFD;
	HttpMethod _method;
	HeaderMap _headers;
	const char *_body = "";
	ResponseParser failed();
public:
	Request(const Url& url, HttpMethod method);
	Request& contentType(const char* mimeType);
	Request& accept(const char* mimeType);
	Request& header(const char* key, const char *value);
	Request& body(const char *body);
	ResponseParser execute();
};

inline Request& Request::body(const char *body) {
	_body = body;
	return *this;
}

inline Request& Request::contentType(const char* mimeType) {
	header(header::CONTENT_TYPE, mimeType);
	return *this;
}

inline Request& Request::accept(const char* mimeType) {
	header(header::ACCEPT, mimeType);
	return *this;
}

inline Request& Request::header(const char* key, const char *value) {
	_headers.put(key, value);
	return *this;
}

}
