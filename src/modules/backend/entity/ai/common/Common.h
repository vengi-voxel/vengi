/**
 * @file
 */
#pragma once

#include "core/Common.h"
#include "core/Log.h"
#include <stdio.h>


#define AI_STRINGIFY_INTERNAL(x) #x
#define AI_STRINGIFY(x) AI_STRINGIFY_INTERNAL(x)

/**
 * @brief If you want to use exceptions in your code and want them to be catched by the library
 * just set this to 1
 */
#ifndef AI_EXCEPTIONS
#define AI_EXCEPTIONS 0
#endif

/**
 * @brief Enable lua sanity checks by default
 */
#ifndef AI_LUA_SANTITY
#define AI_LUA_SANTITY 1
#endif
