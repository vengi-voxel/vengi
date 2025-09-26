/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "imgui.h"

namespace ui {

class ScopedID {
public:
	ScopedID(int id) {
		ImGui::PushID(id);
	}
	ScopedID(const void *id) {
		ImGui::PushID(id);
	}
	ScopedID(const core::String &id) {
		ImGui::PushID(id.c_str());
	}
	ScopedID(const char *id, const char *idEnd = nullptr) {
		ImGui::PushID(id, idEnd);
	}
	~ScopedID() {
		ImGui::PopID();
	}
};

} // namespace ui
