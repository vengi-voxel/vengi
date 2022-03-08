/**
 * @file
 */

#include "STLFormat.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/Var.h"
#include "voxel/Mesh.h"
#include <SDL_stdinc.h>

namespace voxel {

namespace priv {
static constexpr const size_t BinaryHeaderSize = 80;
}

void STLFormat::subdivideShape(const core::DynamicArray<Face> &faces, core::DynamicArray<Tri> &subdivided) {
	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	for (const Face &face : faces) {
		Tri tri;
		for (int i = 0; i < 3; ++i) {
			tri.vertices[i].x = face.tri[i].x * scaleX;
			tri.vertices[i].y = face.tri[i].y * scaleY;
			tri.vertices[i].z = face.tri[i].z * scaleZ;
			tri.uv[i] = glm::vec2(0.0f);
		}

		subdivideTri(tri, subdivided);
	}
}

void STLFormat::calculateAABB(const core::DynamicArray<Face> &faces, glm::vec3 &mins, glm::vec3 &maxs) {
	maxs = glm::vec3(-100000.0f);
	mins = glm::vec3(+100000.0f);

	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	for (const Face &face : faces) {
		for (int i = 0; i < 3; ++i) {
			maxs.x = core_max(maxs.x, face.tri[i].x) * scaleX;
			maxs.y = core_max(maxs.y, face.tri[i].y) * scaleY;
			maxs.z = core_max(maxs.z, face.tri[i].z) * scaleZ;
			mins.x = core_min(mins.x, face.tri[i].x) * scaleX;
			mins.y = core_min(mins.y, face.tri[i].y) * scaleY;
			mins.z = core_min(mins.z, face.tri[i].z) * scaleZ;
		}
	}
}

bool STLFormat::parseAscii(io::SeekableReadStream &stream, core::DynamicArray<Face> &faces) {
	char line[512];
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
					Face face;
					glm::vec3 &norm = face.normal;
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
							glm::vec3 &vert = face.tri[vi];
							if (SDL_sscanf(ptr, "vertex %f %f %f", &vert.x, &vert.y, &vert.z) != 3) {
								Log::error("Failed to parse vertex");
								return false;
							}
							++vi;
						}
						if (vi != 3) {
							return false;
						}
						faces.push_back(face);
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

bool STLFormat::parseBinary(io::SeekableReadStream &stream, core::DynamicArray<Face> &faces) {
	stream.seek(priv::BinaryHeaderSize);
	uint32_t numFaces = 0;
	wrap(stream.readUInt32(numFaces))
	Log::debug("faces: %u", numFaces);
	if (numFaces == 0) {
		Log::error("No faces in stl file");
		return false;
	}
	faces.reserve(numFaces);
	for (uint32_t fn = 0; fn < numFaces; ++fn) {
		Face face{};
		wrap(stream.readFloat(face.normal.x))
		wrap(stream.readFloat(face.normal.y))
		wrap(stream.readFloat(face.normal.z))
		for (int i = 0; i < 3; ++i) {
			wrap(stream.readFloat(face.tri[i].x))
			wrap(stream.readFloat(face.tri[i].y))
			wrap(stream.readFloat(face.tri[i].z))
		}
		stream.skip(2);
		faces.push_back(face);
	}

	return !faces.empty();
}

bool STLFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint32_t magic;
	wrap(stream.readUInt32(magic));
	const bool ascii = FourCC('s', 'o', 'l', 'i') == magic;

	core::DynamicArray<Face> faces;
	if (ascii) {
		Log::debug("found ascii format");
		if (!parseAscii(stream, faces)) {
			Log::error("Failed to parse ascii stl file %s", filename.c_str());
			return false;
		}
	} else {
		Log::debug("found binary format");
		if (!parseBinary(stream, faces)) {
			Log::error("Failed to parse binary stl file %s", filename.c_str());
			return false;
		}
	}

	glm::vec3 mins;
	glm::vec3 maxs;
	calculateAABB(faces, mins, maxs);
	voxel::Region region(glm::floor(mins), glm::ceil(maxs));
	if (!region.isValid()) {
		Log::error("Invalid region: %s", region.toString().c_str());
		return false;
	}

	if (glm::any(glm::greaterThan(region.getDimensionsInVoxels(), glm::ivec3(512)))) {
		Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh!");
	}
	RawVolume *volume = new RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	core::DynamicArray<Tri> subdivided;
	subdivideShape(faces, subdivided);
	voxelizeTris(volume, subdivided);
	sceneGraph.emplace(core::move(node));
	return true;
}

#undef wrap

bool STLFormat::writeVertex(io::SeekableWriteStream &stream, const MeshExt &meshExt, const voxel::VoxelVertex &v1, const glm::vec3 &offset, const glm::vec3 &scale) {
	glm::vec3 pos;
	if (meshExt.applyTransform) {
		pos = meshExt.transform.apply(v1.position, meshExt.size);
	} else {
		pos = v1.position;
	}
	pos = (offset + pos) * scale;
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

bool STLFormat::saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
				const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	stream.writeStringFormat(false, "github.com/mgerhardy/vengi");
	const size_t delta = priv::BinaryHeaderSize - stream.pos();
	for (size_t i = 0; i < delta; ++i) {
		stream.writeUInt8(0);
	}

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

	int idxOffset = 0;
	for (const auto &meshExt : meshes) {
		const voxel::Mesh *mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int nv = (int)mesh->getNoOfVertices();
		const int ni = (int)mesh->getNoOfIndices();
		const glm::vec3 offset(mesh->getOffset());
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();

		for (int i = 0; i < ni; i += 3) {
			const uint32_t one = idxOffset + indices[i + 0] + 1;
			const uint32_t two = idxOffset + indices[i + 1] + 1;
			const uint32_t three = idxOffset + indices[i + 2] + 1;

			const voxel::VoxelVertex &v1 = vertices[one];
			const voxel::VoxelVertex &v2 = vertices[two];
			const voxel::VoxelVertex &v3 = vertices[three];

			// normal
			for (int j = 0; j < 3; ++j) {
				if (!stream.writeFloat(0)) {
					return false;
				}
			}

			if (!writeVertex(stream, meshExt, v1, offset, scale)) {
				return false;
			}

			if (!writeVertex(stream, meshExt, v2, offset, scale)) {
				return false;
			}

			if (!writeVertex(stream, meshExt, v3, offset, scale)) {
				return false;
			}

			stream.writeUInt16(0);
		}
		idxOffset += nv;
	}
	return true;
}

} // namespace voxel
