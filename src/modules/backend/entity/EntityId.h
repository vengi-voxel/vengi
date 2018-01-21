/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <inttypes.h>

namespace backend {

#define PRIEntId "%" PRId64
typedef int64_t EntityId;
constexpr EntityId EntityIdNone = (EntityId)0;

}
