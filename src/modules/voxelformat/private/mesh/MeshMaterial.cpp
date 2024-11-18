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

} // namespace voxelformat
