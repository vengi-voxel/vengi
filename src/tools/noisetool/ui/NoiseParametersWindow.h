/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "noise/SimplexNoise.h"

class NoiseParametersWindow: public ui::Window {
private:
	void make2DNoise(bool append, bool gray, float amplitude, float frequency, int octaves, float persistence) {
		tb::TBStr idStr;
		idStr.SetFormatted("%i-%f-%f-%i-%f", gray ? 1 : 0, amplitude, frequency, octaves, persistence);
		tb::TBBitmapFragment *existingFragment = tb::g_tb_skin->GetFragmentManager()->GetFragment(tb::TBID(idStr.CStr()));
		if (existingFragment != nullptr) {
			return;
		}
		const int width = 512;
		const int height = 512;
		const int components = 4;
		uint8_t buffer[width * height * components];
		if (gray) {
			noise::Simplex::Noise2DGrayA(buffer, width, height, octaves, persistence, frequency, amplitude);
		} else {
			noise::Simplex::Noise2DRGBA(buffer, width, height, octaves, persistence, frequency, amplitude);
		}
		tb::TBLayout* layout = GetWidgetByIDAndType<tb::TBLayout>("imagelayout");
		if (layout == nullptr) {
			Log::error("could not find layout node");
			return;
		}
		if (!append) {
			layout->DeleteAllChildren();
		}
		tb::TBImageWidget* imageWidget = new tb::TBImageWidget();
		tb::TBTextField* caption = new tb::TBTextField();
		caption->SetText(idStr.CStr());
		caption->SetGravity(tb::WIDGET_GRAVITY_BOTTOM | tb::WIDGET_GRAVITY_LEFT_RIGHT);
		imageWidget->AddChild(caption);
		const tb::TBImage& image = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)buffer, width, height);
		imageWidget->SetImage(image);
		layout->AddChild(imageWidget);
		Log::info("a: %f, f: %f, o: %i, p: %f", amplitude, frequency, octaves, persistence);
	}

public:
	NoiseParametersWindow(NoiseTool* tool) :
			ui::Window(tool) {
		core_assert(loadResourceFile("ui/window/noiseparameters.tb.txt"));
	}

	bool OnEvent(const tb::TBWidgetEvent &ev) override {
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("ok")) || ev.special_key == tb::TB_KEY_ENTER) {
			const float amplitude = getFloat("amplitude");
			const float frequency = getFloat("frequency");
			const bool enableoctaves = isToggled("enableoctaves");
			const bool gray = isToggled("gray");
			const bool append = isToggled("append");
			const int octaves = enableoctaves ? getInt("octaves") : 1;
			const float persistence = enableoctaves ? getFloat("persistence") : 1.0f;
			make2DNoise(append, gray, amplitude, frequency, octaves, persistence);
			return true;
		}
		return ui::Window::OnEvent(ev);
	}
};
