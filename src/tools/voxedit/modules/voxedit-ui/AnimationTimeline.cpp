/**
 * @file
 */

#include "AnimationTimeline.h"
#include "IconsLucide.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/dearimgui/imgui_neo_sequencer.h"
#include "voxedit-util/SceneManager.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::header(scenegraph::FrameIndex currentFrame, scenegraph::FrameIndex maxFrame) {
	if (ImGui::DisabledIconButton(ICON_LC_PLUS, _("Add"), _play)) {
		_sceneMgr->nodeAddKeyFrame(InvalidNodeId, currentFrame);
	}
	ImGui::TooltipTextUnformatted(_("Add a new keyframe to the current active node"));
	ImGui::SameLine();
	if (ImGui::DisabledIconButton(ICON_LC_SQUARE_PLUS, _("Add all"), _play)) {
		_sceneMgr->nodeAllAddKeyFrames(currentFrame);
	}
	ImGui::TooltipTextUnformatted(_("Add a new keyframe to all model nodes"));
	ImGui::SameLine();
	if (ImGui::DisabledIconButton(ICON_LC_SQUARE_MINUS, _("Delete"), _play)) {
		_sceneMgr->nodeRemoveKeyFrame(InvalidNodeId, currentFrame);
	}
	ImGui::TooltipTextUnformatted(_("Delete the current keyframe of the active nodes"));
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_ARROW_RIGHT_LEFT)) {
		_startFrame = 0;
		_endFrame = core_max(64, maxFrame + 1);
	}
	ImGui::TooltipTextUnformatted(_("Crop frames"));
	ImGui::SameLine();

	if (_play) {
		if (ImGui::Button(ICON_LC_CIRCLE_STOP)) {
			_play = false;
		}
	} else {
		if (ImGui::DisabledButton(ICON_LC_PLAY, maxFrame <= 0)) {
			_play = true;
		}
		ImGui::TooltipText(_("Max frames for this animation: %i"), maxFrame);
	}
}

void AnimationTimeline::timelineEntry(scenegraph::FrameIndex currentFrame, core::Buffer<Selection> &selectionBuffer,
								  core::Buffer<scenegraph::FrameIndex> &selectedFrames,
								  const scenegraph::SceneGraphNode &modelNode) {
	const core::String &label = core::String::format("%s###node-%i", modelNode.name().c_str(), modelNode.id());
	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (ImGui::BeginNeoTimelineEx(label.c_str(), nullptr, ImGuiNeoTimelineFlags_AllowFrameChanging)) {
		for (scenegraph::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
			int32_t oldFrameIdx = kf.frameIdx;
			ImGui::NeoKeyframe(&kf.frameIdx);
			if (kf.frameIdx < 0) {
				kf.frameIdx = 0;
			}
			if (oldFrameIdx != kf.frameIdx) {
				sceneGraph.markMaxFramesDirty();
			}

			if (ImGui::IsNeoKeyframeHovered()) {
				ImGui::BeginTooltip();
				const char *interpolation = scenegraph::InterpolationTypeStr[(int)kf.interpolation];
				ImGui::Text(_("Keyframe %i, Interpolation: %s"), kf.frameIdx, interpolation);
				ImGui::EndTooltip();
			}
		}
		if (activeNode != _lastActivedNodeId && modelNode.id() == activeNode) {
			// TODO: doesn't work - see issue https://github.com/vengi-voxel/vengi/issues/437
			ImGui::SetScrollHereY();
			_lastActivedNodeId = activeNode;
		}
		_sceneMgr->setCurrentFrame(currentFrame);
		if (ImGui::IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags_NewlySelected)) {
			_sceneMgr->nodeActivate(modelNode.id());
			_lastActivedNodeId = modelNode.id();
		} else if (activeNode == modelNode.id()) {
			ImGui::SetSelectedTimeline(label.c_str());
		}
		uint32_t selectionCount = ImGui::GetNeoKeyframeSelectionSize();
		if (selectionCount > 0) {
			selectedFrames.clear();
			selectedFrames.resizeIfNeeded(selectionCount);
			ImGui::GetNeoKeyframeSelection(selectedFrames.data());
			for (uint32_t i = 0; i < selectionCount; ++i) {
				selectionBuffer.push_back(Selection{selectedFrames[i], modelNode.id()});
			}
		}
		ImGui::EndNeoTimeLine();
	}
}

bool AnimationTimeline::init() {
	const scenegraph::SceneGraph& sceneGraph = _sceneMgr->sceneGraph();
	_lastActivedNodeId = sceneGraph.activeNode();
	return true;
}

void AnimationTimeline::sequencer(scenegraph::FrameIndex &currentFrame) {
	ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_AlwaysShowHeader;
	flags |= ImGuiNeoSequencerFlags_EnableSelection;
	flags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDragging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDeletion;

	if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &_startFrame, &_endFrame, {0, 0}, flags)) {
		core::Buffer<Selection> selectionBuffer;
		if (_clearSelection) {
			ImGui::NeoClearSelection();
			_clearSelection = false;
		}
		core::Buffer<scenegraph::FrameIndex> selectedFrames;
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		for (auto entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isAnyModelNode()) {
				continue;
			}
			timelineEntry(currentFrame, selectionBuffer, selectedFrames, node);
		}
		bool selectionRightClicked = ImGui::IsNeoKeyframeSelectionRightClicked();

		ImGui::EndNeoSequencer();

		if (selectionRightClicked) {
			_selectionBuffer = selectionBuffer;
			ImGui::OpenPopup("keyframe-context-menu");
		}
		ImGuiWindowFlags popupFlags = 0;
		popupFlags |= ImGuiWindowFlags_AlwaysAutoResize;
		popupFlags |= ImGuiWindowFlags_NoTitleBar;
		popupFlags |= ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::BeginPopup("keyframe-context-menu", popupFlags)) {
			if (ImGui::IconSelectable(ICON_LC_SQUARE_PLUS, _("Add"))) {
				_sceneMgr->nodeAddKeyFrame(InvalidNodeId, currentFrame);
				_clearSelection = true;
				ImGui::CloseCurrentPopup();
			}
			if (!_selectionBuffer.empty()) {
#if 0
				if (ImGui::IconSelectable(ICON_LC_COPY, _("Copy"))) {
					// TODO: implement copy
				}
				if (ImGui::IconSelectable(ICON_LC_PASTE, _("Paste"))) {
					// TODO: implement paste
				}
#endif
				if (ImGui::IconSelectable(ICON_LC_COPY, _("Duplicate keyframe"))) {
					for (const Selection &sel : _selectionBuffer) {
						_sceneMgr->nodeAddKeyFrame(sel.nodeId, sel.frameIdx + 1);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::IconSelectable(ICON_LC_TRASH, _("Delete keyframes"))) {
					for (const Selection &sel : _selectionBuffer) {
						_sceneMgr->nodeRemoveKeyFrame(sel.nodeId, sel.frameIdx);
					}
					_clearSelection = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::TooltipText(_("Delete %i keyframes"), (int)_selectionBuffer.size());
			}
			ImGui::EndPopup();
		}
	}
}

bool AnimationTimeline::update(const char *sequencerTitle, double deltaFrameSeconds) {
	scenegraph::FrameIndex currentFrame = _sceneMgr->currentFrame();
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const scenegraph::FrameIndex maxFrame = sceneGraph.maxFrames(sceneGraph.activeAnimation());
	if (_endFrame == -1) {
		_endFrame = core_max(64, maxFrame + 1);
	}
	_seconds += deltaFrameSeconds;

	if (_play) {
		if (maxFrame <= 0) {
			_play = false;
		} else {
			// TODO: anim fps
			currentFrame = (currentFrame + 1) % maxFrame;
			_sceneMgr->setCurrentFrame(currentFrame);
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
