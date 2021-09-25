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

}
}
