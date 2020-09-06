/**
 * @file
 */

#include "HttpParser.h"
#include "core/StringUtil.h"

namespace http {

HttpParser::HttpParser(uint8_t* buffer, const size_t bufferSize) :
		buf(buffer), bufSize(bufferSize) {
}

HttpParser& HttpParser::operator=(HttpParser&& other) {
	buf = other.buf;
	bufSize = other.bufSize;
	_valid = other._valid;
	protocolVersion = other.protocolVersion;
	headers = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.headers);
	content = other.content;
	contentLength = other.contentLength;

	other.buf = nullptr;
	other.bufSize = 0u;
	other.protocolVersion = nullptr;
	other.headers.clear();
	other.content = nullptr;
	other.contentLength = 0u;

	return *this;
}

HttpParser::HttpParser(HttpParser&& other) {
	buf = other.buf;
	bufSize = other.bufSize;
	_valid = other._valid;
	protocolVersion = other.protocolVersion;
	headers = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.headers);
	content = other.content;
	contentLength = other.contentLength;

	other.buf = nullptr;
	other.bufSize = 0u;
	other.protocolVersion = nullptr;
	other.headers.clear();
	other.content = nullptr;
	other.contentLength = 0u;
}

HttpParser& HttpParser::operator=(const HttpParser& other) {
	if (&other == this) {
		return *this;
	}
	buf = (uint8_t*)SDL_malloc(other.bufSize);
	SDL_memcpy(buf, other.buf, other.bufSize);
	bufSize = other.bufSize;
	_valid = other._valid;

	protocolVersion = HTTP_PARSER_NEW_BASE(other.protocolVersion);

	headers = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.headers);

	content = HTTP_PARSER_NEW_BASE(other.content);

	contentLength = other.contentLength;
	return *this;
}

HttpParser::HttpParser(const HttpParser& other) {
	buf = (uint8_t*)SDL_malloc(other.bufSize);
	SDL_memcpy(buf, other.buf, other.bufSize);
	bufSize = other.bufSize;
	_valid = other._valid;

	protocolVersion = HTTP_PARSER_NEW_BASE(other.protocolVersion);

	headers = HTTP_PARSER_NEW_BASE_CHARPTR_MAP(other.headers);

	content = HTTP_PARSER_NEW_BASE(other.content);

	contentLength = other.contentLength;
}

HttpParser::~HttpParser() {
	SDL_free(buf);
	buf = nullptr;
	bufSize = 0;
}

size_t HttpParser::remainingBufSize(const char *bufPos) const {
	if (bufPos < (const char *)buf) {
		return 0u;
	}
	if (bufPos >= (const char *)buf + bufSize) {
		return 0u;
	}
	const size_t alreadyRead = (size_t)((uint8_t*)bufPos - buf);
	const size_t remaining = bufSize - alreadyRead;
	return remaining;
}

char* HttpParser::getHeaderLine(char **buffer) {
	return core::string::getBeforeToken(buffer, "\r\n", remainingBufSize(*buffer));
}

bool HttpParser::parseHeaders(char **bufPos) {
	char *hdrPos = core::string::getBeforeToken(bufPos, "\r\n\r\n", remainingBufSize(*bufPos));
	if (hdrPos == nullptr) {
		return false;
	}

	// re-add one newline to simplify the code
	// for key/value parsing of the header
	const size_t hdrSize = SDL_strlen(hdrPos);
	hdrPos[hdrSize + 0] = '\r';
	hdrPos[hdrSize + 1] = '\n';
	hdrPos[hdrSize + 2] = '\0';

	for (;;) {
		char *headerEntry = core::string::getBeforeToken(&hdrPos, "\r\n", remainingBufSize(hdrPos));
		if (headerEntry == nullptr) {
			break;
		}
		const char *var = core::string::getBeforeToken(&headerEntry, ": ", remainingBufSize(headerEntry));
		const char *value = headerEntry;
		headers.put(var, value);
	}
	return true;
}

const char *HttpParser::headerValue(const char *name) const {
	const char *val = nullptr;
	headers.get(name, val);
	return val;
}

bool HttpParser::isHeaderValue(const char *name, const char *value) const {
	const char *val = nullptr;
	if (!headers.get(name, val)) {
		if (value == nullptr) {
			return val == nullptr;
		}
		return false;
	}

	return !SDL_strncmp(value, val, SDL_strlen(value));
}

}
