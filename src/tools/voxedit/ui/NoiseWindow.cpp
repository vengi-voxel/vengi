/**
 * @file
 */

#include "NoiseWindow.h"
#include "editorscene/EditorScene.h"

namespace voxedit {

NoiseWindow::NoiseWindow(ui::Window* window, EditorScene* scene) :
		Super(window), _scene(scene) {
	core_assert_always(loadResourceFile("ui/window/voxedit-noise.tb.txt"));

	_octaves     = getWidgetByType<tb::TBInlineSelect>("octaves");
	_frequency   = getWidgetByType<tb::TBEditField>("frequency");
	_persistence = getWidgetByType<tb::TBEditField>("persistence");
	_amplitude   = getWidgetByType<tb::TBEditField>("amplitude");

	if (_octaves == nullptr || _frequency == nullptr || _persistence == nullptr || _amplitude == nullptr) {
		Log::error("Not all needed widgets were found");
		Close();
	}
}

bool NoiseWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("ok")) {
			const int octaves = _octaves->GetValue();
			const tb::TBStr& freq = _frequency->GetText();
			const float frequency = core::string::toFloat(freq.CStr());
			const tb::TBStr& pers = _persistence->GetText();
			const tb::TBStr& ampl = _amplitude->GetText();
			const float persistence = core::string::toFloat(pers.CStr());
			const float amplitude = core::string::toFloat(ampl.CStr());
			_scene->noise(octaves, persistence, frequency, amplitude);
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
