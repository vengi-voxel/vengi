/**
 * @file
 */

#pragma once

#include "tb_debug.h"
#include "tb_hash.h"
#include "tb_types.h"

#define TB_VERSION_MAJOR 0
#define TB_VERSION_MINOR 1
#define TB_VERSION_REVISION 1
#define TB_VERSION_STR "0.1.1"

namespace tb {

class TBRenderer;
class TBSkin;
class TBWidgetsReader;
class TBLanguage;
class TBFontManager;

extern TBRenderer *g_renderer;
extern TBSkin *g_tb_skin;
extern TBWidgetsReader *g_widgets_reader;
extern TBLanguage *g_tb_lng;
extern TBFontManager *g_font_manager;

/** Initialize turbo badger. Call this before using any turbo badger API. */
bool tb_core_init(TBRenderer *renderer);

/** Shutdown turbo badger. Call this after deleting the last widget, to free turbo badger internals. */
void tb_core_shutdown();

/** Returns true if turbo badger is initialized. */
bool tb_core_is_initialized();

} // namespace tb
