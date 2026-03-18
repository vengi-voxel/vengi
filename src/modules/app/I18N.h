/**
 * @file
 */

#pragma once

#include "I18NMarkers.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#define _(x) x
#define C_(ctx, x) x
#else

namespace app {
const char *translate(const char *msgid);
const char *translateCtxt(const char *msgctxt, const char *msgid);
}

#define _(x) app::translate(x)
#define C_(ctx, x) app::translateCtxt(ctx, x)
#endif
