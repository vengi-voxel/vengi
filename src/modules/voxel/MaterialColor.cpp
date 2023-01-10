/**
 * @file
 */

#include "MaterialColor.h"
#include "core/Optional.h"
#include "core/Var.h"
#include "voxel/Palette.h"

namespace voxel {

namespace _priv {
static core::Optional<voxel::Palette> globalPalette;
}

void initPalette(const Palette &palette) {
	_priv::globalPalette.setValue(palette);
}

bool hasPalette() {
	return _priv::globalPalette.hasValue();
}

Palette &getPalette() {
	if (!hasPalette()) {
		Palette palette;
		const core::VarPtr &var = core::Var::get(cfg::VoxelPalette, voxel::Palette::getDefaultPaletteName());
		const core::String &defaultPalette = var->strVal();
		if (!palette.load(defaultPalette.c_str())) {
			palette.nippon();
			var->setVal(voxel::Palette::getDefaultPaletteName());
		}
		_priv::globalPalette.setValue(palette);
	}
	return *_priv::globalPalette.value();
}

} // namespace voxel
