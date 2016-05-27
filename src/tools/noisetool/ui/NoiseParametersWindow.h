/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"
#include "noise/SimplexNoise.h"

class NoiseParametersWindow: public ui::Window {
private:
	void make3DNoise(bool append, float amplitude, float frequency, int octaves, float persistence) {
		tb::TBStr idStr;
		idStr.SetFormatted("3d-%f-%f-%i-%f", amplitude, frequency, octaves, persistence);
		cleanup(idStr);
		const int height = 768;
		const int width = 1024;
		const int components = 4;
		uint8_t buffer[width * height * components];
		const glm::vec3 pos = {0, 0, 0};
		noise::Simplex::Noise3D(pos, octaves, persistence, frequency, amplitude);
		addImage(idStr, append, buffer, width, height);
	}

	void make2DNoise(bool append, bool gray, bool seamless, bool alpha, float amplitude, float frequency, int octaves, float persistence) {
		tb::TBStr idStr;
		idStr.SetFormatted("2d-%i-%i-%i-%f-%f-%i-%f", gray ? 1 : 0, seamless ? 1 : 0, alpha ? 1 : 0, amplitude, frequency, octaves, persistence);
		cleanup(idStr);
		const int height = 768;
		const int width = seamless ? height : 1024;
		core_assert(!seamless || width == height);
		const int components = 4;
		uint8_t buffer[width * height * components];
		if (gray) {
			if (seamless) {
				noise::Simplex::SeamlessNoise2DGrayA(buffer, width, octaves, persistence, frequency, amplitude);
			} else {
				noise::Simplex::Noise2DGrayA(buffer, width, height, octaves, persistence, frequency, amplitude);
			}
		} else {
			if (seamless) {
				noise::Simplex::SeamlessNoise2DRGBA(buffer, width, octaves, persistence, frequency, amplitude);
			} else {
				noise::Simplex::Noise2DRGBA(buffer, width, height, octaves, persistence, frequency, amplitude);
			}
		}
		if (!alpha) {
			for (int i = components - 1; i < width * height * components; i += components) {
				buffer[i] = 255;
			}
		}
		addImage(idStr, append, buffer, width, height);
	}

	void cleanup(const tb::TBStr& idStr) {
		tb::TBBitmapFragment *existingFragment = tb::g_tb_skin->GetFragmentManager()->GetFragment(tb::TBID(idStr.CStr()));
		if (existingFragment != nullptr) {
			tb::g_tb_skin->GetFragmentManager()->FreeFragment(existingFragment);
		}
	}

	void addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height) {
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
		caption->SetSkinBg(tb::TBID("image_caption"));
		imageWidget->AddChild(caption, tb::WIDGET_Z_BOTTOM);
		imageWidget->OnInflateChild(caption);

		tb::TBButton* removeButton = new tb::TBButton();
		removeButton->SetID(tb::TBID("remove"));
		removeButton->SetSkinBg(tb::TBID("button_remove"));
		removeButton->SetGravity(tb::WIDGET_GRAVITY_RIGHT);
		imageWidget->AddChild(removeButton, tb::WIDGET_Z_BOTTOM);
		imageWidget->OnInflateChild(removeButton);

		const tb::TBImage& image = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)buffer, width, height);
		imageWidget->SetImage(image);
		layout->AddChild(imageWidget, tb::WIDGET_Z_TOP);
		layout->OnInflateChild(imageWidget);
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
			const bool alpha = isToggled("alpha");
			const bool seamless = isToggled("seamless");
			const bool threedimensional = isToggled("threedimensional");
			const int octaves = enableoctaves ? getInt("octaves") : 1;
			const float persistence = enableoctaves ? getFloat("persistence") : 1.0f;
			Log::info("seamless: %i, gray: %i, amplitude: %f, freq: %f, oct: %i, persist: %f",
					seamless ? 1 : 0, gray ? 1 : 0, amplitude, frequency, octaves, persistence);
			if (threedimensional) {
				make3DNoise(append, amplitude, frequency, octaves, persistence);
			} else {
				make2DNoise(append, gray, seamless, alpha, amplitude, frequency, octaves, persistence);
			}
			return true;
		}
		if ((ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("remove")) || ev.special_key == tb::TB_KEY_DELETE) {
			TBWidget *image = ev.target->GetParent();
			image->GetParent()->RemoveChild(image);
			delete image;
			return true;
		}
		return ui::Window::OnEvent(ev);
	}

	void OnDie() override {
		ui::Window::OnDie();
		_app->requestQuit();
	}
};
