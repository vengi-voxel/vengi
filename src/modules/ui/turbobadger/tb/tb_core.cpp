/**
 * @file
 */

#include "tb_core.h"
#include "animation/tb_animation.h"
#include "image/tb_image_manager.h"
#include "tb_font_renderer.h"
#include "tb_language.h"
#include "tb_skin.h"
#include "tb_system.h"
#include "tb_widgets_reader.h"

namespace tb {

TBRenderer *g_renderer = nullptr;
TBSkin *g_tb_skin = nullptr;
TBWidgetsReader *g_widgets_reader = nullptr;
TBLanguage *g_tb_lng = nullptr;
TBFontManager *g_font_manager = nullptr;

bool tb_core_init(TBRenderer *renderer) {
	Log::debug("Initiating Turbo Badger - version " TB_VERSION_STR);
	g_renderer = renderer;
	g_tb_lng = new TBLanguage;
	g_font_manager = new TBFontManager();
	g_tb_skin = new TBSkin();
	g_widgets_reader = TBWidgetsReader::create();
	g_image_manager = new TBImageManager();
	return true;
}

void tb_core_shutdown() {
	TBAnimationManager::abortAllAnimations();
	delete g_image_manager;
	delete g_widgets_reader;
	delete g_tb_skin;
	delete g_font_manager;
	delete g_tb_lng;
}

bool tb_core_is_initialized() {
	return g_widgets_reader != nullptr;
}

} // namespace tb
