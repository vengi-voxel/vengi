/**
 * @file
 */

#include "MementoPanel.h"
#include "command/CommandHandler.h"
#include "core/collection/DynamicArray.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/MementoHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

static inline core::String toString(const MementoState &state, int n) {
	return core::string::format("%s: node %i, parent %i, keyframe: %u, name: %s##%i", MementoHandler::typeToString(state.type), state.nodeId, state.parentId, state.keyFrameIdx, state.name.c_str(), n);
}

void MementoPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		const int currentStatePos = mementoHandler.statePosition();
		ImGui::Text(_("Current state: %i / %i"), currentStatePos, (int)mementoHandler.stateSize());

		if (ImGui::BeginListBox("##history-actions", ImVec2(-FLT_MIN, -FLT_MIN))) {
			core::DynamicArray<const MementoState*> states;
			states.reserve(mementoHandler.stateSize());
			for (const MementoState &state : mementoHandler.states()) {
				states.push_back(&state);
			}

			int n = 0;
			int newStatePos = -1;
			ImGuiListClipper clipper;
			clipper.Begin((int)mementoHandler.stateSize());
			while (clipper.Step()) {
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
					const MementoState &state = *states[row];
					const core::String &info = toString(state, n);
					const bool selected = n == currentStatePos;
					if (ImGui::Selectable(info.c_str(), selected)) {
						newStatePos = n;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
					++n;
				}
			}
			ImGui::EndListBox();
			if (newStatePos != -1) {
				if (currentStatePos > newStatePos) {
					_sceneMgr->undo(currentStatePos - newStatePos);
				} else {
					_sceneMgr->redo(newStatePos - currentStatePos);
				}
			}
		}
	}
	ImGui::End();
}

} // namespace voxedit
