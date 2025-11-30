/**
 * @file
 */

#include "OBJFormat.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/StdStreamBuf.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/TextureLookup.h"

#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#define TINYOBJLOADER_DONOT_INCLUDE_MAPBOX_EARCUT
#define TINYOBJLOADER_IMPLEMENTATION
#include <array> // needed by tiny_obj_loader.h
#include "voxelformat/external/earcut.hpp"
#include "voxelformat/external/tiny_obj_loader.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to write obj " CORE_STRINGIFY(read));                                                       \
		return false;                                                                                                  \
	}

// TODO: MATERIAL: one material entry per palette color
// https://paulbourke.net/dataformats/mtl/
bool OBJFormat::writeMtlFile(io::SeekableWriteStream &stream, const core::String &mtlId,
							 const core::String &mapKd) const {
	if (!stream.writeStringFormat(false, "\nnewmtl %s\n", mtlId.c_str())) {
		Log::error("Failed to write obj newmtl");
		return false;
	}
	// TODO: MATERIAL: Ka is ambient
	wrapBool(stream.writeString("Ka 1.000000 1.000000 1.000000\n", false))
	// TODO: MATERIAL: Kd is diffuse
	wrapBool(stream.writeString("Kd 1.000000 1.000000 1.000000\n", false))
	// TODO: MATERIAL: Ks is specular
	wrapBool(stream.writeString("Ks 0.000000 0.000000 0.000000\n", false))
	//  0 Color on and Ambient off
	//  1 Color on and Ambient on
	//  2 Highlight on
	//  3 Reflection on and Ray trace on
	//  4 Transparency: Glass on
	//    Reflection: Ray trace on
	//  5 Reflection: Fresnel on and Ray trace on
	//  6 Transparency: Refraction on
	//    Reflection: Fresnel off and Ray trace on
	//  7 Transparency: Refraction on
	//    Reflection: Fresnel on and Ray trace on
	//  8 Reflection on and Ray trace off
	//  9 Transparency: Glass on
	//    Reflection: Ray trace off
	// 10 Casts shadows onto invisible surfaces
	wrapBool(stream.writeString("illum 1\n", false))
	// TODO: MATERIAL: Ns is shininess
	// glm::pow(2, 10.0f * m->shininess + 1) (3ds)
	wrapBool(stream.writeString("Ns 0.000000\n", false))
	// TODO: MATERIAL: d is dissolve (don't define both d and Tr)
	// factor of 1.0 is fully opaque - 0.0 is fully dissolved (completely transparent)
	// 1.0 - transparency (3ds)
	// wrapBool(stream.writeString("d 1.000000\n", false))
	// TODO: MATERIAL: Tr is transparency (don't define both d and Tr)
	// wrapBool(stream.writeString("Tr 0.000000\n", false))
	// TODO: MATERIAL: Ni is ior
	// wrapBool(stream.writeString("Ni 0.000000\n", false))
	// TODO: MATERIAL: Ke is emissive
	// wrapBool(stream.writeString("Ke 0.000000\n", false))
	// TODO: MATERIAL: Kt or Tf is transmission filter
	// wrapBool(stream.writeString("Kt 0.000000\n", false))
	// wrapBool(stream.writeString("Tf 0.000000\n", false))
	// TODO: MATERIAL: Pr is roughness
	// wrapBool(stream.writeString("Pr 0.000000\n", false))
	// TODO: MATERIAL: Pm is metallic
	// wrapBool(stream.writeString("Pm 0.000000\n", false))

	// map_KS is specular map
	// map_d is opacity map
	// map_bump is bump map
	// refl is reflection map
	if (!stream.writeStringFormat(false, "map_Kd %s\n", mapKd.c_str())) {
		Log::error("Failed to write obj map_Kd");
		return false;
	}
	return true;
}

bool OBJFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	stream->writeStringFormat(false, "# version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n");
	wrapBool(stream->writeStringFormat(false, "\n"))
	wrapBool(stream->writeStringFormat(false, "g Model\n"))

	Log::debug("Exporting %i layers", (int)meshes.size());

	const core::String &mtlname = core::string::replaceExtension(filename, "mtl");
	Log::debug("Use mtl file: %s", mtlname.c_str());

	core::ScopedPtr<io::SeekableWriteStream> matlstream(archive->writeStream(mtlname));
	wrapBool(matlstream->writeString("# version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n", false))
	wrapBool(matlstream->writeString("\n", false))

	core::Map<uint64_t, int> paletteMaterialIndices((int)sceneGraph.size());

	int idxOffset = 0;
	int texcoordOffset = 0;
	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[i];
			if (mesh->isEmpty()) {
				continue;
			}
			Log::debug("Exporting layer %s", meshExt.name.c_str());
			const int nv = (int)mesh->getNoOfVertices();
			const int ni = (int)mesh->getNoOfIndices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const palette::Palette &palette = graphNode.palette();

			const core::String hashId = core::String::format("%" PRIu64, palette.hash());

			const voxel::VertexArray &vertices = mesh->getVertexVector();
			const voxel::IndexArray &indices = mesh->getIndexVector();
			const voxel::NormalArray &normals = mesh->getNormalVector();
			const bool withNormals = !normals.empty();
			const char *objectName = meshExt.name.c_str();
			if (objectName[0] == '\0') {
				objectName = "Noname";
			}
			stream->writeStringFormat(false, "o %s\n", objectName);
			stream->writeStringFormat(false, "mtllib %s\n",
									  core::string::extractFilenameWithExtension(mtlname).c_str());
			if (!stream->writeStringFormat(false, "usemtl %s\n", hashId.c_str())) {
				Log::error("Failed to write obj usemtl %s\n", hashId.c_str());
				return false;
			}

			for (int j = 0; j < nv; ++j) {
				const voxel::VoxelVertex &v = vertices[j];

				glm::vec3 pos;
				if (meshExt.applyTransform) {
					pos = transform.apply(v.position, meshExt.pivot * meshExt.size);
				} else {
					pos = v.position;
				}
				pos *= scale;
				stream->writeStringFormat(false, "v %.04f %.04f %.04f", pos.x, pos.y, pos.z);
				if (withColor) {
					const glm::vec4 &color = color::fromRGBA(palette.color(v.colorIndex));
					stream->writeStringFormat(false, " %.03f %.03f %.03f", color.r, color.g, color.b);
				}
				wrapBool(stream->writeStringFormat(false, "\n"))
			}
			if (withNormals) {
				for (int j = 0; j < nv; ++j) {
					const glm::vec3 &norm = normals[j];
					stream->writeStringFormat(false, "vn %.04f %.04f %.04f\n", norm.x, norm.y, norm.z);
				}
			}

			if (quad) {
				if (withTexCoords) {
					for (int j = 0; j < ni; j += 6) {
						const voxel::VoxelVertex &v = vertices[indices[j]];
						const glm::vec2 &uv = paletteUV(v.colorIndex);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					}
				}

				int uvi = texcoordOffset;
				for (int j = 0; j < ni - 5; j += 6, uvi += 4) {
					const uint32_t one = idxOffset + indices[j + 0] + 1;
					const uint32_t two = idxOffset + indices[j + 1] + 1;
					const uint32_t three = idxOffset + indices[j + 2] + 1;
					const uint32_t four = idxOffset + indices[j + 5] + 1;
					if (withTexCoords) {
						if (withNormals) {
							stream->writeStringFormat(false, "f %i/%i/%i %i/%i/%i %i/%i/%i %i/%i/%i\n", (int)one,
													  uvi + 1, (int)one, (int)two, uvi + 2, (int)two, (int)three,
													  uvi + 3, (int)three, (int)four, uvi + 4, (int)four);
						} else {
							stream->writeStringFormat(false, "f %i/%i %i/%i %i/%i %i/%i\n", (int)one, uvi + 1, (int)two,
													  uvi + 2, (int)three, uvi + 3, (int)four, uvi + 4);
						}
					} else {
						if (withNormals) {
							stream->writeStringFormat(false, "f %i//%i %i//%i %i//%i %i//%i\n", (int)one, (int)two,
													  (int)three, (int)four, (int)one, (int)two, (int)three, (int)four);
						} else {
							stream->writeStringFormat(false, "f %i %i %i %i\n", (int)one, (int)two, (int)three,
													  (int)four);
						}
					}
				}
				texcoordOffset += ni / 6 * 4;
			} else {
				if (withTexCoords) {
					for (int j = 0; j < ni; j += 3) {
						const voxel::VoxelVertex &v = vertices[indices[j]];
						const glm::vec2 &uv = paletteUV(v.colorIndex);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
						stream->writeStringFormat(false, "vt %f %f\n", uv.x, uv.y);
					}
				}

				for (int j = 0; j < ni; j += 3) {
					const uint32_t one = idxOffset + indices[j + 0] + 1;
					const uint32_t two = idxOffset + indices[j + 1] + 1;
					const uint32_t three = idxOffset + indices[j + 2] + 1;
					if (withTexCoords) {
						if (withNormals) {
							stream->writeStringFormat(false, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", (int)one,
													  texcoordOffset + j + 1, (int)one, (int)two,
													  texcoordOffset + j + 2, (int)two, (int)three,
													  texcoordOffset + j + 3, (int)three);
						} else {
							stream->writeStringFormat(false, "f %i/%i %i/%i %i/%i\n", (int)one, texcoordOffset + j + 1,
													  (int)two, texcoordOffset + j + 2, (int)three,
													  texcoordOffset + j + 3);
						}
					} else {
						if (withNormals) {
							stream->writeStringFormat(false, "f %i//%i %i//%i %i//%i\n", (int)one, (int)two, (int)three,
													  (int)one, (int)two, (int)three);
						} else {
							stream->writeStringFormat(false, "f %i %i %i\n", (int)one, (int)two, (int)three);
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
				if (!writeMtlFile(*matlstream, hashId, mapKd)) {
					return false;
				}
				if (!palette.save(palettename.c_str())) {
					return false;
				}
			}
		}
	}
	return true;
}

#undef wrapBool

void OBJFormat::loadPointCloud(tinyobj::attrib_t &tinyAttrib, tinyobj::shape_t &tinyShape, PointCloud &pointCloud) {
	pointCloud.resize(tinyShape.points.indices.size());
	for (int i = 0; i < (int)tinyShape.points.indices.size(); ++i) {
		const tinyobj::index_t &idx0 = tinyShape.points.indices[i];
		const glm::vec3 &vertex0{tinyAttrib.vertices[3 * idx0.vertex_index + 0],
								 tinyAttrib.vertices[3 * idx0.vertex_index + 1],
								 tinyAttrib.vertices[3 * idx0.vertex_index + 2]};
		pointCloud[i].position = vertex0;

		if (!tinyAttrib.colors.empty()) {
			const float r0 = tinyAttrib.colors[3 * idx0.vertex_index + 0];
			const float g0 = tinyAttrib.colors[3 * idx0.vertex_index + 1];
			const float b0 = tinyAttrib.colors[3 * idx0.vertex_index + 2];
			pointCloud[i].color = color::getRGBA(glm::vec4(r0, g0, b0, 1.0f));
		}
	}
}

bool OBJFormat::voxelizeMeshShape(const tinyobj::shape_t &tinyShape, const tinyobj::attrib_t &tinyAttrib,
								  const tinyobj::material_t *tinyMaterials, scenegraph::SceneGraph &sceneGraph,
								  MeshMaterialMap &meshMaterials, const MeshMaterialArray &meshMaterialArray) const {
	int indexOffset = 0;
	Mesh mesh;
	const tinyobj::mesh_t &tinyMesh = tinyShape.mesh;
	mesh.vertices.reserve(tinyMesh.num_face_vertices.size());
	mesh.indices.reserve(tinyMesh.indices.size());
	for (size_t faceNum = 0; faceNum < tinyMesh.num_face_vertices.size(); ++faceNum) {
		const int materialIndex = tinyMesh.material_ids[faceNum];
		const tinyobj::material_t *tinyMaterial = materialIndex < 0 ? nullptr : &tinyMaterials[materialIndex];
		const int faceVertices = tinyMesh.num_face_vertices[faceNum];
		core_assert_msg(faceVertices == 3, "Unexpected indices for triangulated mesh: %i", faceVertices);
		voxelformat::MeshTri meshTri;
		const tinyobj::index_t &idx0 = tinyMesh.indices[indexOffset + 0];
		const tinyobj::index_t &idx1 = tinyMesh.indices[indexOffset + 1];
		const tinyobj::index_t &idx2 = tinyMesh.indices[indexOffset + 2];
		const glm::vec3 &vertex0{tinyAttrib.vertices[3 * idx0.vertex_index + 0],
								 tinyAttrib.vertices[3 * idx0.vertex_index + 1],
								 tinyAttrib.vertices[3 * idx0.vertex_index + 2]};
		const glm::vec3 &vertex1{tinyAttrib.vertices[3 * idx1.vertex_index + 0],
								 tinyAttrib.vertices[3 * idx1.vertex_index + 1],
								 tinyAttrib.vertices[3 * idx1.vertex_index + 2]};
		const glm::vec3 &vertex2{tinyAttrib.vertices[3 * idx2.vertex_index + 0],
								 tinyAttrib.vertices[3 * idx2.vertex_index + 1],
								 tinyAttrib.vertices[3 * idx2.vertex_index + 2]};
		meshTri.setVertices(vertex0, vertex1, vertex2);
		if (!tinyAttrib.colors.empty()) {
			const float r0 = tinyAttrib.colors[3 * idx0.vertex_index + 0];
			const float g0 = tinyAttrib.colors[3 * idx0.vertex_index + 1];
			const float b0 = tinyAttrib.colors[3 * idx0.vertex_index + 2];
			const float r1 = tinyAttrib.colors[3 * idx1.vertex_index + 0];
			const float g1 = tinyAttrib.colors[3 * idx1.vertex_index + 1];
			const float b1 = tinyAttrib.colors[3 * idx1.vertex_index + 2];
			const float r2 = tinyAttrib.colors[3 * idx2.vertex_index + 0];
			const float g2 = tinyAttrib.colors[3 * idx2.vertex_index + 1];
			const float b2 = tinyAttrib.colors[3 * idx2.vertex_index + 2];
			meshTri.setColor(color::getRGBA(glm::vec4(r0, g0, b0, 1.0f)),
							 color::getRGBA(glm::vec4(r1, g1, b1, 1.0f)),
							 color::getRGBA(glm::vec4(r2, g2, b2, 1.0f)));
		}
		if (idx0.texcoord_index >= 0 && idx1.texcoord_index >= 0 && idx2.texcoord_index >= 0) {
			const glm::vec2 &uv0{tinyAttrib.texcoords[2 * idx0.texcoord_index + 0],
								 tinyAttrib.texcoords[2 * idx0.texcoord_index + 1]};
			const glm::vec2 &uv1{tinyAttrib.texcoords[2 * idx1.texcoord_index + 0],
								 tinyAttrib.texcoords[2 * idx1.texcoord_index + 1]};
			const glm::vec2 &uv2{tinyAttrib.texcoords[2 * idx2.texcoord_index + 0],
								 tinyAttrib.texcoords[2 * idx2.texcoord_index + 1]};
			meshTri.setUVs(uv0, uv1, uv2);
		}
		if (tinyMaterial != nullptr) {
			const core::String materialName = tinyMaterial->name.c_str();
			if (!materialName.empty()) {
				auto meshMaterialIter = meshMaterials.find(materialName);
				if (meshMaterialIter != meshMaterials.end()) {
					meshTri.materialIdx = meshMaterialIter->second;
				} else {
					Log::warn("Failed to look up texture %s", materialName.c_str());
					meshMaterials.put(materialName, MeshMaterialPtr());
				}
			}
			if (tinyAttrib.colors.empty()) {
				const glm::vec4 diffuseColor(tinyMaterial->diffuse[0], tinyMaterial->diffuse[1],
											 tinyMaterial->diffuse[2], 1.0f);
				meshTri.setColor(diffuseColor);
			}
		}
		mesh.addTriangle(meshTri);

		indexOffset += faceVertices;
	}
	mesh.materials = meshMaterialArray;
	const int nodeId = voxelizeMesh(tinyShape.name.c_str(), sceneGraph, core::move(mesh));
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to voxelize shape %s", tinyShape.name.c_str());
		return false;
	}
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	for (const tinyobj::tag_t &tinyTag : tinyMesh.tags) {
		node.setProperty(tinyTag.name.c_str(), "");
	}
	return true;
}

bool OBJFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	tinyobj::attrib_t tinyAttrib;
	std::vector<tinyobj::shape_t> tinyShapes;
	std::vector<tinyobj::material_t> tinyMaterials;
	std::string tinyWarn;
	std::string tinyErr;
	const core::String &mtlbasedir = core::string::extractDir(filename);
	io::StdIStreamBuf stdStreamBuf(*stream);
	std::istream inputStream(&stdStreamBuf);
	// TODO: VOXELFORMAT: use the archive
	tinyobj::MaterialFileReader tinyMatFileReader(mtlbasedir.c_str());
	Log::debug("Load obj %s", filename.c_str());
	const bool ret = tinyobj::LoadObj(&tinyAttrib, &tinyShapes, &tinyMaterials, &tinyWarn, &tinyErr, &inputStream,
									  &tinyMatFileReader, true, false);
	if (!tinyWarn.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(tinyWarn.c_str(), lines, "\n");
		for (const core::String &str : lines) {
			Log::warn("%s", str.c_str());
		}
	}
	if (!tinyErr.empty()) {
		core::DynamicArray<core::String> lines;
		core::string::splitString(tinyErr.c_str(), lines, "\n");
		for (const core::String &str : lines) {
			Log::error("%s", str.c_str());
		}
	}
	if (!ret) {
		Log::error("Failed to load obj '%s': %s", filename.c_str(), tinyErr.c_str());
		return false;
	}
	if (tinyShapes.size() == 0) {
		Log::error("No shapes found in the model");
		return false;
	}

	MeshMaterialMap meshMaterials;
	MeshMaterialArray meshMaterialArray;
	meshMaterialArray.reserve(tinyMaterials.size());
	Log::debug("%i materials", (int)tinyMaterials.size());

	for (tinyobj::material_t &tinyMaterial : tinyMaterials) {
		core::String materialName = tinyMaterial.name.c_str();
		Log::debug("material: '%s'", tinyMaterial.name.c_str());
		Log::debug("- emissive_texname '%s'", tinyMaterial.emissive_texname.c_str());
		Log::debug("- ambient_texname '%s'", tinyMaterial.ambient_texname.c_str());
		Log::debug("- diffuse_texname '%s'", tinyMaterial.diffuse_texname.c_str());
		Log::debug("- specular_texname '%s'", tinyMaterial.specular_texname.c_str());
		Log::debug("- specular_highlight_texname '%s'", tinyMaterial.specular_highlight_texname.c_str());
		Log::debug("- bump_texname '%s'", tinyMaterial.bump_texname.c_str());
		Log::debug("- displacement_texname '%s'", tinyMaterial.displacement_texname.c_str());
		Log::debug("- alpha_texname '%s'", tinyMaterial.alpha_texname.c_str());
		Log::debug("- reflection_texname '%s'", tinyMaterial.reflection_texname.c_str());
		// TODO: MATERIAL: material.diffuse_texopt.scale
		if (materialName.empty()) {
			continue;
		}

		if (meshMaterials.hasKey(materialName)) {
			Log::debug("texture for material '%s' is already loaded", materialName.c_str());
			continue;
		}

		MeshMaterialPtr meshMaterial = createMaterial(materialName);
		palette::Material &paletteMaterial = meshMaterial->material;
		paletteMaterial.setValue(palette::MaterialProperty::MaterialIndexOfRefraction, tinyMaterial.ior);
		paletteMaterial.setValue(palette::MaterialProperty::MaterialRoughness, tinyMaterial.roughness);
		paletteMaterial.setValue(palette::MaterialProperty::MaterialMetal, tinyMaterial.metallic);
		// TODO: MATERIAL: should be average these values?
		paletteMaterial.setValue(palette::MaterialProperty::MaterialEmit, tinyMaterial.emission[0]);
		// TODO: MATERIAL: is this maybe shininess? (Ns) material specular exponent is multiplied by the texture value
		// see https://www.fileformat.info/format/material/
		paletteMaterial.setValue(palette::MaterialProperty::MaterialSpecular, tinyMaterial.specular[0]);
		meshMaterial->transparency = 1.0f - tinyMaterial.dissolve;

		if (!tinyMaterial.diffuse_texname.empty()) {
			const core::String &diffuseTextureName =
				lookupTexture(filename, tinyMaterial.diffuse_texname.c_str(), archive);
			image::ImagePtr diffuseTexture = image::loadImage(diffuseTextureName);
			if (diffuseTexture->isLoaded()) {
				Log::debug("Use image %s", diffuseTextureName.c_str());
				meshMaterial->texture = diffuseTexture;
			} else {
				Log::warn("Failed to load image %s from %s", materialName.c_str(), tinyMaterial.name.c_str());
			}
		}
		meshMaterialArray.push_back(meshMaterial);
		meshMaterials.put(meshMaterial->name, meshMaterialArray.size() - 1);
	}

	for (tinyobj::shape_t &tinyShape : tinyShapes) {
		// TODO: VOXELFORMAT: shape.lines
		if (!tinyShape.mesh.num_face_vertices.empty()) {
			if (!voxelizeMeshShape(tinyShape, tinyAttrib, tinyMaterials.data(), sceneGraph, meshMaterials,
								   meshMaterialArray)) {
				Log::error("Failed to voxelize shape %s", tinyShape.name.c_str());
			}
		}
		if (!tinyShape.points.indices.empty()) {
			PointCloud pointCloud;
			loadPointCloud(tinyAttrib, tinyShape, pointCloud);
			if (voxelizePointCloud(filename, sceneGraph, core::move(pointCloud)) == InvalidNodeId) {
				Log::error("Failed to voxelize point cloud from shape %s", tinyShape.name.c_str());
			}
		}
	}
	return !sceneGraph.empty();
}

} // namespace voxelformat
