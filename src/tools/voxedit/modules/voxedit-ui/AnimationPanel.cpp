/**
 * @file
 */

#include "AnimationPanel.h"
#include "command/CommandHandler.h"
#include "core/Log.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/AnimationTimeline.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool AnimationPanel::init() {
	_popupCreateAnimation = core::Var::getSafe(cfg::VoxEditPopupCreateAnimation);
	return true;
}

void AnimationPanel::registerPopups() {
	if (_popupCreateAnimation->boolVal()) {
		ImGui::OpenPopup(POPUP_TITLE_CREATE_ANIMATION);
		_popupCreateAnimation->setVal("false");
	}

	popupCreateAnimation();
}

void AnimationPanel::popupCreateAnimation() {
	const core::String title = makeTitle(_("Create animation"), POPUP_TITLE_CREATE_ANIMATION);
	if (ImGui::BeginPopupModal(title.c_str(), nullptr,
							   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
		bool renamed = ImGui::InputText(_("Name"), &_newAnimation, flags);

		// TODO: ANIMATION: Allow to create empty animation

		if (ImGui::BeginCombo(_("Animation"), _selectedAnimation.c_str())) {
			const scenegraph::SceneGraphAnimationIds &animations = _sceneMgr->sceneGraph().animations();
			for (const core::String &animation : animations) {
				const bool isSelected = _selectedAnimation == animation;
				if (ImGui::Selectable(animation.c_str(), isSelected)) {
					_selectedAnimation = animation;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if (ImGui::Button(_("Apply")) || renamed) {
			if (!_sceneMgr->duplicateAnimation(_selectedAnimation, _newAnimation)) {
				Log::error("Failed to add animation %s", _newAnimation.c_str());
			} else {
				_newAnimation = "";
			}
			_animationTimeline->resetFrames();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(_("Close"))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void AnimationPanel::update(const char *id, command::CommandExecutionListener &listener,
							AnimationTimeline *animationTimeline) {
	core_trace_scoped(AnimationPanel);
	const core::String title = makeTitle(ICON_LC_LAYOUT_LIST, _("Animation"), id);
	scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const scenegraph::SceneGraphAnimationIds &animations = sceneGraph.animations();
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (ImGui::IconButton(ICON_LC_PLUS, _("Add new animation"))) {
			_selectedAnimation = _sceneMgr->sceneGraph().activeAnimation();
			command::executeCommands("toggle ve_popupcreateanimation", &listener);
		}

		const core::String &currentAnimation = sceneGraph.activeAnimation();
		if (ImGui::BeginCombo(_("Animation"), currentAnimation.c_str())) {
			for (const core::String &animation : animations) {
				const bool isSelected = currentAnimation == animation;
				if (ImGui::Selectable(animation.c_str(), isSelected)) {
					if (!_sceneMgr->setAnimation(animation)) {
						Log::error("Failed to activate animation %s", animation.c_str());
					}
					animationTimeline->resetFrames();
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::IconButton(ICON_LC_MINUS, _("Delete"))) {
			if (!_sceneMgr->removeAnimation(currentAnimation)) {
				Log::error("Failed to remove animation %s", currentAnimation.c_str());
			}
			animationTimeline->resetFrames();
		}
	}
	ImGui::End();
}

} // namespace voxedit
