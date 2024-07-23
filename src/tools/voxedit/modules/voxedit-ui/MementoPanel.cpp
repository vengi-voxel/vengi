/**
 * @file
 */

#include "MementoPanel.h"
#include "command/CommandHandler.h"
#include "core/collection/DynamicArray.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

static inline core::String toString(const memento::MementoState &state, const core::String &name, int n) {
	return core::string::format("%s (%s): node %i, parent %i, keyframe: %u, name: %s##%i",
								memento::MementoHandler::typeToString(state.type), name.c_str(), state.nodeId, state.parentId,
								state.keyFrameIdx, state.name.c_str(), n);
}

void MementoPanel::update(const char *id, command::CommandExecutionListener &listener) {
	const core::String title = makeTitle(ICON_LC_BOOK_OPEN, _("History"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		const int currentStatePos = mementoHandler.statePosition();
		ImGui::Text(_("Current state: %i / %i"), currentStatePos, (int)mementoHandler.stateSize());

		if (ImGui::BeginListBox("##history-actions", ImVec2(-FLT_MIN, -FLT_MIN))) {
			struct State {
				State(const core::String *name, const memento::MementoState *state, int stateIdx) : name(name), state(state), stateIdx(stateIdx) {}
				const core::String *name;
				const memento::MementoState *state;
				const int stateIdx;
			};
			core::DynamicArray<State> states;
			states.reserve(mementoHandler.stateSize());
			int stateIdx = 0;
			for (const memento::MementoStateGroup &group : mementoHandler.states()) {
				for (const memento::MementoState &state : group.states) {
					states.emplace_back(&group.name, &state, stateIdx);
				}
				++stateIdx;
			}

			int newStatePos = -1;
			ImGuiListClipper clipper;
			clipper.Begin((int)states.size());
			while (clipper.Step()) {
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
					const State &state = states[row];
					const core::String &info = toString(*state.state, *state.name, row);
					const bool selected = state.stateIdx == currentStatePos;
					if (ImGui::Selectable(info.c_str(), selected)) {
						newStatePos = state.stateIdx;
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
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
