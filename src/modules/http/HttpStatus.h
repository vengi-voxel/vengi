/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace http {

enum class HttpStatus : uint16_t {
	Unknown = 0,
	Ok = 200,
	Created = 201,
	Accepted = 202,
	BadRequest = 400,
	Unauthorized = 401,
	Forbidden = 403,
	NotFound = 404,
	RequestUriTooLong = 414,
	InternalServerError = 500,
	NotImplemented = 501,
	BadGateway = 502,
	ServiceUnavailable = 503,
	GatewayTimeout = 504,
	HttpVersionNotSupported = 505,
};

extern const char* toStatusString(HttpStatus status);

}
