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
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::header(scenegraph::FrameIndex &currentFrame, scenegraph::FrameIndex maxFrame) {
	if (ImGui::DisabledButton(ICON_FA_SQUARE_PLUS " Add", _play)) {
		sceneMgr().nodeAddKeyFrame(-1, currentFrame);
	}
	ImGui::SameLine();
	if (ImGui::DisabledButton(ICON_FA_SQUARE_MINUS " Remove", _play)) {
		sceneMgr().nodeRemoveKeyFrame(-1, currentFrame);
	}
	ImGui::TooltipText("Delete the current keyframe on all nodes");
	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_ARROW_RIGHT_ARROW_LEFT)) {
		_startFrame = 0;
		_endFrame = core_max(64, maxFrame + 1);
	}
	ImGui::TooltipText("Crop frames");
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

void AnimationTimeline::sequencer(scenegraph::FrameIndex &currentFrame) {
	ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_AlwaysShowHeader;
	flags |= ImGuiNeoSequencerFlags_EnableSelection;
	flags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDragging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDeletion;

	if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &_startFrame, &_endFrame, {0, 0}, flags)) {
		_selectionBuffer.clear();
		if (_clearSelection) {
			ImGui::NeoClearSelection();
			_clearSelection = false;
		}
		core::Buffer<scenegraph::FrameIndex> selectedFrames;
		const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
		for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &modelNode = *iter;
			const core::String &label = core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
			if (ImGui::BeginNeoTimelineEx(label.c_str(), nullptr, ImGuiNeoTimelineFlags_AllowFrameChanging)) {
				for (scenegraph::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
					ImGui::NeoKeyframe(&kf.frameIdx);
					if (kf.frameIdx < 0) {
						kf.frameIdx = 0;
					}

					if (ImGui::IsNeoKeyframeHovered()) {
						ImGui::BeginTooltip();
						const char *interpolation = scenegraph::InterpolationTypeStr[(int)kf.interpolation];
						ImGui::Text("Keyframe %i, Interpolation: %s", kf.frameIdx, interpolation);
						ImGui::EndTooltip();
					}
				}

				sceneMgr().setCurrentFrame(currentFrame);
				if (ImGui::IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags_NewlySelected)) {
					sceneMgr().nodeActivate(modelNode.id());
				} else if (sceneMgr().sceneGraph().activeNode() == modelNode.id()) {
					ImGui::SetSelectedTimeline(label.c_str());
				}
				uint32_t selectionCount = ImGui::GetNeoKeyframeSelectionSize();
				if (selectionCount > 0) {
					selectedFrames.clear();
					selectedFrames.resizeIfNeeded(selectionCount);
					ImGui::GetNeoKeyframeSelection(selectedFrames.data());
					for (uint32_t i = 0; i < selectionCount; ++i) {
						_selectionBuffer.push_back(Selection{selectedFrames[i], modelNode.id()});
					}
				}
				ImGui::EndNeoTimeLine();
			}
		}
		bool selectionRightClicked = ImGui::IsNeoKeyframeSelectionRightClicked();

		ImGui::EndNeoSequencer();

		if (selectionRightClicked) {
			ImGui::OpenPopup("keyframe-context-menu");
		}
		ImGuiWindowFlags popupFlags = 0;
		popupFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		popupFlags |= ImGuiWindowFlags_NoTitleBar;
		popupFlags |= ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::BeginPopup("keyframe-context-menu", popupFlags)) {
			if (ImGui::MenuItem(ICON_FA_SQUARE_PLUS " Add")) {
				sceneMgr().nodeAddKeyFrame(-1, currentFrame);
				_clearSelection = true;
				ImGui::CloseCurrentPopup();
			}
			if (!_selectionBuffer.empty()) {
#if 0
				if (ImGui::MenuItem(ICON_FA_COPY " Copy")) {
					// TODO: implement copy
				}
				if (ImGui::MenuItem(ICON_FA_PASTE " Paste")) {
					// TODO: implement paste
				}
#endif
				if (ImGui::MenuItem(ICON_FA_COPY " Duplicate keyframe")) {
					for (const Selection &sel : _selectionBuffer) {
						sceneMgr().nodeAddKeyFrame(sel.nodeId, sel.frameIdx + 1);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem(ICON_FA_TRASH " Delete keyframe")) {
					for (const Selection &sel : _selectionBuffer) {
						sceneMgr().nodeRemoveKeyFrame(sel.nodeId, sel.frameIdx);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
	}
}

bool AnimationTimeline::update(const char *sequencerTitle, double deltaFrameSeconds) {
	scenegraph::FrameIndex currentFrame = sceneMgr().currentFrame();
	const scenegraph::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
	scenegraph::FrameIndex maxFrame = 0;
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &modelNode = *iter;
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
