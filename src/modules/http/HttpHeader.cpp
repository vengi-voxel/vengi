/**
 * @file
 */

#include "HttpHeader.h"
#include "SDL_stdinc.h"

namespace http {

bool buildHeaderBuffer(char *headers, size_t len, const HeaderMap& _headers) {
	char *headersP = headers;
	size_t headersSize = len;
	const char *charset = nullptr;
	const char *contentType = nullptr;
	if (_headers.get(header::CHARSET, charset) && _headers.get(header::CONTENT_TYPE, contentType)) {
		const size_t written = SDL_snprintf(headersP, headersSize, "%s: %s;%s=%s\r\n",
				header::CONTENT_TYPE, contentType, header::CHARSET, charset);
		if (written >= headersSize) {
			return false;
		}
		headersSize -= written;
		headersP += written;

	}
	for (const auto& h : _headers) {
		if (charset && contentType) {
			if (!SDL_strcmp(header::CHARSET, h->key) || !SDL_strcmp(header::CONTENT_TYPE, h->key)) {
				// already added above
				continue;
			}
		}
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
