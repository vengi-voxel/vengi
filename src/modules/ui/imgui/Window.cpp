/**
 * @file
 */

#include "Window.h"
#include "IMGUI.h"
#include "command/Command.h"
#include "dearimgui/imgui.h"

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
		ImGuiWindowFlags flags = ImGuiWindowFlags_None;
		if ((_flags & WindowFlag::NoTitle) == WindowFlag::NoTitle) {
			flags |= ImGuiWindowFlags_NoTitleBar;
		}
		if ((_flags & WindowFlag::Modal) == WindowFlag::Modal) {
			flags |= ImGuiWindowFlags_Modal;
		}
		if ((_flags & WindowFlag::FixedPosition) == WindowFlag::FixedPosition) {
			flags |= ImGuiWindowFlags_NoMove;
		}
		if ((_flags & WindowFlag::NoBackground) == WindowFlag::NoBackground) {
			flags |= ImGuiWindowFlags_NoBackground;
		}
		if ((_flags & WindowFlag::FixedSize) == WindowFlag::FixedSize) {
			flags |= ImGuiWindowFlags_NoResize;
		}
		if ((_flags & WindowFlag::NoCollapse) == WindowFlag::NoCollapse) {
			flags |= ImGuiWindowFlags_NoCollapse;
		}
		if ((_flags & WindowFlag::Centered) == WindowFlag::Centered) {
			ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}
		ImGui::Begin(_title.c_str(), nullptr, flags);
	}
	render(deltaFrameSeconds);
	ImGui::End();
}

} // namespace imgui
} // namespace ui
