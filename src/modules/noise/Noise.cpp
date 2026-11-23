/**
 * @file
 */

#include "Noise.h"
#include "core/TraceScoped.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/collection/Buffer.h"
#include <limits>
#include "Simplex.h"
#include "math/Vec4f.h"
#include "math/Vec3d.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"
#include "math/Scalar.h"
#include "math/Conversions.h"

namespace noise {

bool Noise::init() {
	return true;
}

void Noise::shutdown() {
}

int32_t Noise::intValueNoise(const math::Vec3i& pos, int32_t seed) const {
	constexpr int32_t xgen = 1619;
	constexpr int32_t ygen = 31337;
	constexpr int32_t zgen = 6971;
	constexpr int32_t fixedseed = 1013;
	int32_t n = (xgen * pos.x + ygen * pos.y + zgen * pos.z + fixedseed * seed) & (std::numeric_limits<int32_t>::max)();
	n = (n >> 13) ^ n;
	return (n * (n * n * 60493 + 19990303) + 1376312589) & (std::numeric_limits<int32_t>::max)();
}

double Noise::doubleValueNoise(const math::Vec3i& pos, int32_t seed) const {
	constexpr double div = (std::numeric_limits<int32_t>::max)() / 2.0 + 1.0;
	const int ni = intValueNoise(pos, seed);
	const double n = static_cast<double>(ni / div);
	return 1.0 - math::abs(n);
}

double Noise::voronoi(const math::Vec3d& pos, bool enableDistance, double frequency, int seed) const {
	core_trace_scoped(voronoi);
	const math::Vec3d p = pos * frequency;
	const math::Vec3i rp(
			(p.x > 0.0 ? (int)(p.x) : (int)(p.x) - 1),
			(p.y > 0.0 ? (int)(p.y) : (int)(p.y) - 1),
			(p.z > 0.0 ? (int)(p.z) : (int)(p.z) - 1));

	double minDist = static_cast<double>((std::numeric_limits<int32_t>::max)());
	math::Vec3d vp(0.0);

	const int d = 2;
	for (int z = rp.z - d; z <= rp.z + d; ++z) {
		for (int y = rp.y - d; y <= rp.y + d; ++y) {
			for (int x = rp.x - d; x <= rp.x + d; ++x) {
				const math::Vec3i c(x, y, z);
				const double noiseX = x + doubleValueNoise(c, seed);
				const double noiseY = y + doubleValueNoise(c, seed + 1);
				const double noiseZ = z + doubleValueNoise(c, seed + 2);
				const math::Vec3d noisePos(noiseX, noiseY, noiseZ);
				//const double dist = math::abs(noiseX) + math::abs(noiseY) + math::abs(noiseZ) + math::length2(noisePos);
				const double dist = math::length2(noisePos - p);
				if (dist < minDist) {
					minDist = dist;
					vp = noisePos;
				}
			}
		}
	}

	const double value = enableDistance ? math::length(vp - p) * math::rootThree - 1.0 : 0.0;
	const double ret = value + doubleValueNoise(math::Vec3i(math::floor(vp)));
	return ret;
}

float Noise::swissTurbulence(const math::Vec2f& p, float offset, int octaves, float lacunarity, float gain, float warp) const {
	float sum = 0.0f;
	float freq = 1.0f;
	float amp = 1.0f;
	math::Vec2f dsum(0.0f);
	for (int i = 0; i < octaves; i++) {
		const math::Vec2f in(p + math::Vec2f(offset + (float)i) + warp * dsum);
		const math::Vec3f& n = dnoise(in * freq);
		sum += amp * (1.0f - math::abs(n.x));
		dsum += amp * math::Vec2f(n.y, n.z) * -(n.x * 1.5f);
		freq *= lacunarity;
		amp *= gain * math::clamp(sum, 0.0f, 1.0f);
	}
	return (sum - 1.0f) * 0.5f;
}

float Noise::jordanTurbulence(const math::Vec2f& p, float offset, int octaves, float lacunarity, float gain1, float gain, float warp0, float warp, float damp0, float damp, float damp_scale) const {
	math::Vec3f n = dnoise(p + math::Vec2f(offset));
	math::Vec3f n2 = n * n.x;
	float sum = n2.x;
	math::Vec2f dsumWarp = warp0 * math::Vec2f(n2.y, n2.z);
	math::Vec2f dsumDamp = damp0 * math::Vec2f(n2.y, n2.z);

	float amp = gain1;
	float freq = lacunarity;
	float dampedAmp = amp * gain;

	for (int i = 1; i < octaves; i++) {
		const math::Vec2f in((p + math::Vec2f(offset + (float)i / 256.0f)) * freq + math::Vec2f(dsumWarp));
		n = dnoise(in);
		n2 = n * n.x;
		sum += dampedAmp * n2.x;
		dsumWarp += warp * math::Vec2f(n2.y, n2.z);
		dsumDamp += damp * math::Vec2f(n2.y, n2.z);
		freq *= lacunarity;
		amp *= gain;
		dampedAmp = amp * (1 - damp_scale / (1 + dot(dsumDamp, dsumDamp)));
	}
	return sum;
}

float Noise::sphereNoise(float longitude, float latitude) {
	const float latRad = math::radians(latitude);
	const float longRad = math::radians(longitude);
	const float r = math::cos(latRad);
	math::Vec3f pos(math::sin(longRad) * r,
			math::sin(latRad),
			math::cos(longRad) * r);
	return noise(pos);
}

void Noise::seamlessNoise(uint8_t* buffer, int size, int octaves, float persistence, float frequency, float amplitude) const {
	core_trace_scoped(seamlessNoise);
	const int components = 3;
	core::Buffer<uint8_t> bufferChannel(size * size);
	const float pi2 = math::twoPi;
	const float d = 1.0f / (float)size;
	for (int channel = 0; channel < components; ++channel) {
		// seamless noise: https://www.gamedev.net/blog/33/entry-2138456-seamless-noise/
		float s = 0.0f;
		for (int x = 0; x < size; x++, s += d) {
			const float s_pi2 = s * pi2;
			const float nx = math::cos(s_pi2);
			const float nz = math::sin(s_pi2);
			float t = 0.0f;
			for (int y = 0; y < size; y++, t += d) {
				const float t_pi2 = t * pi2;
				const float ny = math::cos(t_pi2);
				const float nw = math::sin(t_pi2);
				float noise = fBm(math::Vec4f(nx, ny, nz, nw) + math::Vec4f((float)channel), octaves, persistence, amplitude);
				noise = norm(noise);
				const unsigned char color = (unsigned char) (noise * 255.0f);
				const int channelIndex = y * size + x;
				bufferChannel[channelIndex] = color;
			}
		}
		int index = 0;
		for (int x = 0; x < size; ++x) {
			for (int y = 0; y < size; ++y, ++index) {
				buffer[index * components + channel] = bufferChannel[index];
			}
		}
	}
}

}
