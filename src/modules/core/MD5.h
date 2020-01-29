/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/String.h"

namespace core {

extern core::String md5sum(const uint8_t *buf, uint32_t len);

}
