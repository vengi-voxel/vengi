/**
 * @file
 */

#include "GLTFFormat.h"

#define TINYGLTF_IMPLEMENTATION
#include "external/tiny_gltf.h"

namespace voxel {

bool GLTFFormat::saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	return false;
}

} // namespace voxel
