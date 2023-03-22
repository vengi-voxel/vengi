/**
 * @file
 */

#include "AnimationPanel.h"
#include "IconsForkAwesome.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "command/CommandHandler.h"
#include "voxedit-util/SceneManager.h"
#include "ui/IMGUIEx.h"

namespace voxedit {

void AnimationPanel::update(const char *title, command::CommandExecutionListener &listener) {
	const scenegraph::SceneGraphAnimationIds &animations = sceneMgr().sceneGraph().animations();
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		ImGui::InputText("##nameanimationpanel", &_newAnimation);
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_PLUS " Add##animationpanel")) {
			if (!sceneMgr().addAnimation(_newAnimation)) {
				Log::error("Failed to add animation %s", _newAnimation.c_str());
			} else {
				_newAnimation = "";
			}
		}

		const core::String& currentAnimation = sceneMgr().sceneGraph().activeAnimation();
		if (ImGui::BeginCombo("Animation##animationpanel", currentAnimation.c_str())) {
			for (const core::String &animation : animations) {
				const bool isSelected = currentAnimation == animation;
				if (ImGui::Selectable(animation.c_str(), isSelected)) {
					if (!sceneMgr().setAnimation(animation)) {
						Log::error("Failed to activate animation %s", animation.c_str());
					}
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FK_MINUS " Delete##animationpanel")) {
			if (!sceneMgr().removeAnimation(currentAnimation)) {
				Log::error("Failed to remove animation %s", currentAnimation.c_str());
			}
		}
	}
	ImGui::End();
}

}
