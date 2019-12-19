/**
 * @file
 */
#pragma once

#include "core/Log.h"

#define ai_log(...) Log::info(__VA_ARGS__)
#define ai_log_error(...) Log::error(__VA_ARGS__)
#define ai_log_warn(...) Log::warn(__VA_ARGS__)
#define ai_log_debug(...) Log::debug(__VA_ARGS__)
#define ai_log_trace(...) Log::trace(__VA_ARGS__)
