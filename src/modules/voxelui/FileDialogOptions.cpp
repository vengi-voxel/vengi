/**
 * @file
 */

#include "FileDialogOptions.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#include "video/OpenFileMode.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelutil/ImageUtils.h"

bool fileDialogOptions(video::OpenFileMode mode, const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	if (mode == video::OpenFileMode::Directory) {
		return false;
	}

	// maybe we've manually specified a file extension that is different from the
	// given description - in that case we try to detect it.
	const io::FormatDescription *descByName = io::getDescription(
		entry.name, 0, mode == video::OpenFileMode::Save ? voxelformat::voxelSave() : voxelformat::voxelLoad());
	if (descByName != nullptr) {
		desc = descByName;
	}
	if (desc == nullptr) {
		return false;
	}

	bool hasOptions = genericOptions(desc);
	if (mode == video::OpenFileMode::Save) {
		hasOptions |= saveOptions(desc, entry);
	} else {
		hasOptions |= loadOptions(desc, entry);
	}
	return hasOptions;
}

bool genericOptions(const io::FormatDescription *desc) {
	if (desc == nullptr) {
		return false;
	}
	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		ImGui::InputVarFloat(_("Uniform scale"), cfg::VoxformatScale);
		ImGui::InputVarFloat(_("X axis scale"), cfg::VoxformatScaleX);
		ImGui::InputVarFloat(_("Y axis scale"), cfg::VoxformatScaleY);
		ImGui::InputVarFloat(_("Z axis scale"), cfg::VoxformatScaleZ);
		return true;
	}
	return false;
}

static void saveOptionsPng(const io::FilesystemEntry &entry) {
	ImGui::SeparatorText(_("Layer information"));
	const core::String basename = core::string::extractFilename(entry.name);
	ImGui::IconDialog(ICON_LC_INFO, _("This is saving several images as layers per object.\n\n"
									  "The name of the files will include the uuid of the node\n"
									  "and the z layer index."));
}

static void saveOptionsMesh(const io::FormatDescription *desc) {
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
	if (*desc == voxelformat::GLTFFormat::format()) {
		ImGui::CheckboxVar("KHR_materials_pbrSpecularGlossiness",
						   cfg::VoxFormatGLTF_KHR_materials_pbrSpecularGlossiness);
		ImGui::CheckboxVar("KHR_materials_specular", cfg::VoxFormatGLTF_KHR_materials_specular);
	}
	ImGui::CheckboxVar(_("Export materials"), cfg::VoxFormatWithMaterials);
}

bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	if (desc == nullptr) {
		return false;
	}
	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		saveOptionsMesh(desc);
	}

	ImGui::CheckboxVar(_("Single object"), cfg::VoxformatMerge);
	ImGui::CheckboxVar(_("Save visible only"), cfg::VoxformatSaveVisibleOnly);

	if (*desc == voxelformat::QBTFormat::format()) {
		ImGui::CheckboxVar(_("Palette mode"), cfg::VoxformatQBTPaletteMode);
		ImGui::CheckboxVar(_("Merge compounds"), cfg::VoxformatQBTMergeCompounds);
	}

	if (*desc == voxelformat::VoxFormat::format()) {
		ImGui::CheckboxVar(_("Create groups"), cfg::VoxformatVOXCreateGroups);
		ImGui::CheckboxVar(_("Create layers"), cfg::VoxformatVOXCreateLayers);
	}

	if (*desc == voxelformat::QBFormat::format()) {
		ImGui::CheckboxVar(_("Left handed"), cfg::VoxformatQBSaveLeftHanded);
		ImGui::CheckboxVar(_("Compressed"), cfg::VoxformatQBSaveCompressed);
	}

	if (*desc == io::format::png()) {
		saveOptionsPng(entry);
	}

	if (*desc == voxelformat::VENGIFormat::format()) {
		ImGui::InputVarInt(_("Empty palette index"), cfg::VoxformatEmptyPaletteIndex);
	}

	return true;
}

static void loadOptionsPng(const io::FilesystemEntry &entry) {
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
		if (!entry.fullPath.empty()) {
			const core::String depthMapName = voxelutil::getDefaultDepthMapFile(entry.fullPath);
			if (io::filesystem()->exists(depthMapName)) {
				ImGui::Text(_("Depth map: %s"), depthMapName.c_str());
			} else {
				core::String name = core::string::extractFilenameWithExtension(depthMapName);
				ImGui::Text(_("Depth map not found: %s"), name.c_str());
				ImGui::TooltipTextUnformatted(depthMapName.c_str());
			}
		}
	}
}

static void loadOptionsMesh() {
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

	ImGui::CheckboxVar(_("RGB weighted average"), cfg::VoxformatRGBWeightedAverage);
}

static void loadOptionsGeneric() {
	// TODO: only for non-palette formats
	imguiApp()->colorReductionOptions();
	ImGui::InputVarInt(_("RGB flatten factor"), cfg::VoxformatRGBFlattenFactor);
	ImGui::CheckboxVar(_("Create palette"), cfg::VoxelCreatePalette);
	// TODO: cfg::PalformatRGB6Bit
}

bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	if (desc == nullptr) {
		return false;
	}

	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		loadOptionsMesh();
	}

	if (*desc == io::format::png()) {
		loadOptionsPng(entry);
	}

	loadOptionsGeneric();
	return true;
}