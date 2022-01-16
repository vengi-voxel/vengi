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

void OBJFormat::writeMtlFile(const core::String &mtlName, const core::String &paletteName) const {
	const io::FilePtr &file = io::filesystem()->open(mtlName, io::FileMode::SysWrite);
	if (!file->validHandle()) {
		Log::error("Failed to create mtl file at %s", file->name().c_str());
		return;
	}
	io::FileStream stream(file);
	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/engine\n");
	stream.writeStringFormat(false, "\n");
	stream.writeStringFormat(false, "newmtl palette\n");
	stream.writeStringFormat(false, "Ka 1.000000 1.000000 1.000000\n");
	stream.writeStringFormat(false, "Kd 1.000000 1.000000 1.000000\n");
	stream.writeStringFormat(false, "Ks 0.000000 0.000000 0.000000\n");
	stream.writeStringFormat(false, "Tr 1.000000\n");
	stream.writeStringFormat(false, "illum 1\n");
	stream.writeStringFormat(false, "Ns 0.000000\n");
	stream.writeStringFormat(false, "map_Kd %s\n", core::string::extractFilenameWithExtension(paletteName).c_str());
}

bool OBJFormat::saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
						   float scale, bool quad, bool withColor, bool withTexCoords) {
	const MaterialColorArray &colors = getMaterialColors();

	// 1 x 256 is the texture format that we are using for our palette
	const float texcoord = 1.0f / (float)colors.size();
	// it is only 1 pixel high - sample the middle
	const float v1 = 0.5f;

	stream.writeStringFormat(false, "# version " PROJECT_VERSION " github.com/mgerhardy/engine\n");
	stream.writeStringFormat(false, "\n");
	stream.writeStringFormat(false, "g Model\n");

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
		stream.writeStringFormat(false, "usemtl palette\n");

		for (int i = 0; i < nv; ++i) {
			const voxel::VoxelVertex &v = vertices[i];
			stream.writeStringFormat(false, "v %.04f %.04f %.04f", (offset.x + (float)v.position.x) * scale,
									 (offset.y + (float)v.position.y) * scale,
									 (offset.z + (float)v.position.z) * scale);
			if (withColor) {
				const glm::vec4 &color = colors[v.colorIndex];
				stream.writeStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
			}
			stream.writeStringFormat(false, "\n");
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

	writeMtlFile(mtlname, palettename);
	voxel::saveMaterialColorPng(palettename);

	return true;
}

struct Tri {
	glm::vec3 vertices[3];
	glm::vec2 uv[3];
	image::ImagePtr texture;
	uint32_t color = 0xFFFFFFFF;

	glm::vec2 centerUV() const {
		return (uv[0] + uv[1] + uv[2]) / 3.0f;
	}

	glm::vec3 center() const {
		return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
	}

	glm::vec3 mins() const {
		glm::vec3 v;
		for (int i = 0; i < 3; ++i) {
			v[i] = core_min(vertices[0][i], core_min(vertices[1][i], vertices[2][i]));
		}
		return v;
	}

	glm::vec3 maxs() const {
		glm::vec3 v;
		for (int i = 0; i < 3; ++i) {
			v[i] = core_max(vertices[0][i], core_max(vertices[1][i], vertices[2][i]));
		}
		return v;
	}

	uint32_t colorAt(const glm::vec2 &uv) const {
		if (texture) {
			const float w = (float)texture->width();
			const float h = (float)texture->height();
			float x = uv.x * w;
			float y = uv.y * h;
			while (x < 0.0f)
				x += w;
			while (x > w)
				x -= w;
			while (y < 0.0f)
				y += h;
			while (y > h)
				y -= h;

			const int xint = (int)glm::round(x - 0.5f);
			const int yint = texture->height() - (int)glm::round(y - 0.5f) - 1;
			const uint8_t *ptr = texture->at(xint, yint);
			return *(const uint32_t*)ptr;
		}
		return color;
	}

	// Sierpinski gasket with keeping the middle
	void subdivide(Tri out[4]) const {
		const glm::vec3 midv[]{
			glm::mix(vertices[0], vertices[1], 0.5f),
			glm::mix(vertices[1], vertices[2], 0.5f),
			glm::mix(vertices[2], vertices[0], 0.5f)
		};
		const glm::vec2 miduv[]{
			glm::mix(uv[0], uv[1], 0.5f),
			glm::mix(uv[1], uv[2], 0.5f),
			glm::mix(uv[2], uv[0], 0.5f)
		};

		// the subdivided new three triangles
		out[0] = Tri{{vertices[0], midv[0], midv[2]}, {uv[0], miduv[0], miduv[2]}, texture, color};
		out[1] = Tri{{vertices[1], midv[1], midv[0]}, {uv[1], miduv[1], miduv[0]}, texture, color};
		out[2] = Tri{{vertices[2], midv[2], midv[1]}, {uv[2], miduv[2], miduv[1]}, texture, color};
		// keep the middle
		out[3] = Tri{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, texture, color};
	}
};

/**
 * Subdivide until we brought the triangles down to the size of 1 or smaller
 */
static void subdivideTri(const Tri& tri, core::DynamicArray<Tri> &tinyTris) {
	const glm::vec3& mins = tri.mins();
	const glm::vec3& maxs = tri.maxs();
	const glm::vec3 size = maxs - mins;
	if (glm::any(glm::greaterThan(size, glm::vec3(1.0f)))) {
		Tri out[4];
		tri.subdivide(out);
		for (int i = 0; i < lengthof(out); ++i) {
			subdivideTri(out[i], tinyTris);
		}
		return;
	}
	tinyTris.push_back(tri);
}

static void voxelizeShape(const tinyobj::shape_t &shape, const core::StringMap<image::ImagePtr> &textures,
						  const tinyobj::attrib_t &attrib, const std::vector<tinyobj::material_t> &materials, voxel::RawVolume *volume) {
	core::DynamicArray<Tri> subdivided;
	const tinyobj::mesh_t &mesh = shape.mesh;

	int indexOffset = 0;
	for (size_t faceNum = 0; faceNum < shape.mesh.num_face_vertices.size(); ++faceNum) {
		const int faceVertices = shape.mesh.num_face_vertices[faceNum];
		core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
		Tri tri;
		for (int i = 0; i < faceVertices; ++i) {
			const tinyobj::index_t &idx = mesh.indices[indexOffset + i];
			tri.vertices[i].x = attrib.vertices[3 * idx.vertex_index + 0];
			tri.vertices[i].y = attrib.vertices[3 * idx.vertex_index + 1];
			tri.vertices[i].z = attrib.vertices[3 * idx.vertex_index + 2];
			if (idx.texcoord_index >= 0) {
				tri.uv[i].x = attrib.texcoords[2 * idx.texcoord_index + 0];
				tri.uv[i].y = attrib.texcoords[2 * idx.texcoord_index + 1];
			} else {
				tri.uv[i] = glm::vec2(0.0f);
			}
		}
		const int materialIndex = shape.mesh.material_ids[faceNum];
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

	core::DynamicArray<uint8_t> palette;
	palette.reserve(subdivided.size());
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();

	for (const Tri &tri : subdivided) {
		const glm::vec2 &uv = tri.centerUV();
		const uint32_t rgba = tri.colorAt(uv);
		const glm::vec4 &color = core::Color::fromRGBA(rgba);
		const uint8_t index = core::Color::getClosestMatch(color, materialColors);
		palette.push_back(index);
	}

	for (size_t i = 0; i < subdivided.size(); ++i) {
		const Tri &tri = subdivided[i];
		const uint8_t index = palette[i];
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		// TODO: different tris might contribute to the same voxel - merge the color values here
		for (int v = 0; v < 3; v++) {
			const glm::ivec3 p(glm::floor(tri.vertices[v]));
			volume->setVoxel(p, voxel);
		}

		const glm::vec3 &center = tri.center();
		const glm::ivec3 p2(glm::floor(center));
		volume->setVoxel(p2, voxel);
	}

	// TODO: fill the inner parts of the model
}

static void calculateAABB(const tinyobj::shape_t &shape, const tinyobj::attrib_t &attrib, glm::vec3 &mins, glm::vec3 &maxs) {
	maxs = glm::vec3(-100000.0f);
	mins = glm::vec3(+100000.0f);

	int indexOffset = 0;
	const tinyobj::mesh_t &mesh = shape.mesh;
	for (size_t faceNum = 0; faceNum < shape.mesh.num_face_vertices.size(); ++faceNum) {
		const int faceVertices = shape.mesh.num_face_vertices[faceNum];
		core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
		for (int i = 0; i < faceVertices; ++i) {
			const tinyobj::index_t &idx = mesh.indices[indexOffset + i];
			const float x = attrib.vertices[3 * idx.vertex_index + 0];
			const float y = attrib.vertices[3 * idx.vertex_index + 1];
			const float z = attrib.vertices[3 * idx.vertex_index + 2];
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
		Log::warn("%s", warn.c_str());
	}
	if (!err.empty()) {
		Log::error("%s", err.c_str());
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

	for (tinyobj::shape_t &shape : shapes) {
		glm::vec3 mins;
		glm::vec3 maxs;
		calculateAABB(shape, attrib, mins, maxs);
		voxel::Region region(glm::floor(mins), glm::ceil(maxs));
		if (glm::any(glm::greaterThan(region.getDimensionsInVoxels(), glm::ivec3(512)))) {
			Log::warn("Large meshes will take a lot of time and use a lot of memory. Consider scaling the mesh!");
		}
		RawVolume *volume = new RawVolume(region);
		SceneGraphNode node;
		node.setVolume(volume, true);
		node.setName(shape.name.c_str());
		voxelizeShape(shape, textures, attrib, materials, volume);
		sceneGraph.emplace(core::move(node));
	}

	return true;
}

} // namespace voxel
