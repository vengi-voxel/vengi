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
	NotFound = 404,

	Unknown = -1
};

}

}
