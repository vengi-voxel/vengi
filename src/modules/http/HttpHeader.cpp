/**
 * @file
 */

#include "HttpHeader.h"

namespace http {

bool buildHeaderBuffer(char *headers, size_t len, const HeaderMap& _headers) {
	char *headersP = headers;
	size_t headersSize = len;
	for (const auto& h : _headers) {
		const size_t written = SDL_snprintf(headersP, headersSize, "%s: %s\r\n", h->key, h->value);
		if (written >= headersSize) {
			return false;
		}
		headersSize -= written;
		headersP += written;
	}
	return true;
}

}
