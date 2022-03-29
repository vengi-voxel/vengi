/**
 * @file
 */

#include "AnimationTimeline.h"
#include "core/ArrayLength.h"
#include "core/collection/DynamicArray.h"
#include "ui/imgui/dearimgui/imgui_neo_sequencer.h"
#include "ui/imgui/dearimgui/imgui.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

void AnimationTimeline::update(const char *sequencerTitle) {
	const EditMode editMode = sceneMgr().editMode();
	uint32_t currentFrame = sceneMgr().currentFrame();
	if (editMode == EditMode::Scene) {
		if (ImGui::Begin(sequencerTitle, nullptr, ImGuiWindowFlags_NoSavedSettings)) {
			uint32_t startFrame = 0;
			uint32_t endFrame = 64;
			if (ImGui::BeginNeoSequencer("##neo-sequencer", &currentFrame, &startFrame, &endFrame)) {
				const voxelformat::SceneGraph &sceneGraph = sceneMgr().sceneGraph();
				for (voxelformat::SceneGraphNode &modelNode : sceneGraph) {
					core::DynamicArray<uint32_t *> keys;
					keys.reserve(modelNode.keyFrames().size());
					for (voxelformat::SceneGraphKeyFrame &kf : modelNode.keyFrames()) {
						keys.push_back(&kf.frame);
					}
					if (ImGui::BeginNeoTimeline(modelNode.name().c_str(), keys.data(), keys.size(), nullptr)) {
						sceneMgr().setCurrentFrame(currentFrame);
						ImGui::EndNeoTimeLine();
					}
				}
				ImGui::EndNeoSequencer();
			}
			ImGui::End();
		}
	}
}

} // namespace voxedit
