/**
 * @file
 */

#include "Style.h"
#include "ui/IMGUIApp.h"
#include "math/Vec4f.h"

namespace style {

const math::Vec4f &color(StyleColor color) {
	ui::IMGUIApp *app = imguiApp();
	if (app) {
		return app->color(color);
	}
	return color::White();
}

} // namespace style
