/**
 * @file
 */

#include "MD2Format.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"

namespace voxelformat {

#define MD2_MAX_TRIANGLES 4096
#define MD2_MAX_VERTS 2048
#define MD2_MAX_FRAMES 1024
#define MD2_MAX_SKINS 32
#define MD2_VERSION 8
#define MD2_MAX_SKINNAME 64

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load md2 file: Failure at " CORE_STRINGIFY(read));                                       \
		return false;                                                                                                  \
	}

bool MD2Format::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	MD2Header hdr;

	const int64_t startOffset = stream->pos();

	wrap(stream->readUInt32(hdr.magic))
	if (hdr.magic != FourCC('I', 'D', 'P', '2')) {
		Log::error("Could not load md2 file: Invalid magic");
		return false;
	}
	wrap(stream->readUInt32(hdr.version))
	if (hdr.version != MD2_VERSION) {
		Log::error("Could not load md2 file: Invalid version");
		return false;
	}
	wrap(stream->readUInt32(hdr.skinWidth))
	wrap(stream->readUInt32(hdr.skinHeight))
	wrap(stream->readUInt32(hdr.frameSize))
	wrap(stream->readUInt32(hdr.numSkins))
	wrap(stream->readUInt32(hdr.numVerts))
	wrap(stream->readUInt32(hdr.numST))
	wrap(stream->readUInt32(hdr.numTris))
	wrap(stream->readUInt32(hdr.numGLCmds))
	wrap(stream->readUInt32(hdr.numFrames))
	wrap(stream->readUInt32(hdr.offsetSkins))
	wrap(stream->readUInt32(hdr.offsetST))
	wrap(stream->readUInt32(hdr.offsetTris))
	wrap(stream->readUInt32(hdr.offsetFrames))
	wrap(stream->readUInt32(hdr.offsetGLCmds))
	wrap(stream->readUInt32(hdr.offsetEnd))

	if (hdr.numVerts >= MD2_MAX_VERTS) {
		Log::error("Max verts exceeded");
		return false;
	}
	if (hdr.numTris >= MD2_MAX_TRIANGLES) {
		Log::error("Max triangles exceeded");
		return false;
	}
	if (hdr.numSkins >= MD2_MAX_SKINS) {
		Log::error("Max skins exceeded");
		return false;
	}

	MeshMaterialMap meshMaterials;
	stream->seek(startOffset + hdr.offsetSkins);
	for (uint32_t i = 0; i < hdr.numSkins; ++i) {
		core::String skinname;
		if (!stream->readString(MD2_MAX_SKINNAME, skinname)) {
			Log::error("Failed to read skin name");
			return false;
		}

		if (skinname[0] == '.') {
			skinname = skinname.substr(1);
		}

		const core::String &imageName = lookupTexture(filename, skinname, archive);
		const image::ImagePtr &image = image::loadImage(imageName);
		meshMaterials.put(skinname, createMaterial(image));
	}

	if (!loadFrame(filename, *stream, startOffset, hdr, 0, sceneGraph, meshMaterials)) {
		Log::error("Failed to load frame");
		return false;
	}

	return !sceneGraph.empty();
}

bool MD2Format::loadFrame(const core::String &filename, io::SeekableReadStream &stream, int64_t startOffset,
						  const MD2Header &hdr, uint32_t frameIndex, scenegraph::SceneGraph &sceneGraph,
						  const MeshMaterialMap &meshMaterials) {
	if (frameIndex >= hdr.numFrames) {
		Log::error("Invalid frame index");
		return false;
	}

	MD2FrameHeader frameHdr;
	if (stream.seek(startOffset + hdr.offsetFrames + frameIndex * hdr.frameSize) == -1) {
		Log::error("Failed to seek to frame header");
		return false;
	}
	wrap(stream.readFloat(frameHdr.scale[0]))
	wrap(stream.readFloat(frameHdr.scale[2]))
	wrap(stream.readFloat(frameHdr.scale[1]))
	Log::debug("Scale: %f %f %f", frameHdr.scale[0], frameHdr.scale[1], frameHdr.scale[2]);
	wrap(stream.readFloat(frameHdr.translate[0]))
	wrap(stream.readFloat(frameHdr.translate[2]))
	wrap(stream.readFloat(frameHdr.translate[1]))
	Log::debug("Translate: %f %f %f", frameHdr.translate[0], frameHdr.translate[1], frameHdr.translate[2]);

	if (!stream.readString(sizeof(frameHdr.name), frameHdr.name)) {
		Log::error("Failed to read frame name");
		return false;
	}

	core::Buffer<glm::vec3> vertices;
	vertices.reserve(hdr.numVerts);
	Log::debug("Reading %i vertices", hdr.numVerts);
	for (uint32_t i = 0; i < hdr.numVerts; ++i) {
		glm::u8vec4 vertex;
		stream.readUInt8(vertex.x);
		stream.readUInt8(vertex.z);
		stream.readUInt8(vertex.y);
		stream.readUInt8(vertex.w); // normal index
		// uncompressed vertex
		vertices.push_back(glm::vec3(vertex) * glm::vec3(frameHdr.scale[0], frameHdr.scale[1], frameHdr.scale[2]) +
						   glm::vec3(frameHdr.translate[0], frameHdr.translate[1], frameHdr.translate[2]));
	}

	Log::debug("Reading %i texture coordinates", hdr.numST);
	if (stream.seek(startOffset + hdr.offsetST) == -1) {
		Log::error("Failed to seek to texture coordinates");
		return false;
	}

	core::Buffer<glm::vec2> uvs;
	uvs.reserve(hdr.numST);
	for (uint32_t i = 0; i < hdr.numST; ++i) {
		glm::i16vec2 uv;
		stream.readInt16(uv.x);
		stream.readInt16(uv.y);
		uvs.push_back(glm::vec2((float)uv.x / (float)hdr.skinWidth, 1.0f - (float)uv.y / (float)hdr.skinHeight));
	}

	const glm::vec3 &scale = getInputScale();

	MeshTriCollection tris;
	tris.reserve(hdr.numTris);
	if (stream.seek(startOffset + hdr.offsetTris) == -1) {
		Log::error("Failed to seek to triangles");
		return false;
	}
	Log::debug("Reading %i triangles", hdr.numTris);
	MeshMaterialPtr material = !meshMaterials.empty() ? meshMaterials.begin()->second : MeshMaterialPtr();
	for (uint32_t i = 0; i < hdr.numTris; ++i) {
		glm::u16vec3 vertexIndices;
		wrap(stream.readUInt16(vertexIndices.x))
		wrap(stream.readUInt16(vertexIndices.z))
		wrap(stream.readUInt16(vertexIndices.y))
		if (vertexIndices.x >= hdr.numVerts || vertexIndices.y >= hdr.numVerts || vertexIndices.z >= hdr.numVerts) {
			Log::error("Invalid vertex index");
			return false;
		}

		glm::u16vec3 uvIndices;
		wrap(stream.readUInt16(uvIndices.x))
		wrap(stream.readUInt16(uvIndices.z))
		wrap(stream.readUInt16(uvIndices.y))
		if (uvIndices.x >= hdr.numST || uvIndices.y >= hdr.numST || uvIndices.z >= hdr.numST) {
			Log::error("Invalid uv index");
			return false;
		}

		voxelformat::MeshTri meshTri;
		for (int j = 0; j < 3; ++j) {
			meshTri.vertices[j] = vertices[vertexIndices[j]] * scale;
			meshTri.uv[j] = uvs[uvIndices[j]];
		}
		meshTri.material = material;
		tris.push_back(meshTri);
	}

	return voxelizeNode(filename, sceneGraph, tris) != InvalidNodeId;
}

#undef wrap

} // namespace voxelformat
