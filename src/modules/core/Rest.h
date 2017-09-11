/**
 * @file
 */

#pragma once

#include "core/JSON.h"
#include <restclient-cpp/connection.h>

namespace core {

namespace rest {
using Connection = ::RestClient::Connection;
using Response = ::RestClient::Response;

core::rest::Response post(const std::string& url, const core::json& json = {});

enum StatusCode {
	OK = 200,
	Created = 201,
	Accepted = 202,
	BadRequest = 400,
	Unauthorized = 401,
	Forbidden = 403,
	NotFound = 404,
	InternalServerError = 500,
	NotImplemented = 501,
	BadGateway = 502,
	ServiceUnavailable = 503,
	GatewayTimeout = 504,
	HttpVersionNotSupported = 505,

	Unknown = -1
};

}

}
