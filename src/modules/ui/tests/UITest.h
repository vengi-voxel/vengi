/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "ui/TurboBadger.h"
#include "ui/FontUtil.h"

namespace ui {

class UITest: public core::AbstractTest {
protected:
	class DummyBitmap: public tb::TBBitmap {
	public:
		bool Init(int width, int height, uint32_t *data) {
			m_w = width;
			m_h = height;
			SetData(data);
			return true;
		}

		virtual int Width() override {
			return m_w;
		}
		virtual int Height() override {
			return m_h;
		}
		virtual void SetData(uint32_t *data) override {
		}
	public:
		int m_w = 0, m_h = 0;
	};

	class DummyRenderer : public tb::TBRendererBatcher {
	private:
		tb::TBRect _clipRect;
	public:
		void BeginPaint(int, int) override {}
		void EndPaint() override {}

		tb::TBBitmap *CreateBitmap(int width, int height, uint32_t* data) override {
			DummyBitmap *bitmap = new DummyBitmap();
			if (!bitmap || !bitmap->Init(width, height, data)) {
				delete bitmap;
				return nullptr;
			}
			return bitmap;
		}

		void RenderBatch(Batch*) override {}
		void SetClipRect(const tb::TBRect&) override {}
	};

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
