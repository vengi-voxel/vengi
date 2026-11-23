/**
 * @file
 */

#include "QuakeBSPFormat.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/FourCC.h"
#include "core/IProgress.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "image/Image.h"
#include "io/ZipArchive.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/MeshTri.h"
#include "voxelformat/private/mesh/PosSampling.h"
#include "voxelutil/VoxelUtil.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace voxelformat {

namespace _priv {

static const uint32_t ufoaiEntitiesLump = 0;
static const uint32_t ufoaiVerticesLump = 2;
static const uint32_t ufoaiTexinfoLump = 5;
static const uint32_t ufoaiFacesLump = 6;
static const uint32_t ufoaiEdgesLump = 11;
static const uint32_t ufoaiSurfedgesLump = 12;
static const uint32_t ufoaiModelsLump = 13;

static const uint32_t quake1PlanesLump = 1;
static const uint32_t quake1VerticesLump = 3;
static const uint32_t quake1TexturesLump = 2;
static const uint32_t quake1NodesLump = 5;
static const uint32_t quake1TexinfoLump = 6;
static const uint32_t quake1FacesLump = 7;
static const uint32_t quake1LeavesLump = 10;
static const uint32_t quake1EdgesLump = 12;
static const uint32_t quake1SurfedgesLump = 13;
static const uint32_t quake1ModelsLump = 14;

static const size_t quake1PlaneSize = 20;
static const size_t quake1NodeSize = 24;
static const size_t quake1LeafSize = 28;
static const size_t quake1ModelSize = 64;

// shortcut: fixed 2 voxel shell. Raise it if thin brushes disappear.
static const float quakeShellVoxels = 2.0f;
static const int quakeShellChunk = 64;

static char asciiLower(char c) {
	if (c >= 'A' && c <= 'Z') {
		return (char)(c - 'A' + 'a');
	}
	return c;
}

static bool asciiEq(const char *a, const char *b) {
	while (*a != '\0' && *b != '\0') {
		if (asciiLower(*a) != asciiLower(*b)) {
			return false;
		}
		++a;
		++b;
	}
	return *a == *b;
}

static bool asciiStarts(const char *a, const char *b) {
	while (*b != '\0') {
		if (asciiLower(*a) != asciiLower(*b)) {
			return false;
		}
		++a;
		++b;
	}
	return true;
}

bool hiddenQuakeTexture(const char *name, uint32_t value) {
	if (name == nullptr || name[0] == '\0' || name[0] == '*') {
		return true;
	}
	if (value == 1) {
		return true;
	}
	if (asciiStarts(name, "sky")) {
		return true;
	}
	return asciiEq(name, "clip") || asciiEq(name, "trigger") || asciiEq(name, "skip") || asciiEq(name, "hint") ||
		   asciiEq(name, "origin");
}

const char *textureLeaf(const char *name) {
	const char *leaf = name != nullptr ? name : "";
	for (const char *p = leaf; *p != '\0'; ++p) {
		if (*p == '/' || *p == '\\') {
			leaf = p + 1;
		}
	}
	return leaf;
}

// Quake 2 and Quetoo share these surface bits. NODE and BEVEL are Quetoo-only brush sides.
bool hiddenSurface(const char *name, uint32_t flags) {
	const uint32_t hideFlags = 0x4u | 0x80u | 0x100u | 0x200u | 0x20000000u | 0x40000000u;
	if ((flags & hideFlags) != 0u) {
		return true;
	}
	const char *leaf = textureLeaf(name);
	if (hiddenQuakeTexture(leaf, 0)) {
		return true;
	}
	return asciiEq(leaf, "nodraw") || asciiEq(leaf, "caulk");
}

core::String textureBaseDir(const core::String &filename) {
	const size_t pos = filename.rfind("maps/");
	if (pos != core::String::npos) {
		return filename.substr(0, pos);
	}
	return core::string::extractDir(filename);
}

image::ImagePtr loadExternalTexture(const io::ArchivePtr &archive, const core::String &baseDir, const core::String &name) {
	static const char *exts[] = {"png", "jpg", "jpeg", "tga", "webp"};
	const core::String rel = core::string::path("textures", name);
	if (archive) {
		for (const char *ext : exts) {
			const core::String file = core::String::format("%s.%s", rel.c_str(), ext);
			if (!archive->exists(file)) {
				continue;
			}
			core::ScopedPtr<io::SeekableReadStream> texStream(archive->readStream(file));
			if (!texStream) {
				continue;
			}
			const image::ImagePtr &img = image::loadImage(file, *texStream);
			if (img && img->isLoaded()) {
				return img;
			}
		}
	}
	if (!baseDir.empty()) {
		const image::ImagePtr &img = image::loadImage(core::string::path(baseDir, rel));
		if (img && img->isLoaded()) {
			return img;
		}
	}
	return image::ImagePtr{};
}

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
		return core::String::Empty;
	}
	return filename.substr(0, pos);
}

bool QuakeBSPFormat::loadQuake1Textures(const core::String &filename, io::SeekableReadStream &stream,
										const BspHeader &header, core::DynamicArray<Texture> &textures,
										MeshMaterialMap &meshMaterials, MeshMaterialArray &meshMaterialArray) {
	core::Buffer<Quake1Texinfo> miptex;

	struct TextureLump {
		int32_t nummiptex;
		core::Buffer<int32_t> dataofs;
	};

	TextureLump m;
	const uint32_t baseOffset = header.lumps[_priv::quake1TexturesLump].offset;
	{
		if (stream.seek(baseOffset) == -1) {
			Log::error("Invalid texture lump offset - can't seek");
			return false;
		}

		stream.readInt32(m.nummiptex);
		m.dataofs.resize(m.nummiptex);
		for (int i = 0; i < m.nummiptex; ++i) {
			stream.readInt32(m.dataofs[i]);
		}

		miptex.resize(m.nummiptex);
		for (int i = 0; i < m.nummiptex; ++i) {
			if (m.dataofs[i] == -1) {
				continue;
			}
			if (stream.seek(baseOffset + m.dataofs[i]) == -1) {
				Log::error("Invalid texinfo offset - can't seek (%i)", m.dataofs[i]);
				return false;
			}

			Quake1Texinfo &mt = miptex[i];
			stream.readString(sizeof(mt.name), mt.name, false);
			stream.readUInt32(mt.width);
			stream.readUInt32(mt.height);
			stream.readUInt32(mt.offset1);
			stream.readUInt32(mt.offset2);
			stream.readUInt32(mt.offset4);
			stream.readUInt32(mt.offset8);
		}
	}

	const int32_t texInfoCount = validateLump(header.lumps[_priv::quake1TexinfoLump], sizeof(BspTextureBase));
	if (texInfoCount <= 0) {
		Log::error("Invalid bsp file with no textures in lump");
		return false;
	}

	if (stream.seek(header.lumps[_priv::quake1TexinfoLump].offset) == -1) {
		Log::error("Invalid texture lump offset - can't seek");
		return false;
	}

	textures.resize(texInfoCount);
	for (int32_t i = 0; i < texInfoCount; i++) {
		Texture &texture = textures[i];
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecS[j]))
		}
		wrap(stream.readFloat(texture.distS))
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecT[j]))
		}
		wrap(stream.readFloat(texture.distT))
		wrap(stream.readUInt32(texture.surfaceFlags)) // miptex index
		wrap(stream.readUInt32(texture.value)) // 0 = solid, 1 = water
		if (texture.surfaceFlags >= (uint32_t)miptex.size()) {
			Log::warn("Invalid miptex index %u (max %i) - skipping texinfo entry", texture.surfaceFlags,
					  (int)miptex.size());
			texture.name[0] = '\0';
			continue;
		}
		SDL_strlcpy(texture.name, miptex[texture.surfaceFlags].name, sizeof(texture.name));
	}

	palette::Palette pal;
	pal.quake1();

	for (int32_t i = 0; i < texInfoCount; i++) {
		Texture &texture = textures[i];

		auto meshMaterialIter = meshMaterials.find(texture.name);
		if (meshMaterialIter != meshMaterials.end()) {
			texture.materialIdx = meshMaterialIter->second;
			continue;
		}

		if (texture.surfaceFlags >= (uint32_t)miptex.size()) {
			Log::warn("Invalid miptex index %u (max %i) - skipping texture", texture.surfaceFlags, (int)miptex.size());
			continue;
		}

		Quake1Texinfo &texinfo = miptex[texture.surfaceFlags];

		image::ImagePtr tex = image::createEmptyImage(texture.name);
		const int width = (int)texinfo.width;
		const int height = (int)texinfo.height;

		if (stream.seek(baseOffset + m.dataofs[texture.surfaceFlags] + (int)texinfo.offset1) == -1) {
			Log::error("Failed to seek to pixel data %i", i);
			continue;
		}
		const int pixelSize = width * height;
		uint8_t *pixels = (uint8_t *)core_malloc(pixelSize);
		if (stream.read(pixels, pixelSize) == -1) {
			core_free(pixels);
			Log::error("Failed to read %i bytes to pixel data %i", pixelSize, i);
			continue;
		}

		core::Buffer<color::RGBA> buffer;
		buffer.resize(pixelSize);
		const bool masked = texture.name[0] == '{';
		for (int j = 0; j < pixelSize; ++j) {
			buffer[j] = pal.color(pixels[j]);
			// Quake { textures use palette index 255 as the hole color.
			if (masked && pixels[j] == 255) {
				buffer[j].a = 0;
			}
		}
		if (tex->loadRGBA((const uint8_t *)buffer.data(), width, height)) {
			Log::debug("Use image %s", texture.name);
			meshMaterialArray.push_back(createMaterial(tex));
			texture.materialIdx = meshMaterialArray.size() - 1;
			meshMaterials.put(texture.name, texture.materialIdx);
		} else {
			Log::warn("Failed to load %s", texture.name);
		}
		core_free(pixels);
	}
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionTextures(const core::String &filename, io::SeekableReadStream &stream,
												  const BspHeader &header, core::DynamicArray<Texture> &textures,
												  MeshMaterialMap &meshMaterials, MeshMaterialArray &meshMaterialArray) {
	const int32_t textureCount = validateLump(header.lumps[_priv::ufoaiTexinfoLump], sizeof(BspTexture));
	if (textureCount <= 0) {
		Log::error("Invalid bsp file with no textures in lump");
		return false;
	}

	if (stream.seek(header.lumps[_priv::ufoaiTexinfoLump].offset) == -1) {
		Log::error("Invalid texture lump offset - can't seek");
		return false;
	}

	textures.resize((size_t)textureCount);
	for (int32_t i = 0; i < textureCount; i++) {
		Texture &texture = textures[i];
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecS[j]))
		}
		wrap(stream.readFloat(texture.distS))
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecT[j]))
		}
		wrap(stream.readFloat(texture.distT))
		wrap(stream.readUInt32(texture.surfaceFlags))
		wrap(stream.readUInt32(texture.value))
		if (!stream.readString(lengthof(texture.name), texture.name, false)) {
			Log::error("Failed to read bsp - texture name invalid");
			return false;
		}

		auto meshMaterialIter = meshMaterials.find(texture.name);
		if (meshMaterialIter != meshMaterials.end()) {
			Log::debug("texture for material '%s' is already loaded", texture.name);
			texture.materialIdx = meshMaterialIter->second;
			continue;
		}

		const core::String &path = extractBaseDir(filename);
		const core::String &textureName = core::string::path(path, "textures", texture.name);
		Log::debug("Search image %s in path %s", textureName.c_str(), path.c_str());
		const image::ImagePtr &tex = image::loadImage(textureName);
		if (tex->isLoaded()) {
			Log::debug("Use image %s", textureName.c_str());
			meshMaterialArray.push_back(createMaterial(tex));
			texture.materialIdx = meshMaterialArray.size() - 1;
			meshMaterials.put(textureName, texture.materialIdx);
		} else {
			Log::warn("Failed to load %s", textureName.c_str());
		}
	}
	return true;
}

bool QuakeBSPFormat::loadQuake1Faces(io::SeekableReadStream &stream, const BspHeader &header,
									 core::Buffer<Face> &faces, const core::DynamicArray<Texture> &textures) {
	const int32_t faceCount = validateLump(header.lumps[_priv::quake1FacesLump], sizeof(BspFace));
	if (faceCount <= 0) {
		Log::error("Invalid bsp file with no faces in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1FacesLump].offset) == -1) {
		Log::error("Invalid faces lump offset - can't seek");
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

		if (face.textureId < 0 || face.textureId >= (int)textures.size()) {
			face.hidden = true;
		} else {
			const Texture &texture = textures[face.textureId];
			face.hidden = _priv::hiddenQuakeTexture(texture.name, texture.value);
			if (face.hidden) {
				Log::debug("skip face with %s", texture.name);
			}
		}

		uint8_t lightType; // type of lighting, for the face
		uint8_t baselight; // from 0xFF (dark) to 0 (bright)
		uint8_t light[2];  // two additional light models
		int32_t lightmap;  // Pointer inside the general light map, or -1
						   // this define the start of the face light map

		wrap(stream.readUInt8(lightType))
		wrap(stream.readUInt8(baselight))
		wrap(stream.readUInt8(light[0]))
		wrap(stream.readUInt8(light[1]))

		wrap(stream.readInt32(lightmap))
	}
	Log::debug("Loaded %i faces", faceCount);
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionFaces(io::SeekableReadStream &stream, const BspHeader &header,
											   core::Buffer<Face> &faces) {
	const int32_t faceCount = validateLump(header.lumps[_priv::ufoaiFacesLump], sizeof(BspFace));
	if (faceCount <= 0) {
		Log::error("Invalid bsp file with no faces in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiFacesLump].offset) == -1) {
		Log::error("Invalid faces lump offset - can't seek");
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

bool QuakeBSPFormat::loadUFOAlienInvasionFacesForLevel(io::SeekableReadStream &stream, const BspHeader &header,
													   core::Buffer<Face> &faces,
													   core::Buffer<Face> &facesLevel,
													   const core::Buffer<Model> &models, int level) {
	const uint32_t size = models.size();
	if (size < 255) {
		return false;
	}
	const uint32_t mask = 1u << level;
	// a face that is in level 1, 2 and 3 is in model 7 - visible everywhere is 255
	// not marked for any level is 0 (we are skipping this)
	for (uint32_t i = 0; i <= 255; ++i) {
		if (i && !(i & mask)) {
			continue;
		}
		const int32_t begin = models[i].faceId;
		const int32_t end = begin + models[i].faceCount;
		for (int32_t f = begin; f < end; ++f) {
			core_assert_msg(f >= 0 && f < (int)faces.size(), "Face index is out of bounds: %i vs %i", f,
							(int)faces.size());
			Face &face = faces[f];
			if (face.used) {
				continue;
			}
			facesLevel.push_back(face);
			face.used = true;
		}
	}
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionEdges(io::SeekableReadStream &stream, const BspHeader &header,
											   core::Buffer<BspEdge> &edges,
											   core::Buffer<int32_t> &surfEdges) {
	const int32_t edgeCount = validateLump(header.lumps[_priv::ufoaiEdgesLump], sizeof(BspEdge));
	if (edgeCount <= 0) {
		Log::error("Invalid bsp file with no edges in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiEdgesLump].offset) == -1) {
		Log::error("Invalid edges lump offset - can't seek");
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
		Log::error("Invalid surfedges lump offset - can't seek");
		return false;
	}
	surfEdges.resize(surfEdgesCount);
	for (int32_t i = 0; i < surfEdgesCount; i++) {
		wrap(stream.readInt32(surfEdges[i]))
	}
	Log::debug("Loaded %i surfedges", surfEdgesCount);

	return true;
}

bool QuakeBSPFormat::loadQuake1Edges(io::SeekableReadStream &stream, const BspHeader &header,
									 core::Buffer<BspEdge> &edges, core::Buffer<int32_t> &surfEdges) {
	const int32_t edgeCount = validateLump(header.lumps[_priv::quake1EdgesLump], sizeof(BspEdge));
	if (edgeCount <= 0) {
		Log::error("Invalid bsp file with no edges in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1EdgesLump].offset) == -1) {
		Log::error("Invalid edges lump offset - can't seek");
		return false;
	}
	edges.resize(edgeCount);
	for (int32_t i = 0; i < edgeCount; i++) {
		wrap(stream.readInt16(edges[i].vertexIndices[0]))
		wrap(stream.readInt16(edges[i].vertexIndices[1]))
	}
	Log::debug("Loaded %i edges", edgeCount);

	const int32_t surfEdgesCount = validateLump(header.lumps[_priv::quake1SurfedgesLump], sizeof(BspEdge));
	if (surfEdgesCount <= 0) {
		Log::error("Invalid bsp file with no surfedges in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1SurfedgesLump].offset) == -1) {
		Log::error("Invalid surfedges lump offset - can't seek");
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
												  core::Buffer<BspVertex> &vertices) {
	const int32_t vertexCount = validateLump(header.lumps[_priv::ufoaiVerticesLump], sizeof(BspVertex));
	if (vertexCount <= 0) {
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiVerticesLump].offset) == -1) {
		Log::error("Invalid vertices lump offset - can't seek");
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

bool QuakeBSPFormat::loadQuake1Vertices(io::SeekableReadStream &stream, const BspHeader &header,
										core::Buffer<BspVertex> &vertices) {
	const int32_t vertexCount = validateLump(header.lumps[_priv::quake1VerticesLump], sizeof(BspVertex));
	if (vertexCount <= 0) {
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1VerticesLump].offset) == -1) {
		Log::error("Invalid vertices lump offset - can't seek");
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

float QuakeBSPFormat::distSqPointTriangle(const math::Vec3f &p, const math::Vec3f &a, const math::Vec3f &b, const math::Vec3f &c) {
	const auto len2 = [](const math::Vec3f &v) {
		return math::dot(v, v);
	};
	const math::Vec3f ab = b - a;
	const math::Vec3f ac = c - a;
	const math::Vec3f ap = p - a;
	const float d1 = math::dot(ab, ap);
	const float d2 = math::dot(ac, ap);
	if (d1 <= 0.0f && d2 <= 0.0f) {
		return len2(ap);
	}
	const math::Vec3f bp = p - b;
	const float d3 = math::dot(ab, bp);
	const float d4 = math::dot(ac, bp);
	if (d3 >= 0.0f && d4 <= d3) {
		return len2(bp);
	}
	const float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
		const float v = d1 / (d1 - d3);
		return len2(a + ab * v - p);
	}
	const math::Vec3f cp = p - c;
	const float d5 = math::dot(ab, cp);
	const float d6 = math::dot(ac, cp);
	if (d6 >= 0.0f && d5 <= d6) {
		return len2(cp);
	}
	const float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
		const float w = d2 / (d2 - d6);
		return len2(a + ac * w - p);
	}
	const float va = d3 * d6 - d5 * d4;
	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
		const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return len2(b + (c - b) * w - p);
	}
	const float denom = 1.0f / (va + vb + vc);
	const float v = vb * denom;
	const float w = vc * denom;
	return len2(a + ab * v + ac * w - p);
}

bool QuakeBSPFormat::quakePointSolid(const core::Buffer<Quake1Plane> &planes, const core::Buffer<Quake1Node> &nodes,
									 const core::Buffer<Quake1Leaf> &leaves, int root, const math::Vec3f &pos,
									 const math::Vec3f &half) {
	if (root < 0) {
		return false;
	}
	// shortcut: stack of 256. A deeper tree is treated as solid.
	int stack[256];
	int sp = 0;
	stack[sp++] = root;
	const math::Vec3f pmin = pos - half;
	const math::Vec3f pmax = pos + half;
	while (sp > 0) {
		const int idx = stack[--sp];
		if (idx < 0) {
			const int leaf = ~idx;
			if (leaf < 0 || leaf >= (int)leaves.size()) {
				continue;
			}
			const int32_t contents = leaves[leaf].contents;
			const bool empty = contents == -1 || (contents <= -3 && contents >= -14);
			if (!empty) {
				return true;
			}
			continue;
		}
		if (idx >= (int)nodes.size()) {
			continue;
		}
		const Quake1Node &node = nodes[idx];
		if (node.plane < 0 || node.plane >= (int)planes.size()) {
			continue;
		}
		const Quake1Plane &plane = planes[node.plane];
		float minD = 0.0f;
		float maxD = 0.0f;
		for (int axis = 0; axis < 3; ++axis) {
			const float n = plane.normal[axis];
			if (n > 0.0f) {
				minD += n * pmin[axis];
				maxD += n * pmax[axis];
			} else {
				minD += n * pmax[axis];
				maxD += n * pmin[axis];
			}
		}
		const float d = plane.normal[0] * pos.x + plane.normal[1] * pos.y + plane.normal[2] * pos.z - plane.dist;
		const bool straddle = minD < plane.dist && maxD > plane.dist;
		const int front = d >= 0.0f ? node.children[0] : node.children[1];
		const int back = d >= 0.0f ? node.children[1] : node.children[0];
		if (straddle) {
			if (sp >= 256) {
				return true;
			}
			stack[sp++] = back;
			if (sp >= 256) {
				return true;
			}
			stack[sp++] = front;
		} else {
			if (sp >= 256) {
				return true;
			}
			stack[sp++] = front;
		}
	}
	return false;
}

bool QuakeBSPFormat::loadQuake1Planes(io::SeekableReadStream &stream, const BspHeader &header,
									  core::Buffer<Quake1Plane> &planes) {
	const int32_t count = validateLump(header.lumps[_priv::quake1PlanesLump], _priv::quake1PlaneSize);
	if (count <= 0) {
		Log::error("Invalid bsp file with no planes");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1PlanesLump].offset) == -1) {
		Log::error("Invalid planes lump offset - can't seek");
		return false;
	}
	planes.resize(count);
	for (int32_t i = 0; i < count; ++i) {
		wrap(stream.readFloat(planes[i].normal[0]))
		wrap(stream.readFloat(planes[i].normal[1]))
		wrap(stream.readFloat(planes[i].normal[2]))
		wrap(stream.readFloat(planes[i].dist))
		if (stream.skip(4) == -1) {
			Log::error("Failed to skip plane type");
			return false;
		}
	}
	return true;
}

bool QuakeBSPFormat::loadQuake1Nodes(io::SeekableReadStream &stream, const BspHeader &header,
									 core::Buffer<Quake1Node> &nodes) {
	const int32_t count = validateLump(header.lumps[_priv::quake1NodesLump], _priv::quake1NodeSize);
	if (count <= 0) {
		Log::error("Invalid bsp file with no nodes");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1NodesLump].offset) == -1) {
		Log::error("Invalid nodes lump offset - can't seek");
		return false;
	}
	nodes.resize(count);
	for (int32_t i = 0; i < count; ++i) {
		wrap(stream.readInt32(nodes[i].plane))
		wrap(stream.readInt16(nodes[i].children[0]))
		wrap(stream.readInt16(nodes[i].children[1]))
		if (stream.skip(16) == -1) {
			Log::error("Failed to skip node bounds");
			return false;
		}
	}
	return true;
}

bool QuakeBSPFormat::loadQuake1Leaves(io::SeekableReadStream &stream, const BspHeader &header,
									  core::Buffer<Quake1Leaf> &leaves) {
	const int32_t count = validateLump(header.lumps[_priv::quake1LeavesLump], _priv::quake1LeafSize);
	if (count <= 0) {
		Log::error("Invalid bsp file with no leaves");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1LeavesLump].offset) == -1) {
		Log::error("Invalid leaves lump offset - can't seek");
		return false;
	}
	leaves.resize(count);
	for (int32_t i = 0; i < count; ++i) {
		wrap(stream.readInt32(leaves[i].contents))
		if (stream.skip(24) == -1) {
			Log::error("Failed to skip leaf data");
			return false;
		}
	}
	return true;
}

bool QuakeBSPFormat::loadQuake1BrushModels(io::SeekableReadStream &stream, const BspHeader &header,
										   core::Buffer<Quake1Model> &models) {
	const int32_t count = validateLump(header.lumps[_priv::quake1ModelsLump], _priv::quake1ModelSize);
	if (count <= 0) {
		Log::error("Invalid bsp file with no models");
		return false;
	}
	if (stream.seek(header.lumps[_priv::quake1ModelsLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	models.resize(count);
	for (int32_t i = 0; i < count; ++i) {
		if (stream.skip(36) == -1) {
			Log::error("Failed to skip model bounds");
			return false;
		}
		wrap(stream.readInt32(models[i].headnode))
		if (stream.skip(16) == -1) {
			Log::error("Failed to skip model hulls");
			return false;
		}
		wrap(stream.readInt32(models[i].faceId))
		wrap(stream.readInt32(models[i].faceCount))
	}
	return true;
}

bool QuakeBSPFormat::voxelizeQuake1Models(const core::DynamicArray<Texture> &textures, core::Buffer<Face> &faces,
										  const core::Buffer<BspEdge> &edges, const core::Buffer<int32_t> &surfEdges,
										  const core::Buffer<BspVertex> &vertices,
										  const core::Buffer<Quake1Plane> &planes, const core::Buffer<Quake1Node> &nodes,
										  const core::Buffer<Quake1Leaf> &leaves, const core::Buffer<Quake1Model> &models,
										  scenegraph::SceneGraph &sceneGraph, const core::String &name,
										  const MeshMaterialArray &meshMaterialArray) {
	const math::Vec3f scale = sceneInputScale();
	if (scale.x == 0.0f || scale.y == 0.0f || scale.z == 0.0f) {
		Log::error("Invalid scale for bsp voxelization");
		return false;
	}
	palette::Palette palette;
	if (!palette.quake1()) {
		Log::error("Failed to load the quake palette");
		return false;
	}
	palette::PaletteLookup palLookup(palette);
	palette::NormalPalette normalPalette;
	const core::VarPtr &normalPaletteVar = core::getVar(cfg::NormalPalette);
	if (!normalPalette.load(normalPaletteVar->strVal().c_str())) {
		normalPalette.redAlert2();
	}
	const math::Vec3f half(0.5f / scale.x, 0.5f / scale.z, 0.5f / scale.y);
	const float shellSq = _priv::quakeShellVoxels * _priv::quakeShellVoxels;
	bool any = false;

	for (int modelIndex = 0; modelIndex < (int)models.size(); ++modelIndex) {
		if (stopExecution()) {
			break;
		}
		const Quake1Model &model = models[modelIndex];
		if (model.faceId < 0 || model.faceCount < 0 || model.faceId + model.faceCount > (int)faces.size()) {
			Log::error("Model %i face range is out of bounds", modelIndex);
			return false;
		}
		const core::String modelName =
			modelIndex == 0 ? name : core::String::format("%s*%i", name.c_str(), modelIndex);

		core::Buffer<Face> visible;
		for (int f = model.faceId; f < model.faceId + model.faceCount; ++f) {
			if (!faces[f].hidden) {
				visible.push_back(faces[f]);
			}
		}
		if (visible.empty()) {
			continue;
		}
		const bool haveHull = model.headnode >= 0 && model.headnode < (int)nodes.size();
		if (!haveHull) {
			if (voxelize(textures, visible, edges, surfEdges, vertices, sceneGraph, modelName, meshMaterialArray,
						 nullptr)) {
				any = true;
			}
			continue;
		}

		core::DynamicArray<MeshTri> tris;
		for (Face &face : visible) {
			core::DynamicArray<math::Vec3f> poly;
			core::DynamicArray<math::Vec2f> uvs;
			bool bad = false;
			for (int j = 0; j < face.edgeCount; ++j) {
				const int surfEdgesIdx = face.edgeId + j;
				if (surfEdgesIdx < 0 || surfEdgesIdx >= (int)surfEdges.size()) {
					bad = true;
					break;
				}
				const int index = surfEdges[surfEdgesIdx];
				const int edgeIndex = index >= 0 ? index : -index;
				if (edgeIndex < 0 || edgeIndex >= (int)edges.size()) {
					bad = true;
					break;
				}
				const BspEdge &edge = edges[edgeIndex];
				const int vertIndex = index > 0 ? edge.vertexIndices[0] : edge.vertexIndices[1];
				if (vertIndex < 0 || vertIndex >= (int)vertices.size()) {
					bad = true;
					break;
				}
				const BspVertex &vert = vertices[vertIndex];
				const math::Vec3f quakePos(vert.x, vert.y, vert.z);
				poly.push_back(math::Vec3f(quakePos.x * scale.x, quakePos.z * scale.y, quakePos.y * scale.z));
				math::Vec2f uv(0.0f);
				if (face.textureId >= 0 && face.textureId < (int)textures.size()) {
					const Texture &texture = textures[face.textureId];
					if (texture.materialIdx >= 0 && texture.materialIdx < (int)meshMaterialArray.size()) {
						const MeshMaterialPtr &material = meshMaterialArray[texture.materialIdx];
						if (material->texture) {
							const math::Vec3f sdir(texture.vecS[0], texture.vecS[1], texture.vecS[2]);
							const math::Vec3f tdir(texture.vecT[0], texture.vecT[1], texture.vecT[2]);
							uv.x = (math::dot(quakePos, sdir) + texture.distS) / (float)material->texture->width();
							uv.y = (math::dot(quakePos, tdir) + texture.distT) / (float)material->texture->height();
						}
					}
				}
				uvs.push_back(uv);
			}
			if (bad) {
				Log::error("Invalid face edges in model %i", modelIndex);
				return false;
			}
			if (poly.size() < 3) {
				continue;
			}
			const color::RGBA white(255, 255, 255, 255);
			const MeshMaterialIndex materialIdx =
				face.textureId >= 0 && face.textureId < (int)textures.size() ? textures[face.textureId].materialIdx : -1;
			for (int k = 1; k + 1 < (int)poly.size(); ++k) {
				MeshTri tri;
				tri.materialIdx = materialIdx;
				tri.setVertices(poly[0], poly[k], poly[k + 1]);
				tri.setUVs(uvs[0], uvs[k], uvs[k + 1]);
				tri.setColor(white);
				tris.push_back(tri);
			}
		}
		if (tris.empty()) {
			continue;
		}

		math::Vec3f boundsMin(tris[0].mins());
		math::Vec3f boundsMax(tris[0].maxs());
		for (const MeshTri &tri : tris) {
			boundsMin = math::min(boundsMin, tri.mins());
			boundsMax = math::max(boundsMax, tri.maxs());
		}
		const math::Vec3f shell(_priv::quakeShellVoxels);
		const int x0 = (int)math::floor(boundsMin.x - shell.x);
		const int y0 = (int)math::floor(boundsMin.y - shell.y);
		const int z0 = (int)math::floor(boundsMin.z - shell.z);
		const int x1 = (int)math::floor(boundsMax.x + shell.x);
		const int y1 = (int)math::floor(boundsMax.y + shell.y);
		const int z1 = (int)math::floor(boundsMax.z + shell.z);

		// shortcut: each occupied chunk tests every triangle. Index triangles per chunk if large maps are slow.
		core::Map<math::Vec3i, int, 1031, math::Vec3iHash> chunkIds(16384);
		bool tooManyChunks = false;
		for (const MeshTri &tri : tris) {
			const int cx0 = ((int)math::floor(tri.mins().x - shell.x) - x0) / _priv::quakeShellChunk;
			const int cy0 = ((int)math::floor(tri.mins().y - shell.y) - y0) / _priv::quakeShellChunk;
			const int cz0 = ((int)math::floor(tri.mins().z - shell.z) - z0) / _priv::quakeShellChunk;
			const int cx1 = ((int)math::floor(tri.maxs().x + shell.x) - x0) / _priv::quakeShellChunk;
			const int cy1 = ((int)math::floor(tri.maxs().y + shell.y) - y0) / _priv::quakeShellChunk;
			const int cz1 = ((int)math::floor(tri.maxs().z + shell.z) - z0) / _priv::quakeShellChunk;
			for (int cz = cz0; cz <= cz1 && !tooManyChunks; ++cz) {
				for (int cy = cy0; cy <= cy1 && !tooManyChunks; ++cy) {
					for (int cx = cx0; cx <= cx1; ++cx) {
						const math::Vec3i key(cx, cy, cz);
						if (chunkIds.find(key) != chunkIds.end()) {
							continue;
						}
						if ((int)chunkIds.size() >= 16384) {
							tooManyChunks = true;
							break;
						}
						chunkIds.put(key, 1);
					}
				}
			}
		}
		if (tooManyChunks) {
			Log::warn("BSP model %s is too large for the solid shell, using triangle voxelization", modelName.c_str());
			if (voxelize(textures, visible, edges, surfEdges, vertices, sceneGraph, modelName, meshMaterialArray,
						 nullptr)) {
				any = true;
			}
			continue;
		}

		core::DynamicArray<math::Vec3i> chunkCoords;
		chunkCoords.reserve((int)chunkIds.size());
		for (auto it = chunkIds.begin(); it != chunkIds.end(); ++it) {
			chunkCoords.push_back(it->key);
		}

		const int parent = [&]() {
			if (chunkCoords.size() <= 1) {
				return 0;
			}
			scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
			group.setName(modelName);
			return sceneGraph.emplace(core::move(group));
		}();

		struct ShellCell {
			float distSq = 1.0e30f;
			uint8_t color = 0;
			uint8_t state = 0;
		};
		core::Buffer<ShellCell> cells;
		int chunkIndex = 0;
		for (const math::Vec3i &chunk : chunkCoords) {
			if (stopExecution()) {
				break;
			}
			const int xStart = x0 + chunk.x * _priv::quakeShellChunk;
			const int yStart = y0 + chunk.y * _priv::quakeShellChunk;
			const int zStart = z0 + chunk.z * _priv::quakeShellChunk;
			const int xEnd = core_min(xStart + _priv::quakeShellChunk - 1, x1);
			const int yEnd = core_min(yStart + _priv::quakeShellChunk - 1, y1);
			const int zEnd = core_min(zStart + _priv::quakeShellChunk - 1, z1);
			const int dx = xEnd - xStart + 1;
			const int dy = yEnd - yStart + 1;
			const int dz = zEnd - zStart + 1;
			if (dx <= 0 || dy <= 0 || dz <= 0) {
				continue;
			}
			const int cellCount = dx * dy * dz;
			cells.resize(cellCount);
			for (int i = 0; i < cellCount; ++i) {
				cells[i] = ShellCell();
			}
			for (const MeshTri &tri : tris) {
				const float eminX = tri.mins().x - shell.x;
				const float eminY = tri.mins().y - shell.y;
				const float eminZ = tri.mins().z - shell.z;
				const float emaxX = tri.maxs().x + shell.x;
				const float emaxY = tri.maxs().y + shell.y;
				const float emaxZ = tri.maxs().z + shell.z;
				if (emaxX < (float)xStart || eminX >= (float)(xEnd + 1) || emaxY < (float)yStart ||
					eminY >= (float)(yEnd + 1) || emaxZ < (float)zStart || eminZ >= (float)(zEnd + 1)) {
					continue;
				}
				const int vx0 = core_max(xStart, (int)math::floor(eminX));
				const int vy0 = core_max(yStart, (int)math::floor(eminY));
				const int vz0 = core_max(zStart, (int)math::floor(eminZ));
				const int vx1 = core_min(xEnd, (int)math::floor(emaxX));
				const int vy1 = core_min(yEnd, (int)math::floor(emaxY));
				const int vz1 = core_min(zEnd, (int)math::floor(emaxZ));
				const math::Vec3f n = math::cross(tri.vertex1() - tri.vertex0(), tri.vertex2() - tri.vertex0());
				const float n2 = math::dot(n, n);
				for (int z = vz0; z <= vz1; ++z) {
					for (int y = vy0; y <= vy1; ++y) {
						for (int x = vx0; x <= vx1; ++x) {
							const math::Vec3f center(x + 0.5f, y + 0.5f, z + 0.5f);
							const float distSq = distSqPointTriangle(center, tri.vertex0(), tri.vertex1(), tri.vertex2());
							if (distSq > shellSq) {
								continue;
							}
							const int cell = (x - xStart) + (y - yStart) * dx + (z - zStart) * dx * dy;
							ShellCell &slot = cells[cell];
							if (distSq >= slot.distSq) {
								continue;
							}
							const math::Vec3f quake = math::Vec3f(center.x / scale.x, center.z / scale.z, center.y / scale.y);
							if (!quakePointSolid(planes, nodes, leaves, model.headnode, quake, half)) {
								continue;
							}
							slot.distSq = distSq;
							math::Vec2f uv = tri.centerUV();
							if (n2 > 1.0e-12f) {
								const math::Vec3f nn = n / sqrtf(n2);
								const math::Vec3f proj = center - nn * math::dot(nn, center - tri.vertex0());
								math::Vec2f projected;
								if (tri.calcUVs(proj, projected)) {
									uv = projected;
								}
							}
							const color::RGBA rgba = colorAt(tri, meshMaterialArray, uv);
							if (rgba.a <= AlphaThreshold) {
								slot.state = 2;
								continue;
							}
							slot.state = 1;
							slot.color = palLookup.findClosestIndex(rgba);
						}
					}
				}
			}

			math::Vec3i tightMin(0);
			math::Vec3i tightMax(0);
			int hitCount = 0;
			for (int z = zStart; z <= zEnd; ++z) {
				for (int y = yStart; y <= yEnd; ++y) {
					for (int x = xStart; x <= xEnd; ++x) {
						const int cell = (x - xStart) + (y - yStart) * dx + (z - zStart) * dx * dy;
						if (cells[cell].state != 1) {
							continue;
						}
						const math::Vec3i pos(x, y, z);
						if (hitCount == 0) {
							tightMin = pos;
							tightMax = pos;
						} else {
							tightMin = math::min(tightMin, pos);
							tightMax = math::max(tightMax, pos);
						}
						++hitCount;
					}
				}
			}
			if (hitCount == 0) {
				continue;
			}
			const voxel::Region region(tightMin, tightMax);
			if (!checkValidRegion(region)) {
				return false;
			}
			voxel::RawVolume *volume = new voxel::RawVolume(region);
			for (int z = zStart; z <= zEnd; ++z) {
				for (int y = yStart; y <= yEnd; ++y) {
					for (int x = xStart; x <= xEnd; ++x) {
						const int cell = (x - xStart) + (y - yStart) * dx + (z - zStart) * dx * dy;
						if (cells[cell].state != 1) {
							continue;
						}
						volume->setVoxel(x, y, z, voxel::createVoxel(palette, cells[cell].color));
					}
				}
			}
			scenegraph::SceneGraphTransform transform;
			transform.setLocalTranslation(region.getLowerCornerf());
			volume->translate(-region.getLowerCorner());
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			if (chunkCoords.size() <= 1) {
				node.setName(modelName);
			} else {
				node.setName(core::String::format("%s_%i", modelName.c_str(), chunkIndex));
			}
			node.setVolume(volume);
			node.setPalette(palette);
			node.setNormalPalette(normalPalette);
			node.setTransform(0, transform);
			if (sceneGraph.emplace(core::move(node), parent) != InvalidNodeId) {
				any = true;
			}
			++chunkIndex;
		}
	}
	return any;
}

bool QuakeBSPFormat::loadQuake1Bsp(const core::String &filename, io::SeekableReadStream &stream,
								   scenegraph::SceneGraph &sceneGraph, const BspHeader &header,
								   core::IProgress *progress) {
	MeshMaterialMap meshMaterials;
	MeshMaterialArray meshMaterialArray;
	core::DynamicArray<Texture> textures;
	if (!loadQuake1Textures(filename, stream, header, textures, meshMaterials, meshMaterialArray)) {
		Log::error("Failed to load textures");
		return false;
	}

	core::Buffer<Face> faces;
	if (!loadQuake1Faces(stream, header, faces, textures)) {
		Log::error("Failed to load faces");
		return false;
	}

	core::Buffer<BspEdge> edges;
	core::Buffer<int32_t> surfEdges;
	if (!loadQuake1Edges(stream, header, edges, surfEdges)) {
		Log::error("Failed to load edges");
		return false;
	}

	core::Buffer<BspVertex> vertices;
	if (!loadQuake1Vertices(stream, header, vertices)) {
		Log::error("Failed to load vertices");
		return false;
	}

	const core::String &name = core::string::extractFilename(filename);
	core::Buffer<Quake1Plane> planes;
	core::Buffer<Quake1Node> nodes;
	core::Buffer<Quake1Leaf> leaves;
	core::Buffer<Quake1Model> models;
	const bool haveTree = !collectingInputBounds() && loadQuake1Planes(stream, header, planes) &&
						  loadQuake1Nodes(stream, header, nodes) && loadQuake1Leaves(stream, header, leaves) &&
						  loadQuake1BrushModels(stream, header, models);
	if (!haveTree) {
		if (!voxelize(textures, faces, edges, surfEdges, vertices, sceneGraph, name, meshMaterialArray, progress)) {
			Log::error("Failed to voxelize %s", filename.c_str());
			return false;
		}
	} else if (!voxelizeQuake1Models(textures, faces, edges, surfEdges, vertices, planes, nodes, leaves, models,
									 sceneGraph, name, meshMaterialArray)) {
		Log::error("Failed to voxelize %s", filename.c_str());
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

bool QuakeBSPFormat::loadUFOAlienInvasionModels(io::SeekableReadStream &stream, const BspHeader &header,
												core::Buffer<Model> &models) {
	const int32_t modelCount = validateLump(header.lumps[_priv::ufoaiModelsLump], sizeof(BspModel));
	if (modelCount <= 0) {
		Log::error("Invalid bsp file with no models in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiModelsLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	models.resize(modelCount);

	const int64_t modelSkipSize = 9 * sizeof(float) + sizeof(int32_t);
	static_assert(sizeof(BspModel) == modelSkipSize + 2 * sizeof(int32_t), "Unexpected BspModel structure size");
	for (int32_t i = 0; i < modelCount; ++i) {
		if (stream.skip(modelSkipSize) == -1) {
			Log::error("Failed to read model %i", i);
			return false;
		}
		Model &mdl = models[i];
		wrap(stream.readInt32(mdl.faceId))
		wrap(stream.readInt32(mdl.faceCount))
		if (mdl.faceCount == 0) {
			Log::debug("model %i (of %i) has no faces", i, modelCount);
		}
	}
	Log::debug("Loaded %i models", modelCount);
	return true;
}

static int parseMaxLevel(const core::String &entities) {
	int maxLevel = 8;
	const size_t start = entities.find("\"maxlevel\"");
	if (start == core::String::npos) {
		Log::debug("No maxlevel found in worldspawn");
		return maxLevel;
	}
	const size_t end = entities.find("\n", start);
	if (end == core::String::npos) {
		Log::warn("Invalid maxlevel found in worldspawn");
		return maxLevel;
	}
	core::String line = entities.substr(start, end).trim();
	line = line.substr(12);
	maxLevel = core::string::toInt(line);
	if (maxLevel == 0) {
		maxLevel = 8;
	}
	Log::debug("Maxlevel: %i", maxLevel);
	return maxLevel;
}

bool QuakeBSPFormat::loadUFOAlienInvasionBsp(const core::String &filename, io::SeekableReadStream &stream,
											 scenegraph::SceneGraph &sceneGraph, const BspHeader &header,
											 core::IProgress *progress) {
	Log::debug("Load textures");
	MeshMaterialMap meshMaterials;
	MeshMaterialArray meshMaterialArray;
	core::DynamicArray<Texture> textures;
	if (!loadUFOAlienInvasionTextures(filename, stream, header, textures, meshMaterials, meshMaterialArray)) {
		Log::error("Failed to load textures");
		return false;
	}

	Log::debug("Load faces");
	core::Buffer<Face> faces;
	if (!loadUFOAlienInvasionFaces(stream, header, faces)) {
		Log::error("Failed to load faces");
		return false;
	}

	Log::debug("Load edges");
	core::Buffer<BspEdge> edges;
	core::Buffer<int32_t> surfEdges;
	if (!loadUFOAlienInvasionEdges(stream, header, edges, surfEdges)) {
		Log::error("Failed to load edges");
		return false;
	}

	Log::debug("Load vertices");
	core::Buffer<BspVertex> vertices;
	if (!loadUFOAlienInvasionVertices(stream, header, vertices)) {
		Log::error("Failed to load vertices");
		return false;
	}

	Log::debug("Load models");
	core::Buffer<Model> models;
	if (!loadUFOAlienInvasionModels(stream, header, models)) {
		Log::error("Failed to load models");
		return false;
	}

	Log::debug("Load entities");
	stream.seek(header.lumps[_priv::ufoaiEntitiesLump].offset);
	core::String entities(header.lumps[_priv::ufoaiEntitiesLump].len, ' ');
	stream.readString((int)header.lumps[_priv::ufoaiEntitiesLump].len, entities.c_str());
	const int maxLevel = parseMaxLevel(entities);

	bool state = false;

	core::Buffer<Face> facesLevel;
	core::StepProgress steps(core::progressOrNull(progress), maxLevel);
	for (int i = 0; i < maxLevel; ++i) {
		Log::debug("Load level %i/%i", i, maxLevel);
		facesLevel.clear();
		if (!loadUFOAlienInvasionFacesForLevel(stream, header, faces, facesLevel, models, i)) {
			Log::debug("No content at level %i - skipping", i);
			continue;
		}
		Log::debug("Voxelize level %i", i);
		core::ProgressRange range = steps.range(i);
		if (voxelize(textures, facesLevel, edges, surfEdges, vertices, sceneGraph,
					 core::String::format("Level %i", i + 1), meshMaterialArray, &range)) {
			state = true;
		}
	}
	sceneGraph.updateTransforms();
	return state;
}

bool QuakeBSPFormat::voxelize(const core::DynamicArray<Texture> &textures, core::Buffer<Face> &faces,
							  const core::Buffer<BspEdge> &edges, const core::Buffer<int32_t> &surfEdges,
							  const core::Buffer<BspVertex> &vertices, scenegraph::SceneGraph &sceneGraph,
							  const core::String &name, const MeshMaterialArray &meshMaterialArray,
							  core::IProgress *progress) {
	int vertexCount = 0;
	int indexCount = 0;
	for (const Face &face : faces) {
		if (face.hidden) {
			continue;
		}
		vertexCount += face.edgeCount;
		if (face.edgeCount > 2) {
			indexCount += (face.edgeCount - 2) * 3;
		}
	}

	Log::debug("Prepare voxeliziation bsp with %i vertices", vertexCount);

	if (vertexCount == 0 || indexCount == 0) {
		Log::debug("No vertices or indices found - skipping");
		return false;
	}
	core::Buffer<math::Vec2f> texcoords(vertexCount);
	core::Buffer<math::Vec3f> verts(vertexCount);
	core::Buffer<int16_t> textureIndices(vertexCount);
	voxel::IndexArray indices(indexCount);

	int offset = 0;
	for (Face &face : faces) {
		if (face.hidden) {
			continue;
		}
		face.index = offset;

		for (int j = 0; j < face.edgeCount; j++) {
			if (stopExecution()) {
				break;
			}
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

			const Texture &texture = textures[face.textureId];
			const math::Vec3f sdir(texture.vecS[0], texture.vecS[1], texture.vecS[2]);
			const math::Vec3f tdir(texture.vecT[0], texture.vecT[1], texture.vecT[2]);

			const math::Vec3f vertex(vert->x, vert->y, vert->z);
			if (texture.materialIdx > 0 && texture.materialIdx < (int)meshMaterialArray.size()) {
				const MeshMaterialPtr &material = meshMaterialArray[texture.materialIdx];
				if (material->texture) {
					/* texture coordinates */
					float s = math::dot(vertex, sdir) + texture.distS;
					s /= (float)material->texture->width();

					float t = math::dot(vertex, tdir) + texture.distT;
					t /= (float)material->texture->height();

					texcoords[offset] = math::Vec2f(s, t);
				} else {
					texcoords[offset] = math::Vec2f(0, 0);
				}
			} else {
				texcoords[offset] = math::Vec2f(0, 0);
			}
			textureIndices[offset] = face.textureId;
			verts[offset] = vertex;
			core::exchange(verts[offset].y, verts[offset].z);
			++offset;
		}
	}
	int numIndices = 0;
	for (const Face &face : faces) {
		if (face.hidden || face.edgeCount <= 2) {
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

	Log::debug("Voxelize bsp with %i vertices", vertexCount);

	Mesh mesh;
	mesh.materials = meshMaterialArray;
	mesh.indices.reserve(numIndices);
	mesh.vertices.reserve(vertexCount);
	for (int i = 0; i < numIndices; i += 3) {
		if (stopExecution()) {
			break;
		}
		voxelformat::MeshTri meshTri;
		const int32_t idx0 = indices[i + 0];
		const int32_t idx1 = indices[i + 1];
		const int32_t idx2 = indices[i + 2];
		meshTri.setVertices(verts[idx0], verts[idx1], verts[idx2]);
		meshTri.setUVs(texcoords[idx0], texcoords[idx1], texcoords[idx2]);
		const uint32_t textureIdx = textureIndices[indices[i]];
		const Texture &texture = textures[textureIdx];
		meshTri.materialIdx = texture.materialIdx;
		mesh.addTriangle(meshTri);
	}

	if (progress != nullptr) {
		progress->setText(name.c_str());
	}
	return importMesh(name, sceneGraph, core::move(mesh), 0, true, progress) != InvalidNodeId;
}

bool QuakeBSPFormat::loadQuake2Textures(const core::String &filename, io::SeekableReadStream &stream,
										const io::ArchivePtr &archive, const BspHeader &header,
										core::DynamicArray<Texture> &textures, MeshMaterialMap &meshMaterials,
										MeshMaterialArray &meshMaterialArray) {
	// Quake 2 texinfo is the UFO layout plus a trailing nexttexinfo int.
	static const size_t quake2TexinfoSize = sizeof(BspTexture) + sizeof(int32_t);
	const int32_t textureCount = validateLump(header.lumps[_priv::ufoaiTexinfoLump], quake2TexinfoSize);
	if (textureCount <= 0) {
		Log::error("Invalid bsp file with no textures in lump");
		return false;
	}
	if (stream.seek(header.lumps[_priv::ufoaiTexinfoLump].offset) == -1) {
		Log::error("Invalid texture lump offset - can't seek");
		return false;
	}

	const core::String &baseDir = _priv::textureBaseDir(filename);
	textures.resize((size_t)textureCount);
	for (int32_t i = 0; i < textureCount; i++) {
		Texture &texture = textures[i];
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecS[j]))
		}
		wrap(stream.readFloat(texture.distS))
		for (int j = 0; j < 3; ++j) {
			wrap(stream.readFloat(texture.vecT[j]))
		}
		wrap(stream.readFloat(texture.distT))
		wrap(stream.readUInt32(texture.surfaceFlags))
		wrap(stream.readUInt32(texture.value))
		if (!stream.readString(lengthof(texture.name), texture.name, false)) {
			Log::error("Failed to read bsp - texture name invalid");
			return false;
		}
		texture.name[lengthof(texture.name) - 1] = '\0';
		stream.skip(4); // nexttexinfo

		auto meshMaterialIter = meshMaterials.find(texture.name);
		if (meshMaterialIter != meshMaterials.end()) {
			texture.materialIdx = meshMaterialIter->second;
			continue;
		}
		const image::ImagePtr &tex = _priv::loadExternalTexture(archive, baseDir, texture.name);
		if (tex && tex->isLoaded()) {
			meshMaterialArray.push_back(createMaterial(tex));
			texture.materialIdx = (MeshMaterialIndex)(meshMaterialArray.size() - 1);
			meshMaterials.put(texture.name, texture.materialIdx);
		} else {
			Log::debug("No image for %s", texture.name);
		}
	}
	return true;
}

bool QuakeBSPFormat::loadQuake2Bsp(const core::String &filename, io::SeekableReadStream &stream,
								   const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
								   const BspHeader &header, core::IProgress *progress) {
	// shortcut: .wal images inside a pak are not read; drop a png/jpg next to the map. A map that fits in one chunk is filled by voxformat_fillhollow.
	MeshMaterialMap meshMaterials;
	MeshMaterialArray meshMaterialArray;
	core::DynamicArray<Texture> textures;
	if (!loadQuake2Textures(filename, stream, archive, header, textures, meshMaterials, meshMaterialArray)) {
		Log::error("Failed to load textures");
		return false;
	}

	core::Buffer<Face> faces;
	if (!loadUFOAlienInvasionFaces(stream, header, faces)) {
		Log::error("Failed to load faces");
		return false;
	}
	for (Face &face : faces) {
		if (face.textureId < 0 || face.textureId >= (int)textures.size()) {
			face.hidden = true;
			continue;
		}
		const Texture &texture = textures[face.textureId];
		face.hidden = _priv::hiddenSurface(texture.name, texture.surfaceFlags);
	}

	core::Buffer<BspEdge> edges;
	core::Buffer<int32_t> surfEdges;
	if (!loadUFOAlienInvasionEdges(stream, header, edges, surfEdges)) {
		Log::error("Failed to load edges");
		return false;
	}
	core::Buffer<BspVertex> vertices;
	if (!loadUFOAlienInvasionVertices(stream, header, vertices)) {
		Log::error("Failed to load vertices");
		return false;
	}
	core::Buffer<Model> models;
	if (!loadUFOAlienInvasionModels(stream, header, models)) {
		Log::error("Failed to load models");
		return false;
	}

	const core::String &baseName = core::string::extractFilename(filename);
	bool any = false;
	for (int i = 0; i < (int)models.size(); ++i) {
		const Model &model = models[i];
		if (model.faceId < 0 || model.faceCount <= 0 || model.faceId + model.faceCount > (int)faces.size()) {
			continue;
		}
		core::Buffer<Face> modelFaces;
		modelFaces.reserve(model.faceCount);
		for (int f = model.faceId; f < model.faceId + model.faceCount; ++f) {
			modelFaces.push_back(faces[f]);
		}
		const core::String modelName =
			i == 0 ? baseName : core::String::format("%s*%i", baseName.c_str(), i);
		if (voxelize(textures, modelFaces, edges, surfEdges, vertices, sceneGraph, modelName, meshMaterialArray,
					 progress)) {
			any = true;
		}
	}
	if (!any) {
		Log::error("Failed to voxelize %s", filename.c_str());
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

bool QuakeBSPFormat::loadQuetooBsp(const core::String &filename, io::SeekableReadStream &stream,
								  const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
								  const BspHeader &header, core::IProgress *progress) {
	// shortcut: Bezier patches are skipped; a map that fits in one chunk is filled by voxformat_fillhollow.
	static const uint32_t materialsLump = 1;
	static const uint32_t brushSidesLump = 3;
	static const uint32_t verticesLump = 6;
	static const uint32_t elementsLump = 7;
	static const uint32_t facesLump = 8;
	static const uint32_t modelsLump = 14;
	static const size_t materialSize = 64;
	static const size_t brushSideSize = 52;
	static const size_t vertexSize = 60;
	static const size_t faceSize = 60;
	static const size_t modelSize = 88;

	struct Vert {
		float x, y, z, u, v;
	};
	struct Side {
		int32_t material = -1;
		int32_t surface = 0;
	};
	struct QFace {
		int32_t brushSide = -1;
		int32_t firstElement = 0;
		int32_t numElements = 0;
		bool hidden = false;
	};
	struct QModel {
		int32_t faceId = 0;
		int32_t faceCount = 0;
	};

	const int32_t materialCount = validateLump(header.lumps[materialsLump], materialSize);
	if (materialCount < 0) {
		return false;
	}
	core::DynamicArray<core::String> materials((size_t)materialCount);
	if (materialCount > 0) {
		if (stream.seek(header.lumps[materialsLump].offset) == -1) {
			Log::error("Invalid material lump offset - can't seek");
			return false;
		}
		for (int32_t i = 0; i < materialCount; ++i) {
			char name[64];
			if (stream.read(name, sizeof(name)) != (int)sizeof(name)) {
				Log::error("Failed to read material %i", i);
				return false;
			}
			name[sizeof(name) - 1] = '\0';
			materials[i] = name;
		}
	}

	const int32_t sideCount = validateLump(header.lumps[brushSidesLump], brushSideSize);
	if (sideCount < 0) {
		return false;
	}
	core::Buffer<Side> sides((size_t)math::max(sideCount, 0));
	if (sideCount > 0) {
		if (stream.seek(header.lumps[brushSidesLump].offset) == -1) {
			Log::error("Invalid brush side lump offset - can't seek");
			return false;
		}
		for (int32_t i = 0; i < sideCount; ++i) {
			stream.skip(4); // plane
			wrap(stream.readInt32(sides[i].material))
			stream.skip(32); // texture axis
			stream.skip(4);	 // contents
			wrap(stream.readInt32(sides[i].surface))
			stream.skip(4); // value
		}
	}

	const int32_t vertexCount = validateLump(header.lumps[verticesLump], vertexSize);
	if (vertexCount <= 0) {
		Log::error("Invalid bsp file with no vertices in lump");
		return false;
	}
	if (stream.seek(header.lumps[verticesLump].offset) == -1) {
		Log::error("Invalid vertices lump offset - can't seek");
		return false;
	}
	core::Buffer<Vert> vertices((size_t)vertexCount);
	for (int32_t i = 0; i < vertexCount; ++i) {
		wrap(stream.readFloat(vertices[i].x))
		wrap(stream.readFloat(vertices[i].y))
		wrap(stream.readFloat(vertices[i].z))
		stream.skip(36); // normal, tangent, bitangent
		wrap(stream.readFloat(vertices[i].u))
		wrap(stream.readFloat(vertices[i].v))
		stream.skip(4); // color
	}

	const int32_t elementCount = validateLump(header.lumps[elementsLump], sizeof(int32_t));
	if (elementCount <= 0) {
		Log::error("Invalid bsp file with no elements in lump");
		return false;
	}
	if (stream.seek(header.lumps[elementsLump].offset) == -1) {
		Log::error("Invalid elements lump offset - can't seek");
		return false;
	}
	core::Buffer<int32_t> elements((size_t)elementCount);
	for (int32_t i = 0; i < elementCount; ++i) {
		wrap(stream.readInt32(elements[i]))
	}

	const int32_t faceCount = validateLump(header.lumps[facesLump], faceSize);
	if (faceCount <= 0) {
		Log::error("Invalid bsp file with no faces in lump");
		return false;
	}
	if (stream.seek(header.lumps[facesLump].offset) == -1) {
		Log::error("Invalid faces lump offset - can't seek");
		return false;
	}
	core::Buffer<QFace> faces((size_t)faceCount);
	for (int32_t i = 0; i < faceCount; ++i) {
		QFace &face = faces[i];
		wrap(stream.readInt32(face.brushSide))
		stream.skip(16); // plane, patch, node, block
		stream.skip(24); // bounds
		stream.skip(8);	 // firstVertex, numVertexes
		wrap(stream.readInt32(face.firstElement))
		wrap(stream.readInt32(face.numElements))
		const char *name = "";
		uint32_t surface = 0;
		if (face.brushSide >= 0 && face.brushSide < sideCount) {
			const Side &side = sides[face.brushSide];
			surface = (uint32_t)side.surface;
			if (side.material >= 0 && side.material < materialCount) {
				name = materials[side.material].c_str();
			}
		}
		face.hidden = _priv::hiddenSurface(name, surface);
	}

	const int32_t modelCount = validateLump(header.lumps[modelsLump], modelSize);
	if (modelCount <= 0) {
		Log::error("Invalid bsp file with no models in lump");
		return false;
	}
	if (stream.seek(header.lumps[modelsLump].offset) == -1) {
		Log::error("Invalid models lump offset - can't seek");
		return false;
	}
	core::Buffer<QModel> models((size_t)modelCount);
	for (int32_t i = 0; i < modelCount; ++i) {
		stream.skip(8);	 // entity, headNode
		stream.skip(48); // bounds, visibleBounds
		wrap(stream.readInt32(models[i].faceId))
		wrap(stream.readInt32(models[i].faceCount))
		stream.skip(24); // depth pass, draw elements, blocks
	}

	const core::String &baseDir = _priv::textureBaseDir(filename);
	MeshMaterialArray meshMaterialArray;
	core::Buffer<MeshMaterialIndex> materialIndex((size_t)materialCount);
	for (int32_t i = 0; i < materialCount; ++i) {
		materialIndex[i] = -1;
		if (_priv::hiddenSurface(materials[i].c_str(), 0)) {
			continue;
		}
		const image::ImagePtr &tex = _priv::loadExternalTexture(archive, baseDir, materials[i]);
		if (tex && tex->isLoaded()) {
			meshMaterialArray.push_back(createMaterial(tex));
			materialIndex[i] = (MeshMaterialIndex)(meshMaterialArray.size() - 1);
		}
	}

	const core::String &baseName = core::string::extractFilename(filename);
	bool any = false;
	for (int m = 0; m < modelCount; ++m) {
		const QModel &model = models[m];
		if (model.faceId < 0 || model.faceCount <= 0 || model.faceId + model.faceCount > faceCount) {
			continue;
		}
		Mesh mesh;
		mesh.materials = meshMaterialArray;
		for (int f = model.faceId; f < model.faceId + model.faceCount; ++f) {
			const QFace &face = faces[f];
			if (face.hidden || face.numElements < 3) {
				continue;
			}
			if (face.firstElement < 0 || face.firstElement + face.numElements > elementCount) {
				Log::error("Face %i element range is out of bounds", f);
				return false;
			}
			MeshMaterialIndex matIdx = -1;
			if (face.brushSide >= 0 && face.brushSide < sideCount) {
				const int32_t mat = sides[face.brushSide].material;
				if (mat >= 0 && mat < materialCount) {
					matIdx = materialIndex[mat];
				}
			}
			const int triCount = face.numElements / 3;
			for (int t = 0; t < triCount; ++t) {
				const int32_t idx[3] = {elements[face.firstElement + t * 3], elements[face.firstElement + t * 3 + 1],
										elements[face.firstElement + t * 3 + 2]};
				math::Vec3f pos[3];
				math::Vec2f uv[3];
				bool ok = true;
				for (int k = 0; k < 3; ++k) {
					if (idx[k] < 0 || idx[k] >= vertexCount) {
						ok = false;
						break;
					}
					const Vert &vert = vertices[idx[k]];
					pos[k] = math::Vec3f(vert.x, vert.z, vert.y);
					uv[k] = math::Vec2f(vert.u, vert.v);
				}
				if (!ok) {
					Log::error("Face %i references a vertex out of bounds", f);
					return false;
				}
				MeshTri meshTri;
				meshTri.setVertices(pos[0], pos[1], pos[2]);
				meshTri.setUVs(uv[0], uv[1], uv[2]);
				meshTri.materialIdx = matIdx;
				mesh.addTriangle(meshTri);
			}
		}
		if (mesh.indices.empty()) {
			continue;
		}
		const core::String modelName = m == 0 ? baseName : core::String::format("%s*%i", baseName.c_str(), m);
		if (progress != nullptr) {
			progress->setText(modelName.c_str());
		}
		if (importMesh(modelName, sceneGraph, core::move(mesh), 0, true, progress) != InvalidNodeId) {
			any = true;
		}
	}
	if (!any) {
		Log::error("Failed to voxelize %s", filename.c_str());
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

bool QuakeBSPFormat::loadBsp(const core::String &filename, io::SeekableReadStream &stream, const io::ArchivePtr &archive,
							 scenegraph::SceneGraph &sceneGraph, core::IProgress *progress) {
	static const uint32_t q1Version = FourCC('\x1d', '\0', '\0', '\0');
	static const uint32_t bspMagic = FourCC('I', 'B', 'S', 'P');

	BspHeader header;
	wrap(stream.readUInt32(header.magic))
	if (header.magic == q1Version) {
		header.version = 29;
	} else {
		wrap(stream.readUInt32(header.version))
	}
	int lumpCount = lengthof(header.lumps);
	if (header.magic == bspMagic && header.version == 38) {
		lumpCount = 19;
	} else if (header.magic == bspMagic && header.version == 84) {
		lumpCount = 21;
	}
	for (int i = 0; i < lumpCount; ++i) {
		wrap(stream.readUInt32(header.lumps[i].offset))
		wrap(stream.readUInt32(header.lumps[i].len))
	}

	if (header.version == 79 && header.magic == bspMagic) {
		return loadUFOAlienInvasionBsp(filename, stream, sceneGraph, header, progress);
	}
	if (header.magic == q1Version) {
		return loadQuake1Bsp(filename, stream, sceneGraph, header, progress);
	}
	if (header.version == 38 && header.magic == bspMagic) {
		return loadQuake2Bsp(filename, stream, archive, sceneGraph, header, progress);
	}
	if (header.version == 84 && header.magic == bspMagic) {
		return loadQuetooBsp(filename, stream, archive, sceneGraph, header, progress);
	}

	uint8_t buf[4];
	FourCCRev(buf, header.magic);
	Log::error("Unsupported bsp file with magic %c%c%c%c and version %i", buf[0], buf[1], buf[2], buf[3],
			   header.version);
	return false;
}

bool QuakeBSPFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	if (io::ZipArchive::validStream(*stream)) {
		stream->seek(0);
		const io::ArchivePtr &zip = io::openZipArchive(stream);
		if (!zip) {
			Log::error("Failed to open pk3 %s", filename.c_str());
			return false;
		}
		io::ArchiveFiles bsps;
		zip->list("*.bsp", bsps);
		if (bsps.empty()) {
			Log::error("No bsp found in %s", filename.c_str());
			return false;
		}
		const io::FilesystemEntry *chosen = &bsps[0];
		for (const io::FilesystemEntry &entry : bsps) {
			if (core::string::icontains(entry.fullPath, "maps/")) {
				chosen = &entry;
				break;
			}
		}
		core::ScopedPtr<io::SeekableReadStream> bsp(zip->readStream(chosen->fullPath));
		if (!bsp) {
			Log::error("Could not read %s from %s", chosen->fullPath.c_str(), filename.c_str());
			return false;
		}
		return loadBsp(filename, *bsp, zip, sceneGraph, ctx.progress);
	}
	return loadBsp(filename, *stream, archive, sceneGraph, ctx.progress);
}

#undef wrap

} // namespace voxelformat
