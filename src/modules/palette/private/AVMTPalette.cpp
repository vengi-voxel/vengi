/**
 * @file
 */

#include "AVMTPalette.h"
#include "core/Color.h"
#include "core/Log.h"
#include "io/TokenStream.h"

namespace palette {

class AVMTStream : public io::TokenStream {
private:
	using Super = io::TokenStream;
	int _blockDepth = 0;
	int _arrayDepth = 0;

public:
	using io::TokenStream::TokenStream;

	core::String next() override {
		core::String n = Super::next();
		if (n == "{") {
			++_blockDepth;
		} else if (n == "}") {
			--_blockDepth;
		} else if (n == "[") {
			++_arrayDepth;
		} else if (n == "]") {
			--_arrayDepth;
		}
		return n;
	}

	core::String nextStringValue() {
		Super::next(); // skip =
		return Super::next();
	}

	int blockDepth() const {
		return _blockDepth;
	}

	int arrayDepth() const {
		return _arrayDepth;
	}
};

bool AVMTPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	AVMTStream ts(stream);

	glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
	int palIdx = 0;
	bool _hasColor = false;
	while (!ts.eos()) {
		const core::String &token = ts.next();
		if (ts.arrayDepth() == 1 && ts.blockDepth() == 2) {
			if (token == "Name") {
				palette.setName(ts.nextStringValue());
			}
		} else if (ts.arrayDepth() == 2 && ts.blockDepth() == 3) {
			if (token == "r") {
				color.r = ts.nextStringValue().toFloat();
				_hasColor = true;
			} else if (token == "g") {
				color.g = ts.nextStringValue().toFloat();
				_hasColor = true;
			} else if (token == "b") {
				color.b = ts.nextStringValue().toFloat();
				_hasColor = true;
			} else if (token == "metallic") {
				/*const float v =*/ts.nextStringValue().toFloat();
			} else if (token == "smooth") {
				/*const float v =*/ts.nextStringValue().toFloat();
			} else if (token == "emissive") {
				/*const float v =*/ts.nextStringValue().toFloat();
			} else if (token == "name") {
				const core::String &colorName = ts.nextStringValue();
				palette.setColorName(palIdx, colorName);
			} else {
				Log::debug("%s (r, g, b, metallic, smooth, emissive)", token.c_str());
			}
		} else {
			if (_hasColor) {
				palette.setColor(palIdx, core::Color::getRGBA(color));
				palIdx++;
				_hasColor = false;
			}
			Log::debug("token %s at depth %i and array depth %i", token.c_str(), ts.blockDepth(), ts.arrayDepth());
		}
	}
	palette.changeSize(palIdx);
	return true;
}

bool AVMTPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeString("VoxelMaterialArray =\t{\n", false);
	stream.writeString("\tmaterials =\t[\n", false);
	stream.writeString("\t\t{\n", false);
	stream.writeStringFormat(false, "\t\t\tName =\t\"%s\"\n", palette.name().c_str());
	stream.writeString("\t\t\tType =\t1\n", false);
	stream.writeString("\t\t\tPaletteSize =\t{\n", false);
	stream.writeString("\t\t\t\tx =\t1\n", false);
	stream.writeString("\t\t\t\ty =\t256\n", false);
	stream.writeString("\t\t\t}\n", false);
	stream.writeString("\t\t\tVoxMaterialParams =\t[\n", false);
	stream.writeString("\t\t\t\t{\n", false);
	int added = 0;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color(i).a == 0) {
			continue;
		}
		if (added != 0) {
			stream.writeString(",\n", false);
		}
		++added;
		const glm::vec4 &c = palette.color4(i);
		stream.writeStringFormat(false, "\t\t\t\t\tr =\t%f\n", c.r);
		stream.writeStringFormat(false, "\t\t\t\t\tg =\t%f\n", c.g);
		stream.writeStringFormat(false, "\t\t\t\t\tb =\t%f\n", c.b);
		const Material &mat = palette.material(i);
		stream.writeStringFormat(false, "\t\t\t\t\tmetallic =\t%f\n", mat.metal);
		// stream.writeStringFormat(false, "\t\t\t\t\tsmooth =\t%f\n", mat.smooth);
		stream.writeStringFormat(false, "\t\t\t\t\temissive =\t%f\n", mat.emit);
		stream.writeString("\t\t\t\t\tmaterialTransparency =\t{\n", false);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tsurfaceTransmission =\t%f\n", mat.surfaceTransmission);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tabsorptionLength =\t%f\n", mat.absorptionLength);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tscatterLength =\t%f\n", mat.scatterLength);
		stream.writeStringFormat(false, "\t\t\t\t\t\tindexOfRefraction =\t%f\n", mat.indexOfRefraction);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tphase =\t%f\n", mat.phase);
		stream.writeString("\t\t\t\t\t}\n", false);
		stream.writeStringFormat(false, "\t\t\t\t\tname =\t\"%s\"\n", palette.colorName(i).c_str());
		stream.writeString("\t\t\t\t}", false);
	}
	stream.writeString("\n\t\t\t\t}\n", false);
	stream.writeString("\t\t\t]\n", false);
	stream.writeString("\t\t\tStrength =\t1\n", false);
	stream.writeString("\t\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\tpalette =\t[]\n", false);
	stream.writeString("\tpalettes =\t[\n", false);
	stream.writeString("\t\t{\n", false);
	stream.writeString("\t\t\tname =\t\"Default\"\n", false);
	stream.writeString("\t\t\tpalette =\t[]\n", false);
	stream.writeString("\t\t\twidth =\t15\n", false);
	stream.writeString("\t\t}\n", false);
	stream.writeString("\t]\n", false);
	stream.writeString("\tactivePaletteEditToolShapes =\t0\n", false);
	stream.writeString("\tactivePaletteEditToolProcedural =\t0\n", false);
	stream.writeString("\tactivePaletteEditToolModifierRandomise =\t0\n", false);
	stream.writeString("\tactivePaletteMaterials =\t0\n", false);
	return true;
}

} // namespace palette
