/**
 * @file
 */

#pragma once

#include "HttpStatus.h"
#include "HttpHeader.h"
#include "HttpParser.h"
#include <stdint.h>

namespace http {

class ResponseParser : public HttpParser {
private:
	using Super = HttpParser;
public:
	HttpStatus status = HttpStatus::Unknown;
	const char* statusText = nullptr;

	ResponseParser(uint8_t* responseBuffer, size_t responseBufferSize);

	ResponseParser(ResponseParser&& other) noexcept;
	ResponseParser(const ResponseParser& other);

	ResponseParser& operator=(ResponseParser&& other) noexcept;
	ResponseParser& operator=(const ResponseParser& other);
};

}
