/**
 * @file
 */

#include "OBJFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "core/concurrent/ThreadPool.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/StdStreamBuf.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

namespace voxelformat {

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Failed to write obj " CORE_STRINGIFY(read)); \
		return false; \
	}

bool OBJFormat::writeMtlFile(io::SeekableWriteStream &stream, const core::String &mtlId, const core::String &mapKd) const {
	if (!stream.writeStringFormat(false, "\nnewmtl %s\n", mtlId.c_str())) {
		Log::error("Failed to write obj newmtl");
		return false;
	}
	wrapBool(stream.writeString("Ka 1.000000 1.000000 1.000000\n", false))
	wrapBool(stream.writeString("Kd 1.000000 1.000000 1.000000\n", false))
	wrapBool(stream.writeString("Ks 0.000000 0.000000 0.000000\n", false))
	wrapBool(stream.writeString("Tr 1.000000\n", false))
	wrapBool(stream.writeString("illum 1\n", false))
	wrapBool(stream.writeString("Ns 0.000000\n", false))
	if (!stream.writeStringFormat(false, "map_Kd %s\n", mapKd.c_str())) {
		Log::error("Failed to write obj map_Kd");
		return false;
	}
	return true;
}

bool OBJFormat::saveMeshes(const core::Map<int, int> &, const SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/vengi\n");
	wrapBool(stream.writeStringFormat(false, "\n"))
	wrapBool(stream.writeStringFormat(false, "g Model\n"))

	Log::debug("Exporting %i layers", (int)meshes.size());

	core::String mtlname = core::string::stripExtension(filename);
	mtlname.append(".mtl");
	Log::debug("Use mtl file: %s", mtlname.c_str());

	const io::FilePtr &file = io::filesystem()->open(mtlname, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		Log::error("Failed to create mtl file at %s", file->name().c_str());
		return false;
	}
	io::FileStream matlstream(file);
	wrapBool(matlstream.writeString("# version " PROJECT_VERSION " github.com/mgerhardy/vengi\n", false))
	wrapBool(matlstream.writeString("\n", false))

	core::Map<uint64_t, int> paletteMaterialIndices((int)sceneGraph.size());

	int idxOffset = 0;
	int texcoordOffset = 0;
	for (const auto &meshExt : meshes) {
		const voxel::Mesh *mesh = &meshExt.mesh->mesh; // TODO: alpha support
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int nv = (int)mesh->getNoOfVertices();
		const int ni = (int)mesh->getNoOfIndices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		const SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
		KeyFrameIndex keyFrameIdx = 0;
		const SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
		const voxel::Palette &palette = graphNode.palette();

		const core::String hashId = core::String::format("%" PRIu64, palette.hash());

		const voxel::VertexArray &vertices = mesh->getVertexVector();
		const voxel::IndexArray &indices = mesh->getIndexVector();
		const voxel::NormalArray &normals = mesh->getNormalVector();
		const bool withNormals = !normals.empty();
		const char *objectName = meshExt.name.c_str();
		if (objectName[0] == '\0') {
			objectName = "Noname";
		}
		stream.writeStringFormat(false, "o %s\n", objectName);
		stream.writeStringFormat(false, "mtllib %s\n", core::string::extractFilenameWithExtension(mtlname).c_str());
		if (!stream.writeStringFormat(false, "usemtl %s\n", hashId.c_str())) {
			Log::error("Failed to write obj usemtl %s\n", hashId.c_str());
			return false;
		}

		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex &v = vertices[i];

			glm::vec3 pos;
			if (meshExt.applyTransform) {
				pos = transform.apply(v.position, meshExt.size);
			} else {
				pos = v.position;
			}
			pos *= scale;
			stream.writeStringFormat(false, "v %.04f %.04f %.04f", pos.x, pos.y, pos.z);
			if (withColor) {
				const glm::vec4& color = core::Color::fromRGBA(palette.colors[v.colorIndex]);
				stream.writeStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
			}
			wrapBool(stream.writeStringFormat(false, "\n"))
		}
		if (withNormals) {
			for (int i = 0; i < nv; ++i) {
				const glm::vec3 &norm = normals[i];
				stream.writeStringFormat(false, "vn %.04f %.04f %.04f\n", norm.x, norm.y, norm.z);
			}
		}

		if (quad) {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 6) {
					const voxel::VoxelVertex &v = vertices[indices[i]];
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
				}
			}

			int uvi = texcoordOffset;
			for (int i = 0; i < ni - 5; i += 6, uvi += 4) {
				const uint32_t one = idxOffset + indices[i + 0] + 1;
				const uint32_t two = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				const uint32_t four = idxOffset + indices[i + 5] + 1;
				if (withTexCoords) {
					if (withNormals) {
						stream.writeStringFormat(false, "f %i/%i/%i %i/%i/%i %i/%i/%i %i/%i/%i\n", (int)one, uvi + 1,(int)one,  (int)two, uvi + 2,
												(int)two, (int)three, uvi + 3, (int)three, (int)four, uvi + 4, (int)four);
					} else {
						stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i %i/%i\n", (int)one, uvi + 1, (int)two, uvi + 2,
												(int)three, uvi + 3, (int)four, uvi + 4);
					}
				} else {
					if (withNormals) {
						stream.writeStringFormat(false, "f %i//%i %i//%i %i//%i %i//%i\n", (int)one, (int)two, (int)three, (int)four, (int)one, (int)two, (int)three, (int)four);
					} else {
						stream.writeStringFormat(false, "f %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
					}
				}
			}
			texcoordOffset += ni / 6 * 4;
		} else {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 3) {
					const voxel::VoxelVertex &v = vertices[indices[i]];
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					stream.writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
				}
			}

			for (int i = 0; i < ni; i += 3) {
				const uint32_t one = idxOffset + indices[i + 0] + 1;
				const uint32_t two = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				if (withTexCoords) {
					if (withNormals) {
						stream.writeStringFormat(false, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", (int)one, texcoordOffset + i + 1, (int)one, (int)two,
												texcoordOffset + i + 2, (int)two, (int)three, texcoordOffset + i + 3, (int)three);
					} else {
						stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i\n", (int)one, texcoordOffset + i + 1, (int)two,
												texcoordOffset + i + 2, (int)three, texcoordOffset + i + 3);
					}
				} else {
					if (withNormals) {
						stream.writeStringFormat(false, "f %i//%i %i//%i %i//%i\n", (int)one, (int)two, (int)three, (int)one, (int)two, (int)three);
					} else {
						stream.writeStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
					}
				}
			}
			texcoordOffset += ni;
		}
		idxOffset += nv;

		if (paletteMaterialIndices.find(palette.hash()) == paletteMaterialIndices.end()) {
			core::String palettename = core::string::stripExtension(filename);
			palettename.append(hashId);
			palettename.append(".png");
			paletteMaterialIndices.put(palette.hash(), 1);
			const core::String &mapKd = core::string::extractFilenameWithExtension(palettename);
			if (!writeMtlFile(matlstream, hashId, mapKd)) {
				return false;
			}
			if (!palette.save(palettename.c_str())) {
				return false;
			}
		}
	}
	return true;
}

#undef wrapBool

bool OBJFormat::voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	const core::String& mtlbasedir = core::string::extractPath(filename);
	io::StdIStreamBuf stdStreamBuf(stream);
	std::istream inputStream(&stdStreamBuf);
	tinyobj::MaterialFileReader matFileReader(mtlbasedir.c_str());
	const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream, &matFileReader, true, false);
	if (!warn.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(warn.c_str(), lines, "\n");
		for (const core::String& str : lines) {
			Log::warn("%s", str.c_str());
		}
	}
	if (!err.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(err.c_str(), lines, "\n");
		for (const core::String& str : lines) {
			Log::error("%s", str.c_str());
		}
	}
	if (!ret) {
		Log::error("Failed to load obj '%s': %s", filename.c_str(), err.c_str());
		return false;
	}
	if (shapes.size() == 0) {
		Log::error("No shapes found in the model");
		return false;
	}

	core::StringMap<image::ImagePtr> textures;
	Log::debug("%i materials", (int)materials.size());

	for (tinyobj::material_t &material : materials) {
		core::String name = material.diffuse_texname.c_str();
		Log::debug("material: '%s'", material.name.c_str());
		Log::debug("- emissive_texname '%s'", material.emissive_texname.c_str());
		Log::debug("- ambient_texname '%s'", material.ambient_texname.c_str());
		Log::debug("- diffuse_texname '%s'", material.diffuse_texname.c_str());
		Log::debug("- specular_texname '%s'", material.specular_texname.c_str());
		Log::debug("- specular_highlight_texname '%s'", material.specular_highlight_texname.c_str());
		Log::debug("- bump_texname '%s'", material.bump_texname.c_str());
		Log::debug("- displacement_texname '%s'", material.displacement_texname.c_str());
		Log::debug("- alpha_texname '%s'", material.alpha_texname.c_str());
		Log::debug("- reflection_texname '%s'", material.reflection_texname.c_str());
		// TODO: material.diffuse_texopt.scale
		if (name.empty()) {
			continue;
		}

		if (textures.hasKey(name)) {
			Log::debug("texture for material '%s' is already loaded", name.c_str());
			continue;
		}

		name = lookupTexture(filename, name);
		image::ImagePtr tex = image::loadImage(name, false);
		if (tex->isLoaded()) {
			Log::debug("Use image %s", name.c_str());
			textures.put(material.diffuse_texname.c_str(), tex);
		} else {
			Log::warn("Failed to load image %s from %s", name.c_str(), material.name.c_str());
		}
	}

	const glm::vec3 &scale = getScale();
	for (tinyobj::shape_t &shape : shapes) {
		int indexOffset = 0;
		// TODO: shape.lines, shape.points
		const tinyobj::mesh_t &mesh = shape.mesh;
		for (const tinyobj::tag_t &tag : mesh.tags) {
			Log::debug("tag: %s", tag.name.c_str());
		}
		TriCollection tris;
		tris.reserve(mesh.num_face_vertices.size());
		for (size_t faceNum = 0; faceNum < mesh.num_face_vertices.size(); ++faceNum) {
			const int faceVertices = mesh.num_face_vertices[faceNum];
			core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
			Tri tri;
			for (int i = 0; i < faceVertices; ++i) {
				const tinyobj::index_t &idx = mesh.indices[indexOffset + i];
				tri.vertices[i].x = attrib.vertices[3 * idx.vertex_index + 0] * scale.x;
				tri.vertices[i].y = attrib.vertices[3 * idx.vertex_index + 1] * scale.y;
				tri.vertices[i].z = attrib.vertices[3 * idx.vertex_index + 2] * scale.z;
				if (!attrib.colors.empty()) {
					const float r = attrib.colors[3 * idx.vertex_index + 0];
					const float g = attrib.colors[3 * idx.vertex_index + 1];
					const float b = attrib.colors[3 * idx.vertex_index + 2];
					tri.color[i] = core::Color::getRGBA(glm::vec4(r, g, b, 1.0f));
				}
				if (idx.texcoord_index >= 0) {
					tri.uv[i].x = attrib.texcoords[2 * idx.texcoord_index + 0];
					tri.uv[i].y = attrib.texcoords[2 * idx.texcoord_index + 1];
				}
			}
			const int materialIndex = mesh.material_ids[faceNum];
			const tinyobj::material_t *material = materialIndex < 0 ? nullptr : &materials[materialIndex];
			if (material != nullptr) {
				const core::String diffuseTexture = material->diffuse_texname.c_str();
				if (!diffuseTexture.empty()) {
					auto textureIter = textures.find(diffuseTexture);
					if (textureIter != textures.end()) {
						tri.texture = textureIter->second.get();
					}
				}
				if (attrib.colors.empty()) {
					const glm::vec4 diffuseColor(material->diffuse[0], material->diffuse[1], material->diffuse[2], 1.0f);
					tri.color[0] = tri.color[1] = tri.color[2] = core::Color::getRGBA(diffuseColor);
				}
			}
			tris.push_back(tri);

			indexOffset += faceVertices;
		}
		if (voxelizeNode(shape.name.c_str(), sceneGraph, tris) < 0) {
			Log::error("Failed to voxelize shape %s", shape.name.c_str());
			return false;
		}
	}
	return !sceneGraph.empty();
}

} // namespace voxel
