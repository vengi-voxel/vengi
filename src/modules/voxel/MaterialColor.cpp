/**
 * @file
 */

#include "MaterialColor.h"
#include "core/Optional.h"
#include "core/Var.h"
#include "palette/Palette.h"

namespace voxel {

namespace _priv {
static core::Optional<palette::Palette> globalPalette;
}

void initPalette(const palette::Palette &palette) {
	_priv::globalPalette.setValue(palette);
}

bool hasPalette() {
	return _priv::globalPalette.hasValue();
}

palette::Palette &getPalette() {
	if (!hasPalette()) {
		palette::Palette palette;
		const core::VarPtr &var = core::Var::get(cfg::VoxelPalette, palette::Palette::getDefaultPaletteName());
		const core::String &defaultPalette = var->strVal();
		if (!palette.load(defaultPalette.c_str())) {
			palette.nippon();
			var->setVal(palette::Palette::getDefaultPaletteName());
		}
		_priv::globalPalette.setValue(palette);
	}
	return *_priv::globalPalette.value();
}

} // namespace voxel
