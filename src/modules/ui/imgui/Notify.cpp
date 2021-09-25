#include "Notify.h"

namespace ImGui {

int RenderNotifications(core::DynamicArray<ImGuiToast> &notifications) {
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
		const char *title = currentToast->title();
		const char *content = currentToast->content();
		const char *default_title = currentToast->defaultTitle();
		const float opacity = currentToast->fadePercent(); // Get opacity based of the current phase

		// Window rendering
		auto text_color = currentToast->color();
		text_color.w = opacity;

		// Generate new unique name for this toast
		char windowName[50];
		SDL_snprintf(windowName, sizeof(windowName), "##TOAST%i", (int)i);

		// PushStyleColor(ImGuiCol_Text, text_color);
		ImGui::SetNextWindowBgAlpha(opacity);
		const ImVec2 windowPos(vpSize.x - NOTIFY_PADDING_X, vpSize.y - NOTIFY_PADDING_Y - height);
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		ImGui::Begin(windowName, nullptr, NOTIFY_TOAST_FLAGS);

		// Here we render the toast content
		{
			// We want to support multi-line text, this will wrap the text after 1/3 of the screen width
			ImGui::PushTextWrapPos(vpSize.x / 3.0f);

			bool wasTitleRendered = false;

			// If an icon is set
			if (!NOTIFY_NULL_OR_EMPTY(icon)) {
				ImGui::TextColored(text_color, "%s", icon);
				wasTitleRendered = true;
			}

			// If a title is set
			if (!NOTIFY_NULL_OR_EMPTY(title)) {
				// If a title and an icon is set, we want to render on same line
				if (!NOTIFY_NULL_OR_EMPTY(icon)) {
					ImGui::SameLine();
				}

				ImGui::TextUnformatted(title); // Render title text
				wasTitleRendered = true;
			} else if (!NOTIFY_NULL_OR_EMPTY(default_title)) {
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
		}

		// Save height for next toasts
		height += ImGui::GetWindowHeight() + NOTIFY_PADDING_MESSAGE_Y;

		ImGui::End();
	}
	return n;
}

} // namespace ImGui
