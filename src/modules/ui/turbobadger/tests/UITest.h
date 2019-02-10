/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "ui/turbobadger/TurboBadger.h"
#include "ui/turbobadger/FontUtil.h"
#include "ui/turbobadger/UIDummies.h"

namespace ui {
namespace turbobadger {

class UITest: public core::AbstractTest {
protected:
	DummyRenderer _renderer;

	virtual void onCleanupApp() override {
		tb::TBAnimationManager::AbortAllAnimations();
		tb::TBWidgetsAnimationManager::Shutdown();
		tb::tb_core_shutdown();
	}

	virtual bool onInitApp() override {
		if (!tb::tb_core_init(&_renderer)) {
			Log::error("failed to initialize the ui");
			return false;
		}
		if (!tb::g_tb_lng->load("ui/lang/en.tb.txt")) {
			Log::warn("could not load the translation");
		}
		if (!tb::g_tb_skin->load("ui/skin/skin.tb.txt", nullptr)) {
			Log::error("could not load the skin");
			return false;
		}
		tb::TBWidgetsAnimationManager::Init();
		initFonts();
		return true;
	}
};

}
}
