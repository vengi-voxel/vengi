/**
 * @file
 */

#include "MeshMaterial.h"

namespace voxelformat {

MeshMaterialPtr createMaterial(const image::ImagePtr &texture) {
	if (!texture) {
		return {};
	}
	MeshMaterialPtr material = core::make_shared<MeshMaterial>(texture->name());
	material->texture = texture;
	return material;
}

MeshMaterialPtr createMaterial(const core::String &name) {
	return core::make_shared<MeshMaterial>(name);
}

MeshMaterialPtr cloneMaterial(const MeshMaterialPtr &material) {
	return core::make_shared<MeshMaterial>(*material.get());
}

MeshMaterialPtr cloneMaterial(const MeshMaterial &material) {
	return core::make_shared<MeshMaterial>(material);
}

int MeshMaterial::width() const {
	return texture ? texture->width() : 0;
}

int MeshMaterial::height() const {
	return texture ? texture->width() : 0;
}

color::RGBA MeshMaterial::apply(color::RGBA color) const {
	if (baseColorFactor > 0.0f) {
		const float contribution = (1.0f - baseColorFactor);
		color = color::RGBA((float)color.r * contribution + baseColor.r * baseColorFactor,
						   (float)color.g * contribution + baseColor.g * baseColorFactor,
						   (float)color.b * contribution + baseColor.b * baseColorFactor, color.a);
	}
	if (transparency > 0.0f) {
		color.a = color.a * (1.0f - transparency);
	}
	return color;
}

bool MeshMaterial::colorAt(color::RGBA &color, const glm::vec2 &uv, bool originUpperLeft) const {
	if (!texture || !texture->isLoaded()) {
		if (baseColorFactor <= 0.0f) {
			return false;
		}
		color = color::RGBA(0, 0, 0);
	} else {
		color = texture->colorAt(uv, wrapS, wrapT, originUpperLeft);
	}
	color = apply(color);
	return true;
}

} // namespace voxelformat
