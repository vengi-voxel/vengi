/**
 * @file
 */

#include "MementoPanel.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/MementoHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

static inline core::String toString(const MementoState &state, int n) {
	return core::string::format("%s: node %i, parent %i, keyframe: %u, name: %s##%i", MementoHandler::typeToString(state.type), state.nodeId, state.parentId, state.keyFrame, state.name.c_str(), n);
}

void MementoPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const MementoHandler &mementoHandler = sceneMgr().mementoHandler();
		const int currentStatePos = mementoHandler.statePosition();
		ImGui::Text("pos: %i/%i", currentStatePos, (int)mementoHandler.stateSize());
		if (ImGui::BeginListBox("##history-actions", ImVec2(-FLT_MIN, -FLT_MIN))) {
			int n = 0;
			int newStatePos = -1;
			for (const MementoState &state : mementoHandler.states()) {
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
			ImGui::EndListBox();
			if (newStatePos != -1) {
				if (currentStatePos > newStatePos) {
					sceneMgr().undo(currentStatePos - newStatePos);
				} else {
					sceneMgr().redo(newStatePos - currentStatePos);
				}
			}
		}
	}
	ImGui::End();
}

} // namespace voxedit
