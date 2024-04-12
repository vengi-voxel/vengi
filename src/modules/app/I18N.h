/**
 * @file
 */

#pragma once

#ifdef IMGUI_ENABLE_TEST_ENGINE
#define _(x) x
#else
#include "App.h"
#define _(x) app::App::getInstance()->translate(x)
#endif
