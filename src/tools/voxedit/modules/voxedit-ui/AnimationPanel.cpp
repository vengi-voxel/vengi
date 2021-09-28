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
			static int animation = (int)animation::Animation::IDLE;
			if (ImGui::Combo("Animation", &animation,
					[](void *pmds, int idx, const char **pOut) {
						*pOut = network::EnumNameAnimation((animation::Animation)idx);
						return **pOut != '\0';
					},
					(void *)network::EnumNamesAnimation(), (int)network::Animation::MAX + 1, 10)) {
				sceneMgr().animationEntity().setAnimation((animation::Animation)animation, true);
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
