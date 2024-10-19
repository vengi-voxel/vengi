/**
 * @file
 */

#include "FileDialogOptions.h"
#include "core/GameConfig.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"
#include "voxelformat/VolumeFormat.h"

void fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	if (mode == video::OpenFileMode::Directory) {
		return;
	}
	if (desc == nullptr) {
		return;
	}

	if (mode == video::OpenFileMode::Save) {
		ImGui::Text(_("Save as: %s"), entry.name.c_str());
		ImGui::TooltipTextUnformatted(entry.fullPath.c_str());
	} else {
		ImGui::Text(_("Open: %s"), entry.name.c_str());
		ImGui::TooltipTextUnformatted(entry.fullPath.c_str());
	}

	const bool allSupportedFormats = (desc->flags & FORMAT_FLAG_ALL) == FORMAT_FLAG_ALL;
	const core::String extension = core::string::extractExtension(entry.name);
	if (allSupportedFormats) {
		ImGui::TextUnformatted(_("Showing all options because you've selected all supported formats"));
		ImGui::Separator();
	} else {
		if (!desc->matchesExtension(extension)) {
			ImGui::Text(_("Warning: Selected file type does not match extension %s: %s"), extension.c_str(),
						desc->name.c_str());
			ImGui::Separator();
		}
	}

	if (allSupportedFormats || *desc == io::format::palPalette()) {
		ImGui::CheckboxVar(_("Command & Conquer palette"), cfg::PalformatRGB6Bit);
	}

	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (allSupportedFormats || meshFormat) {
		ImGui::InputVarFloat(_("Uniform scale"), cfg::VoxformatScale);
		ImGui::InputVarFloat(_("X axis scale"), cfg::VoxformatScaleX);
		ImGui::InputVarFloat(_("Y axis scale"), cfg::VoxformatScaleY);
		ImGui::InputVarFloat(_("Z axis scale"), cfg::VoxformatScaleZ);

		if (mode == video::OpenFileMode::Save) {
			ImGui::CheckboxVar(_("Merge quads"), cfg::VoxformatMergequads);
			ImGui::CheckboxVar(_("Reuse vertices"), cfg::VoxformatReusevertices);
			ImGui::CheckboxVar(_("Ambient occlusion"), cfg::VoxformatAmbientocclusion);
			ImGui::CheckboxVar(_("Apply transformations"), cfg::VoxformatTransform);
			ImGui::CheckboxVar(_("Apply optimizations"), cfg::VoxformatOptimize);
			ImGui::CheckboxVar(_("Exports quads"), cfg::VoxformatQuads);
			ImGui::CheckboxVar(_("Vertex colors"), cfg::VoxformatWithColor);
			ImGui::CheckboxVar(_("Normals"), cfg::VoxformatWithNormals);
			ImGui::BeginDisabled(!core::Var::get(cfg::VoxformatWithColor)->boolVal());
			ImGui::CheckboxVar(_("Vertex colors as float"), cfg::VoxformatColorAsFloat);
			ImGui::EndDisabled();
			ImGui::CheckboxVar(_("Texture coordinates"), cfg::VoxformatWithtexcoords);
			if (*desc == voxelformat::gltf()) {
				ImGui::CheckboxVar("KHR_materials_pbrSpecularGlossiness",
								   cfg::VoxFormatGLTF_KHR_materials_pbrSpecularGlossiness);
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
		if (allSupportedFormats || !meshFormat) {
			ImGui::CheckboxVar(_("Single object"), cfg::VoxformatMerge);
			ImGui::SliderVarInt(_("Empty palette index"), cfg::VoxformatEmptyPaletteIndex, -1,
								palette::PaletteMaxColors);
		}
		ImGui::CheckboxVar(_("Save visible only"), cfg::VoxformatSaveVisibleOnly);
		if (allSupportedFormats || *desc == voxelformat::qubicleBinaryTree()) {
			ImGui::CheckboxVar(_("Palette mode"), cfg::VoxformatQBTPaletteMode);
			ImGui::CheckboxVar(_("Merge compounds"), cfg::VoxformatQBTMergeCompounds);
		}
		if (allSupportedFormats || *desc == voxelformat::magicaVoxel()) {
			ImGui::CheckboxVar(_("Create groups"), cfg::VoxformatVOXCreateGroups);
			ImGui::CheckboxVar(_("Create layers"), cfg::VoxformatVOXCreateLayers);
		}
		if (allSupportedFormats || *desc == voxelformat::qubicleBinary()) {
			ImGui::CheckboxVar(_("Left handed"), cfg::VoxformatQBSaveLeftHanded);
			ImGui::CheckboxVar(_("Compressed"), cfg::VoxformatQBSaveCompressed);
		}
	} else {
		if (allSupportedFormats || *desc == io::format::png()) {
			const char *importTypes[] = {_("Plane"), _("Heightmap"), _("Volume")};
			const core::VarPtr &importTypeVar = core::Var::getSafe(cfg::VoxformatImageImportType);
			const int currentImportType = importTypeVar->intVal();

			if (ImGui::BeginCombo(_("Import type"), importTypes[currentImportType])) {
				for (int i = 0; i < lengthof(importTypes); ++i) {
					const char *importType = importTypes[i];
					if (importType == nullptr) {
						continue;
					}
					const bool selected = i == currentImportType;
					if (ImGui::Selectable(importType, selected)) {
						importTypeVar->setVal(core::string::toString(i));
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			if (currentImportType == 2) {
				ImGui::InputVarInt(_("Max depth"), cfg::VoxformatImageVolumeMaxDepth);
				ImGui::CheckboxVar(_("Both sides"), cfg::VoxformatImageVolumeBothSides);
			}
		}
		ImGui::InputVarInt(_("RGB flatten factor"), cfg::VoxformatRGBFlattenFactor);
		ImGui::CheckboxVar(_("RGB weighted average"), cfg::VoxformatRGBWeightedAverage);
		ImGui::CheckboxVar(_("Create palette"), cfg::VoxelCreatePalette);
	}
}
