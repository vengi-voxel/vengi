#pragma once

#include "PolyVox/CubicSurfaceExtractor.h"
#include "PolyVox/Vertex.h"
#include "BitField.h"

#include <glm/glm.hpp>

namespace Cubiquity {

class Color {
public:
	static const uint32_t MaxInOutValue = 255;

	static const uint32_t RedMSB = 31;
	static const uint32_t RedLSB = 27;
	static const uint32_t GreenMSB = 26;
	static const uint32_t GreenLSB = 21;
	static const uint32_t BlueMSB = 20;
	static const uint32_t BlueLSB = 16;
	static const uint32_t AlphaMSB = 15;
	static const uint32_t AlphaLSB = 12;

	static const uint32_t NoOfRedBits = RedMSB - RedLSB + 1;
	static const uint32_t NoOfGreenBits = GreenMSB - GreenLSB + 1;
	static const uint32_t NoOfBlueBits = BlueMSB - BlueLSB + 1;
	static const uint32_t NoOfAlphaBits = AlphaMSB - AlphaLSB + 1;

	static const uint32_t RedScaleFactor = MaxInOutValue / ((0x01 << NoOfRedBits) - 1);
	static const uint32_t GreenScaleFactor = MaxInOutValue / ((0x01 << NoOfGreenBits) - 1);
	static const uint32_t BlueScaleFactor = MaxInOutValue / ((0x01 << NoOfBlueBits) - 1);
	static const uint32_t AlphaScaleFactor = MaxInOutValue / ((0x01 << NoOfAlphaBits) - 1);

	Color() {
		_channels.clearAllBits();
	}

	Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = MaxInOutValue) {
		_channels.clearAllBits();
		setColor(red, green, blue, alpha);
	}

	Color(const glm::vec4& color) {
		_channels.clearAllBits();
		setColor(color.r, color.g, color.b, color.a);
	}

	bool operator==(const Color& rhs) const {
		return _channels == rhs._channels;
	}

	bool operator!=(const Color& rhs) const {
		return !(*this == rhs);
	}

	uint8_t getRed() {
		return static_cast<uint8_t>(_channels.getBits(RedMSB, RedLSB) * RedScaleFactor);
	}

	uint8_t getGreen() {
		return static_cast<uint8_t>(_channels.getBits(GreenMSB, GreenLSB) * GreenScaleFactor);
	}

	uint8_t getBlue() {
		return static_cast<uint8_t>(_channels.getBits(BlueMSB, BlueLSB) * BlueScaleFactor);
	}

	uint8_t getAlpha() {
		return static_cast<uint8_t>(_channels.getBits(AlphaMSB, AlphaLSB) * AlphaScaleFactor);
	}

	void setRed(uint8_t value) {
		setBits(RedMSB, RedLSB, value / RedScaleFactor);
	}

	void setGreen(uint8_t value) {
		setBits(GreenMSB, GreenLSB, value / GreenScaleFactor);
	}

	void setBlue(uint8_t value) {
		setBits(BlueMSB, BlueLSB, value / BlueScaleFactor);
	}

	void setAlpha(uint8_t value) {
		setBits(AlphaMSB, AlphaLSB, value / AlphaScaleFactor);
	}

	void setColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
		setRed(red);
		setGreen(green);
		setBlue(blue);
		setAlpha(alpha);
	}

	inline void setBits(size_t MSB, size_t LSB, uint32_t bitsToSet) {
		_channels.setBits(MSB, LSB, bitsToSet);
	}

private:
	BitField<uint32_t> _channels;
};

// These operations are used by the smooth raycast to perform trilinear interpolation.
// We never actually do that on this type (because colors are used for cubic surfaces
// not smooth ones) but our use of templates means that this code path still gets compiled.
// The actual implementations simply assert if they are ever called by mistake.
Color operator+(const Color& lhs, const Color& rhs);
Color operator-(const Color& lhs, const Color& rhs);
Color operator*(const Color& lhs, float rhs);
Color operator/(const Color& lhs, float rhs);

class ColoredCubesIsQuadNeeded {
public:
	bool operator()(Color back, Color front, Color& materialToUse) {
		if (back.getAlpha() > 0 && front.getAlpha() == 0) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

typedef ::PolyVox::CubicVertex<Color> ColoredCubesVertex;
typedef ::PolyVox::Mesh<ColoredCubesVertex, uint16_t> ColoredCubesMesh;

}
