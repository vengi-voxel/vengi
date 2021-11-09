/**
 * @file
 */

#include "Window.h"
#include "IMGUI.h"
#include "command/Command.h"

namespace ui {
namespace imgui {

void Window::close() {
	command::Command::execute("ui_pop");
}

void Window::open(const core::String &name) {
	command::Command::execute("ui_push " + name);
}

void Window::update() {
	if (isFullscreen()) {
		ImGui::Fullscreen(_title.c_str());
	} else {
		ImGui::Begin(_title.c_str());
	}
	render();
	ImGui::End();
}

} // namespace imgui
} // namespace ui
