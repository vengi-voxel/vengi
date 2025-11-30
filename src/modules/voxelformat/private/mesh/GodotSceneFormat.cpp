/**
 * @file
 */

#include "GodotSceneFormat.h"
#include "color/Color.h"
#include "core/Hash.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "glm/common.hpp"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/VoxelVertex.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not save escn file");                                                                        \
		return false;                                                                                                  \
	}

// https://docs.godotengine.org/de/4.x/classes/class_transform3d.html
static core::String createTransform(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
									int frameIdx) {
	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
	const glm::mat4 &m = transform.worldMatrix();
	return core::String::format("Transform3D(%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f)", m[0][0], m[1][0],
								m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2],
								/* translation */ m[3][0], m[3][1], m[3][2]);
}

static core::String resolveParent(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	if (node.parent() == sceneGraph.root().id() || node.parent() == InvalidNodeId) {
		return ".";
	}
	const scenegraph::SceneGraphNode &parent = sceneGraph.node(node.parent());
	return parent.name();
}

// https://docs.godotengine.org/de/4.x/classes/class_color.html
static core::String createColor(const core::RGBA &color) {
	const glm::vec4 &colorf = core::Color::fromRGBA(color);
	return core::String::format("Color(%f, %f, %f, %f)", colorf.r, colorf.g, colorf.b, colorf.a);
}

// https://docs.godotengine.org/de/4.x/classes/class_standardmaterial3d.html
static bool saveMaterial(const scenegraph::SceneGraphNode &node, io::SeekableWriteStream &stream, int &subResourceId) {
	const palette::Palette &palette = node.palette();
	for (size_t i = 0; i < palette.size(); ++i) {
		wrapBool(stream.writeStringFormat(false, "[sub_resource type=\"StandardMaterial3D\" id=\"mat-%i\"]\n",
										  subResourceId))
		const core::RGBA &color = palette.color(i);
		const core::String &colorStr = createColor(color);
		wrapBool(stream.writeStringFormat(false, "albedo_color = %s\n", colorStr.c_str()))
		if (color.a < 255) {
			wrapBool(stream.writeString("transparency = 1\n", false))
		}
		const palette::Material &material = palette.material(i);
		if (material.has(palette::MaterialMetal)) {
			wrapBool(stream.writeStringFormat(false, "metallic = %f\n", material.value(palette::MaterialMetal)))
		}
		if (material.has(palette::MaterialSpecular)) {
			wrapBool(
				stream.writeStringFormat(false, "metallic_specular = %f\n", material.value(palette::MaterialSpecular)))
		}
		if (material.has(palette::MaterialRoughness)) {
			wrapBool(stream.writeStringFormat(false, "roughness = %f\n", material.value(palette::MaterialRoughness)))
		}
		if (material.has(palette::MaterialIndexOfRefraction)) {
			wrapBool(stream.writeString("refraction_enabled = true\n", false))
			wrapBool(stream.writeStringFormat(false, "refraction_scale = %f\n",
											  material.value(palette::MaterialIndexOfRefraction)))
		}
		if (material.has(palette::MaterialEmit)) {
			wrapBool(stream.writeString("emission_enabled = true\n", false))
			wrapBool(stream.writeStringFormat(false, "emission_energy_multiplier = %f\n",
											  material.value(palette::MaterialEmit)))
		}
		// TODO: MATERIAL: save other properties, too
		wrapBool(stream.writeString("\n", false))
		++subResourceId;
	}
	return true;
}

bool GodotSceneFormat::saveNode(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
								const scenegraph::SceneGraphNode &node, io::SeekableWriteStream &stream,
								const ChunkMeshes &meshes, int &subResourceId, WriterStage stage) const {
	if (stopExecution()) {
		return false;
	}
	if (stage == WriterStage::SUB_RESOURCE) {
		if (node.isAnyModelNode()) {
			auto iter = meshIdxNodeMap.find(node.id());
			if (iter == meshIdxNodeMap.end()) {
				Log::error("Could not find mesh for node %s", node.name().c_str());
				return false;
			}

			int paletteStartOffset = subResourceId;
			if (!saveMaterial(node, stream, subResourceId)) {
				Log::error("Could not save material for node %s", node.name().c_str());
				return false;
			}

			const ChunkMeshExt &meshExt = meshes[iter->value];
			Log::debug("Exporting model %s (%i) (%i meshes total)", meshExt.name.c_str(), node.id(),
					   (int)meshIdxNodeMap.size());
			wrapBool(stream.writeStringFormat(false, "[sub_resource type=\"ArrayMesh\" id=\"%i\"]\n", node.id()))
			wrapBool(stream.writeStringFormat(false, "resource_name = \"%s\"\n", node.name().c_str()))

			int surfaceIdx = 0;
			const palette::Palette &palette = node.palette();
			wrapBool(stream.writeStringFormat(false, "_surfaces = ["))
			for (size_t c = 0; c < palette.size(); ++c) {
				glm::vec3 mins(std::numeric_limits<float>::max());
				glm::vec3 maxs(std::numeric_limits<float>::lowest());

				if (stopExecution()) {
					break;
				}

				int vertexCount = 0;
				io::BufferedReadWriteStream buffer(1024 * 3 * sizeof(float));
				meshExt.visitByMaterial(
					c, [&buffer, &mins, &maxs, &vertexCount](const voxel::Mesh &mesh, voxel::IndexType index0,
															 voxel::IndexType index1, voxel::IndexType index2) {
						const voxel::VoxelVertex &vertex0 = mesh.getVertex(index0);
						const voxel::VoxelVertex &vertex1 = mesh.getVertex(index1);
						const voxel::VoxelVertex &vertex2 = mesh.getVertex(index2);
						maxs = glm::max(maxs, vertex0.position);
						mins = glm::min(mins, vertex0.position);
						maxs = glm::max(maxs, vertex1.position);
						mins = glm::min(mins, vertex1.position);
						maxs = glm::max(maxs, vertex2.position);
						mins = glm::min(mins, vertex2.position);
						buffer.writeFloat(vertex2.position.x);
						buffer.writeFloat(vertex2.position.y);
						buffer.writeFloat(vertex2.position.z);
						buffer.writeFloat(vertex1.position.x);
						buffer.writeFloat(vertex1.position.y);
						buffer.writeFloat(vertex1.position.z);
						buffer.writeFloat(vertex0.position.x);
						buffer.writeFloat(vertex0.position.y);
						buffer.writeFloat(vertex0.position.z);
						vertexCount += 3;
					});
				if (vertexCount <= 0) {
					continue;
				}
				if (surfaceIdx > 0) {
					wrapBool(stream.writeString(",\n", false))
				}
				wrapBool(stream.writeStringFormat(false, "{\n"))
				wrapBool(stream.writeStringFormat(false, "\t\"material\":SubResource(\"mat-%i\"),\n",
												  paletteStartOffset + (int)c))
				wrapBool(stream.writeStringFormat(false, "\t\"primitive\":3,\n")) // triangles
				// vertex is 3 * sizeof(float), normals and tangents are 2 * sizeof(uint16_t)
				wrapBool(stream.writeStringFormat(false,
												  "\t\"format\":1,\n")) // vertex=1 | normal=2 | tangent=4 | color=8 |
																		// uv=16 | uv2 = 32 | indices = 4096
				wrapBool(stream.writeString("\t\"vertex_data\": PackedByteArray(", false))
				for (int64_t n = 0; n < buffer.size(); ++n) {
					wrapBool(stream.writeStringFormat(false, "%i", buffer.getBuffer()[n]))
					if (n < buffer.size() - 1) {
						wrapBool(stream.writeString(", ", false))
					}
				}
				wrapBool(stream.writeString("),\n", false))
				wrapBool(stream.writeStringFormat(false, "\t\"vertex_count\": %i,\n", vertexCount))
				wrapBool(stream.writeStringFormat(false, "\t\"aabb\": AABB(%f, %f, %f, %f, %f, %f)\n", mins.x, mins.y,
												  mins.z, maxs.x, maxs.y, maxs.z))
				wrapBool(stream.writeStringFormat(false, "}"))
				surfaceIdx++;
			}
			wrapBool(stream.writeStringFormat(false, "\n]\n"))

			wrapBool(stream.writeString("\n", false))

			++subResourceId;
		}
	} else if (stage == WriterStage::NODES) {
		const core::String parent = resolveParent(sceneGraph, node);
		const core::String transform = createTransform(sceneGraph, node, 0);
		if (node.isRootNode()) {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"Node3D\"]\n", node.name().c_str()))
		} else if (node.isAnyModelNode()) {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"MeshInstance3D\" parent=\"%s\"]\n",
											  node.name().c_str(), parent.c_str()))
			wrapBool(stream.writeStringFormat(false, "mesh = SubResource(\"%i\")\n", node.id()))
			wrapBool(stream.writeStringFormat(false, "visible = %s\n", node.visible() ? "true" : "false"))
			wrapBool(stream.writeStringFormat(false, "transform = %s\n", transform.c_str()))
		} else if (node.isCameraNode()) {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"Camera3D\" parent=\"%s\"]\n",
											  node.name().c_str(), parent.c_str()))
			wrapBool(stream.writeStringFormat(false, "transform = %s\n", transform.c_str()))
		} else if (node.isPointNode()) {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"Marker3D\" parent=\"%s\"]\n",
											  node.name().c_str(), parent.c_str()))
			wrapBool(stream.writeStringFormat(false, "transform = %s\n", transform.c_str()))
		}
	}
	wrapBool(stream.writeString("\n", false))

	for (int child : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(child);
		if (cnode.isRootNode()) {
			return false;
		}
		if (!saveNode(meshIdxNodeMap, sceneGraph, cnode, stream, meshes, subResourceId, stage)) {
			Log::error("Failed to save node %s", cnode.name().c_str());
			return false;
		}
	}
	return true;
}

bool GodotSceneFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
								  const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
								  const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	Log::debug("Create godot scene file %s", filename.c_str());
	const core::String &uuid = core::UUID::generate().str();
	const int steps = sceneGraph.size();
	stream->writeStringFormat(false, "[gd_scene load_steps=%i format=3 uid=\"uid://%s\"]\n", steps, uuid.c_str());

	int subResourceId = 0;
	const scenegraph::SceneGraphNode &node = sceneGraph.root();
	if (!saveNode(meshIdxNodeMap, sceneGraph, node, *stream, meshes, subResourceId, WriterStage::SUB_RESOURCE)) {
		Log::error("Failed to save root node");
		return false;
	}
	return saveNode(meshIdxNodeMap, sceneGraph, node, *stream, meshes, subResourceId, WriterStage::NODES);
}

#undef wrapBool

} // namespace voxelformat
