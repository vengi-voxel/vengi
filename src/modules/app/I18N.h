/**
 * @file
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
#define _(x) x
#define C_(ctx, x) x
#else
#include "App.h"
#define _(x) app::App::getInstance()->translate(x)
#define C_(ctx, x) app::App::getInstance()->translateCtxt(ctx, x)
#endif
// no-op translation marker that still could be used for string extraction
#define N_(x) x
#define NC_(ctx, x) x
