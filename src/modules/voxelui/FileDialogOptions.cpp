/**
 * @file
 */

#include "FileDialogOptions.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/FormatDescription.h"
#include "palette/PaletteCache.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/private/GimpPalette.h"
#include "palette/private/RGBPalette.h"
#include "ui/IMGUIEx.h"
#include "video/FileDialogOptions.h"
#include "video/OpenFileMode.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "voxelformat/private/image/AsepriteFormat.h"
#include "voxelformat/private/image/PNGFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/gis/GMLFormat.h"
#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "voxelformat/private/minecraft/SkinFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"
#include "voxelutil/ImageUtils.h"

namespace voxelui {

FileDialogOptions::FileDialogOptions(palette::PaletteCache &paletteCache, bool palette)
	: _paletteCache(paletteCache), _palette(palette) {
}

bool FileDialogOptions::operator()(video::OpenFileMode mode, const io::FormatDescription *desc,
								   const io::FilesystemEntry &entry) {
	if (mode == video::OpenFileMode::Directory) {
		return false;
	}

	// maybe we've manually specified a file extension that is different from the
	// given description - in that case we try to detect it.
	if (desc == nullptr || !desc->matchesExtension(core::string::extractExtension(entry.name))) {
		const io::FormatDescription *formats;
		if (mode == video::OpenFileMode::Save) {
			if (_palette) {
				formats = palette::palettes();
			} else {
				formats = voxelformat::voxelSave();
			}
		} else {
			if (_palette) {
				formats = palette::palettes();
			} else {
				formats = voxelformat::voxelLoad();
			}
		}
		const io::FormatDescription *descByName = io::getDescription(entry.name, 0, formats);
		if (descByName != nullptr) {
			desc = descByName;
		}
	}
	if (desc == nullptr) {
		return false;
	}

	bool hasOptions;
	if (_palette) {
		hasOptions = paletteOptions(mode, desc);
	} else {
		hasOptions = genericOptions(desc);
		if (mode == video::OpenFileMode::Save) {
			hasOptions |= saveOptions(desc, entry);
		} else {
			hasOptions |= loadOptions(desc, entry, _paletteCache);
		}
	}
	return hasOptions;
}

video::FileDialogOptions FileDialogOptions::build(palette::PaletteCache &paletteCache, bool palette) {
	FileDialogOptions options(paletteCache, palette);
	return options;
}

bool paletteOptions(video::OpenFileMode mode, const io::FormatDescription *desc) {
	if (desc == nullptr) {
		return false;
	}
	ImGui::TextUnformatted(desc->name.c_str());
	ImGui::Separator();

	if (*desc == palette::RGBPalette::format()) {
		ImGui::CheckboxVar(cfg::PalformatRGB6Bit);
		return true;
	}
	if (mode == video::OpenFileMode::Save && *desc == palette::GimpPalette::format()) {
		ImGui::CheckboxVar(cfg::PalformatGimpRGBA);
	}
	imguiApp()->colorReductionOptions();
	return false;
}

bool genericOptions(const io::FormatDescription *desc) {
	if (desc == nullptr) {
		return false;
	}
	ImGui::TextUnformatted(desc->name.c_str());
	ImGui::Separator();

	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		ImGui::InputVarFloat(cfg::VoxformatScale);
		ImGui::InputVarFloat(cfg::VoxformatScaleX);
		ImGui::InputVarFloat(cfg::VoxformatScaleY);
		ImGui::InputVarFloat(cfg::VoxformatScaleZ);
		return true;
	}
	return false;
}

static int genericPngOptions(bool load, const core::VarPtr &imageTypeVar) {
	const char *imageTypes[] = {_("Plane"), _("Heightmap"), _("Volume"), _("Thumbnail")};
	static_assert(voxelformat::PNGFormat::ImageType::Thumbnail == 3, "Thumbnail must be at index 3");
	static_assert(voxelformat::PNGFormat::ImageType::Volume == 2, "Volume must be at index 2");
	static_assert(voxelformat::PNGFormat::ImageType::Heightmap == 1, "Heightmap must be at index 1");
	static_assert(voxelformat::PNGFormat::ImageType::Plane == 0, "Plane must be at index 0");
	const int currentImageType = imageTypeVar->intVal();

	if (ImGui::BeginCombo(_("Image mode"), imageTypes[currentImageType])) {
		for (int i = 0; i < lengthof(imageTypes); ++i) {
			if (i == voxelformat::PNGFormat::ImageType::Thumbnail && load) {
				// Thumbnails are only available for saving
				continue;
			}
			const char *imageType = imageTypes[i];
			if (imageType == nullptr) {
				continue;
			}
			const bool selected = i == currentImageType;
			if (ImGui::Selectable(imageType, selected)) {
				imageTypeVar->setVal(core::string::toString(i));
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	return currentImageType;
}

static void saveOptionsPng(const io::FilesystemEntry &entry) {
	const core::VarPtr &imageTypeVar = core::getVar(cfg::VoxformatImageSaveType);
	const int currentImageType = genericPngOptions(false, imageTypeVar);

	if (currentImageType == voxelformat::PNGFormat::ImageType::Plane) {
		ImGui::SeparatorText(_("Layer information"));
		const core::String basename = core::string::extractFilename(entry.name);
		ImGui::IconDialog(ICON_LC_INFO, _("This is saving several images as layers per object.\n\n"
										"The name of the files will include the uuid of the node\n"
										"and the z layer index."));
	}
}

static void saveOptionsMesh(const io::FormatDescription *desc) {
	ImGui::CheckboxVar(cfg::VoxformatMergequads);
	ImGui::CheckboxVar(cfg::VoxformatReusevertices);
	ImGui::CheckboxVar(cfg::VoxformatAmbientocclusion);
	ImGui::CheckboxVar(cfg::VoxformatTransform);
	ImGui::CheckboxVar(cfg::VoxformatOptimize);
	ImGui::CheckboxVar(cfg::VoxformatQuads);
	ImGui::CheckboxVar(cfg::VoxformatWithColor);
	ImGui::CheckboxVar(cfg::VoxformatWithNormals);
	ImGui::BeginDisabled(!core::getVar(cfg::VoxformatWithColor)->boolVal());
	ImGui::CheckboxVar(cfg::VoxformatColorAsFloat);
	ImGui::EndDisabled();
	ImGui::CheckboxVar(cfg::VoxformatWithtexcoords);
	if (*desc == voxelformat::GLTFFormat::format()) {
		ImGui::CheckboxVar(						   cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness);
		ImGui::CheckboxVar(cfg::VoxformatGLTF_KHR_materials_specular);
	}
	ImGui::CheckboxVar(cfg::VoxformatWithMaterials);

	voxelui::meshModeOption();
}

bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	if (desc == nullptr) {
		return false;
	}
	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		saveOptionsMesh(desc);
	}

	if (*desc == voxelformat::BinVoxFormat::format()) {
		const char *binvoxVersions[] = {_("Binvox 1 (white)"), _("Binvox 2 (multi colors)"), _("Binvox 3 (unofficial)")};
		const core::VarPtr &binvoxVersion = core::getVar(cfg::VoxformatBinvoxVersion);
		if (ImGui::BeginCombo(_("Binvox version"), binvoxVersion->strVal().c_str())) {
			for (int i = 0; i < lengthof(binvoxVersions); ++i) {
				const bool selected = binvoxVersion->intVal() == i + 1;
				if (ImGui::Selectable(binvoxVersions[i], selected)) {
					binvoxVersion->setVal(i + 1);
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
	}

	if (*desc == voxelformat::SchematicFormat::format()) {
		ImGui::ComboVar(cfg::VoxformatSchematicType);
	}

	ImGui::CheckboxVar(cfg::VoxformatMerge);
	ImGui::CheckboxVar(cfg::VoxformatSaveVisibleOnly);

	if (*desc == voxelformat::QBTFormat::format()) {
		ImGui::CheckboxVar(cfg::VoxformatQBTPaletteMode);
		ImGui::CheckboxVar(cfg::VoxformatQBTMergeCompounds);
	}

	if (*desc == voxelformat::VoxFormat::format()) {
		ImGui::CheckboxVar(cfg::VoxformatVOXCreateGroups);
		ImGui::CheckboxVar(cfg::VoxformatVOXCreateLayers);
	}

	if (*desc == voxelformat::QBFormat::format()) {
		ImGui::CheckboxVar(cfg::VoxformatQBSaveLeftHanded);
		ImGui::CheckboxVar(cfg::VoxformatQBSaveCompressed);
	}

	if (*desc == io::format::png()) {
		saveOptionsPng(entry);
	}

	if (*desc == voxelformat::VENGIFormat::format()) {
		ImGui::InputVarInt(cfg::VoxformatEmptyPaletteIndex);
	}

	return true;
}

static void loadOptionsAseprite(const io::FilesystemEntry &entry) {
	ImGui::InputVarInt(cfg::VoxformatImageSliceOffset);
	ImGui::ComboVar(cfg::VoxformatImageSliceOffsetAxis);
}

static void loadOptionsPng(const io::FilesystemEntry &entry) {
	const core::VarPtr &imageTypeVar = core::getVar(cfg::VoxformatImageImportType);
	const int currentImageType = genericPngOptions(true, imageTypeVar);

	if (currentImageType == voxelformat::PNGFormat::ImageType::Volume) {
		ImGui::InputVarInt(cfg::VoxformatImageVolumeMaxDepth);
		ImGui::CheckboxVar(cfg::VoxformatImageVolumeBothSides);
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
	} else if (currentImageType == voxelformat::PNGFormat::ImageType::Volume) {
		ImGui::InputVarInt(cfg::VoxformatImageHeightmapMinHeight);
	}
}

static void loadOptionsMesh(const io::FormatDescription *desc) {
	ImGui::InputVarString(cfg::VoxformatTexturePath);
	ImGui::CheckboxVar(cfg::VoxformatFillHollow);
	ImGui::InputVarInt(cfg::VoxformatPointCloudSize);
	ImGui::CheckboxVar(cfg::VoxformatMeshSimplify);

	const core::VarPtr &normalPaletteVar = core::getVar(cfg::NormalPalette);
	if (ImGui::BeginCombo(_("Normal palette"), normalPaletteVar->strVal().c_str(), 0)) {
		for (const char *palette : palette::NormalPalette::builtIn) {
			if (ImGui::Selectable(palette, palette == normalPaletteVar->strVal())) {
				normalPaletteVar->setVal(palette);
			}
		}
		ImGui::EndCombo();
		// TODO: allow other normal palettes to be loaded
	}


	if (*desc == voxelformat::GMLFormat::format()) {
		const core::VarPtr &regionVar = core::getVar(cfg::VoxformatGMLRegion);
		ImGui::InputVarString(regionVar);
		ImGui::TooltipTextUnformatted(
			_("Only load objects that intersect with the given region. The coordinates are in the order of minX minY "
			  "minZ maxX maxY maxZ. They are in world coordinates of the given GML file, so you might need to check "
			  "the envelope of the file first to determine the correct region."));
		ImGui::InputVarString(core::getVar(cfg::VoxformatGMLFilenameFilter));
	}

	const char *voxelizationModes[] = {_("high quality"), _("faster and less memory")};
	static_assert(voxelformat::MeshFormat::VoxelizeMode::HighQuality == 0, "HighQuality must be at index 0");
	static_assert(voxelformat::MeshFormat::VoxelizeMode::Fast == 1, "Fast must be at index 1");
	const core::VarPtr &voxelizationVar = core::getVar(cfg::VoxformatVoxelizeMode);
	const int currentVoxelizationMode = voxelizationVar->intVal();

	if (ImGui::BeginCombo(_("Voxelization mode"), voxelizationModes[currentVoxelizationMode])) {
		for (int i = 0; i < lengthof(voxelizationModes); ++i) {
			const char *type = voxelizationModes[i];
			if (type == nullptr) {
				continue;
			}
			const bool selected = i == currentVoxelizationMode;
			if (ImGui::Selectable(type, selected)) {
				voxelizationVar->setVal(i);
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::CheckboxVar(cfg::VoxformatRGBWeightedAverage);
}

static void loadOptionsGeneric(const io::FormatDescription *desc, const io::FilesystemEntry &entry,
							   const palette::PaletteCache &paletteCache) {
	if (voxelformat::isRGBFormat(*desc) || voxelformat::isMeshFormat(*desc)) {
		imguiApp()->colorReductionOptions();
		ImGui::InputVarInt(cfg::VoxformatRGBFlattenFactor);
		ImGui::InputVarInt(cfg::VoxformatTargetColors);
	}
	const core::VarPtr &createPalette = core::getVar(cfg::VoxelCreatePalette);
	ImGui::CheckboxVar(createPalette);
	if (!createPalette->boolVal()) {
		core::VarPtr paletteVar = core::getVar(cfg::VoxelPalette);
		if (ImGui::BeginCombo(_("Map colors to palette"), paletteVar->strVal().c_str(), 0)) {
			for (const core::String &palette : paletteCache.availablePalettes()) {
				if (ImGui::Selectable(palette.c_str(), palette == paletteVar->strVal())) {
					paletteVar->setVal(palette);
				}
			}
			ImGui::EndCombo();
		}
	}
	if (*desc == palette::RGBPalette::format()) {
		ImGui::CheckboxVar(cfg::PalformatRGB6Bit);
		ImGui::InputVarInt(cfg::PalformatMaxSize);
	}
}

static void loadOptionsMinecraftSkin(const io::FilesystemEntry &entry) {
	ImGui::CheckboxVar(cfg::VoxformatSkinApplyTransform);
	ImGui::CheckboxVar(cfg::VoxformatSkinAddGroups);
	ImGui::CheckboxVar(cfg::VoxformatSkinMergeFaces);
}

bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry,
				 const palette::PaletteCache &paletteCache) {
	if (desc == nullptr) {
		return false;
	}

	const bool meshFormat = voxelformat::isMeshFormat(*desc);
	if (meshFormat) {
		loadOptionsMesh(desc);
	}

	if (*desc == io::format::png()) {
		loadOptionsPng(entry);
	}

	if (*desc == voxelformat::AsepriteFormat::format()) {
		loadOptionsAseprite(entry);
	}

	if (*desc == voxelformat::SkinFormat::format()) {
		loadOptionsMinecraftSkin(entry);
	}

	if (*desc == voxelformat::VXLFormat::format()) {
		ImGui::CheckboxVar(cfg::VoxformatVXLLoadHVA);
	}

	loadOptionsGeneric(desc, entry, paletteCache);
	return true;
}

void meshModeOption() {
	static const core::Array<core::String, (int)voxel::SurfaceExtractionType::Max> meshModes = {
		_("Cubes"), _("Marching cubes"), _("Binary"), _("Greedy texture")};
	static_assert(4 == (int)voxel::SurfaceExtractionType::Max, "Invalid amount of mesh modes");
	ImGui::ComboVar(cfg::VoxformatMeshMode, meshModes);
}

} // namespace voxelui
