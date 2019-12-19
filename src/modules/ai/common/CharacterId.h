/**
 * @file
 */

#pragma once

#include <inttypes.h>

namespace ai {

/**
 * @brief Defines the type of the id to identify an ai controlled entity.
 *
 * @note @c -1 is reserved. You should use ids >= 0
 * @sa NOTHING_SELECTED
 */
typedef int CharacterId;
#define PRIChrId PRId32

}
