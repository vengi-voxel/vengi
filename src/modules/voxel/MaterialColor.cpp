/**
 * @file
 */

#include "MaterialColor.h"
#include "app/I18N.h"
#include "core/Optional.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "core/concurrent/Lock.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"

namespace voxel {

namespace _priv {
core::Optional<palette::Palette> globalPalette;
core::Optional<palette::NormalPalette> globalNormalPalette;
core_trace_mutex(core::Lock, _lock, "MaterialColor");
} // namespace _priv

static bool hasNormalPalette() {
	return _priv::globalNormalPalette.hasValue();
}

palette::NormalPalette &getNormalPalette() {
	core::ScopedLock lock(_priv::_lock);
	if (!hasNormalPalette()) {
		palette::NormalPalette normalPalette;
		const core::VarDef normalPaletteDef(cfg::NormalPalette, palette::NormalPalette::getDefaultPaletteName(),
											N_("Normal palette"),
											N_("A palette of normal vectors converted to RGBA values"));
		const core::VarPtr &var = core::Var::registerVar(normalPaletteDef);
		const core::String &defaultNormalPalette = var->strVal();
		if (!normalPalette.load(defaultNormalPalette.c_str())) {
			normalPalette.redAlert2();
			var->setVal(palette::NormalPalette::getDefaultPaletteName());
		}
		_priv::globalNormalPalette.setValue(normalPalette);
	}
	return *_priv::globalNormalPalette.value();
}

static bool hasPalette() {
	return _priv::globalPalette.hasValue();
}

palette::Palette &getPalette() {
	core::ScopedLock lock(_priv::_lock);
	if (!hasPalette()) {
		palette::Palette palette;
		const core::VarDef voxelPaletteDef(cfg::VoxelPalette, palette::Palette::getDefaultPaletteName(),
										   N_("Voxel palette"), N_("A palette of voxel colors"));
		const core::VarPtr &var = core::Var::registerVar(voxelPaletteDef);
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
