/**
 * @file
 */

#include "AVMTHelper.h"
#include "color/Color.h"
#include "color/ColorUtil.h"

namespace palette {

bool parseMaterials(io::SeekableReadStream &stream, core::DynamicArray<AVMTMaterial> &materials,
					core::String &paletteName) {
	AVMTStream avmtStream(stream);
	AVMTMaterial currentMaterial;
	bool currentMatColorFound = false;

	while (!avmtStream.eos()) {
		const core::String &token = avmtStream.next();
		if (avmtStream.arrayDepth() == 1 && avmtStream.blockDepth() == 2) {
			if (token == "Name") {
				paletteName = avmtStream.nextStringValue();
				if (paletteName.size() >= 2 && paletteName.first() == '"' && paletteName.last() == '"') {
					paletteName = paletteName.substr(1, paletteName.size() - 2);
				}
			}
		} else if (avmtStream.arrayDepth() == 2 && avmtStream.blockDepth() >= 3) {
			if (token == "r") {
				currentMaterial.color.r = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "g") {
				currentMaterial.color.g = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "b") {
				currentMaterial.color.b = avmtStream.nextStringValue().toFloat();
				currentMatColorFound = true;
			} else if (token == "metallic") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialMetal, v);
			} else if (token == "indexOfRefraction") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialIndexOfRefraction, 1.0f - v);
			} else if (token == "surfaceTransmission") {
				const float alpha = avmtStream.nextStringValue().toFloat();
				if (alpha >= 1.0f) {
					currentMaterial.mat.type = palette::MaterialType::Media;
					currentMaterial.mat.setValue(MaterialProperty::MaterialMedia, 1.0f);
				} else if (alpha > 0.0f) {
					currentMaterial.mat.type = palette::MaterialType::Blend;
					// currentMaterial.color.a = alpha; // TODO: MATERIAL: not really the alpha value...
				}
			} else if (token == "absorptionLength") {
				/*const float v =*/avmtStream.nextStringValue().toFloat();
				// currentMaterial.mat.setValue(MaterialProperty::MaterialAbsorptionLength, v);
			} else if (token == "scatterLength") {
				/*const float v =*/avmtStream.nextStringValue().toFloat();
				// currentMaterial.mat.setValue(MaterialProperty::MaterialScatterLength, v);
			} else if (token == "phase") {
				const float v = avmtStream.nextStringValue().toFloat();
				if (v > 0.0f) {
					currentMaterial.mat.setValue(MaterialProperty::MaterialPhase, v);
				}
			} else if (token == "smooth") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialRoughness, 1.0f - v);
			} else if (token == "emissive") {
				const float v = avmtStream.nextStringValue().toFloat();
				currentMaterial.mat.setValue(MaterialProperty::MaterialEmit, v);
			} else if (token == "name") {
				currentMaterial.name = avmtStream.nextStringValue();
				if (currentMaterial.name.size() >= 2 && currentMaterial.name.first() == '"' && currentMaterial.name.last() == '"') {
					currentMaterial.name = currentMaterial.name.substr(1, currentMaterial.name.size() - 2);
				}
			} else if (token == "materialTransparency") {
				// skip =
				if (avmtStream.next() != "=") {
					Log::error("Expected '=' after materialTransparency but got '%s'", token.c_str());
				}
			} else {
				Log::debug("Unhandled token: '%s' (expected are: r, g, b, metallic, smooth, emissive, name)",
						   token.c_str());
			}
		} else {
			if (currentMatColorFound) {
				currentMaterial.rgba = color::getRGBA(currentMaterial.color);
				materials.push_back(currentMaterial);
				currentMaterial = {};
				currentMatColorFound = false;
			}
			Log::trace("Token %s at depth %i and array depth %i", token.c_str(), avmtStream.blockDepth(),
					   avmtStream.arrayDepth());
		}
	}

	return !materials.empty();
}

} // namespace palette
