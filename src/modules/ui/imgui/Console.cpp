#include "Console.h"
#include "IMGUI.h"
#include "IMGUIApp.h"
#include "core/Color.h"

namespace ui {
namespace imgui {

Console::Console() :
		Super() {
}

void Console::addLogLine(int category, SDL_LogPriority priority, const char *message) {
	Super::addLogLine(category, priority, message);
	if (priority < SDL_LOG_PRIORITY_WARN) {
		return;
	}

	int toastType;
	switch (priority) {
	case SDL_LOG_PRIORITY_VERBOSE:
	case SDL_LOG_PRIORITY_DEBUG:
		toastType = ImGuiToastType_Debug;
		break;
	case SDL_LOG_PRIORITY_WARN:
		toastType = ImGuiToastType_Warning;
		break;
	case SDL_LOG_PRIORITY_ERROR:
	case SDL_LOG_PRIORITY_CRITICAL:
		toastType = ImGuiToastType_Error;
		break;
	case SDL_LOG_PRIORITY_INFO:
		toastType = ImGuiToastType_Info;
		break;
	default:
		return;
	}
	_notifications.emplace_back(toastType, message);
}

void Console::drawString(int x, int y, const glm::ivec4& color, int, const char* str, int len) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddText(ImVec2(x, y), IM_COL32(color.r, color.g, color.b, color.a), str);
}

void Console::afterRender(const math::Rect<int> &rect) {
	ImGui::End();
}

void Console::beforeRender(const math::Rect<int> &rect) {
	ImGui::SetNextWindowSize(ImVec2(rect.getMaxX(), rect.getMaxZ()), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("in-game-console", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
}

int Console::lineHeight() {
	return GImGui->FontSize + 0.5f;
}

glm::ivec2 Console::stringSize(const char* s, int length) {
	const ImVec2& v = ImGui::CalcTextSize(s);
	return glm::ivec2(v.x + 0.5f, v.y + 0.5f);
}

void Console::renderNotifications() {
	ImGui::RenderNotifications(_notifications);
}

}
}
