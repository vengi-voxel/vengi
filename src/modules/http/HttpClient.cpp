/**
 * @file
 */

#include "HttpClient.h"
#include "Request.h"
#include "core/Log.h"

namespace http {

HttpClient::HttpClient(const core::String &baseUrl) : _baseUrl(baseUrl) {
}

bool HttpClient::setBaseUrl(const core::String &baseUrl) {
	Url u(baseUrl.c_str());
	_baseUrl = baseUrl;
	return u.valid();
}

ResponseParser HttpClient::get(const char *msg, ...) {
	va_list ap;
	constexpr std::size_t bufSize = 2048;
	char text[bufSize];

	va_start(ap, msg);
	SDL_snprintf(text, bufSize, "%s", _baseUrl.c_str());
	SDL_vsnprintf(text + _baseUrl.size(), bufSize - _baseUrl.size(), msg, ap);
	text[sizeof(text) - 1] = '\0';
	va_end(ap);

	Url u(text);
	if (!u.valid()) {
		Log::error("Invalid url given: '%s'", text);
		return ResponseParser(nullptr, 0u);
	}
	Request request(u, HttpMethod::GET);
	return request.execute();
}

}
