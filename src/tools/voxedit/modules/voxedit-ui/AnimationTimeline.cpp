/**
 * @file
 */

#include "AnimationTimeline.h"
#include "IconsForkAwesome.h"
#include "core/ArrayLength.h"
#include "core/collection/DynamicArray.h"
#include "ui/imgui/dearimgui/imgui.h"
#include "ui/imgui/dearimgui/imgui_neo_sequencer.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::update(const char *sequencerTitle, ImGuiID dockIdMainDown) {
	const EditMode editMode = sceneMgr().editMode();
	uint32_t currentFrame = sceneMgr().currentFrame();
	const bool changedOutside = sceneMgr().sceneGraph().activeNode() != _lastActiveNode;
	if (editMode == EditMode::Scene) {
		const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		ImGui::SetNextWindowDockID(dockIdMainDown, ImGuiCond_Appearing);
		if (ImGui::Begin(sequencerTitle, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
			if (ImGui::Button(ICON_FA_PLUS_SQUARE " Add")) {
				sceneMgr().nodeForeachGroup([&](int nodeId) {
					voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
					node.addKeyFrame(currentFrame);
				});
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_MINUS_SQUARE " Remove")) {
				sceneMgr().nodeForeachGroup([&](int nodeId) {
					voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
					node.removeKeyFrame(currentFrame);
				});
			}
			uint32_t startFrame = 0;
			uint32_t endFrame = 64; // TODO:
			if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &startFrame, &endFrame)) {
				for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
					core::DynamicArray<uint32_t *> keys;
					keys.reserve(modelNode.keyFrames().size());
					for (voxelformat::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
						keys.push_back(&kf.frame);
					}
					const core::String &label =
						core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
					uint32_t **keyframes = keys.data();
					const uint32_t keyframeCount = keys.size();
					if (ImGui::BeginNeoTimeline(label.c_str(), keyframes, keyframeCount, nullptr,
												ImGuiNeoTimelineFlags_None)) {
						sceneMgr().setCurrentFrame(currentFrame);
						if (changedOutside) {
							if (sceneMgr().sceneGraph().activeNode() == modelNode.id()) {
								ImGui::SetSelectedTimeline(label.c_str());
							}
						} else {
							if (ImGui::IsNeoTimeLineSelected(label.c_str())) {
								sceneMgr().nodeActivate(modelNode.id());
							}
						}
						ImGui::EndNeoTimeLine();
					}
				}
				ImGui::EndNeoSequencer();
			}
		}
		ImGui::End();
	}
	_lastActiveNode = sceneMgr().sceneGraph().activeNode();
}

} // namespace voxedit
