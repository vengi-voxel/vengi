/**
 * @file
 */

#pragma once

#include <cstdint>
#include <string>

namespace core {

extern std::string md5sum(const uint8_t *data, uint32_t len);

}
