#include "NoiseType.h"
#include <SDL.h>

static const char *NoiseTypeStr[] = {
	"simplex noise",
	"ridged noise",
	"flow noise (rot. gradients)",
	"fractal brownian motion sum",
	"fbm",
	"fbm cascade",
	"fbm analytical derivatives",
	"flow noise fbm (time)",
	"ridged multi fractal",
	"ridged multi fractal cascade",
	"ridged multi fractal scaled",
	"iq noise",
	"iq noise scaled",
	"analytical derivatives",
	"noise curl noise (time)",
	"voronoi"
};
static_assert((int)SDL_arraysize(NoiseTypeStr) == (int)NoiseType::Max, "String array size doesn't match noise types");

const char *getNoiseTypeName(NoiseType t) {
	return NoiseTypeStr[(int)t];
}
