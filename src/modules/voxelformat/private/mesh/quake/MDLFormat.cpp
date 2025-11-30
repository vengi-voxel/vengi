/**
 * @file
 */

#include "MDLFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "glm/ext/vector_uint3.hpp"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include <cstdint>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Error: Could not load mdl file with " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Error: Could not load mdl file with " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);     \
		return false;                                                                                                  \
	}

bool MDLFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	MDLHeader hdr;

	bool idPolyModel = false;
	wrap(stream->readUInt32(hdr.magic))
	if (hdr.magic == FourCC('I', 'D', 'P', 'O')) {
		idPolyModel = true;
	} else if (hdr.magic == FourCC('R', 'A', 'P', 'O')) {
		idPolyModel = false;
	} else {
		Log::error("Could not load mdl file: Invalid magic");
		return false;
	}
	wrap(stream->readUInt32(hdr.version))
	if (idPolyModel && hdr.version != 6) { // q1
		Log::error("Could not load mdl file: Invalid version");
		return false;
	}
	if (!idPolyModel && hdr.version != 50) { // hexen2
		Log::error("Could not load mdl file: Invalid version");
		return false;
	}
	wrap(stream->readFloat(hdr.scale.x))
	wrap(stream->readFloat(hdr.scale.z))
	wrap(stream->readFloat(hdr.scale.y))

	wrap(stream->readFloat(hdr.origin.x))
	wrap(stream->readFloat(hdr.origin.z))
	wrap(stream->readFloat(hdr.origin.y))

	wrap(stream->readFloat(hdr.radius))

	wrap(stream->readFloat(hdr.eye.x))
	wrap(stream->readFloat(hdr.eye.z))
	wrap(stream->readFloat(hdr.eye.y))

	wrap(stream->readUInt32(hdr.numSkins))
	wrap(stream->readUInt32(hdr.skinWidth))
	wrap(stream->readUInt32(hdr.skinHeight))

	wrap(stream->readUInt32(hdr.numVerts))
	wrap(stream->readUInt32(hdr.numTris))
	wrap(stream->readUInt32(hdr.numFrames))
	wrap(stream->readUInt32(hdr.synctype))
	wrap(stream->readUInt32(hdr.flags))
	wrap(stream->readFloat(hdr.size))

	if (!idPolyModel) {
		wrap(stream->readUInt32(hdr.numTexCoords))
	}

	palette::Palette pal;
	pal.quake1();

	if (archive->exists("palette.lmp")) {
		core::ScopedPtr<io::SeekableReadStream> palStream(archive->readStream("palette.lmp"));
		if (palStream && palStream->size() == 768) {
			for (int i = 0; i < 256; ++i) {
				color::RGBA rgba(0, 0, 0, 255);
				wrap(palStream->readUInt8(rgba.r))
				wrap(palStream->readUInt8(rgba.g))
				wrap(palStream->readUInt8(rgba.b))
				pal.setColor(i, rgba);
			}
		}
	}

	// skins
	MeshMaterialArray materials;
	for (uint32_t i = 0; i < hdr.numSkins; ++i) {
		uint32_t group;
		wrap(stream->readUInt32(group))
		int32_t numberOfTextures = 1;
		Log::debug("skin group %i", (int)group);
		if (group == 1) {
			wrap(stream->readInt32(numberOfTextures))
			core::Buffer<float> times; // ignored for now
			times.reserve(numberOfTextures);
			for (int32_t j = 0; j < numberOfTextures; ++j) {
				float time;
				wrap(stream->readFloat(time))
				times.push_back(time);
			}
		}

		Log::debug("Found %i skins with %i textures", (int)hdr.numSkins, (int)numberOfTextures);

		for (int32_t j = 0; j < numberOfTextures; ++j) {
			const image::ImagePtr &image = image::createEmptyImage("skin_" + core::string::toString(j));
			if (!image->load(hdr.skinWidth, hdr.skinHeight, [&](int x, int y, color::RGBA &rgba) -> void {
					uint8_t index = 0;
					if (stream->readUInt8(index) == -1) {
						Log::error("Could not load mdl file: Failed to load skin");
					}
					rgba = pal.color(index);
				})) {
				Log::error("Could not load mdl file: Failed to load skin");
				return false;
			}

			materials.push_back(createMaterial(image));
		}
	}
	Log::debug("Loaded %i materials", (int)materials.size());

	/* To get uv coordinates (ranging from 0.0 to 1.0), you have to add 0.5 to the uv coordinates and then
	 * divide the result by hdr.skinwidth for u and hdr.skinheight for v */
	core::Buffer<glm::vec2> texCoords;
	const uint32_t maxTexCoords = idPolyModel ? hdr.numVerts : hdr.numTexCoords;
	for (uint32_t i = 0; i < maxTexCoords; ++i) {
		uint32_t onseam;
		wrap(stream->readUInt32(onseam))
		glm::ivec2 uv;
		wrap(stream->readInt32(uv.x))
		wrap(stream->readInt32(uv.y))
		const float u = ((float)uv.x + 0.5f) / (float)hdr.skinWidth;
		const float v = ((float)uv.y + 0.5f) / (float)hdr.skinHeight;
		texCoords.emplace_back(u, v);
	}

	struct MDLTriangle {
		bool isFrontFace; /* textures include both sides of a face - the back face is the right side of the texture half
						   */
		union {
			struct {
				glm::uvec3 indices;
			} id;
			struct {
				glm::u16vec3 indices;
				glm::u16vec3 uv;
			} ra;
		};
	};

	// tris
	core::Buffer<MDLTriangle> tris;
	tris.reserve(hdr.numTris);
	if (idPolyModel) {
		for (uint32_t i = 0; i < hdr.numTris; ++i) {
			uint32_t frontFace; // 0 = backface, 1 = frontface
			wrap(stream->readUInt32(frontFace))
			MDLTriangle tri;
			tri.isFrontFace = frontFace != 0;
			wrap(stream->readUInt32(tri.id.indices.x))
			wrap(stream->readUInt32(tri.id.indices.y))
			wrap(stream->readUInt32(tri.id.indices.z))
			tris.emplace_back(tri);
		}
	} else {
		for (uint32_t i = 0; i < hdr.numTris; ++i) {
			uint32_t frontFace; // 0 = backface, 1 = frontface
			wrap(stream->readUInt32(frontFace))
			MDLTriangle tri;
			tri.isFrontFace = frontFace != 0;
			wrap(stream->readUInt16(tri.ra.indices.x))
			wrap(stream->readUInt16(tri.ra.indices.y))
			wrap(stream->readUInt16(tri.ra.indices.z))
			wrap(stream->readUInt16(tri.ra.uv.x))
			wrap(stream->readUInt16(tri.ra.uv.y))
			wrap(stream->readUInt16(tri.ra.uv.z))
			tris.emplace_back(tri);
		}
	}

	// frames
	struct MDLFrame {
		uint32_t type;
		glm::u8vec4 bboxmin; // vertex[0-3], normalindex
		glm::u8vec4 bboxmax; // vertex[0-3], normalindex
		core::String name;	 // 16 chars
		core::Buffer<glm::vec3> vertices;
	};

	struct MDLPose {
		glm::u8vec4 bboxmin;
		glm::u8vec4 bboxmax;
		core::Buffer<float> times; // for group frames
		core::DynamicArray<MDLFrame> frames;
	};

	core::DynamicArray<MDLPose> poses;
	poses.reserve(hdr.numFrames);
	for (uint32_t i = 0; i < hdr.numFrames; ++i) {
		uint32_t type;
		wrap(stream->readUInt32(type))
		Log::debug("Frame type for frame %i is %i", i, type);
		if (type == 0) {
			MDLPose simpleFrame;
			MDLFrame frame;
			frame.type = type;
			wrap(stream->readUInt8(frame.bboxmin.x))
			wrap(stream->readUInt8(frame.bboxmin.z))
			wrap(stream->readUInt8(frame.bboxmin.y))
			wrap(stream->readUInt8(frame.bboxmin.w)) // lightnormalindex (unused)
			wrap(stream->readUInt8(frame.bboxmax.x))
			wrap(stream->readUInt8(frame.bboxmax.z))
			wrap(stream->readUInt8(frame.bboxmax.y))
			wrap(stream->readUInt8(frame.bboxmax.w)) // lightnormalindex (unused)
			wrapBool(stream->readString(16, frame.name, false))
			frame.vertices.reserve(hdr.numVerts);
			for (uint32_t j = 0; j < hdr.numVerts; ++j) {
				glm::u8vec4 vertex;
				wrap(stream->readUInt8(vertex.x))
				wrap(stream->readUInt8(vertex.z))
				wrap(stream->readUInt8(vertex.y))
				wrap(stream->readUInt8(vertex.w)) // lightnormalindex (unused)
				frame.vertices.push_back(glm::vec3(vertex) * hdr.scale + hdr.origin);
			}
			simpleFrame.bboxmax = frame.bboxmax;
			simpleFrame.bboxmin = frame.bboxmin;
			simpleFrame.frames.emplace_back(frame);
			poses.emplace_back(simpleFrame);
		} else {
			MDLPose groupFrame;

			uint32_t numFrames;
			wrap(stream->readUInt32(numFrames))
			groupFrame.frames.reserve(numFrames);
			groupFrame.times.reserve(numFrames);

			wrap(stream->readUInt8(groupFrame.bboxmin.x))
			wrap(stream->readUInt8(groupFrame.bboxmin.z))
			wrap(stream->readUInt8(groupFrame.bboxmin.y))
			wrap(stream->readUInt8(groupFrame.bboxmin.w)) // lightnormalindex (unused)
			wrap(stream->readUInt8(groupFrame.bboxmax.x))
			wrap(stream->readUInt8(groupFrame.bboxmax.z))
			wrap(stream->readUInt8(groupFrame.bboxmax.y))
			wrap(stream->readUInt8(groupFrame.bboxmax.w)) // lightnormalindex (unused)

			for (uint32_t j = 0; j < numFrames; ++j) {
				float time;
				wrap(stream->readFloat(time))
				groupFrame.times.push_back(time);
			}
			Log::debug("Found %i group frames", numFrames);
			for (uint32_t j = 0; j < numFrames; ++j) {
				MDLFrame frame;

				uint32_t frameType;
				wrap(stream->readUInt32(frameType))

				wrap(stream->readUInt8(frame.bboxmin.x))
				wrap(stream->readUInt8(frame.bboxmin.z))
				wrap(stream->readUInt8(frame.bboxmin.y))
				wrap(stream->readUInt8(frame.bboxmin.w))
				wrap(stream->readUInt8(frame.bboxmax.x))
				wrap(stream->readUInt8(frame.bboxmax.z))
				wrap(stream->readUInt8(frame.bboxmax.y))
				wrap(stream->readUInt8(frame.bboxmax.w))

				wrapBool(stream->readString(16, frame.name, false))
				frame.vertices.reserve(hdr.numVerts);
				for (uint32_t k = 0; k < hdr.numVerts; ++k) {
					glm::u8vec4 vertex;
					wrap(stream->readUInt8(vertex.x))
					wrap(stream->readUInt8(vertex.z))
					wrap(stream->readUInt8(vertex.y))
					wrap(stream->readUInt8(vertex.w)) // normalindex
					frame.vertices.push_back(glm::vec3(vertex) * hdr.scale + hdr.origin);
				}
				groupFrame.frames.emplace_back(frame);
			}
			poses.emplace_back(groupFrame);
		}
	}

	// create textured triangle instances
	bool first = true;
	for (uint32_t p = 0; p < poses.size(); ++p) {
		const MDLPose &pose = poses[p];
		for (const MDLFrame &frame : pose.frames) {
			Mesh mesh;
			for (uint32_t i = 0; i < hdr.numTris; ++i) {
				const MDLTriangle &tri = tris[i];
				uint32_t idx0;
				uint32_t idx1;
				uint32_t idx2;
				if (idPolyModel) {
					idx0 = tri.id.indices.x;
					idx1 = tri.id.indices.y;
					idx2 = tri.id.indices.z;
				} else {
					idx0 = tri.ra.indices.x;
					idx1 = tri.ra.indices.y;
					idx2 = tri.ra.indices.z;
				}
				if (idx0 >= hdr.numVerts || idx1 >= hdr.numVerts || idx2 >= hdr.numVerts) {
					Log::error("Invalid triangle indices %u %u %u in frame %s", idx0, idx1, idx2, frame.name.c_str());
					continue;
				}

				MeshVertex vert0;
				vert0.pos = frame.vertices[idx0];
				vert0.uv = texCoords[idx0];

				MeshVertex vert1;
				vert1.pos = frame.vertices[idx1];
				vert1.uv = texCoords[idx1];

				MeshVertex vert2;
				vert2.pos = frame.vertices[idx2];
				vert2.uv = texCoords[idx2];

				if (!materials.empty()) {
					vert0.materialIdx = 0;
					vert1.materialIdx = 0;
					vert2.materialIdx = 0;
				}

				mesh.indices.push_back(mesh.vertices.size());
				mesh.vertices.push_back(vert0);
				mesh.indices.push_back(mesh.vertices.size());
				mesh.vertices.push_back(vert1);
				mesh.indices.push_back(mesh.vertices.size());
				mesh.vertices.push_back(vert2);
			}
			mesh.materials = materials;
			const int nodeId = voxelizeMesh(frame.name, sceneGraph, core::move(mesh));
			if (!first && nodeId != -1) {
				sceneGraph.node(nodeId).setVisible(false);
			}
			first = false;
		}
	}

	return !sceneGraph.empty();
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
