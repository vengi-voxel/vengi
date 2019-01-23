/**
 * @file
 */

#include "NoiseWindow.h"

#include "editorscene/ViewportSingleton.h"

namespace voxedit {

NoiseWindow::NoiseWindow(ui::turbobadger::Window* window) :
		Super(window) {
	core_assert_always(loadResourceFile("ui/window/voxedit-noise.tb.txt"));

	_octaves     = getWidgetByType<tb::TBEditField>("octaves");
	_frequency   = getWidgetByType<tb::TBEditField>("frequency");
	_lacunarity  = getWidgetByType<tb::TBEditField>("lacunarity");
	_gain        = getWidgetByType<tb::TBEditField>("gain");

	if (_octaves == nullptr || _frequency == nullptr || _lacunarity == nullptr || _gain == nullptr) {
		Log::error("Not all needed widgets were found");
		Close();
	}
}

bool NoiseWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			const int octaves = _octaves->GetValue();
			const tb::TBStr& frequencyStr = _frequency->GetText();
			const float frequency = core::string::toFloat(frequencyStr.CStr());
			const tb::TBStr& lacunarityStr = _lacunarity->GetText();
			const tb::TBStr& gainStr = _gain->GetText();
			const float lacunarity = core::string::toFloat(lacunarityStr.CStr());
			const float gain = core::string::toFloat(gainStr.CStr());
			ViewportSingleton::getInstance().noise(octaves, lacunarity, frequency, gain, voxel::noisegen::NoiseType::ridgedMF);
			Close();
			return true;
		} else if (ev.target->GetID() == TBIDC("cancel")) {
			Close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			Close();
			return true;
		}
	}
	return Super::OnEvent(ev);
}

}
