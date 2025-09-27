/**
 * @file
 */

#include "MementoPanel.h"
#include "command/CommandHandler.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

static inline core::String toString(const memento::MementoState &state, const core::String &name, int n) {
	const core::String &uuidStr = state.nodeUUID.str();
	const core::String &parentUUIDStr = state.parentUUID.str();
	return core::String::format("%s (%s): node %s, parent %s, name: %s##%i",
								memento::MementoHandler::typeToString(state.type), name.c_str(), uuidStr.c_str(),
								parentUUIDStr.c_str(), state.name.c_str(), n);
}

static void stateTooltip(const memento::MementoState &state) {
	core::String palHash = core::string::toString(state.palette.hash());
	if (ImGui::BeginItemTooltip()) {
		const core::String &uuidStr = state.nodeUUID.str();
		const core::String &parentUUIDStr = state.parentUUID.str();
		ImGui::Text("%s: node id: %s", memento::MementoHandler::typeToString(state.type), uuidStr.c_str());
		ImGui::Text(" - parent: %s", parentUUIDStr.c_str());
		ImGui::Text(" - name: %s", state.name.c_str());
		ImGui::Text(" - type: %s", scenegraph::SceneGraphNodeTypeStr[(int)state.nodeType]);
		ImGui::Text(" - volume: %s", state.data.hasVolume() ? "volume" : "empty");
		const glm::ivec3 &dataMins = state.dataRegion().getLowerCorner();
		const glm::ivec3 &dataMaxs = state.dataRegion().getUpperCorner();
		ImGui::Text(" - dataregion: mins(%i:%i:%i)/maxs(%i:%i:%i)", dataMins.x, dataMins.y, dataMins.z, dataMaxs.x, dataMaxs.y, dataMaxs.z);
		const glm::ivec3 &volumeMins = state.volumeRegion().getLowerCorner();
		const glm::ivec3 &volumeMaxs = state.volumeRegion().getUpperCorner();
		ImGui::Text(" - volumeregion: mins(%i:%i:%i)/maxs(%i:%i:%i)", volumeMins.x, volumeMins.y, volumeMins.z, volumeMaxs.x, volumeMaxs.y, volumeMaxs.z);
		ImGui::Text(" - size: %ib", (int)state.data.size());
		ImGui::Text(" - palette: %s", palHash.c_str());
		ImGui::Text(" - pivot: %f:%f:%f", state.pivot.x, state.pivot.y, state.pivot.z);
		const scenegraph::SceneGraphKeyFramesMap &keyFrames = state.keyFrames;
		ImGui::Text(" - key frames");
		for (const auto &e : keyFrames) {
			ImGui::Text("   - animation: %s", e->first.c_str());
			const scenegraph::SceneGraphKeyFrames &frames = e->second;
			for (const auto &f : frames) {
				ImGui::Text("     - frame: %i", f.frameIdx);
				ImGui::Text("       - interpolation: %s", scenegraph::InterpolationTypeStr[(int)f.interpolation]);
				ImGui::Text("       - long rotation: %s", f.longRotation ? "true" : "false");
				ImGui::Text("       - transform");
				const glm::mat4 &m = f.transform().worldMatrix();
				ImGui::Text("         - %f:%f:%f:%f", m[0][0], m[0][1], m[0][2], m[0][3]);
				ImGui::Text("         - %f:%f:%f:%f", m[1][0], m[1][1], m[1][2], m[1][3]);
				ImGui::Text("         - %f:%f:%f:%f", m[2][0], m[2][1], m[2][2], m[2][3]);
				ImGui::Text("         - %f:%f:%f:%f", m[3][0], m[3][1], m[3][2], m[3][3]);
			}
		}
		if (!state.properties.empty()) {
			const scenegraph::SceneGraphNodeProperties &props = state.properties;
			ImGui::Text(" - properties");
			for (const auto &e : props) {
				ImGui::Text("   - %s: %s", e->first.c_str(), e->second.c_str());
			}
		} else {
			ImGui::Text(" - properties: none");
		}
		ImGui::EndTooltip();
	}
}

void MementoPanel::update(const char *id, command::CommandExecutionListener &listener) {
	core_trace_scoped(MementoPanel);
	const core::String title = makeTitle(ICON_LC_BOOK_OPEN, _("History"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		const int currentStatePos = mementoHandler.statePosition();
		ImGui::Text(_("Current state: %i / %i"), currentStatePos, (int)mementoHandler.stateSize());

		if (ImGui::BeginListBox("##history-actions", ImVec2(-FLT_MIN, -FLT_MIN))) {
			struct State {
				State(const core::String *_name, const memento::MementoState *_state, int _stateIdx)
					: name(_name), state(_state), stateIdx(_stateIdx) {
				}
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
					stateTooltip(*state.state);
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
