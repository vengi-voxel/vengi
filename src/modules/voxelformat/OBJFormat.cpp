/**
 * @file
 */

#include "OBJFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/concurrent/ThreadPool.h"
#include "core/Var.h"
#include "core/collection/StringMap.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/VoxelVertex.h"
#include "voxel/Mesh.h"
#include "voxelformat/SceneGraph.h"
#include "engine-config.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tiny_obj_loader.h"

namespace voxel {

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Failed to write obj " CORE_STRINGIFY(read)); \
		return false; \
	}

bool OBJFormat::writeMtlFile(const core::String &mtlName, const core::String &paletteName) const {
	const io::FilePtr &file = io::filesystem()->open(mtlName, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		Log::error("Failed to create mtl file at %s", file->name().c_str());
		return false;
	}
	io::FileStream stream(file);
	wrapBool(stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/vengi\n"))
	wrapBool(stream.writeStringFormat(false, "\n"))
	wrapBool(stream.writeStringFormat(false, "newmtl palette\n"))
	wrapBool(stream.writeStringFormat(false, "Ka 1.000000 1.000000 1.000000\n"))
	wrapBool(stream.writeStringFormat(false, "Kd 1.000000 1.000000 1.000000\n"))
	wrapBool(stream.writeStringFormat(false, "Ks 0.000000 0.000000 0.000000\n"))
	wrapBool(stream.writeStringFormat(false, "Tr 1.000000\n"))
	wrapBool(stream.writeStringFormat(false, "illum 1\n"))
	wrapBool(stream.writeStringFormat(false, "Ns 0.000000\n"))
	const core::String& paletteFilename = core::string::extractFilenameWithExtension(paletteName);
	if (!stream.writeStringFormat(false, "map_Kd %s\n", paletteFilename.c_str())) {
		Log::error("Failed to write obj map_Kd");
		return false;
	}
	return true;
}

bool OBJFormat::saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	const voxel::Palette& palette = voxel::getPalette();
	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)palette.colorCount;
	// it is only 1 pixel high - sample the middle
	const float v1 = 0.5f;

	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/vengi\n");
	wrapBool(stream.writeStringFormat(false, "\n"))
	wrapBool(stream.writeStringFormat(false, "g Model\n"))

	core::String mtlname = core::string::stripExtension(filename);
	mtlname.append(".mtl");

	core::String palettename = core::string::stripExtension(filename);
	palettename.append(".png");

	Log::debug("Exporting %i layers", (int)meshes.size());

	int idxOffset = 0;
	int texcoordOffset = 0;
	for (const auto &meshExt : meshes) {
		const voxel::Mesh *mesh = meshExt.mesh;
		Log::debug("Exporting layer %s", meshExt.name.c_str());
		const int nv = (int)mesh->getNoOfVertices();
		const int ni = (int)mesh->getNoOfIndices();
		if (ni % 3 != 0) {
			Log::error("Unexpected indices amount");
			return false;
		}
		const glm::vec3 offset(mesh->getOffset());
		const voxel::VoxelVertex *vertices = mesh->getRawVertexData();
		const voxel::IndexType *indices = mesh->getRawIndexData();
		const char *objectName = meshExt.name.c_str();
		if (objectName[0] == '\0') {
			objectName = "Noname";
		}
		stream.writeStringFormat(false, "o %s\n", objectName);
		stream.writeStringFormat(false, "mtllib %s\n", core::string::extractFilenameWithExtension(mtlname).c_str());
		wrapBool(stream.writeStringFormat(false, "usemtl palette\n"))

		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex &v = vertices[i];

			glm::vec3 pos;
			if (meshExt.applyTransform) {
				pos = meshExt.transform.apply(v.position, meshExt.size);
			} else {
				pos = v.position;
			}
			pos = (offset + pos) * scale;
			stream.writeStringFormat(false, "v %.04f %.04f %.04f", pos.x, pos.y, pos.z);
			if (withColor) {
				const glm::vec4& color = core::Color::fromRGBA(palette.colors[v.colorIndex]);
				stream.writeStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
			}
			wrapBool(stream.writeStringFormat(false, "\n"))
		}

		if (quad) {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 6) {
					const voxel::VoxelVertex &v = vertices[indices[i]];
					const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
				}
			}

			int uvi = texcoordOffset;
			for (int i = 0; i < ni; i += 6, uvi += 4) {
				const uint32_t one = idxOffset + indices[i + 0] + 1;
				const uint32_t two = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				const uint32_t four = idxOffset + indices[i + 5] + 1;
				if (withTexCoords) {
					stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i %i/%i\n", (int)one, uvi + 1, (int)two, uvi + 2,
											 (int)three, uvi + 3, (int)four, uvi + 4);
				} else {
					stream.writeStringFormat(false, "f %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
				}
			}
			texcoordOffset += ni / 6 * 4;
		} else {
			if (withTexCoords) {
				for (int i = 0; i < ni; i += 3) {
					const voxel::VoxelVertex &v = vertices[indices[i]];
					const float u = ((float)(v.colorIndex) + 0.5f) * texcoord;
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
					stream.writeStringFormat(false, "vt %f %f\n", u, v1);
				}
			}

			for (int i = 0; i < ni; i += 3) {
				const uint32_t one = idxOffset + indices[i + 0] + 1;
				const uint32_t two = idxOffset + indices[i + 1] + 1;
				const uint32_t three = idxOffset + indices[i + 2] + 1;
				if (withTexCoords) {
					stream.writeStringFormat(false, "f %i/%i %i/%i %i/%i\n", (int)one, texcoordOffset + i + 1, (int)two,
											 texcoordOffset + i + 2, (int)three, texcoordOffset + i + 3);
				} else {
					stream.writeStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
				}
			}
			texcoordOffset += ni;
		}
		idxOffset += nv;
	}

	if (!writeMtlFile(mtlname, palettename)) {
		return false;
	}
	return voxel::getPalette().save(palettename.c_str());
}

#undef wrapBool

void OBJFormat::subdivideShape(const tinyobj::mesh_t &mesh, const core::StringMap<image::ImagePtr> &textures,
							  const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials,
							  core::DynamicArray<Tri> &subdivided) {
	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	int indexOffset = 0;
	for (size_t faceNum = 0; faceNum < mesh.num_face_vertices.size(); ++faceNum) {
		const int faceVertices = mesh.num_face_vertices[faceNum];
		core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
		Tri tri;
		for (int i = 0; i < faceVertices; ++i) {
			const tinyobj::index_t &idx = mesh.indices[indexOffset + i];
			tri.vertices[i].x = attrib.vertices[3 * idx.vertex_index + 0] * scaleX;
			tri.vertices[i].y = attrib.vertices[3 * idx.vertex_index + 1] * scaleY;
			tri.vertices[i].z = attrib.vertices[3 * idx.vertex_index + 2] * scaleZ;
			if (idx.texcoord_index >= 0) {
				tri.uv[i].x = attrib.texcoords[2 * idx.texcoord_index + 0];
				tri.uv[i].y = attrib.texcoords[2 * idx.texcoord_index + 1];
			} else {
				tri.uv[i] = glm::vec2(0.0f);
			}
		}
		const int materialIndex = mesh.material_ids[faceNum];
		const tinyobj::material_t *material = materialIndex < 0 ? nullptr : &materials[materialIndex];
		if (material != nullptr) {
			const core::String diffuseTexture = material->diffuse_texname.c_str();
			if (!diffuseTexture.empty()) {
				auto textureIter = textures.find(diffuseTexture);
				if (textureIter != textures.end()) {
					tri.texture = textureIter->second;
				}
			}
			const glm::vec4 diffuseColor(material->diffuse[0], material->diffuse[1], material->diffuse[2], 1.0f);
			tri.color = core::Color::getRGBA(diffuseColor);
		}

		indexOffset += faceVertices;
		subdivideTri(tri, subdivided);
	}
}

void OBJFormat::calculateAABB(const tinyobj::mesh_t &mesh, const tinyobj::attrib_t &attrib, glm::vec3 &mins,
							  glm::vec3 &maxs) {
	maxs = glm::vec3(-100000.0f);
	mins = glm::vec3(+100000.0f);

	const float scale = core::Var::getSafe(cfg::VoxformatScale)->floatVal();

	float scaleX = core::Var::getSafe(cfg::VoxformatScaleX)->floatVal();
	float scaleY = core::Var::getSafe(cfg::VoxformatScaleY)->floatVal();
	float scaleZ = core::Var::getSafe(cfg::VoxformatScaleZ)->floatVal();

	scaleX = scaleX != 1.0f ? scaleX : scale;
	scaleY = scaleY != 1.0f ? scaleY : scale;
	scaleZ = scaleZ != 1.0f ? scaleZ : scale;

	int indexOffset = 0;
	for (size_t faceNum = 0; faceNum < mesh.num_face_vertices.size(); ++faceNum) {
		const int faceVertices = mesh.num_face_vertices[faceNum];
		core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
		for (int i = 0; i < faceVertices; ++i) {
			const tinyobj::index_t &idx = mesh.indices[indexOffset + i];
			const float x = attrib.vertices[3 * idx.vertex_index + 0] * scaleX;
			const float y = attrib.vertices[3 * idx.vertex_index + 1] * scaleY;
			const float z = attrib.vertices[3 * idx.vertex_index + 2] * scaleZ;
			maxs.x = core_max(maxs.x, x);
			maxs.y = core_max(maxs.y, y);
			maxs.z = core_max(maxs.z, z);
			mins.x = core_min(mins.x, x);
			mins.y = core_min(mins.y, y);
			mins.z = core_min(mins.z, z);
		}
		indexOffset += faceVertices;
	}
}

bool OBJFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	const core::String& mtlbasedir = core::string::extractPath(filename);
	const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), mtlbasedir.c_str(), true, true);
	if (!warn.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(warn.c_str(), lines, "\n");
		for (const core::String& str : lines) {
			Log::debug("%s", str.c_str());
		}
	}
	if (!err.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(err.c_str(), lines, "\n");
		for (const core::String& str : lines) {
			Log::debug("%s", str.c_str());
		}
	}
	if (!ret) {
		Log::error("Failed to load: %s", filename.c_str());
		return false;
	}
	if (shapes.size() == 0) {
		Log::error("No shapes found in the model");
		return false;
	}

	core::StringMap<image::ImagePtr> textures;
	for (tinyobj::material_t &material : materials) {
		core::String name = material.diffuse_texname.c_str();
		if (name.empty()) {
			continue;
		}

		if (textures.hasKey(name)) {
			continue;
		}

		if (!core::string::isAbsolutePath(name)) {
			const core::String& path = core::string::extractPath(filename);
			Log::debug("Search image %s in path %s", name.c_str(), path.c_str());
			name = path + name;
		}
		image::ImagePtr tex = image::loadImage(name, false);
		if (tex->isLoaded()) {
			Log::debug("Use image %s", name.c_str());
			const core::String texname(material.diffuse_texname.c_str());
			textures.put(texname, tex);
		} else {
			Log::warn("Failed to load %s", name.c_str());
		}
	}

	core::DynamicArray<std::future<voxel::SceneGraphNode>> futures;
	futures.reserve(shapes.size());

	core::ThreadPool &threadPool = app::App::getInstance()->threadPool();
	for (tinyobj::shape_t &shape : shapes) {
		auto func = [&shape, attrib, materials, &textures]() {
			glm::vec3 mins;
			glm::vec3 maxs;
			calculateAABB(shape.mesh, attrib, mins, maxs);
			voxel::Region region(glm::floor(mins), glm::ceil(maxs));
			if (glm::any(glm::greaterThan(region.getDimensionsInVoxels(), glm::ivec3(512)))) {
				Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh!");
			}
			RawVolume *volume = new RawVolume(region);
			SceneGraphNode node;
			node.setVolume(volume, true);
			node.setName(shape.name.c_str());
			core::DynamicArray<Tri> subdivided;
			subdivideShape(shape.mesh, textures, attrib, materials, subdivided);
			voxelizeTris(volume, subdivided);
			return core::move(node);
		};
		if (shapes.size() > 1) {
			futures.emplace_back(threadPool.enqueue(func));
		} else {
			sceneGraph.emplace(func());
		}
	}
	for (auto & f : futures) {
		sceneGraph.emplace(core::move(f.get()));
	}

	return true;
}

} // namespace voxel
