/**
 * @file
 */

#include "MD3Format.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"

namespace voxelformat {

#define MD3_MAX_FRAMES 1024
#define MD3_MAX_TAGS 16
#define MD3_MAX_SURFACES 32
#define MD3_MAX_SHADERS 256
#define MD3_MAX_VERTS 4096
#define MD3_MAX_TRIANGLES 8192
#define MD3_VERSION 15

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load md3 file: Failure at " CORE_STRINGIFY(read));                                       \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not load md3 file: Failure at " CORE_STRINGIFY(read));                                       \
		return false;                                                                                                  \
	}

bool MD3Format::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	const int64_t startOffset = stream->pos();

	MD3Header hdr;
	wrap(stream->readUInt32(hdr.magic))
	if (hdr.magic != FourCC('I', 'D', 'P', '3')) {
		Log::error("Could not load md3 file: Invalid magic");
		return false;
	}
	wrap(stream->readInt32(hdr.version))
	if (hdr.version != MD3_VERSION) {
		Log::error("Could not load md3 file: Invalid version %i", hdr.version);
		return false;
	}
	if (!stream->readString(sizeof(hdr.name), hdr.name)) {
		Log::error("Failed to read model name");
		return false;
	}
	wrap(stream->readInt32(hdr.flags))
	wrap(stream->readInt32(hdr.numFrames))
	wrap(stream->readInt32(hdr.numTags))
	wrap(stream->readInt32(hdr.numSurfaces))
	wrap(stream->readInt32(hdr.numSkins))
	wrap(stream->readInt32(hdr.ofsFrames))
	wrap(stream->readInt32(hdr.ofsTags))
	wrap(stream->readInt32(hdr.ofsSurfaces))
	wrap(stream->readInt32(hdr.ofsEnd))

	Log::debug("MD3 model '%s': frames=%i tags=%i surfaces=%i", hdr.name, hdr.numFrames, hdr.numTags, hdr.numSurfaces);

	if (hdr.numFrames <= 0 || hdr.numFrames > MD3_MAX_FRAMES) {
		Log::error("Invalid number of frames: %i", hdr.numFrames);
		return false;
	}
	if (hdr.numTags < 0 || hdr.numTags > MD3_MAX_TAGS) {
		Log::error("Invalid number of tags: %i", hdr.numTags);
		return false;
	}
	if (hdr.numSurfaces <= 0 || hdr.numSurfaces > MD3_MAX_SURFACES) {
		Log::error("Invalid number of surfaces: %i", hdr.numSurfaces);
		return false;
	}

	// iterate surfaces - each surface has its own offset chain
	int64_t surfaceOffset = startOffset + hdr.ofsSurfaces;
	for (int32_t i = 0; i < hdr.numSurfaces; ++i) {
		if (!loadSurface(filename, archive, *stream, surfaceOffset, hdr, sceneGraph)) {
			Log::error("Failed to load surface %i", i);
			return false;
		}
	}

	return !sceneGraph.empty();
}

bool MD3Format::loadSurface(const core::String &filename, const io::ArchivePtr &archive,
							io::SeekableReadStream &stream, int64_t &surfaceStart,
							const MD3Header &hdr, scenegraph::SceneGraph &sceneGraph) {
	if (stream.seek(surfaceStart) == -1) {
		Log::error("Failed to seek to surface header");
		return false;
	}

	MD3SurfaceHeader surfHdr;
	wrap(stream.readUInt32(surfHdr.magic))
	if (surfHdr.magic != FourCC('I', 'D', 'P', '3')) {
		Log::error("Invalid surface magic");
		return false;
	}
	if (!stream.readString(sizeof(surfHdr.name), surfHdr.name)) {
		Log::error("Failed to read surface name");
		return false;
	}
	wrap(stream.readInt32(surfHdr.flags))
	wrap(stream.readInt32(surfHdr.numFrames))
	wrap(stream.readInt32(surfHdr.numShaders))
	wrap(stream.readInt32(surfHdr.numVerts))
	wrap(stream.readInt32(surfHdr.numTriangles))
	wrap(stream.readInt32(surfHdr.ofsTriangles))
	wrap(stream.readInt32(surfHdr.ofsShaders))
	wrap(stream.readInt32(surfHdr.ofsST))
	wrap(stream.readInt32(surfHdr.ofsXYZNormals))
	wrap(stream.readInt32(surfHdr.ofsEnd))

	Log::debug("  Surface '%s': frames=%i shaders=%i verts=%i tris=%i", surfHdr.name, surfHdr.numFrames,
			   surfHdr.numShaders, surfHdr.numVerts, surfHdr.numTriangles);

	if (surfHdr.numVerts <= 0 || surfHdr.numVerts > MD3_MAX_VERTS) {
		Log::error("Invalid number of vertices: %i", surfHdr.numVerts);
		return false;
	}
	if (surfHdr.numTriangles <= 0 || surfHdr.numTriangles > MD3_MAX_TRIANGLES) {
		Log::error("Invalid number of triangles: %i", surfHdr.numTriangles);
		return false;
	}
	if (surfHdr.numShaders < 0 || surfHdr.numShaders > MD3_MAX_SHADERS) {
		Log::error("Invalid number of shaders: %i", surfHdr.numShaders);
		return false;
	}

	// load shader/skin textures
	MeshMaterialArray surfaceMaterials;
	if (stream.seek(surfaceStart + surfHdr.ofsShaders) == -1) {
		Log::error("Failed to seek to shaders");
		return false;
	}
	for (int32_t i = 0; i < surfHdr.numShaders; ++i) {
		MD3Shader shader;
		if (!stream.readString(sizeof(shader.name), shader.name)) {
			Log::error("Failed to read shader name");
			return false;
		}
		wrap(stream.readInt32(shader.shaderIndex))
		Log::debug("    Shader %i: '%s'", i, shader.name);

		core::String skinName(shader.name);
		if (skinName[0] == '.') {
			skinName = skinName.substr(1);
		}
		const core::String &imageName = lookupTexture(filename, skinName, archive);
		const image::ImagePtr &image = image::loadImage(imageName);
		surfaceMaterials.push_back(createMaterial(image));
	}

	// read triangles
	if (stream.seek(surfaceStart + surfHdr.ofsTriangles) == -1) {
		Log::error("Failed to seek to triangles");
		return false;
	}
	core::Buffer<MD3Triangle> triangles(surfHdr.numTriangles);
	for (int32_t i = 0; i < surfHdr.numTriangles; ++i) {
		wrap(stream.readInt32(triangles[i].indices[0]))
		wrap(stream.readInt32(triangles[i].indices[1]))
		wrap(stream.readInt32(triangles[i].indices[2]))
	}

	// read texture coordinates
	if (stream.seek(surfaceStart + surfHdr.ofsST) == -1) {
		Log::error("Failed to seek to texture coordinates");
		return false;
	}
	core::Buffer<glm::vec2> uvs(surfHdr.numVerts);
	for (int32_t i = 0; i < surfHdr.numVerts; ++i) {
		wrap(stream.readFloat(uvs[i].x))
		wrap(stream.readFloat(uvs[i].y))
	}

	// read vertices from frame 0 only
	// MD3 uses Z-up, vengi uses Y-up: swap Y and Z
	if (stream.seek(surfaceStart + surfHdr.ofsXYZNormals) == -1) {
		Log::error("Failed to seek to vertices");
		return false;
	}
	core::Buffer<glm::vec3> vertices(surfHdr.numVerts);
	for (int32_t i = 0; i < surfHdr.numVerts; ++i) {
		int16_t x, y, z;
		int16_t encodedNormal;
		wrap(stream.readInt16(x))
		wrap(stream.readInt16(y))
		wrap(stream.readInt16(z))
		wrap(stream.readInt16(encodedNormal))
		// MD3 Z-up -> Y-up coordinate swap
		vertices[i] = glm::vec3((float)x * MD3_XYZ_SCALE, (float)z * MD3_XYZ_SCALE, (float)y * MD3_XYZ_SCALE);
	}

	// build mesh triangles
	MeshMaterialIndex materialIdx = !surfaceMaterials.empty() ? 0 : -1;
	Mesh mesh;
	mesh.indices.reserve(surfHdr.numTriangles * 3);
	mesh.vertices.reserve(surfHdr.numVerts);
	mesh.materials = surfaceMaterials;

	for (int32_t i = 0; i < surfHdr.numTriangles; ++i) {
		const MD3Triangle &tri = triangles[i];
		if (tri.indices[0] < 0 || tri.indices[0] >= surfHdr.numVerts || tri.indices[1] < 0 ||
			tri.indices[1] >= surfHdr.numVerts || tri.indices[2] < 0 || tri.indices[2] >= surfHdr.numVerts) {
			Log::error("Invalid triangle index");
			return false;
		}

		voxelformat::MeshTri meshTri;
		// MD3 uses clockwise winding; swap indices 1 and 2 for counter-clockwise
		meshTri.setVertices(vertices[tri.indices[0]], vertices[tri.indices[2]], vertices[tri.indices[1]]);
		meshTri.setUVs(uvs[tri.indices[0]], uvs[tri.indices[2]], uvs[tri.indices[1]]);
		meshTri.materialIdx = materialIdx;
		mesh.addTriangle(meshTri);
	}

	const int nodeId = voxelizeMesh(filename, sceneGraph, core::move(mesh));

	// advance the surface offset for the next surface
	surfaceStart = surfaceStart + surfHdr.ofsEnd;

	return nodeId != InvalidNodeId;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
