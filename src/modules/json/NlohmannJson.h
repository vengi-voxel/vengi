/**
 * @file
 * @brief Provides the nlohmann::json header for code that still needs it (e.g. tiny_gltf).
 *
 * New code should use json/JSON.h (the cJSON-based wrapper) instead.
 */

#pragma once

#include "core/Assert.h"

#define JSON_THROW_USER(exception) core_assert_msg(false, "%s", exception.what()); std::abort()
#define JSON_NO_IO
#include "private/json.hpp"
