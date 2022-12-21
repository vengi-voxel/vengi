/**
 * @file
 */

#include "AnimationTimeline.h"
#include "core/ArrayLength.h"
#include "core/collection/DynamicArray.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsFontAwesome6.h"
#include "ui/IconsForkAwesome.h"
#include "ui/dearimgui/imgui_neo_sequencer.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::update(const char *sequencerTitle, ImGuiID dockId) {
	const EditMode editMode = sceneMgr().editMode();
	voxelformat::FrameIndex currentFrame = sceneMgr().currentFrame();
	if (editMode == EditMode::Scene) {
		const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		uint32_t startFrame = 0;
		uint32_t endFrame = 0;
		for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
			endFrame = core_max(modelNode.maxFrame(), endFrame);
		}

		if (_play) {
			if (endFrame <= 0) {
				_play = false;
			} else {
				// TODO: anim fps
				currentFrame = (currentFrame + 1) % endFrame;
				sceneMgr().setCurrentFrame(currentFrame);
			}
		}
		ImGui::SetNextWindowDockID(dockId, ImGuiCond_Appearing);
		if (ImGui::Begin(sequencerTitle, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
			if (ImGui::DisabledButton(ICON_FA_SQUARE_PLUS " Add", _play)) {
				sceneMgr().nodeForeachGroup([&](int nodeId) {
					voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
					if (node.addKeyFrame(currentFrame) == InvalidKeyFrame) {
						Log::error("Failed to add keyframe for frame %i", (int)currentFrame);
						return;
					}
					const uint32_t newKeyFrameIdx = node.keyFrameForFrame(currentFrame);
					if (newKeyFrameIdx > 0) {
						node.keyFrame(newKeyFrameIdx).setTransform(node.keyFrame(newKeyFrameIdx - 1).transform());
					}
				});
			}
			ImGui::SameLine();
			if (ImGui::DisabledButton(ICON_FA_SQUARE_MINUS " Remove", _play)) {
				sceneMgr().nodeForeachGroup([&](int nodeId) {
					voxelformat::SceneGraphNode &node = sceneGraph.node(nodeId);
					if (!node.removeKeyFrame(currentFrame)) {
						Log::error("Failed to remove frame %i", (int)currentFrame);
					}
				});
			}
			ImGui::SameLine();

			if (_play) {
				if (ImGui::Button(ICON_FA_STOP)) {
					_play = false;
				}
			} else {
				if (ImGui::DisabledButton(ICON_FA_PLAY, endFrame <= 0)) {
					_play = true;
				}
			}

			if (endFrame < 10) {
				endFrame = 64;
			} else {
				endFrame += endFrame / 10;
			}

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f,0.0f});
			if (ImGui::BeginChild("##neo-sequencer-child")) {
				if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &startFrame, &endFrame, {0, 0}, ImGuiNeoSequencerFlags_AlwaysShowHeader)) {
					for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
						core::DynamicArray<voxelformat::FrameIndex *> keys;
						keys.reserve(modelNode.keyFrames().size());
						for (voxelformat::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
							keys.push_back(&kf.frameIdx);
						}
						const core::String &label =
							core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
						uint32_t **keyframes = keys.data();
						const uint32_t keyframeCount = keys.size();
						if (ImGui::BeginNeoTimeline(label.c_str(), keyframes, keyframeCount, nullptr,
													ImGuiNeoTimelineFlags_None)) {
							sceneMgr().setCurrentFrame(currentFrame);
							if (ImGui::IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags_NewlySelected)) {
								sceneMgr().nodeActivate(modelNode.id());
							} else if (sceneMgr().sceneGraph().activeNode() == modelNode.id()) {
								ImGui::SetSelectedTimeline(label.c_str());
							}
							ImGui::EndNeoTimeLine();
						}
					}
					ImGui::EndNeoSequencer();
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
		}
		ImGui::End();
	}
}

} // namespace voxedit
