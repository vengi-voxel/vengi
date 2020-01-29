/**
 * @file
 */

#pragma once

#include "ResponseParser.h"
#include <SDL_stdinc.h>
#include "core/String.h"

namespace http {

class HttpClient {
private:
	std::string _baseUrl;
public:
	HttpClient(const std::string &baseUrl = "");

	/**
	 * @brief Change the base url for the http client that is put in front of every request
	 * @return @c false if the given base url is not a valid url
	 */
	bool setBaseUrl(const std::string &baseUrl);

	ResponseParser get(SDL_PRINTF_FORMAT_STRING const char *msg, ...) SDL_PRINTF_VARARG_FUNC(2);
};

}