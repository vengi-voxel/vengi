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

#define NOTIFY_MAX_MSG_LENGTH 4096	  // Max message content length
#define NOTIFY_PADDING_X 20.0f		  // Bottom-left X padding
#define NOTIFY_PADDING_Y 20.0f		  // Bottom-left Y padding
#define NOTIFY_PADDING_MESSAGE_Y 10.0f // Padding Y between each message
#define NOTIFY_FADE_IN_OUT_TIME 500u	  // Fade in and out duration

// Auto dismiss after X ms (default, applied only of no data provided in constructors)
#define NOTIFY_DEFAULT_DISMISS 3000u

#define NOTIFY_OPACITY 1.0f
#define NOTIFY_TOAST_FLAGS                                                                                             \
	ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |                    \
		ImGuiWindowFlags_NoNav

#define NOTIFY_NULL_OR_EMPTY(str) (!str || !strlen(str))

typedef int ImGuiToastType;
typedef int ImGuiToastPhase;
typedef int ImGuiToastPos;

enum ImGuiToastType_ {
	ImGuiToastType_None,
	ImGuiToastType_Debug,
	ImGuiToastType_Warning,
	ImGuiToastType_Error,
	ImGuiToastType_Info,
	ImGuiToastType_COUNT
};

enum ImGuiToastPhase_ {
	ImGuiToastPhase_FadeIn,
	ImGuiToastPhase_Wait,
	ImGuiToastPhase_FadeOut,
	ImGuiToastPhase_Expired,
	ImGuiToastPhase_COUNT
};

enum ImGuiToastPos_ {
	ImGuiToastPos_TopLeft,
	ImGuiToastPos_TopCenter,
	ImGuiToastPos_TopRight,
	ImGuiToastPos_BottomLeft,
	ImGuiToastPos_BottomCenter,
	ImGuiToastPos_BottomRight,
	ImGuiToastPos_Center,
	ImGuiToastPos_COUNT
};

class ImGuiToast {
private:
	ImGuiToastType _type = ImGuiToastType_None;
	core::String _message;
	int _dismissTime = NOTIFY_DEFAULT_DISMISS;
	uint64_t _creationTime = 0;

public:
	ImGuiToast(ImGuiToastType type, const core::String &message);

	const char *defaultTitle() const;
	ImGuiToastType type() const;
	ImVec4 color() const;
	const char *icon() const;
	const char *content() const;
	uint32_t elapsedTime() const;
	ImGuiToastPhase phase() const;
	float fadePercent() const;
};

namespace ImGui {

/// Render toasts, call at the end of your rendering!
int RenderNotifications(core::DynamicArray<ImGuiToast>& notifications);

}
