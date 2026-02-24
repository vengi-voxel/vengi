/**
 * @file
 */

#include "FormatConfig.h"
#include "app/I18N.h"
#include "color/Color.h"
#include "color/Quantize.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"

namespace palette {

static bool colorReductionValidator(const core::String &value) {
	return color::toColorReductionType(value.c_str()) != color::ColorReductionType::Max;
}

bool FormatConfig::init() {
	const core::VarDef coreColorReduction(cfg::CoreColorReduction,
				   color::toColorReductionTypeString(color::ColorReductionType::MedianCut), core::CV_NONE,
				   N_("Color reduction"), N_("Controls the algorithm that is used to perform the color reduction"), colorReductionValidator);
	core::Var::registerVar(coreColorReduction);

	const core::VarDef palformatRGB6Bit(cfg::PalformatRGB6Bit, false, core::CV_NOPERSIST,
				   N_("6 bit colors"), N_("Use 6 bit color values for the palette (0-63) - used e.g. in C&C pal files"));
	core::Var::registerVar(palformatRGB6Bit);
	const core::VarDef palformatMaxSize(cfg::PalformatMaxSize, 512, core::CV_NOPERSIST,
				   N_("Max image size"), N_("The maximum size of an image in x and y direction to quantize to a palette"));
	core::Var::registerVar(palformatMaxSize);
	const core::VarDef palformatGimpRGBA(cfg::PalformatGimpRGBA, false, core::CV_NOPERSIST,
				   N_("GIMP RGBA"), N_("Use RGBA format for GIMP palettes (instead of RGB / Aseprite extension)"));
	core::Var::registerVar(palformatGimpRGBA);

	const core::VarDef voxelPalette(cfg::VoxelPalette, palette::Palette::getDefaultPaletteName(), core::CV_NONE,
				   N_("Palette"), N_("This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)"));
	core::Var::registerVar(voxelPalette);
	const core::VarDef normalPalette(cfg::NormalPalette, palette::NormalPalette::getDefaultPaletteName(), core::CV_NONE,
				   N_("Normal palette"), NC_("A palette of normal vectors converted to RGBA values", "The normal palette"));
	core::Var::registerVar(normalPalette);

	return true;
}

} // namespace palette
