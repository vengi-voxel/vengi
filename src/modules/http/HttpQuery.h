/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "core/Common.h"
#include "core/Log.h"

namespace http {

using HttpQuery = core::CharPointerMap;

#define HTTP_QUERY_GET_INT(name) \
	const char *name##value; \
	if (!request.query.get(CORE_STRINGIFY(name), name##value)) { \
		Log::debug("Missing query parameter " #name); \
		response->status = http::HttpStatus::InternalServerError; \
		response->setText("Missing parameter " #name); \
		return; \
	} \
	int name = SDL_atoi(name##value)

}
