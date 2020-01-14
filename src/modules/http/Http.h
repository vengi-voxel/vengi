/**
 * @file
 */

#pragma once

#include "ResponseParser.h"

namespace http {

ResponseParser GET(const char *url);
ResponseParser POST(const char *url, const char *body = nullptr);

}
