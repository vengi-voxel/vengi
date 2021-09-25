// imgui-notify by patrickcjk
// https://github.com/patrickcjk/imgui-notify

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
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing

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
	char _title[NOTIFY_MAX_MSG_LENGTH];
	char _content[NOTIFY_MAX_MSG_LENGTH];
	int _dismissTime = NOTIFY_DEFAULT_DISMISS;
	uint64_t _creationTime = 0;

public:
	void setTitle(const char *format, ...) {
		if (!format) {
			return;
		}
		va_list args;
		va_start(args, format);
		SDL_vsnprintf(_title, sizeof(_title), format, args);
		va_end(args);
	}

	void setContent(const char *format, ...) {
		if (!format) {
			return;
		}
		va_list args;
		va_start(args, format);
		SDL_vsnprintf(_content, sizeof(_content), format, args);
		va_end(args);
	}

	void setType(ImGuiToastType type) {
		IM_ASSERT(type < ImGuiToastType_COUNT);
		_type = type;
	}

	const char *title() const {
		return _title;
	}

	const char *defaultTitle() const {
		if (!strlen(_title)) {
			switch (_type) {
			case ImGuiToastType_None:
				return nullptr;
			case ImGuiToastType_Debug:
				return "Success";
			case ImGuiToastType_Warning:
				return "Warning";
			case ImGuiToastType_Error:
				return "Error";
			case ImGuiToastType_Info:
				return "Info";
			}
		}

		return _title;
	}

	ImGuiToastType type() const {
		return _type;
	}

	ImVec4 color() const {
		switch (_type) {
		case ImGuiToastType_None:
			return {255, 255, 255, 255}; // White
		case ImGuiToastType_Debug:
			return {0, 255, 0, 255}; // Green
		case ImGuiToastType_Warning:
			return {255, 255, 0, 255}; // Yellow
		case ImGuiToastType_Error:
			return {255, 0, 0, 255}; // Error
		case ImGuiToastType_Info:
		default:
			return {0, 157, 255, 255}; // Blue
		}
	}

	const char *icon() const {
		switch (_type) {
		case ImGuiToastType_None:
			return nullptr;
		case ImGuiToastType_Debug:
			return ICON_FA_CHECK_CIRCLE;
		case ImGuiToastType_Warning:
			return ICON_FA_EXCLAMATION_TRIANGLE;
		case ImGuiToastType_Error:
			return ICON_FA_TIMES_CIRCLE;
		case ImGuiToastType_Info:
		default:
			return ICON_FA_INFO_CIRCLE;
		}
	}

	const char *content() const {
		return _content;
	}

	uint32_t elapsedTime() const {
		return SDL_GetTicks() - _creationTime;
	}

	ImGuiToastPhase phase() const {
		const uint32_t elapsed = elapsedTime();

		if (elapsed > NOTIFY_FADE_IN_OUT_TIME + _dismissTime + NOTIFY_FADE_IN_OUT_TIME) {
			return ImGuiToastPhase_Expired;
		} else if (elapsed > NOTIFY_FADE_IN_OUT_TIME + _dismissTime) {
			return ImGuiToastPhase_FadeOut;
		} else if (elapsed > NOTIFY_FADE_IN_OUT_TIME) {
			return ImGuiToastPhase_Wait;
		}
		return ImGuiToastPhase_FadeIn;
	}

	float fadePercent() const {
		const ImGuiToastPhase p = phase();
		const uint32_t elapsed = elapsedTime();

		if (p == ImGuiToastPhase_FadeIn) {
			return ((float)elapsed / (float)NOTIFY_FADE_IN_OUT_TIME) * NOTIFY_OPACITY;
		} else if (p == ImGuiToastPhase_FadeOut) {
			return (1.0f - (((float)elapsed - (float)NOTIFY_FADE_IN_OUT_TIME - (float)_dismissTime) /
							(float)NOTIFY_FADE_IN_OUT_TIME)) *
				   NOTIFY_OPACITY;
		}

		return NOTIFY_OPACITY;
	}

public:
	ImGuiToast(ImGuiToastType type, int dismiss_time = NOTIFY_DEFAULT_DISMISS) {
		IM_ASSERT(type < ImGuiToastType_COUNT);

		_type = type;
		_dismissTime = dismiss_time;
		_creationTime = SDL_GetTicks();

		memset(_title, 0, sizeof(_title));
		memset(_content, 0, sizeof(_content));
	}

	ImGuiToast(ImGuiToastType type, const char *format, ...) : ImGuiToast(type) {
		if (!format) {
			return;
		}
		va_list args;
		va_start(args, format);
		SDL_vsnprintf(_content, sizeof(_content), format, args);
		va_end(args);
	}

	ImGuiToast(ImGuiToastType type, int dismiss_time, const char *format, ...) : ImGuiToast(type, dismiss_time) {
		if (!format) {
			return;
		}
		va_list args;
		va_start(args, format);
		SDL_vsnprintf(_content, sizeof(_content), format, args);
		va_end(args);
	}
};

namespace ImGui {

/// Render toasts, call at the end of your rendering!
int RenderNotifications(core::DynamicArray<ImGuiToast>& notifications);

} // namespace ImGui
