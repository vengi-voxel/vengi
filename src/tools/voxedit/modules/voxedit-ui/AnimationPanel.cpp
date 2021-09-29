/**
 * @file
 */

#include "AnimationPanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

namespace voxedit {

void AnimationPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, 0)) {
		if (sceneMgr().editMode() == EditMode::Animation) {
			static animation::Animation animation = animation::Animation::IDLE;
			if (ImGui::BeginCombo("Animation", network::EnumNameAnimation(animation), ImGuiComboFlags_None)) {
				for (int i = ((int)animation::Animation::MIN); i <= (int)animation::Animation::MAX; ++i) {
					const animation::Animation type = (animation::Animation)i;
					bool selected = type == animation;
					if (ImGui::Selectable(network::EnumNameAnimation(type), selected)) {
						animation = type;
						sceneMgr().animationEntity().setAnimation(type, true);
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			animation::SkeletonAttribute *skeletonAttributes = sceneMgr().skeletonAttributes();
			for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
				const animation::SkeletonAttributeMeta& meta = *metaIter;
				float *v = (float*)(((uint8_t*)skeletonAttributes) + meta.offset);
				ImGui::InputFloat(meta.name, v);
			}
		} else {
			ImGui::TextDisabled("No animation loaded");
			ImGui::NewLine();
			if (ImGui::Button("Load Animation")) {
				command::executeCommands("animation_load", &listener);
			}
		}
	}
	ImGui::End();
}

}
