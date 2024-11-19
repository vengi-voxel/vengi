/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/collection/StringMap.h"
#include "image/Image.h"
#include "palette/Palette.h"

namespace voxelformat {

class MeshMaterial {
public:
	MeshMaterial(const core::String &_name) : name(_name) {
	}
	core::String name;
	image::ImagePtr texture;
	image::TextureWrap wrapS = image::TextureWrap::Repeat;
	image::TextureWrap wrapT = image::TextureWrap::Repeat;
	palette::Material material;
	core::RGBA baseColor{255, 255, 255, 255};
	core::RGBA emitColor{0, 0, 0, 0};
	float baseColorFactor = 0.0f;
	float transparency = 0.0f;

	[[nodiscard]] core::RGBA blendWithBaseColor(core::RGBA color) const {
		if (baseColorFactor <= 0.0f) {
			return color;
		}
		const float contribution = (1.0f - baseColorFactor);
		return core::RGBA((float)color.r * contribution + baseColor.r * baseColorFactor,
						  (float)color.g * contribution + baseColor.g * baseColorFactor,
						  (float)color.b * contribution + baseColor.b * baseColorFactor, color.a);
	}
};

using MeshMaterialPtr = core::SharedPtr<MeshMaterial>;

MeshMaterialPtr createMaterial(const image::ImagePtr &texture);
MeshMaterialPtr createMaterial(const core::String &name);
MeshMaterialPtr cloneMaterial(const MeshMaterialPtr &material);
MeshMaterialPtr cloneMaterial(const MeshMaterial &material);

using MeshMaterialMap = core::StringMap<MeshMaterialPtr>;

} // namespace voxelformat
