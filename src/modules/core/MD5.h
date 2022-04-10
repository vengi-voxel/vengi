/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/String.h"

namespace core {

extern core::String md5ToString(uint8_t digest[16]);
extern core::String md5sum(const uint8_t *buf, uint32_t len);
extern void md5sum(const uint8_t *buf, uint32_t len, uint8_t digest[16]);

}
