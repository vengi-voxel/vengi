/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "ui/TurboBadger.h"
#include "ui/FontUtil.h"
#include "ui/UIDummies.h"

namespace ui {

class UITest: public core::AbstractTest {
protected:
	DummyRenderer _renderer;

	virtual bool onInitApp() override {
		if (!tb::tb_core_init(&_renderer)) {
			Log::error("failed to initialize the ui");
			return false;
		}
		if (!tb::g_tb_lng->Load("ui/lang/en.tb.txt")) {
			Log::warn("could not load the translation");
		}
		if (!tb::g_tb_skin->Load("ui/skin/skin.tb.txt", nullptr)) {
			Log::error("could not load the skin");
			return false;
		}
		tb::TBWidgetsAnimationManager::Init();
		initFonts();
		return true;
	}
};

}
