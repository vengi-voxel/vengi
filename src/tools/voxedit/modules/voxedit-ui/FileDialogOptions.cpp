/**
 * @file
 */

#include "FileDialogOptions.h"
#include "core/GameConfig.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
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
		ImGui::InputVarFloat(_("Uniform scale"), cfg::VoxformatScale);
		ImGui::InputVarFloat(_("X axis scale"), cfg::VoxformatScaleX);
		ImGui::InputVarFloat(_("Y axis scale"), cfg::VoxformatScaleY);
		ImGui::InputVarFloat(_("Z axis scale"), cfg::VoxformatScaleZ);

		if (mode == video::OpenFileMode::Save) {
			ImGui::CheckboxVar(_("Merge quads"), cfg::VoxformatMergequads);
			ImGui::CheckboxVar(_("Reuse vertices"), cfg::VoxformatReusevertices);
			ImGui::CheckboxVar(_("Ambient occlusion"), cfg::VoxformatAmbientocclusion);
			ImGui::CheckboxVar(_("Apply transformations"), cfg::VoxformatTransform);
			ImGui::CheckboxVar(_("Exports quads"), cfg::VoxformatQuads);
			ImGui::CheckboxVar(_("Vertex colors"), cfg::VoxformatWithColor);
			ImGui::CheckboxVar(_("Normals"), cfg::VoxformatWithNormals);
			ImGui::BeginDisabled(!core::Var::get(cfg::VoxformatWithColor)->boolVal());
			ImGui::CheckboxVar(_("Vertex colors as float"), cfg::VoxformatColorAsFloat);
			ImGui::EndDisabled();
			ImGui::CheckboxVar(_("Texture coordinates"), cfg::VoxformatWithtexcoords);
			if (*desc == voxelformat::gltf()) {
				ImGui::CheckboxVar("KHR_materials_pbrSpecularGlossiness", cfg::VoxFormatGLTF_KHR_materials_pbrSpecularGlossiness);
				ImGui::CheckboxVar("KHR_materials_specular", cfg::VoxFormatGLTF_KHR_materials_specular);
			}
			ImGui::CheckboxVar(_("Export materials"), cfg::VoxFormatWithMaterials);
		} else if (mode == video::OpenFileMode::Open) {
			ImGui::CheckboxVar(_("Fill hollow"), cfg::VoxformatFillHollow);
			ImGui::InputVarInt(_("Point cloud size"), cfg::VoxformatPointCloudSize);

			const char *voxelizationModes[] = {_("high quality"), _("faster and less memory")};
			const core::VarPtr &voxelizationVar = core::Var::getSafe(cfg::VoxformatVoxelizeMode);
			const int currentVoxelizationMode = voxelizationVar->intVal();

			if (ImGui::BeginCombo(_("Voxelization mode"), voxelizationModes[currentVoxelizationMode])) {
				for (int i = 0; i < lengthof(voxelizationModes); ++i) {
					const char *type = voxelizationModes[i];
					if (type == nullptr) {
						continue;
					}
					const bool selected = i == currentVoxelizationMode;
					if (ImGui::Selectable(type, selected)) {
						voxelizationVar->setVal(core::string::toString(i));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
	}

	if (mode == video::OpenFileMode::Save) {
		if (forceApplyOptions || !meshFormat) {
			ImGui::CheckboxVar(_("Single object"), cfg::VoxformatMerge);
		}
		ImGui::CheckboxVar(_("Save visible only"), cfg::VoxformatSaveVisibleOnly);
		if (forceApplyOptions || *desc == voxelformat::qubicleBinaryTree()) {
			ImGui::CheckboxVar(_("Palette mode"), cfg::VoxformatQBTPaletteMode);
			ImGui::CheckboxVar(_("Merge compounds"), cfg::VoxformatQBTMergeCompounds);
		}
		if (forceApplyOptions || *desc == voxelformat::magicaVoxel()) {
			ImGui::CheckboxVar(_("Create groups"), cfg::VoxformatVOXCreateGroups);
			ImGui::CheckboxVar(_("Create layers"), cfg::VoxformatVOXCreateLayers);
		}
		if (forceApplyOptions || *desc == voxelformat::qubicleBinary()) {
			ImGui::CheckboxVar(_("Left handed"), cfg::VoxformatQBSaveLeftHanded);
			ImGui::CheckboxVar(_("Compressed"), cfg::VoxformatQBSaveCompressed);
		}
		if (forceApplyOptions || *desc == voxelformat::tiberianSun()) {
			const char *normalTypes[] = {nullptr, nullptr, _("Tiberian Sun"), nullptr, _("Red Alert")};
			const core::VarPtr &normalTypeVar = core::Var::getSafe(cfg::VoxformatVXLNormalType);
			const int currentNormalType = normalTypeVar->intVal();

			if (ImGui::BeginCombo(_("Normal type"), normalTypes[currentNormalType])) {
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
		ImGui::InputVarInt(_("RGB flatten factor"), cfg::VoxformatRGBFlattenFactor);
		ImGui::CheckboxVar(_("RGB weighted average"), cfg::VoxformatRGBWeightedAverage);
		ImGui::CheckboxVar(_("Create palette"), cfg::VoxelCreatePalette);
	}
}
