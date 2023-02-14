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

void AnimationTimeline::header(voxelformat::FrameIndex &currentFrame, voxelformat::FrameIndex maxFrame) {
	if (ImGui::DisabledButton(ICON_FA_SQUARE_PLUS " Add", _play)) {
		sceneMgr().nodeAddKeyFrame(-1, currentFrame);
	}
	ImGui::SameLine();
	if (ImGui::DisabledButton(ICON_FA_SQUARE_MINUS " Remove", _play)) {
		sceneMgr().nodeRemoveKeyFrameByIndex(-1, currentFrame);
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_ARROW_RIGHT_ARROW_LEFT)) {
		_startFrame = 0;
		_endFrame = core_max(64, maxFrame + 1);
	}
	ImGui::SameLine();

	if (_play) {
		if (ImGui::Button(ICON_FA_STOP)) {
			_play = false;
		}
	} else {
		if (ImGui::DisabledButton(ICON_FA_PLAY, maxFrame <= 0)) {
			_play = true;
		}
	}
}

void AnimationTimeline::sequencer(voxelformat::FrameIndex &currentFrame) {
	ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_AlwaysShowHeader;
	flags |= ImGuiNeoSequencerFlags_EnableSelection;
	flags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDragging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDeletion;

	if (ImGui::BeginNeoSequencer("##neo-sequencer", (uint32_t *)&currentFrame, (uint32_t *)&_startFrame,
								 (uint32_t *)&_endFrame, {0, 0}, flags)) {
		voxelformat::KeyFrameIndex deleteKeyFrameIdx = InvalidKeyFrame;
		int deleteFrameNode = -1;

		const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
			const core::String &label = core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
			if (ImGui::BeginNeoTimelineEx(label.c_str(), nullptr, ImGuiNeoTimelineFlags_AllowFrameChanging)) {
				voxelformat::KeyFrameIndex keyFrameIdx = 0;
				for (voxelformat::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
					ImGui::NeoKeyframe(&kf.frameIdx);
					if (ImGui::IsNeoKeyframeRightClicked() && ImGui::NeoCanDeleteSelection()) {
						deleteKeyFrameIdx = keyFrameIdx;
						deleteFrameNode = modelNode.id();
					}

					if (ImGui::IsNeoKeyframeHovered()) {
						ImGui::BeginTooltip();
						const char *interpolation = voxelformat::InterpolationTypeStr[(int)kf.interpolation];
						ImGui::Text("Keyframe %i, Interpolation: %s", kf.frameIdx, interpolation);
						ImGui::EndTooltip();
					}
					++keyFrameIdx;
				}

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

		if (deleteFrameNode != -1) {
			ImGui::OpenPopup("keyframe-context-menu");
			_deleteKeyFrameIdx = deleteKeyFrameIdx;
			_deleteFrameNode = deleteFrameNode;
		}
		ImGuiWindowFlags popupFlags = 0;
		popupFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		popupFlags |= ImGuiWindowFlags_NoTitleBar;
		popupFlags |= ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::BeginPopup("keyframe-context-menu", popupFlags)) {
			// TODO: implement copy & paste
			// TODO: implement duplicate
			if (ImGui::MenuItem(ICON_FA_TRASH " Delete keyframe")) {
				// TODO: delete all selected keyframes
				if (!sceneMgr().nodeRemoveKeyFrameByIndex(_deleteFrameNode, _deleteKeyFrameIdx)) {
					Log::warn("Failed to delete key frame index %i for node %i", _deleteKeyFrameIdx, _deleteFrameNode);
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
}

bool AnimationTimeline::update(const char *sequencerTitle, double deltaFrameSeconds) {
	voxelformat::FrameIndex currentFrame = sceneMgr().currentFrame();
	const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	voxelformat::FrameIndex maxFrame = 0;
	for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
		maxFrame = core_max(modelNode.maxFrame(), maxFrame);
	}
	_seconds += deltaFrameSeconds;

	if (_play) {
		if (maxFrame <= 0) {
			_play = false;
		} else {
			// TODO: anim fps
			currentFrame = (currentFrame + 1) % maxFrame;
			sceneMgr().setCurrentFrame(currentFrame);
		}
	}
	if (ImGui::Begin(sequencerTitle)) {
		header(currentFrame, maxFrame);
		if (ImGui::BeginChild("##neo-sequencer-child")) {
			sequencer(currentFrame);
		}
		ImGui::EndChild();
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
