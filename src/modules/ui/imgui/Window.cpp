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

void Window::update(double deltaFrameSeconds) {
	if (isFullscreen()) {
		ImGui::Fullscreen(_title.c_str());
	} else {
		ImGuiWindowFlags flags = 0;
		if ((_flags & WindowFlag::NoTitle) == WindowFlag::NoTitle) {
			flags |= ImGuiWindowFlags_NoTitleBar;
		}
		if ((_flags & WindowFlag::Modal) == WindowFlag::Modal) {
			flags |= ImGuiWindowFlags_Modal;
		}
		if ((_flags & WindowFlag::Modal) == WindowFlag::FixedPosition) {
			flags |= ImGuiWindowFlags_NoMove;
		}
		ImGui::Begin(_title.c_str(), nullptr, flags);
	}
	render(deltaFrameSeconds);
	ImGui::End();
}

} // namespace imgui
} // namespace ui
