#include "Notify.h"
#include "SDL_stdinc.h"
#include "core/Trace.h"

ImGuiToast::ImGuiToast(ImGuiToastType type, const core::String &message) {
	IM_ASSERT(type < ImGuiToastType_COUNT);

	_type = type;
	_dismissTime = NOTIFY_DEFAULT_DISMISS;
	_creationTime = SDL_GetTicks();
	_message = message;
}

const char *ImGuiToast::defaultTitle() const {
	switch (_type) {
	case ImGuiToastType_Debug:
		return "Debug";
	case ImGuiToastType_Warning:
		return "Warning";
	case ImGuiToastType_Error:
		return "Error";
	case ImGuiToastType_Info:
		return "Info";
	}
	return "Unknown";
}

ImGuiToastType ImGuiToast::type() const {
	return _type;
}

ImVec4 ImGuiToast::color() const {
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

const char *ImGuiToast::icon() const {
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

const char *ImGuiToast::content() const {
	return _message.c_str();
}

uint32_t ImGuiToast::elapsedTime() const {
	return SDL_GetTicks() - _creationTime;
}

ImGuiToastPhase ImGuiToast::phase() const {
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

float ImGuiToast::fadePercent() const {
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

namespace ImGui {

int RenderNotifications(core::DynamicArray<ImGuiToast> &notifications) {
	core_trace_scoped(RenderNotifications)
	const ImVec2 &vpSize = ImGui::GetMainViewport()->Size;

	float height = 0.0f;
	int n = 0;

	for (size_t i = 0; i < notifications.size(); i++) {
		ImGuiToast *currentToast = &notifications[i];

		// Remove toast if expired
		if (currentToast->phase() == ImGuiToastPhase_Expired) {
			notifications.erase(notifications.begin() + i);
			continue;
		}
		++n;

		// Get icon, title and other data
		const char *icon = currentToast->icon();
		const char *content = currentToast->content();
		const char *default_title = currentToast->defaultTitle();
		const float opacity = currentToast->fadePercent(); // Get opacity based of the current phase

		// Window rendering
		ImVec4 textColor = currentToast->color();
		textColor.w = opacity;

		// Generate new unique name for this toast
		char windowName[50];
		SDL_snprintf(windowName, sizeof(windowName), "##TOAST%i", (int)i);

		// PushStyleColor(ImGuiCol_Text, text_color);
		ImGui::SetNextWindowBgAlpha(opacity);
		const ImVec2 windowPos(vpSize.x - NOTIFY_PADDING_X, vpSize.y - NOTIFY_PADDING_Y - height);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		if (ImGui::Begin(windowName, nullptr, NOTIFY_TOAST_FLAGS)) {
			// We want to support multi-line text, this will wrap the text after 1/3 of the screen width
			ImGui::PushTextWrapPos(vpSize.x / 3.0f);

			bool wasTitleRendered = false;

			// If an icon is set
			if (!NOTIFY_NULL_OR_EMPTY(icon)) {
				ImGui::TextColored(textColor, "%s", icon);
				wasTitleRendered = true;
			}

			// If a title is set
			if (!NOTIFY_NULL_OR_EMPTY(default_title)) {
				if (!NOTIFY_NULL_OR_EMPTY(icon)) {
					ImGui::SameLine();
				}

				// Render default title text (ImGuiToastType_Success -> "Success", etc...)
				ImGui::TextUnformatted(default_title);
				wasTitleRendered = true;
			}

			// In case ANYTHING was rendered in the top, we want to add a small padding so the text (or icon) looks
			// centered vertically
			if (wasTitleRendered && !NOTIFY_NULL_OR_EMPTY(content)) {
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f); // Must be a better way to do this!!!!
			}

			// If a content is set
			if (!NOTIFY_NULL_OR_EMPTY(content)) {
				if (wasTitleRendered) {
					ImGui::Separator();
				}

				ImGui::TextUnformatted(content); // Render content text
			}

			ImGui::PopTextWrapPos();

			// Save height for next toasts
			height += ImGui::GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;
		}

		ImGui::End();
	}
	return n;
}

} // namespace ImGui
