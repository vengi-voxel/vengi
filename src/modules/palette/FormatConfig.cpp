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
	const core::VarDef coreColorReduction(
		cfg::CoreColorReduction, color::toColorReductionTypeString(color::ColorReductionType::MedianCut),
		{color::toColorReductionTypeString(color::ColorReductionType::Octree),
		 color::toColorReductionTypeString(color::ColorReductionType::Wu),
		 color::toColorReductionTypeString(color::ColorReductionType::MedianCut),
		 color::toColorReductionTypeString(color::ColorReductionType::KMeans),
		 color::toColorReductionTypeString(color::ColorReductionType::NeuQuant)},
		N_("Color reduction"), N_("Controls the algorithm that is used to perform the color reduction"));
	static_assert((int)color::ColorReductionType::Max == 5, "Please update the color reduction options");
	core::Var::registerVar(coreColorReduction);

	const core::VarDef palformatRGB6Bit(
		cfg::PalformatRGB6Bit, false, N_("6 bit colors"),
		N_("Use 6 bit color values for the palette (0-63) - used e.g. in C&C pal files"), core::CV_NOPERSIST);
	core::Var::registerVar(palformatRGB6Bit);
	const core::VarDef palformatMaxSize(
		cfg::PalformatMaxSize, 512, N_("Max image size"),
		N_("The maximum size of an image in x and y direction to quantize to a palette"), core::CV_NOPERSIST);
	core::Var::registerVar(palformatMaxSize);
	const core::VarDef palformatGimpRGBA(cfg::PalformatGimpRGBA, false, N_("GIMP RGBA"),
										 N_("Use RGBA format for GIMP palettes (instead of RGB / Aseprite extension)"),
										 core::CV_NOPERSIST);
	core::Var::registerVar(palformatGimpRGBA);

	const core::VarDef voxelPalette(
		cfg::VoxelPalette, palette::Palette::getDefaultPaletteName(), N_("Palette"),
		N_("This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)"));
	core::Var::registerVar(voxelPalette);
	const core::VarDef normalPalette(cfg::NormalPalette, palette::NormalPalette::getDefaultPaletteName(),
									 N_("Normal palette"),
									 NC_("A palette of normal vectors converted to RGBA values", "The normal palette"));
	core::Var::registerVar(normalPalette);

	return true;
}

} // namespace palette
