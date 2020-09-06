/**
 * @file
 */

#pragma once

#include "HttpMethod.h"
#include "HttpParser.h"
#include "HttpQuery.h"

namespace http {

class RequestParser : public HttpParser {
private:
	using Super = HttpParser;
public:
	RequestParser(uint8_t* requestBuffer, size_t requestBufferSize);

	// arrays are not supported as query parameters - but
	// that's fine for our use case
	HttpQuery query;
	HttpMethod method = HttpMethod::NOT_SUPPORTED;
	const char* path = nullptr;

	RequestParser(RequestParser&& other) noexcept;
	RequestParser(const RequestParser& other);

	RequestParser& operator=(RequestParser&& other) noexcept;
	RequestParser& operator=(const RequestParser& other);
};

}
