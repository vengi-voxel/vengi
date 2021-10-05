/**
 * @file
 */

#include "NoisePanel.h"
#include "voxedit-util/SceneManager.h"
#include "ui/imgui/IMGUI.h"

namespace voxedit {

void NoisePanel::update(const char *title) {
	if (ImGui::Begin(title)) {
		core_trace_scoped(NoisePanel);
		ImGui::InputInt("Octaves##noise", &_noiseData.octaves);
		ImGui::InputFloat("Frequency##noise", &_noiseData.frequency);
		ImGui::InputFloat("Lacunarity##noise", &_noiseData.lacunarity);
		ImGui::InputFloat("Gain##noise", &_noiseData.gain);

		if (ImGui::Button(ICON_FA_CHECK " OK##noise")) {
			sceneMgr().noise(_noiseData.octaves, _noiseData.lacunarity, _noiseData.frequency, _noiseData.gain,
							 voxelgenerator::noise::NoiseType::ridgedMF);
		}
	}
	ImGui::End();
}

}
