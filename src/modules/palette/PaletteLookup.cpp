/**
 * @file
 */

#include "PaletteLookup.h"
#include "color/Color.h"
#include "color/ColorUtil.h"
#include "palette/Palette.h"
#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace palette {

namespace priv {

static constexpr int Q_BITS = 5;											 // 5 bits per channel
static constexpr int Q_LEVELS = 1 << Q_BITS;								 // 32
static constexpr int CACHE_SIZE = Q_LEVELS * Q_LEVELS * Q_LEVELS * Q_LEVELS; // 32^4 = 1048576

static inline uint16_t quantizeChannel(uint8_t value) {
	return value >> (8 - Q_BITS); // shift to keep top Q_BITS
}

static inline size_t computeIndex(color::RGBA rgba) {
	uint16_t r = quantizeChannel(rgba.r);
	uint16_t g = quantizeChannel(rgba.g);
	uint16_t b = quantizeChannel(rgba.b);
	uint16_t a = quantizeChannel(rgba.a);
	return ((r << (3 * Q_BITS)) | (g << (2 * Q_BITS)) | (b << Q_BITS) | a);
}

} // namespace priv

PaletteLookup::PaletteLookup(const palette::Palette &palette)
	: _palette(palette), _cache(priv::CACHE_SIZE, PaletteColorNotFound) {
}

uint8_t PaletteLookup::findClosestIndex(const glm::vec4 &color) {
	return findClosestIndex(color::toRGBA(color));
}

uint8_t PaletteLookup::findClosestIndex(color::RGBA rgba) {
	size_t idx = priv::computeIndex(rgba);

#if defined(_MSC_VER)
	uint16_t oldValue = _cache[idx];
	if (oldValue == (uint16_t)PaletteColorNotFound) {
		core_assert_always(_palette.colorCount() > 0);
		uint16_t newValue = _palette.getClosestMatch(rgba);
		InterlockedExchange16(reinterpret_cast<volatile int16_t *>(&_cache[idx]), newValue);
	}
#elif defined(__GNUC__) || defined(__clang__)
	uint16_t oldValue = __atomic_load_n(&_cache[idx], __ATOMIC_RELAXED);
	if (oldValue == (uint16_t)PaletteColorNotFound) {
		core_assert_always(_palette.colorCount() > 0);
		uint16_t newValue = _palette.getClosestMatch(rgba);
		__atomic_exchange_n(&_cache[idx], newValue, __ATOMIC_SEQ_CST);
	}
#else
#error "Atomic operations not implemented for this compiler"
#endif
	return _cache[idx];
}

} // namespace palette
