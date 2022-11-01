/**
 * @file
 */

#include "STLFormat.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "voxel/Mesh.h"
#include "voxelformat/SceneGraph.h"
#include <SDL_stdinc.h>

namespace voxelformat {

namespace priv {
static constexpr const size_t BinaryHeaderSize = 80;
}

bool STLFormat::parseAscii(io::SeekableReadStream &stream, TriCollection &tris) {
	char line[512];
	const glm::vec3 &scale = getScale();
	stream.seek(0);
	while (stream.readLine(sizeof(line), line)) {
		if (!strncmp(line, "solid", 5)) {
			while (stream.readLine(sizeof(line), line)) {
				const char *ptr = line;
				while (*ptr == ' ') {
					++ptr;
				}
				if (!strncmp(ptr, "endsolid", 8)) {
					break;
				}
				if (!strncmp(ptr, "facet", 5)) {
					Tri tri;
					glm::vec3 norm;
					if (SDL_sscanf(ptr, "facet normal %f %f %f", &norm.x, &norm.y, &norm.z) != 3) {
						Log::error("Failed to parse facet normal");
						return false;
					}
					if (!stream.readLine(sizeof(line), line)) {
						return false;
					}
					ptr = line;
					while (*ptr == ' ') {
						++ptr;
					}
					if (!strncmp(ptr, "outer loop", 10)) {
						int vi = 0;
						while (stream.readLine(sizeof(line), line)) {
							ptr = line;
							while (*ptr == ' ') {
								++ptr;
							}
							if (!strncmp(ptr, "endloop", 7)) {
								break;
							}
							if (vi >= 3) {
								return false;
							}
							glm::vec3 &vert = tri.vertices[vi];
							if (SDL_sscanf(ptr, "vertex %f %f %f", &vert.x, &vert.y, &vert.z) != 3) {
								Log::error("Failed to parse vertex");
								return false;
							}
							vert *= scale;
							++vi;
						}
						if (vi != 3) {
							return false;
						}
						tris.push_back(tri);
					}
				}
			}
		}
	}
	return true;
}

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Failed to read stl " CORE_STRINGIFY(read)); \
		return false; \
	}

bool STLFormat::parseBinary(io::SeekableReadStream &stream, TriCollection &tris) {
	const glm::vec3 &scale = getScale();
	if (stream.seek(priv::BinaryHeaderSize) == -1) {
		Log::error("Failed to seek after the binary stl header");
		return false;
	}
	uint32_t numFaces = 0;
	wrap(stream.readUInt32(numFaces))
	Log::debug("faces: %u", numFaces);
	if (numFaces == 0) {
		Log::error("No faces in stl file");
		return false;
	}
	tris.resize(numFaces);
	for (uint32_t fn = 0; fn < numFaces; ++fn) {
		Tri &tri = tris[fn];
		glm::vec3 normal;
		wrap(stream.readFloat(normal.x))
		wrap(stream.readFloat(normal.y))
		wrap(stream.readFloat(normal.z))
		for (int i = 0; i < 3; ++i) {
			wrap(stream.readFloat(tri.vertices[i].x))
			wrap(stream.readFloat(tri.vertices[i].y))
			wrap(stream.readFloat(tri.vertices[i].z))
			tri.vertices[i] *= scale;
		}
		if (stream.skip(2) == -1) {
			Log::error("Failed to seek while parsing the frames");
			return false;
		}
	}

	return true;
}

bool STLFormat::voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint32_t magic;
	wrap(stream.readUInt32(magic));
	const bool ascii = FourCC('s', 'o', 'l', 'i') == magic;

	TriCollection tris;
	if (ascii) {
		Log::debug("found ascii format");
		if (!parseAscii(stream, tris)) {
			Log::error("Failed to parse ascii stl file %s", filename.c_str());
			return false;
		}
	} else {
		Log::debug("found binary format");
		if (!parseBinary(stream, tris)) {
			Log::error("Failed to parse binary stl file %s", filename.c_str());
			return false;
		}
	}

	return voxelizeNode(filename, sceneGraph, tris);
}

#undef wrap

bool STLFormat::writeVertex(io::SeekableWriteStream &stream, const MeshExt &meshExt, const voxel::VoxelVertex &v1, const SceneGraphTransform &transform, const glm::vec3 &scale) {
	glm::vec3 pos;
	if (meshExt.applyTransform) {
		pos = transform.apply(v1.position, meshExt.size);
	} else {
		pos = v1.position;
	}
	pos *= scale;
	if (!stream.writeFloat(pos.x)) {
		return false;
	}
	if (!stream.writeFloat(pos.y)) {
		return false;
	}
	if (!stream.writeFloat(pos.z)) {
		return false;
	}
	return true;
}

bool STLFormat::saveMeshes(const core::Map<int, int> &, const SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	stream.writeStringFormat(false, "github.com/mgerhardy/vengi");
	const size_t delta = priv::BinaryHeaderSize - stream.pos();
	for (size_t i = 0; i < delta; ++i) {
		stream.writeUInt8(0);
	}
	core_assert(stream.pos() == priv::BinaryHeaderSize);

	int faceCount = 0;
	for (const auto &meshExt : meshes) {
		const voxel::Mesh *mesh = meshExt.mesh;
		const int ni = (int)mesh->getNoOfIndices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		faceCount += ni / 3;
	}
	stream.writeUInt32(faceCount);

	for (const auto &meshExt : meshes) {
		const voxel::Mesh *mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int ni = (int)mesh->getNoOfIndices();
		const SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
		KeyFrameIndex keyFrameIdx = 0;
		const SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();

		for (int i = 0; i < ni; i += 3) {
			const uint32_t one = indices[i + 0];
			const uint32_t two = indices[i + 1];
			const uint32_t three = indices[i + 2];

			const voxel::VoxelVertex &v1 = vertices[one];
			const voxel::VoxelVertex &v2 = vertices[two];
			const voxel::VoxelVertex &v3 = vertices[three];

			// normal
			const glm::vec3 edge1 = glm::vec3(v2.position - v1.position);
			const glm::vec3 edge2 = glm::vec3(v3.position - v1.position);
			const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
			for (int j = 0; j < 3; ++j) {
				if (!stream.writeFloat(normal[j])) {
					return false;
				}
			}

			if (!writeVertex(stream, meshExt, v1, transform, scale)) {
				return false;
			}

			if (!writeVertex(stream, meshExt, v2, transform, scale)) {
				return false;
			}

			if (!writeVertex(stream, meshExt, v3, transform, scale)) {
				return false;
			}

			stream.writeUInt16(0);
		}
	}
	return true;
}

} // namespace voxel
