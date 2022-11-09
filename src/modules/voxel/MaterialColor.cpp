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

Palette &getPalette() {
	if (!_priv::globalPalette.hasValue()) {
		Palette palette;
		const core::String &defaultPalette =
			core::Var::get(cfg::VoxelPalette, voxel::Palette::getDefaultPaletteName())->strVal();
		if (!palette.load(defaultPalette.c_str())) {
			palette.nippon();
		}
		_priv::globalPalette.setValue(palette);
	}
	return *_priv::globalPalette.value();
}

} // namespace voxel
