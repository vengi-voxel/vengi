/**
 * @file
 */

#include "ResponseParser.h"
#include "core/String.h"
#include "core/Log.h"

namespace http {

ResponseParser& ResponseParser::operator=(ResponseParser &&other) {
	if (&other != this) {
		Super::operator=(std::move(other));
		status = other.status;
		statusText = HTTP_PARSER_NEW_BASE(other.statusText);
	}
	return *this;
}

ResponseParser::ResponseParser(ResponseParser &&other) :
		Super(std::move(other)) {
	status = other.status;
	statusText = HTTP_PARSER_NEW_BASE(other.statusText);
}

ResponseParser& ResponseParser::operator=(const ResponseParser& other) {
	if (&other != this) {
		Super::operator=(other);
		status = other.status;
		statusText = HTTP_PARSER_NEW_BASE(other.statusText);
	}
	return *this;
}

ResponseParser::ResponseParser(const ResponseParser &other) :
		Super(other) {
	status = other.status;
	statusText = HTTP_PARSER_NEW_BASE(other.statusText);
}

/**
 * https://tools.ietf.org/html/rfc2616
 *
 * @code
 * HTTP/1.0 200 OK
 * Server: SimpleHTTP/0.6 Python/2.7.17
 * Date: Tue, 10 Jan 2020 13:37:42 GMT
 * Content-type: text/html; charset=UTF-8
 * Content-Length: 940
 *
 * <!DOCTYPE html PUBLIC "-//W3C//DTD HTML 3.2 Final//EN"><html>
 * [...]
 * </html>
 * @endcode
 */
ResponseParser::ResponseParser(uint8_t* responseBuffer, size_t responseBufferSize)
		: Super(responseBuffer, responseBufferSize) {
	if (buf == nullptr || bufSize == 0) {
		return;
	}
	char *bufPos = (char *)buf;

	char *statusLine = getHeaderLine(&bufPos);
	if (statusLine == nullptr) {
		return;
	}
	protocolVersion = core::string::getBeforeToken(&statusLine, " ", remainingBufSize(statusLine));
	if (protocolVersion == nullptr) {
		return;
	}

	const char *statusNumber = core::string::getBeforeToken(&statusLine, " ", remainingBufSize(statusLine));
	if (statusNumber == nullptr) {
		return;
	}
	status = (HttpStatus)SDL_atoi(statusNumber);

	statusText = statusLine;

	if (!parseHeaders(&bufPos)) {
		Log::info("failed to parse the headers");
		return;
	}

	content = bufPos;
	contentLength = remainingBufSize(bufPos);
	Log::debug("content length: %i", (int)contentLength);

	const char *value;
	if (!headers.get(header::CONTENT_LENGTH, value)) {
		Log::info("no content-length header entry found");
		_valid = false;
	} else {
		_valid = contentLength == SDL_atoi(value);
		if (!_valid) {
			Log::debug("content-length and received data differ: %i vs %s", (int)contentLength, value);
		}
	}
}

}
