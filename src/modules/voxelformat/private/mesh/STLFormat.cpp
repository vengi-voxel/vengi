/**
 * @file
 */

#include "STLFormat.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/Mesh.h"

namespace voxelformat {

namespace priv {
static constexpr const size_t BinaryHeaderSize = 80;
}

bool STLFormat::parseAscii(io::SeekableReadStream &stream, MeshTriCollection &tris) {
	char line[512];
	const glm::vec3 &scale = getInputScale();
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
				if (!strncmp(ptr, "facet normal ", 13)) {
					voxelformat::MeshTri meshTri;
					glm::vec3 norm;
					ptr += 13;
					core::string::parseReal3(&norm.x, &norm.y, &norm.z, &ptr);
					if (!stream.readLine(sizeof(line), line)) {
						return false;
					}
					ptr = line;
					while (*ptr == ' ') {
						++ptr;
					}
					if (!strncmp(ptr, "outer loop", 10)) {
						glm::vec3 verts[3];
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
							glm::vec3 &vert = verts[vi];
							vert.x = vert.y = vert.z = 0.0f;
							ptr += 7; // "vertex "
							core::string::parseReal3(&vert.x, &vert.y, &vert.z, &ptr);
							++vi;
						}
						if (vi != 3) {
							return false;
						}
						meshTri.setVertices(verts[0] * scale, verts[1] * scale, verts[2] * scale);
						tris.push_back(meshTri);
					}
				}
			}
		}
	}
	return true;
}

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Failed to read stl " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

bool STLFormat::parseBinary(io::SeekableReadStream &stream, MeshTriCollection &tris) {
	const glm::vec3 &scale = getInputScale();
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
		voxelformat::MeshTri &meshTri = tris[fn];
		glm::vec3 normal;
		glm::vec3 vertices[3];
		wrap(stream.readFloat(normal.x))
		wrap(stream.readFloat(normal.y))
		wrap(stream.readFloat(normal.z))
		for (int i = 0; i < 3; ++i) {
			wrap(stream.readFloat(vertices[i].x))
			wrap(stream.readFloat(vertices[i].y))
			wrap(stream.readFloat(vertices[i].z))
		}
		if (stream.skip(2) == -1) {
			Log::error("Failed to seek while parsing the frames");
			return false;
		}
		meshTri.setVertices(vertices[0] * scale, vertices[1] * scale, vertices[2] * scale);
	}

	return true;
}

bool STLFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic));
	const bool ascii = FourCC('s', 'o', 'l', 'i') == magic;

	MeshTriCollection tris;
	if (ascii) {
		Log::debug("found ascii format");
		if (!parseAscii(*stream, tris)) {
			Log::error("Failed to parse ascii stl file %s", filename.c_str());
			return false;
		}
	} else {
		Log::debug("found binary format");
		if (!parseBinary(*stream, tris)) {
			Log::error("Failed to parse binary stl file %s", filename.c_str());
			return false;
		}
	}

	return voxelizeNode(filename, sceneGraph, core::move(tris), {}) > 0;
}

#undef wrap

bool STLFormat::writeVertex(io::SeekableWriteStream &stream, const MeshExt &meshExt, const voxel::VoxelVertex &v1,
							const scenegraph::SceneGraphTransform &transform, const glm::vec3 &scale) {
	glm::vec3 pos;
	if (meshExt.applyTransform) {
		pos = transform.apply(v1.position, meshExt.pivot * meshExt.size);
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

bool STLFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	stream->writeStringFormat(false, "github.com/vengi-voxel/vengi");
	const size_t delta = priv::BinaryHeaderSize - stream->pos();
	for (size_t i = 0; i < delta; ++i) {
		stream->writeUInt8(0);
	}
	core_assert(stream->pos() == priv::BinaryHeaderSize);

	int faceCount = 0;
	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			const int ni = (int)mesh->getNoOfIndices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}
			faceCount += ni / 3;
		}
	}
	stream->writeUInt32(faceCount);

	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			Log::debug("Exporting model %s", meshExt.name.c_str());
			const int ni = (int)mesh->getNoOfIndices();
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
			const voxel::IndexType *indices = mesh->getRawIndexData();

			for (int j = 0; j < ni; j += 3) {
				const uint32_t one = indices[j + 0];
				const uint32_t two = indices[j + 1];
				const uint32_t three = indices[j + 2];

				const voxel::VoxelVertex &v1 = vertices[one];
				const voxel::VoxelVertex &v2 = vertices[two];
				const voxel::VoxelVertex &v3 = vertices[three];

				// normal
				const glm::vec3 edge1 = glm::vec3(v2.position - v1.position);
				const glm::vec3 edge2 = glm::vec3(v3.position - v1.position);
				const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
				for (int k = 0; k < 3; ++k) {
					if (!stream->writeFloat(normal[k])) {
						return false;
					}
				}

				if (!writeVertex(*stream, meshExt, v1, transform, scale)) {
					return false;
				}

				if (!writeVertex(*stream, meshExt, v2, transform, scale)) {
					return false;
				}

				if (!writeVertex(*stream, meshExt, v3, transform, scale)) {
					return false;
				}

				stream->writeUInt16(0);
			}
		}
	}
	return true;
}

} // namespace voxelformat
