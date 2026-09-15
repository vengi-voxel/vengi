/**
 * @file
 */

#include "FileDialogOptions.h"
#include "ScenePreview.h"
#include "IMGUIApp.h"
#include "IconsLucide.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/FormatDescription.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteCache.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/private/GimpPalette.h"
#include "palette/private/RGBPalette.h"
#include "ui/IMGUIEx.h"
#include "video/FileDialogOptions.h"
#include "video/OpenFileMode.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/image/PNGFormat.h"
#include "voxelutil/ImageUtils.h"

#include <string.h>

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

	ImGui::TextUnformatted(desc->name.c_str());
	ImGui::Separator();

	bool hasOptions;
	if (_palette) {
		hasOptions = paletteOptions(mode, desc);
	} else {
		if (mode == video::OpenFileMode::Save) {
			hasOptions = saveOptions(desc, entry);
		} else {
			hasOptions = loadOptions(desc, entry, _paletteCache);
		}
	}
	return hasOptions;
}

video::FileDialogOptions FileDialogOptions::build(palette::PaletteCache &paletteCache, bool palette,
												  ScenePreview *preview) {
	video::FileDialogOptions result;
	result.options = FileDialogOptions(paletteCache, palette);
	if (preview != nullptr && !palette) {
		result.preview = [preview](const io::FilesystemEntry &entry, video::OpenFileMode mode,
								   const io::FormatDescription *desc) {
			preview->renderForFileDialog(entry, mode, desc);
		};
	}
	return result;
}

bool paletteOptions(video::OpenFileMode mode, const io::FormatDescription *desc) {
	if (desc == nullptr) {
		return false;
	}

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

static bool hasValueTitles(const voxelformat::FormatVarMeta &meta) {
	for (int i = 0; i < voxelformat::FormatVarMeta::MaxValueTitles; ++i) {
		if (meta.valueTitles[i] != nullptr && meta.valueTitles[i][0] != '\0') {
			return true;
		}
	}
	return false;
}

static bool skipHiddenCVar(const char *name, bool save) {
	if (save) {
		return false;
	}
	const core::VarPtr &imageType = core::getVar(cfg::VoxformatImageImportType);
	const int current = imageType ? imageType->intVal() : 0;
	if (!strcmp(name, cfg::VoxformatImageVolumeMaxDepth) ||
		!strcmp(name, cfg::VoxformatImageVolumeBothSides)) {
		return current != voxelformat::PNGFormat::ImageType::Volume;
	}
	if (!strcmp(name, cfg::VoxformatImageHeightmapMinHeight)) {
		return current != voxelformat::PNGFormat::ImageType::Heightmap;
	}
	if (!strcmp(name, cfg::VoxelPalette)) {
		const core::VarPtr &createPalette = core::getVar(cfg::VoxelCreatePalette);
		return createPalette && createPalette->boolVal();
	}
	return false;
}

static void renderPaletteCombo(const core::VarPtr &var, const palette::PaletteCache &paletteCache) {
	if (ImGui::BeginCombo(_("Map colors to palette"), var->strVal().c_str(), 0)) {
		for (const core::String &palette : paletteCache.availablePalettes()) {
			if (ImGui::Selectable(palette.c_str(), palette == var->strVal())) {
				var->setVal(palette);
			}
		}
		ImGui::EndCombo();
	}
}

static void renderNormalPaletteCombo(const core::VarPtr &var) {
	if (ImGui::BeginCombo(_("Normal palette"), var->strVal().c_str(), 0)) {
		for (const char *palette : palette::NormalPalette::builtIn) {
			if (ImGui::Selectable(palette, palette == var->strVal())) {
				var->setVal(palette);
			}
		}
		ImGui::EndCombo();
		// TODO: allow other normal palettes to be loaded
	}
}

static void renderGenericCVar(const core::VarPtr &var) {
	switch (var->type()) {
	case core::VarType::Boolean:
		ImGui::CheckboxVar(var);
		break;
	case core::VarType::Int:
		ImGui::InputVarInt(var);
		break;
	case core::VarType::Float:
		ImGui::InputVarFloat(var);
		break;
	case core::VarType::Enum:
		ImGui::ComboVar(var);
		break;
	case core::VarType::Directory:
		ImGui::InputFolderVar(var);
		break;
	case core::VarType::Path:
		ImGui::InputFileVar(var, nullptr);
		break;
	default:
		ImGui::InputVarString(var);
		break;
	}
}

static void extraAfterCVar(const char *name, bool save, const io::FilesystemEntry &entry) {
	if (save && !strcmp(name, cfg::VoxformatImageSaveType)) {
		const core::VarPtr &imageTypeVar = core::getVar(cfg::VoxformatImageSaveType);
		if (imageTypeVar && imageTypeVar->intVal() == voxelformat::PNGFormat::ImageType::Plane) {
			ImGui::SeparatorText(_("Layer information"));
			ImGui::IconDialog(ICON_LC_INFO, _("This is saving several images as layers per object.\n\n"
											"The name of the files will include the uuid of the node\n"
											"and the z layer index."));
		}
	}
	if (!save && !strcmp(name, cfg::VoxformatImageImportType)) {
		const core::VarPtr &imageTypeVar = core::getVar(cfg::VoxformatImageImportType);
		if (imageTypeVar && imageTypeVar->intVal() == voxelformat::PNGFormat::ImageType::Volume &&
			!entry.fullPath.empty()) {
			const core::String depthMapName = voxelutil::getDefaultDepthMapFile(entry.fullPath);
			if (io::filesystem()->exists(depthMapName)) {
				ImGui::Text(_("Depth map: %s"), depthMapName.c_str());
			} else {
				core::String fileName = core::string::extractFilenameWithExtension(depthMapName);
				ImGui::Text(_("Depth map not found: %s"), fileName.c_str());
				ImGui::TooltipTextUnformatted(depthMapName.c_str());
			}
		}
	}
}

static void renderFormatCVar(const voxelformat::FormatVarMeta &meta, bool save, const io::FormatDescription &desc,
							const io::FilesystemEntry &entry, const palette::PaletteCache *paletteCache) {
	if (skipHiddenCVar(meta.name, save)) {
		return;
	}
	const core::VarPtr &var = core::getVar(meta.name);
	if (!var) {
		return;
	}

	const bool supportsQuads = voxelformat::FormatConfig::meshSaveSupportsQuads(desc);
	const bool supportsColor = voxelformat::FormatConfig::meshSaveSupportsColor(desc);
	const bool supportsTexCoords = voxelformat::FormatConfig::meshSaveSupportsTexCoords(desc);

	bool disable = false;
	if (!strcmp(meta.name, cfg::VoxformatScale) || !strcmp(meta.name, cfg::VoxformatScaleX) ||
		!strcmp(meta.name, cfg::VoxformatScaleY) || !strcmp(meta.name, cfg::VoxformatScaleZ)) {
		const core::VarPtr &voxelSize = core::getVar(cfg::VoxformatVoxelSize);
		disable = voxelSize && voxelSize->intVal() > 0;
	} else if (!strcmp(meta.name, cfg::VoxformatMeshSimplifyRatio)) {
		const core::VarPtr &optimize = core::getVar(cfg::VoxformatOptimize);
		disable = !optimize || !optimize->boolVal();
	} else if (!strcmp(meta.name, cfg::VoxformatColorAsFloat)) {
		const core::VarPtr &withColor = core::getVar(cfg::VoxformatWithColor);
		disable = !supportsColor || !withColor || !withColor->boolVal();
	} else if (!strcmp(meta.name, cfg::VoxformatQuads)) {
		disable = !supportsQuads;
	} else if (!strcmp(meta.name, cfg::VoxformatWithColor)) {
		disable = !supportsColor;
	} else if (!strcmp(meta.name, cfg::VoxformatWithtexcoords)) {
		disable = !supportsTexCoords;
	} else if (!strcmp(meta.name, cfg::VoxformatVoxelizeChunkSize)) {
		const core::VarPtr &chunked = core::getVar(cfg::VoxformatVoxelizeChunked);
		disable = !chunked || !chunked->boolVal();
	}

	ImGui::BeginDisabled(disable);
	if (!strcmp(meta.name, cfg::VoxelPalette) && paletteCache != nullptr) {
		renderPaletteCombo(var, *paletteCache);
	} else if (!strcmp(meta.name, cfg::NormalPalette)) {
		renderNormalPaletteCombo(var);
	} else if (hasValueTitles(meta)) {
		ImGui::ComboVar(var, meta.valueTitles, voxelformat::FormatVarMeta::MaxValueTitles);
	} else {
		renderGenericCVar(var);
	}
	ImGui::EndDisabled();
	extraAfterCVar(meta.name, save, entry);
}

static bool assembleOptions(bool save, const io::FormatDescription *desc, const io::FilesystemEntry &entry,
							const palette::PaletteCache *paletteCache) {
	if (desc == nullptr) {
		return false;
	}
	const voxelformat::FormatVarMeta *cvars = voxelformat::FormatConfig::varsMeta();
	const int count = voxelformat::FormatConfig::cvarCount();
	for (int i = 0; i < count; ++i) {
		if (!voxelformat::FormatConfig::appliesTo(cvars[i], save, *desc)) {
			continue;
		}
		renderFormatCVar(cvars[i], save, *desc, entry, paletteCache);
	}
	return true;
}

bool saveOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry) {
	return assembleOptions(true, desc, entry, nullptr);
}

bool loadOptions(const io::FormatDescription *desc, const io::FilesystemEntry &entry,
				 const palette::PaletteCache &paletteCache) {
	return assembleOptions(false, desc, entry, &paletteCache);
}

void meshModeOption() {
	comboVar(cfg::VoxformatMeshMode);
}

bool comboVar(const char *varName, const char *titlesFromVar) {
	const voxelformat::FormatVarMeta *meta =
		voxelformat::FormatConfig::findVarMeta(titlesFromVar != nullptr ? titlesFromVar : varName);
	if (meta == nullptr || !hasValueTitles(*meta)) {
		return false;
	}
	return ImGui::ComboVar(varName, meta->valueTitles, voxelformat::FormatVarMeta::MaxValueTitles);
}

} // namespace voxelui
