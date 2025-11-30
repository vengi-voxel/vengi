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
#include "app/Async.h"
#include "core/Common.h"
#include "core/Log.h"
#include "glm/ext/scalar_integer.hpp"
#include "math/Random.h"
#include "noise/Noise.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxel/Voxel.h"

namespace voxelgenerator {

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

static uint8_t noisep[512];
static uint8_t noisep15[512];

static void noiseinit(math::Random &rand) {
	for (long i = (lengthof(noisep) / 2) - 1; i >= 0; i--) {
		noisep[i] = i;
	}
	for (long i = (lengthof(noisep) / 2) - 1; i > 0; i--) {
		const long n = rand.random(0, 32767);
		const long j = ((n * (i + 1)) >> 15);
		const long k = noisep[i];
		noisep[i] = noisep[j];
		noisep[j] = k;
	}
	for (long i = (lengthof(noisep) / 2) - 1; i >= 0; i--) {
		noisep[i + (lengthof(noisep) / 2)] = noisep[i];
	}
	for (long i = lengthof(noisep15) - 1; i >= 0; i--) {
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

class TempBuffer {
private:
	const int _bufSize;

public:
	color::RGBA *buf; // 2d heightmap colors for writing out the voxels or the heightmap image
	color::RGBA *amb; // ambient
	float *hgt;		 // height values
	uint8_t *sh;	 // shadows
	TempBuffer(int size) : _bufSize(size * size) {
		buf = new color::RGBA[_bufSize];
		amb = new color::RGBA[_bufSize];
		hgt = new float[_bufSize];
		sh = new uint8_t[_bufSize];
	}
	~TempBuffer() {
		delete[] buf;
		delete[] amb;
		delete[] hgt;
		delete[] sh;
	}
	void clearShadow() {
		core_memset(sh, 0, sizeof(uint8_t) * _bufSize);
	}
};

voxel::RawVolume *genland(GenlandSettings &settings) {
	if (!glm::isPowerOfTwo(settings.size)) {
		Log::error("Size must be a power of two, got %d", settings.size);
		return nullptr;
	}

	if (settings.octaves < 1) {
		Log::error("Octaves must be at least 1, got %d", settings.octaves);
		return nullptr;
	}

	if (settings.height >= 256) {
		Log::error("Height must be less than 256, got %d", settings.height);
		return nullptr;
	}

	if (settings.offset[0] < 0) {
		Log::error("Offset X must be at least 0, got %d", settings.offset[0]);
		return nullptr;
	}
	if (settings.offset[1] < 0) {
		Log::error("Offset Y must be at least 0, got %d", settings.offset[1]);
		return nullptr;
	}

	TempBuffer tempBuffer(settings.size);
	math::Random rand;
	rand.setSeed(settings.seed);

	constexpr double EPS = 0.1;
	core::Buffer<double> amplut;
	core::Buffer<long> msklut;

	amplut.resize(settings.octaves);
	msklut.resize(settings.octaves);

	noiseinit(rand);

	// Tom's algorithm from 12/04/2005
	Log::debug("Generating landscape with seed %d, height %d, octaves %d", settings.seed, settings.height,
			   settings.octaves);
	double d;
	d = settings.amplitude;
	for (int i = 0; i < settings.octaves; i++) {
		amplut[i] = d;
		d *= settings.persistence;
		msklut[i] = core_min((1 << (i + 2)) - 1, 255);
	}

	const double freq = (1.0 / 64.0);
	// TODO: PERF: FOR_PARALLEL
	for (int z = 0; z < settings.size; z++) {
		for (int x = 0; x < settings.size; x++) {
			double samp[3];
			double csamp[3];
			// Get 3 samples (0,0), (EPS,0), (0,EPS):
			for (int i = 0; i < lengthof(samp); i++) {
				double dx = ((settings.offset[0] + x) * (256.0 / (double)(settings.offset[0] + settings.size)) + (double)(i & 1) * EPS) * freq;
				double dy = ((settings.offset[1] + z) * (256.0 / (double)(settings.offset[1] + settings.size)) + (double)(i >> 1) * EPS) * freq;
				double temp1 = 0.0;
				double river = 0.0;
				for (long o = 0; o < settings.octaves; o++) {
					temp1 += noise3d(dx, dy, settings.freqGround, msklut[o]) * amplut[o] * (temp1 * 1.6 + 1.0); // multi-fractal
					river += noise3d(dx, dy, settings.freqRiver, msklut[o]) * amplut[o];
					dx *= 2.0;
					dy *= 2.0;
				}
				samp[i] = temp1 * -20.0 + 28.0;
				if (settings.river) {
					temp1 = sin((settings.offset[0] + x) * (glm::pi<double>() / 256.0) + river * 4.0) * (0.5 + settings.riverWidth) +
						(0.5 - settings.riverWidth);
					if (temp1 > 1.0) {
						temp1 = 1.0;
					}
					csamp[i] = samp[i] * temp1;
					if (temp1 < 0.0) {
						temp1 = 0.0;
					}
					samp[i] *= temp1;
				} else {
					csamp[i] = samp[i];
				}
				if (csamp[i] < samp[i]) {
					csamp[i] = -log(1.0 - csamp[i]); // simulate water normal ;)
				}
			}
			// Get normal using cross-product
			double nx = csamp[1] - csamp[0];
			double ny = csamp[2] - csamp[0];
			double nz = -EPS;
			const double temp2 = 1.0 / sqrt(nx * nx + ny * ny + nz * nz);
			nx *= temp2;
			ny *= temp2;
			nz *= temp2;

			// Ground colors
			double gr = settings.ground.r;
			double gg = settings.ground.g;
			double gb = settings.ground.b;
			// blend factor
			double g = core_min(core_max(core_max(-nz, 0.0) * 1.4 - csamp[0] / 32.0 +
											 noise3d((settings.offset[0] + x) * freq, (settings.offset[1] + z) * freq, 0.3, 15) * 0.3,
										 0),
								1);
			// Grass
			gr += (settings.grass.r - gr) * g;
			gg += (settings.grass.g - gg) * g;
			gb += (settings.grass.b - gb) * g;

			// Grass2
			double g2 = (1.0 - fabs(g - 0.5) * 2.0) * 0.7;
			gr += (settings.grass2.r - gr) * g2;
			gg += (settings.grass2.g - gg) * g2;
			gb += (settings.grass2.b - gb) * g2;

			// Water
			g2 = core_max(core_min((samp[0] - csamp[0]) * 1.5, 1), 0);
			g = 1.0 - g2 * 0.2;
			gr += (settings.water.r * g - gr) * g2;
			gg += (settings.water.g * g - gg) * g2;
			gb += (settings.water.b * g - gb) * g2;

			const int k = z * settings.size + x;
			tempBuffer.amb[k].r = (uint8_t)core_min(core_max(gr * 0.3, 0), 255);
			tempBuffer.amb[k].g = (uint8_t)core_min(core_max(gg * 0.3, 0), 255);
			tempBuffer.amb[k].b = (uint8_t)core_min(core_max(gb * 0.3, 0), 255);
			const uint8_t maxa = core_max(core_max(tempBuffer.amb[k].r, tempBuffer.amb[k].g), tempBuffer.amb[k].b);

			// lighting
			double temp3 = (nx * 0.5 + ny * 0.25 - nz) / sqrt(0.5 * 0.5 + 0.25 * 0.25 + 1.0 * 1.0);
			temp3 *= 1.2;
			tempBuffer.buf[k].a = (uint8_t)samp[0];
			tempBuffer.buf[k].r = (uint8_t)core_min(core_max(gr * temp3, 0), 255 - maxa);
			tempBuffer.buf[k].g = (uint8_t)core_min(core_max(gg * temp3, 0), 255 - maxa);
			tempBuffer.buf[k].b = (uint8_t)core_min(core_max(gb * temp3, 0), 255 - maxa);

			tempBuffer.hgt[k] = csamp[0];
		}
		Log::debug("%i percent done", (int)(((z + 1) * 100) / settings.size));
	}

	tempBuffer.clearShadow();
	if (settings.shadow) {
		Log::debug("Applying shadows");
		const int VSHL = glm::log2((float)settings.size);
		for (int z = 0; z < settings.size; z++) {
			for (int x = 0; x < settings.size; x++) {
				const int k = z * settings.size + x;
				float f = tempBuffer.hgt[k] + 0.44f;
				int i, j;
				for (i = j = 1; i < (settings.size >> 2); j++, i++, f += 0.44f) {
					const int heightz = ((z - (j >> 1)) & (settings.size - 1));
					const int heightx = ((x - i) & (settings.size - 1));
					const int heightidx = (heightz << VSHL) + heightx;
					if (tempBuffer.hgt[heightidx] > f) {
						tempBuffer.sh[k] = 32;
						break;
					}
				}
			}
		}
		for (int i = settings.smoothing; i > 0; i--) {
			for (int z = 0; z < settings.size; z++) {
				for (int x = 0; x < settings.size; x++) {
					const int k = z * settings.size + x;
					tempBuffer.sh[k] =
						(tempBuffer.sh[k] + tempBuffer.sh[(((z + 1) & (settings.size - 1)) << VSHL) + x] +
						tempBuffer.sh[(z << VSHL) + ((x + 1) & (settings.size - 1))] +
						tempBuffer.sh[(((z + 1) & (settings.size - 1)) << VSHL) + ((x + 1) & (settings.size - 1))] + 2) >>
						2;
				}
			}
		}
	}

	if (settings.ambience) {
		for (int y = 0; y < settings.size; y++) {
			for (int x = 0; x < settings.size; x++) {
				const int k = y * settings.size + x;
				const int i = 256 - (tempBuffer.sh[k] << 2);
				tempBuffer.buf[k].r = ((tempBuffer.buf[k].r * i) >> 8) + tempBuffer.amb[k].r;
				tempBuffer.buf[k].g = ((tempBuffer.buf[k].g * i) >> 8) + tempBuffer.amb[k].g;
				tempBuffer.buf[k].b = ((tempBuffer.buf[k].b * i) >> 8) + tempBuffer.amb[k].b;
			}
		}
	}

	palette::Palette palette;
	palette.nippon();

	const voxel::Region region(0, 0, 0, settings.size - 1, settings.height - 1, settings.size - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup paletteLookup(palette);
	app::for_parallel(0, settings.size, [&paletteLookup, &tempBuffer, &settings, volume] (int start, int end) {
		const color::RGBA *heightmap = tempBuffer.buf + start * settings.size;
		// TODO: PERF: use volume sampler
		for (int vz = start; vz < end; ++vz) {
			for (int vx = 0; vx < settings.size; ++vx, ++heightmap) {
				const int maxsy = glm::clamp((int)heightmap->a, 0, settings.height);
				if (maxsy == 0) {
					const color::RGBA color{heightmap->r, heightmap->g, heightmap->b, 255};
					const int palIdx = paletteLookup.findClosestIndex(color);
					volume->setVoxel(vx, 0, vz, voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
					continue;
				} else if (maxsy < 0) {
					continue;
				}
				const color::RGBA color{heightmap->r, heightmap->g, heightmap->b, 255};
				const int palIdx = paletteLookup.findClosestIndex(color);
				core::Array<voxel::Voxel, 256> voxels;
				voxels.fill(voxel::createVoxel(voxel::VoxelType::Generic, palIdx));
				voxel::setVoxels(*volume, vx, vz, voxels.data(), maxsy);
			}
		}
	});
	// volume->translate(glm::ivec3(settings.offset[0], 0, settings.offset[1]));
	return volume;
}

} // namespace voxelgenerator
