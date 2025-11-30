/**
 * @file
 */

#include "Style.h"
#include "ui/IMGUIApp.h"

namespace style {

const glm::vec4 &color(StyleColor color) {
	ui::IMGUIApp *app = imguiApp();
	if (app) {
		return app->color(color);
	}
	return color::Color::White();
}

} // namespace style
