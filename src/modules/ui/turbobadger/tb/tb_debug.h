/**
 * @file
 */

#pragma once

#include "tb_types.h"

namespace tb {

#ifdef TB_RUNTIME_DEBUG_INFO

class TBDebugInfo
{
public:
	TBDebugInfo();

	enum SETTING {
		/** Show widgets bounds */
		LAYOUT_BOUNDS,
		/** Show child widget clipping set by some widgets. */
		LAYOUT_CLIPPING,
		/** Show highlights on widgets that recalculate their preferred
			size, and those who recalculate their layout. */
		LAYOUT_PS_DEBUGGING,
		/** Show render batch info and log batch info in the debug
			output. It depends on the renderer backend if this is available. */
		RENDER_BATCHES,
		/** Render the bitmap fragments of the skin. */
		RENDER_SKIN_BITMAP_FRAGMENTS,
		/** Render the bitmap fragments of the font that's set on the hovered
			or focused widget. */
		RENDER_FONT_BITMAP_FRAGMENTS,

		NUM_SETTINGS
	};
	int settings[NUM_SETTINGS];
};

extern TBDebugInfo g_tb_debug;

/** Show a window containing runtime debugging settings. */
void ShowDebugInfoSettingsWindow(class TBWidget *root);

#define TB_DEBUG_SETTING(setting) g_tb_debug.settings[TBDebugInfo::setting]
#define TB_IF_DEBUG_SETTING(setting, code) if (TB_DEBUG_SETTING(setting)) { code; }

#else // TB_RUNTIME_DEBUG_INFO

/** Show a window containing runtime debugging settings. */
#define ShowDebugInfoSettingsWindow(root) ((void)0)

#define TB_DEBUG_SETTING(setting) false
#define TB_IF_DEBUG_SETTING(setting, code) 

#endif // TB_RUNTIME_DEBUG_INFO

} // namespace tb
