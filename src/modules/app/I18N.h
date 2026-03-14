/**
 * @file
 */

#pragma once

#include "I18NMarkers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#define _(x) x
#define C_(ctx, x) x
#else
#include "App.h"
#define _(x) app::App::getInstance()->translate(x)
#define C_(ctx, x) app::App::getInstance()->translateCtxt(ctx, x)
#endif
