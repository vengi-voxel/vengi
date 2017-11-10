/**
 * @file
 */

#pragma once

#include <cstdint>
#include <cinttypes>

namespace backend {

#define PRIEntId "%" PRId64
typedef int64_t EntityId;
constexpr EntityId EntityIdNone = (EntityId)0;

}
