/**
 * @file
 */

#include "NoiseWindow.h"
#include "core/String.h"
#include "voxedit-util/SceneManager.h"

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
		close();
	}
}

bool NoiseWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->getID() == TBIDC("ok")) {
			const int octaves = _octaves->getValue();
			const tb::TBStr& frequencyStr = _frequency->getText();
			const float frequency = core::string::toFloat(frequencyStr.c_str());
			const tb::TBStr& lacunarityStr = _lacunarity->getText();
			const tb::TBStr& gainStr = _gain->getText();
			const float lacunarity = core::string::toFloat(lacunarityStr.c_str());
			const float gain = core::string::toFloat(gainStr.c_str());
			sceneMgr().noise(octaves, lacunarity, frequency, gain, voxelgenerator::noise::NoiseType::ridgedMF);
			close();
			return true;
		} else if (ev.target->getID() == TBIDC("cancel")) {
			close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_ESC) {
			close();
			return true;
		}
	}
	return Super::onEvent(ev);
}

}
