/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "color/RGBA.h"
#include "io/TokenStream.h"
#include "palette/Material.h"
#include <glm/vec4.hpp>

namespace palette {

class AVMTStream : public io::TokenStream {
private:
	using Super = io::TokenStream;
	int _blockDepth = 0;
	int _arrayDepth = 0;

public:
	AVMTStream(io::SeekableReadStream &stream) : Super(stream, {}, " (){},;\n\r\t") {
	}

	core::String next() override {
		core::String n = Super::next();
		if (n == "{") {
			++_blockDepth;
			return next();
		} else if (n == "}") {
			--_blockDepth;
			return next();
		} else if (n == "[") {
			++_arrayDepth;
			return next();
		} else if (n == "]") {
			--_arrayDepth;
			return next();
		}
		return n;
	}

	core::String nextStringValue() {
		// skip =
		const core::String equalSign = next();
		if (equalSign != "=") {
			Log::error("Expected '=' but got '%s'", equalSign.c_str());
		}
		return next();
	}

	int blockDepth() const {
		return _blockDepth;
	}

	int arrayDepth() const {
		return _arrayDepth;
	}
};

struct AVMTMaterial {
	color::RGBA rgba;
	glm::vec4 color{0.0f, 0.0f, 0.0f, 1.0f};
	core::String name;
	palette::Material mat;

	bool operator>(const AVMTMaterial &other) const {
		return name > other.name;
	}
};

bool parseMaterials(io::SeekableReadStream &stream, core::DynamicArray<AVMTMaterial> &materials,
					core::String &paletteName);

} // namespace palette
