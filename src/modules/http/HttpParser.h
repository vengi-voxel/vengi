/**
 * @file
 */

#pragma once

#include "HttpHeader.h"
#include <stdint.h>

#define HTTP_PARSER_NEW_BASE(ptr) newBase(buf, other.buf, ptr)

#define HTTP_PARSER_NEW_BASE_CHARPTR_MAP(map) newBase(buf, other.buf, map)

namespace http {

/**
 * @brief Base class for http response/request protocol messages.
 *
 * @note It does not copy any string, it modifies the input buffer string so make sure to hand in copies.
 * The stored data are just pointers to the modified input buffer.
 */
class HttpParser {
protected:
	static inline const char* newBase(const void* newBufPtr, const void* oldBufPtr, const void* oldPtr) {
		if (oldPtr == nullptr || oldBufPtr == nullptr) {
			return nullptr;
		}
		return (const char*)newBufPtr + (intptr_t)((const uint8_t*)oldPtr - (const uint8_t*)oldBufPtr);
	}
	static inline auto newBase(const void* newBufPtr, const void* oldBufPtr, const core::CharPointerMap& map) {
		core::CharPointerMap newMap(map.size());
		if (oldBufPtr == nullptr) {
			return newMap;
		}
		for (auto i = map.begin(); i != map.end(); ++i) {
			newMap.put(newBase(newBufPtr, oldBufPtr, i->key), newBase(newBufPtr, oldBufPtr, i->value));
		}
		return newMap;
	}
	uint8_t *buf = nullptr;
	size_t bufSize = 0u;
	bool _valid = false;

	size_t remainingBufSize(const char *bufPos) const;
	char* getHeaderLine(char **buffer);
	bool parseHeaders(char **bufPos);

public:
	/**
	 * @brief Parses a http response/request buffer
	 * @note The given memory is owned by this class. You may not
	 * release it on your own.
	 */
	HttpParser(uint8_t* buffer, const size_t bufferSize);

	/**
	 * @brief Pointer to that part of the protocol header that stores
	 * the http version
	 */
	const char* protocolVersion = nullptr;
	/**
	 * @brief The map key/value pairs are just pointers to the
	 * protocol header buffer. It's safe to copy this structure, but
	 * don't manually modify the @c headers map
	 */
	HeaderMap headers;
	/**
	 * @brief The pointer to the data after the protocol header
	 */
	const char *content = nullptr;
	/**
	 * @brief The length of the @c content buffer
	 */
	int contentLength = -1;

	bool valid() const;

	HttpParser(HttpParser&& other) noexcept;
	HttpParser(const HttpParser& other);
	~HttpParser();

	const char *headerValue(const char *name) const;
	bool isHeaderValue(const char *name, const char *value) const;

	HttpParser& operator=(HttpParser&& other) noexcept;
	HttpParser& operator=(const HttpParser& other);
};

inline bool HttpParser::valid() const {
	return _valid;
}

}
