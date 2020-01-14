/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"

namespace http {

using HeaderMap = core::CharPointerMap;

namespace header {

static constexpr const char *CONTENT_ENCODING = "Content-Encoding";
static constexpr const char *ACCEPT_ENCODING = "Accept-Encoding";
static constexpr const char *USER_AGENT = "User-agent";
static constexpr const char *CONNECTION = "Connection";
static constexpr const char *KEEP_ALIVE = "Keep-Alive";
static constexpr const char *CONTENT_TYPE = "Content-Type";
static constexpr const char *ACCEPT = "Accept";
static constexpr const char *SERVER = "Server";
static constexpr const char *HOST = "Host";
static constexpr const char *CONTENT_LENGTH = "Content-length";
}

extern bool buildHeaderBuffer(char *buf, size_t len, const HeaderMap& headers);

}
