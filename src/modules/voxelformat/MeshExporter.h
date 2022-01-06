/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief Convert the volume data into a mesh
 */
class MeshExporter : public Format {
protected:
	struct MeshExt {
		MeshExt(voxel::Mesh* mesh, const core::String& name);
		voxel::Mesh* mesh = nullptr;
		core::String name;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, float scale = 1.0f, bool quad = false, bool withColor = true, bool withTexCoords = true) = 0;
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& file, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
