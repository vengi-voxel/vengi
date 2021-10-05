// imgui-notify by patrickcjk
//

#pragma once

#include "IMGUI.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>

#include "IconsFontAwesome5.h"
#include "core/collection/DynamicArray.h"

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;

enum ImGuiToastType_ {
	ImGuiToastType_None,
	ImGuiToastType_Debug,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Info,
	ImGuiToastType_COUNT
};

class ImGuiToast {
private:
	ImGuiToastType _type;
	core::String _message;
	uint64_t _creationTime;

public:
	ImGuiToast(ImGuiToastType type, const core::String &message);

	const char *defaultTitle() const;
	ImGuiToastType type() const;
	ImVec4 color() const;
	const char *icon() const;
	const char *content() const;
	uint32_t elapsedTime() const;
	ImGuiToastPhase phase(int dismissMillis) const;
	float fadePercent(int dismissMillis) const;
};

namespace ImGui {

/// Render toasts, call at the end of your rendering!
int RenderNotifications(core::DynamicArray<ImGuiToast>& notifications);

}
