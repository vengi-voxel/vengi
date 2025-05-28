/**
 * @file
 * @brief Genland - procedural landscape generator
 * by Tom Dobrowolski (http://ged.ax.pl/~tomkh) (heightmap generator)
 * and Ken Silverman (https://advsys.net/ken) (DTA/PNG/VXL writers)
 *
 * If you do something cool, feel free to write us
 * (contact info can be found at our websites)
 *
 * License for this code:
 *    * No commercial exploitation please
 *    * Do not remove our names from the code or credits
 *    * You may distribute modified code/executables,
 *      but please make it clear that it is modified.
 * History:
 *    2005-12-24: Released GENLAND.EXE with Ken's GROUDRAW demos.
 *    2006-03-10: Released GENLAND.CPP source code
 *    2025-05: included in the voxelgenerator module and adapt to work with vengi
 */

#include "Genland.h"
#include "core/Common.h"
#include "core/Log.h"
#include "math/Random.h"
#include "noise/Noise.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/VolumeSampler.h"
#include "voxel/Voxel.h"

namespace voxelgenerator {

#define VSID 512

//----------------------------------------------------------------------------
// Noise algo based on "Improved Perlin Noise" by Ken Perlin
// http://mrl.nyu.edu/~perlin/

static CORE_FORCE_INLINE float fgrad(long h, float x, float y, float z) {
	switch (h) // h masked before call (h&15)
	{
	case 0:
		return (x + y);
	case 1:
		return (-x + y);
	case 2:
		return (x - y);
	case 3:
		return (-x - y);
	case 4:
		return (x + z);
	case 5:
		return (-x + z);
	case 6:
		return (x - z);
	case 7:
		return (-x - z);
	case 8:
		return (y + z);
	case 9:
		return (-y + z);
	case 10:
		return (y - z);
	case 11:
		return (-y - z);
	case 12:
		return (x + y);
	case 13:
		return (-x + y);
		// case 12: return(   y+z);
		// case 13: return(  -y+z);
	case 14:
		return (y - z);
	case 15:
		return (-y - z);
	}
	return (0);
}

static uint8_t noisep[512], noisep15[512];

static void noiseinit(math::Random &rand) {
	for (long i = 256 - 1; i >= 0; i--) {
		noisep[i] = i;
	}
	for (long i = 256 - 1; i > 0; i--) {
		const long n = rand.random(0, 32767);
		const long j = ((n * (i + 1)) >> 15);
		const long k = noisep[i];
		noisep[i] = noisep[j];
		noisep[j] = k;
	}
	for (long i = 256 - 1; i >= 0; i--) {
		noisep[i + 256] = noisep[i];
	}
	for (long i = 512 - 1; i >= 0; i--) {
		noisep15[i] = noisep[i] & 15;
	}
}

static double noise3d(double fx, double fy, double fz, long mask) {
	long i, l[6], a[4];
	float p[3], f[8];

	// if (mask > 255) mask = 255; //Checked before call
	l[0] = floor(fx);
	p[0] = fx - ((float)l[0]);
	l[0] &= mask;
	l[3] = (l[0] + 1) & mask;
	l[1] = floor(fy);
	p[1] = fy - ((float)l[1]);
	l[1] &= mask;
	l[4] = (l[1] + 1) & mask;
	l[2] = floor(fz);
	p[2] = fz - ((float)l[2]);
	l[2] &= mask;
	l[5] = (l[2] + 1) & mask;
	i = noisep[l[0]];
	a[0] = noisep[i + l[1]];
	a[2] = noisep[i + l[4]];
	i = noisep[l[3]];
	a[1] = noisep[i + l[1]];
	a[3] = noisep[i + l[4]];
	f[0] = fgrad(noisep15[a[0] + l[2]], p[0], p[1], p[2]);
	f[1] = fgrad(noisep15[a[1] + l[2]], p[0] - 1, p[1], p[2]);
	f[2] = fgrad(noisep15[a[2] + l[2]], p[0], p[1] - 1, p[2]);
	f[3] = fgrad(noisep15[a[3] + l[2]], p[0] - 1, p[1] - 1, p[2]);
	p[2]--;
	f[4] = fgrad(noisep15[a[0] + l[5]], p[0], p[1], p[2]);
	f[5] = fgrad(noisep15[a[1] + l[5]], p[0] - 1, p[1], p[2]);
	f[6] = fgrad(noisep15[a[2] + l[5]], p[0], p[1] - 1, p[2]);
	f[7] = fgrad(noisep15[a[3] + l[5]], p[0] - 1, p[1] - 1, p[2]);
	p[2]++;
	p[2] = (3.0 - 2.0 * p[2]) * p[2] * p[2];
	p[1] = (3.0 - 2.0 * p[1]) * p[1] * p[1];
	p[0] = (3.0 - 2.0 * p[0]) * p[0] * p[0];
	f[0] = (f[4] - f[0]) * p[2] + f[0];
	f[1] = (f[5] - f[1]) * p[2] + f[1];
	f[2] = (f[6] - f[2]) * p[2] + f[2];
	f[3] = (f[7] - f[3]) * p[2] + f[3];
	f[0] = (f[2] - f[0]) * p[1] + f[0];
	f[1] = (f[3] - f[1]) * p[1] + f[1];
	return ((f[1] - f[0]) * p[0] + f[0]);
}

static core::RGBA buf[VSID * VSID];	// 2d heightmap colors for writing out the voxels or the heightmap image
static core::RGBA amb[VSID * VSID];	// ambient
static float hgt[VSID * VSID];	// height values
static uint8_t sh[VSID * VSID]; // shadows

voxel::RawVolume *genland(GenlandSettings &settings) {
	constexpr double EPS = 0.1;
	core::Buffer<double> amplut;
	double samp[3], csamp[3];
	core::Buffer<long> msklut;

	if (settings.height >= 256) {
		Log::error("Height must be less than 256, got %d", settings.height);
		return nullptr;
	}

	math::Random rand;
	rand.setSeed(settings.seed);

	amplut.resize(settings.octaves);
	msklut.resize(settings.octaves);

	noiseinit(rand);

	// Tom's algorithm from 12/04/2005
	Log::debug("Generating landscape with seed %d, height %d, octaves %d", settings.seed, settings.height,
			   settings.octaves);
	double d;
	d = 1.0;
	for (int i = 0; i < settings.octaves; i++) {
		amplut[i] = d;
		d *= settings.persistence;
		msklut[i] = core_min((1 << (i + 2)) - 1, 255);
	}
	for (int k = 0, y = 0; y < VSID; y++) {
		for (int x = 0; x < VSID; x++, k++) {
			// Get 3 samples (0,0), (EPS,0), (0,EPS):
			for (int i = 0; i < 3; i++) {
				double dx = (x * (256.0 / (double)VSID) + (double)(i & 1) * EPS) * (1.0 / 64.0);
				double dy = (y * (256.0 / (double)VSID) + (double)(i >> 1) * EPS) * (1.0 / 64.0);
				d = 0;
				double river = 0;
				for (long o = 0; o < settings.octaves; o++) {
					d += noise3d(dx, dy, 9.5, msklut[o]) * amplut[o] * (d * 1.6 + 1.0); // multi-fractal
					river += noise3d(dx, dy, 13.2, msklut[o]) * amplut[o];
					dx *= 2;
					dy *= 2;
				}
				samp[i] = d * -20.0 + 28.0;
				d = sin(x * (glm::pi<double>() / 256.0) + river * 4.0) * (0.5 + settings.riverWidth) +
					(0.5 - settings.riverWidth);
				if (d > 1.0) {
					d = 1.0;
				}
				csamp[i] = samp[i] * d;
				if (d < 0.0) {
					d = 0.0;
				}
				samp[i] *= d;
				if (csamp[i] < samp[i]) {
					csamp[i] = -log(1.0 - csamp[i]); // simulate water normal ;)
				}
			}
			// Get normal using cross-product
			double nx = csamp[1] - csamp[0];
			double ny = csamp[2] - csamp[0];
			double nz = -EPS;
			d = 1.0 / sqrt(nx * nx + ny * ny + nz * nz);
			nx *= d;
			ny *= d;
			nz *= d;

			// Ground colors
			double gr = 140;
			double gg = 125;
			double gb = 115;
			// blend factor
			double g = core_min(core_max(core_max(-nz, 0) * 1.4 - csamp[0] / 32.0 +
											 noise3d(x * (1.0 / 64.0), y * (1.0 / 64.0), 0.3, 15) * 0.3,
										 0),
								1);
			gr += (72 - gr) * g;
			gg += (80 - gg) * g;
			gb += (32 - gb) * g; // Grass

			double g2 = (1 - fabs(g - 0.5) * 2) * 0.7;
			gr += (68 - gr) * g2;
			gg += (78 - gg) * g2;
			gb += (40 - gb) * g2; // Grass2

			g2 = core_max(core_min((samp[0] - csamp[0]) * 1.5, 1), 0);
			g = 1 - g2 * 0.2;
			gr += (60 * g - gr) * g2;
			gg += (100 * g - gg) * g2;
			gb += (120 * g - gb) * g2; // Water

			d = 0.3;
			amb[k].r = (uint8_t)core_min(core_max(gr * d, 0), 255);
			amb[k].g = (uint8_t)core_min(core_max(gg * d, 0), 255);
			amb[k].b = (uint8_t)core_min(core_max(gb * d, 0), 255);
			const uint8_t maxa = core_max(core_max(amb[k].r, amb[k].g), amb[k].b);

			// lighting
			d = (nx * 0.5 + ny * 0.25 - nz) / sqrt(0.5 * 0.5 + 0.25 * 0.25 + 1.0 * 1.0);
			d *= 1.2;
			buf[k].a = (uint8_t)(settings.height - samp[0]);
			buf[k].r = (uint8_t)core_min(core_max(gr * d, 0), 255 - maxa);
			buf[k].g = (uint8_t)core_min(core_max(gg * d, 0), 255 - maxa);
			buf[k].b = (uint8_t)core_min(core_max(gb * d, 0), 255 - maxa);

			hgt[k] = csamp[0];
		}
		Log::debug("%i percent done", (int)(((y + 1) * 100) / VSID));
	}

	Log::debug("Applying shadows");

	const int VSHL = glm::log2((float)VSID);

	// Shadows:
	core_memset(sh, 0, sizeof(sh));
	for (int k = 0, y = 0; y < VSID; y++) {
		for (int x = 0; x < VSID; x++, k++) {
			float f = hgt[k] + 0.44f;
			int i, j;
			for (i = j = 1; i < (VSID >> 2); j++, i++, f += 0.44f) {
				if (hgt[(((y - (j >> 1)) & (VSID - 1)) << VSHL) + ((x - i) & (VSID - 1))] > f) {
					sh[k] = 32;
					break;
				}
			}
		}
	}
	// for(i=2;i>0;i--) // smooth sh 2 times
	for (int y = 0, k = 0; y < VSID; y++) {
		for (int x = 0; x < VSID; x++, k++) {
			sh[k] = (sh[k] + sh[(((y + 1) & (VSID - 1)) << VSHL) + x] + sh[(y << VSHL) + ((x + 1) & (VSID - 1))] +
					 sh[(((y + 1) & (VSID - 1)) << VSHL) + ((x + 1) & (VSID - 1))] + 2) >>
					2;
		}
	}
	for (int y = 0, k = 0; y < VSID; y++) {
		for (int x = 0; x < VSID; x++, k++) {
			const int i = 256 - (sh[k] << 2);
			buf[k].r = ((buf[k].r * i) >> 8) + amb[k].r;
			buf[k].g = ((buf[k].g * i) >> 8) + amb[k].g;
			buf[k].b = ((buf[k].b * i) >> 8) + amb[k].b;
		}
	}

	palette::Palette palette;
	palette.nippon();
	palette::PaletteLookup paletteLookup(palette);

	const core::RGBA *heightmap = buf;
	core::Array<voxel::Voxel, 256> voxels;
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, VSID - 1, settings.height - 1, VSID - 1));
	for (int vz = 0; vz < VSID; ++vz) {
		for (int vx = 0; vx < VSID; ++vx, ++heightmap) {
			const int maxsy = glm::clamp((settings.height - 1) - (int)heightmap->a, 0, settings.height);
			if (maxsy == 0) {
				const core::RGBA color{heightmap->r, heightmap->g, heightmap->b, 255};
				const int palIdx = paletteLookup.findClosestIndex(color);
				volume->setVoxel(vx, 0, vz, voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
				continue;
			} else if (maxsy < 0) {
				continue;
			}
			const core::RGBA color{heightmap->r, heightmap->g, heightmap->b, 255};
			const int palIdx = paletteLookup.findClosestIndex(color);
			voxels.fill(voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
			voxel::setVoxels(*volume, vx, vz, voxels.data(), maxsy);
		}
	}

	return volume;
}

} // namespace voxelgenerator
