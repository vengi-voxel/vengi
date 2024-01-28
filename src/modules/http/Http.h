/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Stream.h"

namespace http {

bool download(const core::String &url, io::WriteStream &stream, int *statusCode = nullptr);

}
