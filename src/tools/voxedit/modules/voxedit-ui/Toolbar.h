/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "ui/Toolbar.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

class Toolbar : public ui::Toolbar {
private:
	using Super = ui::Toolbar;
	SceneManagerPtr _sceneMgr;

	bool commandButton(const char *label, const char *command, const char *tooltip, const ImVec2 &size,
					   command::CommandExecutionListener *listener) {
		if (ImGui::Button(label, size)) {
			if (_sceneMgr->executeCommandsAsync(command)) {
				return true;
			}
		}
		if (tooltip != nullptr) {
			ImGui::TooltipTextUnformatted(tooltip);
		} else {
			ImGui::TooltipCommand(command);
		}
		return false;
	}

public:
	Toolbar(const core::String &name, const ImVec2 &size, command::CommandExecutionListener *listener,
			const SceneManagerPtr &sceneMgr)
		: Super(name, size, listener), _sceneMgr(sceneMgr) {
	}

	Toolbar(const core::String &name, command::CommandExecutionListener *listener, const SceneManagerPtr &sceneMgr)
		: Toolbar(name, ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), listener, sceneMgr) {
	}

	bool asyncButton(const char *icon, const char *command, bool disable = false) {
		newline();
		ui::ScopedStyle style;
		applyIconStyle(style);
		if (disable) {
			applyDisabledStyle(style);
		}
		ImGui::PushID(_id.c_str());
		char label[64];
		core::String::formatBuf(label, sizeof(label), "%s###button%d", icon, _nextId);
		bool pressed = commandButton(label, command, nullptr, _size, _listener);
		ImGui::PopID();
		next();
		return pressed;
	}
};

} // namespace voxedit
