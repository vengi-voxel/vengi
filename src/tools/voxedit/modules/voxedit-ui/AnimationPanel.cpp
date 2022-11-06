/**
 * @file
 */

#include "AnimationPanel.h"
#include "core/Trace.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUIEx.h"

namespace voxedit {

void AnimationPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, 0)) {
		core_trace_scoped(AnimationPanel);
		const core::DynamicArray<core::String> &animations = sceneMgr().sceneGraph().animations();
		for (const core::String& animation : animations) {
			// TODO: allow to select it
			ImGui::TextDisabled("%s", animation.c_str());
		}
	}
	ImGui::End();
}

}
