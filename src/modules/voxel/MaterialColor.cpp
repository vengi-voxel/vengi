/**
 * @file
 */

#include "MaterialColor.h"
#include "core/Optional.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "palette/Palette.h"

namespace voxel {

namespace _priv {
core::Optional<palette::Palette> globalPalette;
core::Lock _lock;
} // namespace _priv

static bool hasPalette() {
	return _priv::globalPalette.hasValue();
}

palette::Palette &getPalette() {
	core::ScopedLock lock(_priv::_lock);
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
