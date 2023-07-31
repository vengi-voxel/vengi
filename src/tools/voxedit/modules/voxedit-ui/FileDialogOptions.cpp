/**
 * @file
 */

#include "FileDialogOptions.h"
#include "core/GameConfig.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "voxelformat/VolumeFormat.h"

void fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc) {
	if (mode == video::OpenFileMode::Directory) {
		return;
	}
	if (desc == nullptr) {
		return;
	}

	const bool forceApplyOptions = (desc->flags & FORMAT_FLAG_ALL) == FORMAT_FLAG_ALL;
	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (forceApplyOptions || meshFormat) {
		ImGui::InputVarFloat("Uniform scale", cfg::VoxformatScale);
		ImGui::InputVarFloat("X axis scale", cfg::VoxformatScaleX);
		ImGui::InputVarFloat("Y axis scale", cfg::VoxformatScaleY);
		ImGui::InputVarFloat("Z axis scale", cfg::VoxformatScaleZ);

		if (mode == video::OpenFileMode::Save) {
			ImGui::CheckboxVar("Merge quads", cfg::VoxformatMergequads);
			ImGui::CheckboxVar("Reuse vertices", cfg::VoxformatReusevertices);
			ImGui::CheckboxVar("Ambient occlusion", cfg::VoxformatAmbientocclusion);
			ImGui::CheckboxVar("Apply transformations", cfg::VoxformatTransform);
			ImGui::CheckboxVar("Exports quads", cfg::VoxformatQuads);
			ImGui::CheckboxVar("Vertex colors", cfg::VoxformatWithColor);
			ImGui::BeginDisabled(core::Var::get(cfg::VoxformatWithColor)->boolVal());
			ImGui::CheckboxVar("Vertex colors as float", cfg::VoxformatColorAsFloat);
			ImGui::EndDisabled();
			ImGui::CheckboxVar("Texture coordinates", cfg::VoxformatWithtexcoords);
		} else if (mode == video::OpenFileMode::Open) {
			ImGui::CheckboxVar("Fill hollow", cfg::VoxformatFillHollow);
		}
	}

	if (mode == video::OpenFileMode::Save) {
		if (forceApplyOptions || !meshFormat) {
			ImGui::CheckboxVar("Single object", cfg::VoxformatMerge);
		}
		if (forceApplyOptions || *desc == voxelformat::qubicleBinaryTree()) {
			ImGui::CheckboxVar("Palette mode", cfg::VoxformatQBTPaletteMode);
			ImGui::CheckboxVar("Merge compounds", cfg::VoxformatQBTMergeCompounds);
		}
		if (forceApplyOptions || *desc == voxelformat::magicaVoxel()) {
			ImGui::CheckboxVar("Create groups", cfg::VoxformatVOXCreateGroups);
			ImGui::CheckboxVar("Create layers", cfg::VoxformatVOXCreateLayers);
		}
		if (forceApplyOptions || *desc == voxelformat::qubicleBinary()) {
			ImGui::CheckboxVar("Left handed", cfg::VoxformatQBSaveLeftHanded);
		}
		if (forceApplyOptions || *desc == voxelformat::tiberianSun()) {
			const char *normalTypes[] = {nullptr, nullptr, "Tiberian Sun", nullptr, "Red Alert"};
			const core::VarPtr &normalTypeVar = core::Var::getSafe(cfg::VoxformatVXLNormalType);
			const int currentNormalType = normalTypeVar->intVal();

			if (ImGui::BeginCombo("Normal type", normalTypes[currentNormalType])) {
				for (int i = 0; i < lengthof(normalTypes); ++i) {
					const char *normalType = normalTypes[i];
					if (normalType == nullptr) {
						continue;
					}
					const bool selected = i == currentNormalType;
					if (ImGui::Selectable(normalType, selected)) {
						normalTypeVar->setVal(core::string::toString(i));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
	} else {
		ImGui::InputVarInt("RGB flatten factor", cfg::VoxformatRGBFlattenFactor);
		ImGui::CheckboxVar("Create palette", cfg::VoxelCreatePalette);
	}
}
