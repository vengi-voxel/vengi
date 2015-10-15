#include "AccidentalNoise.h"

namespace noise {

// TODO: erosion: https://code.google.com/p/accidental-noise-library/wiki/ModuleBasics
#if 0
impad=anl.CImplicitBufferImplicitAdapter(turb, anl.SEAMLESS_NONE, anl.SMappingRanges(0,4,0,4,0,4), true, 0)
erode=anl.CImplicitBufferSimpleErode(impad, 0, 0.35)
blur=anl.CImplicitBufferBlur(erode, 3/256)
bump=anl.CImplicitBufferBumpMap(blur, -1.5, 3.5, 1.5, 0.5, false)
#endif
AccidentalNoise::AccidentalNoise() :
#if 0
	groundGradient(0.0, 0.0, 0.0, 1.0, 0.0, 0.0),

	plainFractal(anl::EFractalTypes::BILLOW, anl::GRADIENT, anl::QUINTIC, 1, 0.25, false),
	plainAutoCorrect(&plainFractal, 0.0, 1.0),
	plainScaleOffset(&plainAutoCorrect, 0.125, 0.3),
	plainScaleDomain(&plainScaleOffset, 1.0, 0.0),
	plainTranslateDomain(&groundGradient, 0.0, &plainScaleDomain, 0.0),

	highlandFractal(anl::EFractalTypes::FBM, anl::GRADIENT, anl::QUINTIC, 2, 0.5, false),
	highlandAutoCorrect(&highlandFractal, 0.0, 1.0),
	highlandScaleOffset(&highlandAutoCorrect, 0.15, 0.0),
	highlandScaleDomain(&highlandScaleOffset, 1.0, 0.0),
	highlandTranslateDomain(&groundGradient, 0.0, &highlandScaleDomain, 0.0),

	mountainFractal(anl::EFractalTypes::RIDGEDMULTI, anl::GRADIENT, anl::QUINTIC, 4, 0.8, false),
	mountainAutoCorrect(&mountainFractal, 0.0, 1.0),
	mountainScaleOffset(&mountainAutoCorrect, 1.5, -0.75),
	mountainScaleDomain(&mountainScaleOffset, 1.0, 0.5),
	mountainTranslateDomain(&groundGradient, 0.0, &mountainScaleDomain, 0.0),

	terrainControl(anl::EFractalTypes::BILLOW, anl::GRADIENT, anl::QUINTIC, 1, 0.05, false),
	terrainAutoCorrect(&terrainControl, 0.0, 1.0),
	terrainScaleOffset(&terrainAutoCorrect, 10.0, 0.0),
	terrainScaleDomain(&terrainScaleOffset, 1.0, 0.0),
	terrainCache(&terrainScaleDomain),

	highlandMountainSelect(&highlandTranslateDomain, &mountainTranslateDomain, &terrainCache, 9.0, 1.5),
	highlandMountainColorSelect(&highland, &mountain, &terrainCache, 9.0, 0.0),
	highlandMountainColorCache(&highlandMountainColorSelect),
	plainHighlandSelect(&plainTranslateDomain, &highlandMountainSelect, &terrainCache, 4.0, 1.5),
	plainHighlandColorSelect(&plain, &highlandMountainColorSelect, &terrainCache, 4.0, 0.0),

	plainHighlandCache(&plainHighlandSelect),

	groundBase()
#endif
	plainFractal(anl::EFractalTypes::FBM, anl::GRADIENT, anl::QUINTIC, 10, 1.0, false)
{
#if 0
	air.setConstant(0.0);
	plain.setConstant(1.0);
	highland.setConstant(2.0);
	mountain.setConstant(3.0);

	groundBase.setLowSource(&plainHighlandColorSelect);
	groundBase.setHighSource(&air);
	groundBase.setControlSource(&plainHighlandCache);
	groundBase.setThreshold(0.5);
#endif
}

void AccidentalNoise::setSeed(long seed) {
	anl::CMWC4096 rnd;
	rnd.setSeed(seed);
#if 0
	plainFractal.setSeed(rnd.get());
	highlandFractal.setSeed(rnd.get());
	mountainFractal.setSeed(rnd.get());
	terrainControl.setSeed(rnd.get());
#endif
}

}
