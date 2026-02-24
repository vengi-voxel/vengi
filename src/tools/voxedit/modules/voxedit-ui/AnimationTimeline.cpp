/**
 * @file
 */

#include "AnimationTimeline.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/dearimgui/imgui_neo_sequencer.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void AnimationTimeline::header(scenegraph::FrameIndex currentFrame, scenegraph::FrameIndex maxFrame) {
	if (ImGui::DisabledIconButton(ICON_LC_PLUS, _("Add"), _animationPlaying->boolVal())) {
		_sceneMgr->nodeAddKeyFrame(InvalidNodeId, currentFrame);
	}
	ImGui::TooltipTextUnformatted(_("Add a new keyframe to the current active node"));
	ImGui::SameLine();
	if (ImGui::DisabledIconButton(ICON_LC_SQUARE_PLUS, _("Add all"), _animationPlaying->boolVal())) {
		_sceneMgr->nodeAllAddKeyFrames(currentFrame);
	}
	ImGui::TooltipTextUnformatted(_("Add a new keyframe to all nodes"));
	ImGui::SameLine();
	if (ImGui::DisabledIconButton(ICON_LC_SQUARE_MINUS, _("Delete"), _animationPlaying->boolVal())) {
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

	if (_animationPlaying->boolVal()) {
		if (ImGui::Button(ICON_LC_PAUSE)) {
			_animationPlaying->setVal(false);
		}
	} else {
		if (ImGui::DisabledButton(ICON_LC_PLAY, maxFrame <= 0)) {
			_animationPlaying->setVal(true);
			_frameTimeSeconds = 0.0;
			if (!_loop && currentFrame >= maxFrame) {
				_sceneMgr->setCurrentFrame(0);
			}
		}
		ImGui::TooltipText(_("Max frames for this animation: %i"), maxFrame);
	}

	ImGui::SameLine();
	ImGui::Checkbox(_("Loop"), &_loop);

	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::Size(5));
	if (ImGui::InputDouble(_("FPS"), &_fps, 0.0, 0.0, "%.0f")) {
		_frameTimeSeconds = 0.0;
	}
}

void AnimationTimeline::timelineEntry(scenegraph::FrameIndex currentFrame, core::Buffer<Selection> &selectionBuffer,
									  core::Buffer<scenegraph::FrameIndex> &selectedFrames,
									  const scenegraph::SceneGraphNode &node) {
	const core::String &label = core::String::format("%s###node-%i", node.name().c_str(), node.id());
	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const int activeNode = sceneGraph.activeNode();
	if (ImGui::BeginNeoTimelineEx(label.c_str(), nullptr, ImGuiNeoTimelineFlags_AllowFrameChanging)) {
		for (scenegraph::SceneGraphKeyFrame &kf : node.keyFrames()) {
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
		if (activeNode != _lastActivedNodeId && node.id() == activeNode) {
			// TODO: UI: doesn't work - see issue https://github.com/vengi-voxel/vengi/issues/437
			ImGui::SetScrollHereY();
			_lastActivedNodeId = activeNode;
		}
		if (ImGui::IsNeoTimelineSelected(ImGuiNeoTimelineIsSelectedFlags_NewlySelected)) {
			_sceneMgr->nodeActivate(node.id());
			_lastActivedNodeId = node.id();
		} else if (activeNode == node.id()) {
			ImGui::SetSelectedTimeline(label.c_str());
		}
		uint32_t selectionCount = ImGui::GetNeoKeyframeSelectionSize();
		if (selectionCount > 0) {
			selectedFrames.clear();
			selectedFrames.resize(selectionCount);
			ImGui::GetNeoKeyframeSelection(selectedFrames.data());
			for (uint32_t i = 0; i < selectionCount; ++i) {
				selectionBuffer.push_back(Selection{selectedFrames[i], node.id()});
			}
		}
		ImGui::EndNeoTimeLine();
	}
}

bool AnimationTimeline::init() {
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	_lastActivedNodeId = sceneGraph.activeNode();
	_animationPlaying = core::getVar(cfg::VoxEditAnimationPlaying);
	return true;
}

void AnimationTimeline::sequencer(scenegraph::FrameIndex &currentFrame) {
	ImGuiNeoSequencerFlags flags = ImGuiNeoSequencerFlags_AlwaysShowHeader;
	flags |= ImGuiNeoSequencerFlags_EnableSelection;
	flags |= ImGuiNeoSequencerFlags_AllowLengthChanging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDragging;
	flags |= ImGuiNeoSequencerFlags_Selection_EnableDeletion;

	const scenegraph::FrameIndex frame = currentFrame;
	if (ImGui::BeginNeoSequencer("sequencer", &currentFrame, &_startFrame, &_endFrame, {0, 0}, flags)) {
		core::Buffer<Selection> selectionBuffer;
		if (_clearSelection) {
			ImGui::NeoClearSelection();
			_clearSelection = false;
		}
		core::Buffer<scenegraph::FrameIndex> selectedFrames;
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		for (auto entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (!node.isAnyModelNode() && !node.isCameraNode() && !node.isPointNode()) {
				continue;
			}
			timelineEntry(currentFrame, selectionBuffer, selectedFrames, node);
		}
		bool selectionRightClicked = ImGui::IsNeoKeyframeSelectionRightClicked();
		// check if current frame was changed by dragging the handle
		if (frame != currentFrame) {
			_sceneMgr->setCurrentFrame(currentFrame);
		}
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

bool AnimationTimeline::update(const char *id, double deltaFrameSeconds) {
	core_trace_scoped(AnimationTimeline);
	scenegraph::FrameIndex currentFrame = _sceneMgr->currentFrame();
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const scenegraph::FrameIndex maxFrame = sceneGraph.maxFrames();
	if (_endFrame == -1) {
		_endFrame = core_max(64, maxFrame + 1);
	}

	if (_animationPlaying->boolVal()) {
		_frameTimeSeconds += deltaFrameSeconds;
		if (maxFrame <= 0) {
			_animationPlaying->setVal(false);
		} else {
			const double targetFrameDuration = 1.0 / _fps;
			if (_frameTimeSeconds > targetFrameDuration) {
				++currentFrame;
				if (currentFrame > maxFrame) {
					if (_loop) {
						currentFrame = 0;
					} else {
						currentFrame = maxFrame;
						_animationPlaying->setVal(false);
					}
				}
				if (_animationPlaying->boolVal()) {
					_sceneMgr->setCurrentFrame(currentFrame);
					_frameTimeSeconds -= targetFrameDuration;
				} else {
					_frameTimeSeconds = 0.0;
				}
			}
		}
	}
	const core::String title = makeTitle(ICON_LC_TABLE, _("Animation"), id);
	if (ImGui::Begin(title.c_str())) {
		header(currentFrame, maxFrame);
		sequencer(currentFrame);
	}
	ImGui::End();
	return true;
}

} // namespace voxedit
