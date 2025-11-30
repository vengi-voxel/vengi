/**
 * @file
 */

#include "AVMTPalette.h"
#include "AVMTHelper.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "palette/Material.h"

namespace palette {

bool AVMTPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::ColorPalette &palette) {
	core::DynamicArray<AVMTMaterial> materials;
	core::String paletteName;
	if (!parseMaterials(stream, materials, paletteName)) {
		Log::error("Failed to parse materials from %s", filename.c_str());
		return false;
	}
	if (materials.empty()) {
		Log::error("No materials found in %s", filename.c_str());
		return false;
	}

	palette.reserve(materials.size());
	for (const auto &e : materials) {
		palette.add(e.rgba, e.name, e.mat);
	}
	return true;
}

bool AVMTPalette::save(const palette::ColorPalette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	stream.writeString("VoxelMaterialArray =\t{\n", false);
	stream.writeString("\tmaterials =\t[\n", false);
	stream.writeString("\t\t{\n", false);
	stream.writeStringFormat(false, "\t\t\tName =\t\"%s\"\n", palette.name().c_str());
	stream.writeString("\t\t\tType =\t1\n", false);
	stream.writeString("\t\t\tPaletteSize =\t{\n", false);
	stream.writeString("\t\t\t\tx =\t1\n", false);
	stream.writeStringFormat(false, "\t\t\t\ty =\t%i\n", palette.colorCount());
	stream.writeString("\t\t\t}\n", false);
	stream.writeString("\t\t\tVoxMaterialParams =\t[\n", false);
	int added = 0;
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color(i).a == 0) {
			continue;
		}
		if (added != 0) {
			stream.writeString(",\n", false);
		}
		++added;
		const color::RGBA &color = palette.color(i);
		const glm::vec4 c = color::Color::fromRGBA(color);
		stream.writeString("\t\t\t\t{\n", false);
		stream.writeStringFormat(false, "\t\t\t\t\tr =\t%0.6f\n", c.r);
		stream.writeStringFormat(false, "\t\t\t\t\tg =\t%0.6f\n", c.g);
		stream.writeStringFormat(false, "\t\t\t\t\tb =\t%0.6f\n", c.b);
		const Material &mat = palette.material(i);
		stream.writeStringFormat(false, "\t\t\t\t\tmetallic =\t%f\n", mat.metal);
		stream.writeStringFormat(false, "\t\t\t\t\tsmooth =\t%f\n", 1.0f - mat.roughness);
		stream.writeStringFormat(false, "\t\t\t\t\temissive =\t%f\n", mat.emit);
		stream.writeString("\t\t\t\t\tmaterialTransparency =\t{\n", false);
		if (mat.type == palette::MaterialType::Glass || mat.type == palette::MaterialType::Blend) {
			// TODO: MATERIAL: not really the alpha value...
			stream.writeStringFormat(false, "\t\t\t\t\t\tsurfaceTransmission =\t%f\n", c.a);
		} else if (mat.type == palette::MaterialType::Media) {
			stream.writeString("\t\t\t\t\t\tsurfaceTransmission =\t1.0\n", false);
		} else {
			stream.writeString("\t\t\t\t\t\tsurfaceTransmission =\t0.0\n", false);
		}
		// stream.writeStringFormat(false, "\t\t\t\t\t\tabsorptionLength =\t%f\n", mat.absorptionLength);
		// stream.writeStringFormat(false, "\t\t\t\t\t\tscatterLength =\t%f\n", mat.scatterLength);
		stream.writeStringFormat(false, "\t\t\t\t\t\tindexOfRefraction =\t%f\n", 1.0f + mat.indexOfRefraction);
		if (mat.media == 1.0f) {
			stream.writeStringFormat(false, "\t\t\t\t\t\tphase =\t%f\n", mat.phase);
		} else {
			stream.writeString("\t\t\t\t\t\tphase =\t0.0\n", false);
		}
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
