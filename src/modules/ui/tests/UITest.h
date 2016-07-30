/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "ui/TurboBadger.h"
#include "ui/FontUtil.h"

namespace ui {

class UITest: public core::AbstractTest {
protected:
	class DummyRenderer : public tb::TBRendererBatcher {
	private:
		tb::TBRect _clipRect;
	public:
		void BeginPaint(int, int) {}
		void EndPaint() {}

		tb::TBBitmap *CreateBitmap(int, int, uint32_t*) { return nullptr; }

		void RenderBatch(Batch*) {}
		void SetClipRect(const tb::TBRect&) {}
	};

	DummyRenderer _renderer;

	virtual bool onInitApp() override {
		if (!tb::tb_core_init(&_renderer)) {
			Log::error("failed to initialize the ui");
			return false;
		}
		tb::g_tb_lng->Load("ui/lang/en.tb.txt");
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
