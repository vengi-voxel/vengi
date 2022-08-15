/**
 * @file
 */

#include "QuakeBSPFormat.h"
#include "app/App.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Concurrency.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelformat {

namespace _priv {

static const uint32_t ufoaiVerticesLump = 2;
static const uint32_t ufoaiTexinfoLump = 5;
static const uint32_t ufoaiFacesLump = 6;
static const uint32_t ufoaiEdgesLump = 11;
static const uint32_t ufoaiSurfedgesLump = 12;

} // namespace _priv

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Failed to read bsp " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

int32_t QuakeBSPFormat::validateLump(const BspLump &lump, size_t elementSize) const {
	if (lump.len % elementSize) {
		Log::error("Unexpected lump size found: %u versus element size %u", lump.len, (uint32_t)elementSize);
		return -1;
	}
	const uint32_t count = lump.len / elementSize;
	Log::debug("%u entries found in lump at offset %u of size %u", count, lump.offset, lump.len);
	return (int32_t)count;
}

static core::String extractBaseDir(const core::String &filename) {
	const size_t pos = filename.rfind("maps/");
	if (pos == core::String::npos) {
		return "";
	}
	return filename.substr(0, pos);
}

bool QuakeBSPFormat::loadUFOAlienInvasionTextures(const core::String &filename, io::SeekableReadStream &stream,
												  const BspHeader &header, core::DynamicArray<Texture> &textures,
												  core::StringMap<image::ImagePtr> &textureMap) {
	const int32_t textureCount = validateLump(header.lumps[_priv::ufoaiTexinfoLump], sizeof(BspTexture));
	if (textureCount <= 0) {
		Log::error("Invalid bsp file with no textures in lump");
		return false;
	}

	if (stream.seek(header.lumps[_priv::ufoaiTexinfoLump].offset) == -1) {
		Log::error("Invalid texture lump offset - can't seek");
		return false;
	}

	textures.resize(textureCount);
	for (int32_t i = 0; i < textureCount; i++) {
		Texture &texture = textures[i];
		for (int k = 0; k < 2; ++k) {
			for (int j = 0; j < 4; ++j) {
				wrap(stream.readFloat(texture.st[k][j]))
			}
		}
		wrap(stream.readUInt32(texture.surfaceFlags))
		wrap(stream.readUInt32(texture.value))
		if (!stream.readString(lengthof(texture.name), texture.name, false)) {
			Log::error("Failed to read bsp - texture name invalid");
			return false;
		}

		auto iter = textureMap.find(texture.name);
		if (iter != textureMap.end()) {
			Log::debug("texture for material '%s' is already loaded", texture.name);
			texture.image = iter->second;
			continue;
		}

		core::String textureName = texture.name;
		const core::String &path = extractBaseDir(filename);
		textureName = core::string::path(path, "textures", textureName);
		Log::debug("Search image %s in path %s", textureName.c_str(), path.c_str());
		image::ImagePtr tex = image::loadImage(textureName, false);
		if (tex->isLoaded()) {
			Log::debug("Use image %s", textureName.c_str());
			textureMap.put(textureName, tex);
			texture.image = tex;
		} else {
			Log::warn("Failed to load %s", textureName.c_str());
		}
	}
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionFaces(io::SeekableReadStream &stream, const BspHeader &header,
											   core::DynamicArray<Face> &faces) {
	const int32_t faceCount = validateLump(header.lumps[_priv::ufoaiFacesLump], sizeof(BspFace));
	if (faceCount <= 0) {
		Log::error("Invalid bsp file with no faces in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiFacesLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	faces.resize(faceCount);
	for (int32_t i = 0; i < faceCount; i++) {
		stream.skip(2); // planeId
		stream.skip(2); // side

		Face &face = faces[i];
		wrap(stream.readInt32(face.edgeId))
		wrap(stream.readInt16(face.edgeCount))
		wrap(stream.readInt16(face.textureId))

		stream.skip(4); // lightofsDay
		stream.skip(4); // lightofsNight
	}
	Log::debug("Loaded %i faces", faceCount);
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionEdges(io::SeekableReadStream &stream, const BspHeader &header,
											   core::DynamicArray<BspEdge> &edges,
											   core::DynamicArray<int32_t> &surfEdges) {
	const int32_t edgeCount = validateLump(header.lumps[_priv::ufoaiEdgesLump], sizeof(BspEdge));
	if (edgeCount <= 0) {
		Log::error("Invalid bsp file with no edges in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiEdgesLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	edges.resize(edgeCount);
	for (int32_t i = 0; i < edgeCount; i++) {
		wrap(stream.readInt16(edges[i].vertexIndices[0]))
		wrap(stream.readInt16(edges[i].vertexIndices[1]))
	}
	Log::debug("Loaded %i edges", edgeCount);

	const int32_t surfEdgesCount = validateLump(header.lumps[_priv::ufoaiSurfedgesLump], sizeof(BspEdge));
	if (surfEdgesCount <= 0) {
		Log::error("Invalid bsp file with no surfedges in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiSurfedgesLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	surfEdges.resize(surfEdgesCount);
	for (int32_t i = 0; i < surfEdgesCount; i++) {
		wrap(stream.readInt32(surfEdges[i]))
	}
	Log::debug("Loaded %i surfedges", surfEdgesCount);

	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionVertices(io::SeekableReadStream &stream, const BspHeader &header,
												  core::DynamicArray<BspVertex> &vertices) {
	const int32_t vertexCount = validateLump(header.lumps[_priv::ufoaiVerticesLump], sizeof(BspVertex));
	if (vertexCount <= 0) {
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiVerticesLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	vertices.resize(vertexCount);
	for (int32_t i = 0; i < vertexCount; i++) {
		wrap(stream.readFloat(vertices[i].x))
		wrap(stream.readFloat(vertices[i].y))
		wrap(stream.readFloat(vertices[i].z))
	}
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionBsp(const core::String &filename, io::SeekableReadStream &stream,
											 SceneGraph &sceneGraph, const BspHeader &header) {
	core::StringMap<image::ImagePtr> textureMap;
	core::DynamicArray<Texture> textures;
	if (!loadUFOAlienInvasionTextures(filename, stream, header, textures, textureMap)) {
		Log::error("Failed to load textures");
		return false;
	}

	core::DynamicArray<Face> faces;
	if (!loadUFOAlienInvasionFaces(stream, header, faces)) {
		Log::error("Failed to load faces");
		return false;
	}

	core::DynamicArray<BspEdge> edges;
	core::DynamicArray<int32_t> surfEdges;
	if (!loadUFOAlienInvasionEdges(stream, header, edges, surfEdges)) {
		Log::error("Failed to load edges");
		return false;
	}

	core::DynamicArray<BspVertex> vertices;
	if (!loadUFOAlienInvasionVertices(stream, header, vertices)) {
		Log::error("Failed to load vertices");
		return false;
	}

	int vertexCount = 0;
	int indexCount = 0;
	for (const Face &face : faces) {
		vertexCount += face.edgeCount;
		if (face.edgeCount > 2) {
			indexCount += (face.edgeCount - 2) * 3;
		}
	}

	core::DynamicArray<glm::vec2> texcoords(vertexCount);
	core::DynamicArray<glm::vec3> verts(vertexCount);
	core::DynamicArray<int32_t> textureIndices(vertexCount);
	core::DynamicArray<int32_t> indices(indexCount);

	glm::vec3 mins(100000);
	glm::vec3 maxs(-100000);
	const glm::vec3 &scale = getScale();

	int offset = 0;
	for (Face &face : faces) {
		face.index = offset;

		for (int j = 0; j < face.edgeCount; j++) {
			const int surfEdgesIdx = face.edgeId + j;
			if (surfEdgesIdx < 0 || surfEdgesIdx >= (int)surfEdges.size()) {
				Log::error("Invalid surf edge index given: %i", surfEdgesIdx);
				return false;
			}
			const int index = surfEdges[surfEdgesIdx];

			const BspVertex *vert;
			if (index > 0) {
				const BspEdge *edge = &edges[index];
				vert = &vertices[edge->vertexIndices[0]];
			} else {
				const BspEdge *edge = &edges[-index];
				vert = &vertices[edge->vertexIndices[1]];
			}

			maxs = (glm::max)(maxs, *vert * scale);
			mins = (glm::min)(mins, *vert * scale);

			const Texture &texture = textures[face.textureId];
			const glm::vec3 sdir(texture.st[0]);
			const glm::vec3 tdir(texture.st[1]);

			if (texture.image) {
				/* texture coordinates */
				float s = glm::dot(*vert, sdir) + texture.st[0].w;
				s /= (float)texture.image->width();

				float t = glm::dot(*vert, tdir) + texture.st[1].w;
				t /= (float)texture.image->height();
				texcoords[offset] = glm::vec2(s, t);
			} else {
				texcoords[offset] = glm::vec2(0, 0);
			}
			textureIndices[offset] = face.textureId;
			verts[offset] = *vert;
			++offset;
		}
	}
	int numIndices = 0;
	for (Face &face : faces) {
		if (face.edgeCount <= 2) {
			continue;
		}
		// triangle fan
		const int numTris = face.edgeCount - 2;
		for (int k = 0; k < numTris; k++) {
			indices[numIndices++] = face.index;
			indices[numIndices++] = face.index + k + 1;
			indices[numIndices++] = face.index + k + 2;
		}
	}

	core_assert(numIndices == indexCount);

	core::exchange(mins.y, mins.z);
	core::exchange(maxs.y, maxs.z);

	voxel::Region region(glm::floor(mins), glm::ceil(maxs));
	if (!region.isValid()) {
		Log::error("Invalid region for model: %s", region.toString().c_str());
		return false;
	}

	const glm::ivec3 &vdim = region.getDimensionsInVoxels();
	if (glm::any(glm::greaterThan(vdim, glm::ivec3(512)))) {
		Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh! "
				  "(%i:%i:%i)",
				  vdim.x, vdim.y, vdim.z);
	}

	const int maxLevel = 1; // TODO: 8 - but we need to load the submodels (submodels 0-255 are visible) (CONTENTS_LEVEL_)
	TriCollection subdivided[maxLevel];
	for (int i = 0; i < numIndices; i += 3) {
		Tri tri;
		for (int k = 0; k < 3; ++k) {
			const int idx = indices[i + k];
			tri.vertices[k] = verts[idx] * scale;
			core::exchange(tri.vertices[k].y, tri.vertices[k].z);
			tri.uv[k] = texcoords[idx];
		}
		const Texture &texture = textures[textureIndices[i]];
		// TODO: content flags to get the levels
		int level = 0;
		tri.texture = texture.image;
		Log::debug("idx %i - tri area: %f", i, tri.area());
		subdivideTri(tri, subdivided[level]);
	}

	const bool fillHollow = core::Var::getSafe(cfg::VoxformatFillHollow)->boolVal();
	for (int i = 0; i < maxLevel; ++i) {
		if (subdivided[i].empty()) {
			continue;
		}
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		SceneGraphNode node;
		node.setVolume(volume, true);
		node.setName(core::string::format("Level %i", i));
		PosMap posMap((int)subdivided[i].size() * 3);
		transformTris(subdivided[i], posMap);
		voxelizeTris(node, posMap, fillHollow);
		sceneGraph.emplace(core::move(node));
	}

	return true;
}

bool QuakeBSPFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	BspHeader header;
	wrap(stream.readUInt32(header.magic))
	wrap(stream.readUInt32(header.version))
	for (int i = 0; i < lengthof(header.lumps); ++i) {
		wrap(stream.readUInt32(header.lumps[i].offset))
		wrap(stream.readUInt32(header.lumps[i].len))
	}
	const uint32_t bspMagic = FourCC('I', 'B', 'S', 'P');

	if (header.version == 79 && header.magic == bspMagic) {
		return loadUFOAlienInvasionBsp(filename, stream, sceneGraph, header);
	}

	uint8_t buf[4];
	FourCCRev(buf, header.magic);
	Log::error("Unsupported bsp file with magic %c%c%c%c and version %i", buf[0], buf[1], buf[2], buf[3],
			   header.version);
	return false;
}

#undef wrap

} // namespace voxelformat
