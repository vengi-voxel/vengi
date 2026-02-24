/**
 * @file
 */

#include "JsonExporter.h"
#include "core/Var.h"
#include "palette/Material.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelutil/VolumeVisitor.h"

namespace scenegraph {

NodeStats NodeStats::operator+(const NodeStats &other) const {
	NodeStats result;
	result.voxels = voxels + other.voxels;
	result.vertices = vertices + other.vertices;
	result.indices = indices + other.indices;
	return result;
}

NodeStats &NodeStats::operator+=(const NodeStats &other) {
	voxels += other.voxels;
	vertices += other.vertices;
	indices += other.indices;
	return *this;
}

NodeStats sceneGraphNodeJson(const scenegraph::SceneGraph &sceneGraph, int nodeId, io::WriteStream &stream,
							 uint32_t flags) {
	const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);

	const scenegraph::SceneGraphNodeType type = node.type();

	stream.writeStringFormat(false, "{");
	stream.writeStringFormat(false, "\"id\":%i,", nodeId);
	stream.writeStringFormat(false, "\"uuid\":\"%s\",", node.uuid().str().c_str());
	stream.writeStringFormat(false, "\"parent\":%i,", node.parent());
	stream.writeStringFormat(false, "\"name\":\"%s\",", node.name().c_str());
	stream.writeStringFormat(false, "\"type\":\"%s\",", scenegraph::SceneGraphNodeTypeStr[core::enumVal(type)]);
	const glm::vec3 &pivot = node.pivot();
	stream.writeStringFormat(false, "\"pivot\":\"%f:%f:%f\"", pivot.x, pivot.y, pivot.z);
	if ((flags & JSONEXPORTER_PALETTE) && node.hasPalette()) {
		stream.writeStringFormat(false, ",\"palette\":{\n");
		const palette::Palette &palette = node.palette();
		stream.writeStringFormat(false, "\"name\":\"%s\"", palette.name().c_str());
		stream.writeStringFormat(false, ",\"color_count\":%i", palette.colorCount());
		stream.writeStringFormat(false, ",\"colors\":[");
		for (size_t i = 0; i < palette.size(); ++i) {
			if (i > 0) {
				stream.writeStringFormat(false, ",");
			}
			const color::RGBA &color = palette.color(i);
			stream.writeStringFormat(false, "{");
			stream.writeStringFormat(false, "\"r\":%i", color.r);
			stream.writeStringFormat(false, ",\"g\":%i", color.g);
			stream.writeStringFormat(false, ",\"b\":%i", color.b);
			stream.writeStringFormat(false, ",\"a\":%i", color.a);
			if (!palette.colorName(i).empty()) {
				stream.writeStringFormat(false, ",\"name\":\"%s\"", palette.colorName(i).c_str());
			}
			if ((flags & JSONEXPORTER_PALETTEMATERIALS)) {
				const palette::Material &mat = palette.material(i);
				stream.writeStringFormat(false, ",\"material\":{");
				stream.writeStringFormat(false, "\"type\":\"%i\"", (int)mat.type);
				for (int m = 0; m < (int)palette::MaterialProperty::MaterialMax; ++m) {
					if (m == palette::MaterialProperty::MaterialNone) {
						continue;
					}
					palette::MaterialProperty prop = (palette::MaterialProperty)m;
					if (!mat.has(prop)) {
						continue;
					}
					const float value = mat.value(prop);
					stream.writeStringFormat(false, ",\"%s\":%f", palette::MaterialPropertyName(prop), value);
				}
				stream.writeStringFormat(false, "}");
			}
			stream.writeStringFormat(false, "}");
		}
		stream.writeStringFormat(false, "]");
		stream.writeStringFormat(false, "}");
	}
	NodeStats stats;
	if (flags & JSONEXPORTER_NODEDETAILS) {
		if (type == scenegraph::SceneGraphNodeType::Model) {
			const voxel::RawVolume *v = node.volume();
			const voxel::Region &region = node.region();
			stream.writeStringFormat(false, ",\"volume\":{");
			stream.writeStringFormat(false, "\"region\":{");
			stream.writeStringFormat(false, "\"mins\":\"%i:%i:%i\",", region.getLowerX(), region.getLowerY(),
									 region.getLowerZ());
			stream.writeStringFormat(false, "\"maxs\":\"%i:%i:%i\",", region.getUpperX(), region.getUpperY(),
									 region.getUpperZ());
			stream.writeStringFormat(false, "\"size\":\"%i:%i:%i\"", region.getWidthInVoxels(),
									 region.getHeightInVoxels(), region.getDepthInVoxels());
			stream.writeStringFormat(false, "},");
			if (v) {
				stats.voxels = voxelutil::countVoxels(*v);
			}
			stream.writeStringFormat(false, "\"voxels\":%i", stats.voxels);
			stream.writeStringFormat(false, "}");
		} else if (type == scenegraph::SceneGraphNodeType::Camera) {
			const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
			stream.writeStringFormat(false, ",\"camera\":{");
			stream.writeStringFormat(false, "\"field_of_view\":%i,", cameraNode.fieldOfView());
			stream.writeStringFormat(false, "\"nearplane\":%f,", cameraNode.nearPlane());
			stream.writeStringFormat(false, "\"farplane\":%f,", cameraNode.farPlane());
			stream.writeStringFormat(false, "\"mode\":\"%s\"", cameraNode.isOrthographic() ? "ortho" : "perspective");
			stream.writeStringFormat(false, "}");
		}
		if (!node.properties().empty()) {
			stream.writeStringFormat(false, ",\"properties\":{");
			auto piter = node.properties().begin();
			for (size_t i = 0; i < node.properties().size(); ++i) {
				const auto &entry = *piter;
				stream.writeStringFormat(false, "\"%s\":\"%s\"", entry->key.c_str(), entry->value.c_str());
				if (i + 1 < node.properties().size()) {
					stream.writeStringFormat(false, ",");
				}
				++piter;
			}
			stream.writeStringFormat(false, "}");
		}
		stream.writeStringFormat(false, ",\"animations\":[");
		for (size_t a = 0; a < sceneGraph.animations().size(); ++a) {
			stream.writeStringFormat(false, "{");
			stream.writeStringFormat(false, "\"name\":\"%s\",", sceneGraph.animations()[a].c_str());
			stream.writeStringFormat(false, "\"keyframes\":[");
			for (size_t i = 0; i < node.keyFrames().size(); ++i) {
				const scenegraph::SceneGraphKeyFrame &kf = node.keyFrames()[i];
				stream.writeStringFormat(false, "{");
				stream.writeStringFormat(false, "\"id\":%i,", kf.frameIdx);
				stream.writeStringFormat(false, "\"long_rotation\":%s,", kf.longRotation ? "true" : "false");
				stream.writeStringFormat(false, "\"interpolation\":\"%s\",",
										 scenegraph::InterpolationTypeStr[core::enumVal(kf.interpolation)]);
				stream.writeStringFormat(false, "\"transform\":{");
				const scenegraph::SceneGraphTransform &transform = kf.transform();
				const glm::vec3 &tr = transform.worldTranslation();
				stream.writeStringFormat(false, "\"world_translation\":{");
				stream.writeStringFormat(false, "\"x\":%f,", tr.x);
				stream.writeStringFormat(false, "\"y\":%f,", tr.y);
				stream.writeStringFormat(false, "\"z\":%f", tr.z);
				stream.writeStringFormat(false, "},");
				const glm::vec3 &ltr = transform.localTranslation();
				stream.writeStringFormat(false, "\"local_translation\":{");
				stream.writeStringFormat(false, "\"x\":%f,", ltr.x);
				stream.writeStringFormat(false, "\"y\":%f,", ltr.y);
				stream.writeStringFormat(false, "\"z\":%f", ltr.z);
				stream.writeStringFormat(false, "},");
				const glm::quat &rt = transform.worldOrientation();
				const glm::vec3 &rtEuler = glm::degrees(glm::eulerAngles(rt));
				stream.writeStringFormat(false, "\"world_orientation\":{");
				stream.writeStringFormat(false, "\"x\":%f,", rt.x);
				stream.writeStringFormat(false, "\"y\":%f,", rt.y);
				stream.writeStringFormat(false, "\"z\":%f,", rt.z);
				stream.writeStringFormat(false, "\"w\":%f", rt.w);
				stream.writeStringFormat(false, "},");
				stream.writeStringFormat(false, "\"world_euler\":{");
				stream.writeStringFormat(false, "\"x\":%f,", rtEuler.x);
				stream.writeStringFormat(false, "\"y\":%f,", rtEuler.y);
				stream.writeStringFormat(false, "\"z\":%f", rtEuler.z);
				stream.writeStringFormat(false, "},");
				const glm::quat &lrt = transform.localOrientation();
				const glm::vec3 &lrtEuler = glm::degrees(glm::eulerAngles(lrt));
				stream.writeStringFormat(false, "\"local_orientation\":{");
				stream.writeStringFormat(false, "\"x\":%f,", lrt.x);
				stream.writeStringFormat(false, "\"y\":%f,", lrt.y);
				stream.writeStringFormat(false, "\"z\":%f,", lrt.z);
				stream.writeStringFormat(false, "\"w\":%f", lrt.w);
				stream.writeStringFormat(false, "},");
				stream.writeStringFormat(false, "\"local_euler\":{");
				stream.writeStringFormat(false, "\"x\":%f,", lrtEuler.x);
				stream.writeStringFormat(false, "\"y\":%f,", lrtEuler.y);
				stream.writeStringFormat(false, "\"z\":%f", lrtEuler.z);
				stream.writeStringFormat(false, "},");
				const glm::vec3 &sc = transform.worldScale();
				stream.writeStringFormat(false, "\"world_scale\":{");
				stream.writeStringFormat(false, "\"x\":%f,", sc.x);
				stream.writeStringFormat(false, "\"y\":%f,", sc.y);
				stream.writeStringFormat(false, "\"z\":%f", sc.z);
				stream.writeStringFormat(false, "},");
				const glm::vec3 &lsc = transform.localScale();
				stream.writeStringFormat(false, "\"local_scale\":{");
				stream.writeStringFormat(false, "\"x\":%f,", lsc.x);
				stream.writeStringFormat(false, "\"y\":%f,", lsc.y);
				stream.writeStringFormat(false, "\"z\":%f", lsc.z);
				stream.writeStringFormat(false, "}");
				stream.writeStringFormat(false, "}"); // transform
				stream.writeStringFormat(false, "}"); // keyframe
				if (i + 1 < node.keyFrames().size()) {
					stream.writeStringFormat(false, ",");
				}
			}
			stream.writeStringFormat(false, "]"); // keyframes
			stream.writeStringFormat(false, "}"); // animation
			if (a + 1 < sceneGraph.animations().size()) {
				stream.writeStringFormat(false, ",");
			}
		}
		stream.writeStringFormat(false, "]"); // animations

		if ((flags & JSONEXPORTER_MESHDETAILS) && node.isModelNode()) {
			const bool mergeQuads = core::Var::getVar(cfg::VoxformatMergequads)->boolVal();
			const bool reuseVertices = core::Var::getVar(cfg::VoxformatReusevertices)->boolVal();
			const bool ambientOcclusion = core::Var::getVar(cfg::VoxformatAmbientocclusion)->boolVal();
			const voxel::SurfaceExtractionType meshType =
				(voxel::SurfaceExtractionType)core::Var::getVar(cfg::VoxformatMeshMode)->intVal();
			voxel::ChunkMesh mesh;
			voxel::SurfaceExtractionContext ctx =
				voxel::createContext(meshType, node.volume(), node.region(), node.palette(), mesh, {0, 0, 0},
									 mergeQuads, reuseVertices, ambientOcclusion);

			voxel::extractSurface(ctx);
			const size_t vertices = mesh.mesh[0].getNoOfVertices() + mesh.mesh[1].getNoOfVertices();
			const size_t indices = mesh.mesh[0].getNoOfIndices() + mesh.mesh[1].getNoOfIndices();
			stream.writeStringFormat(false, ",\"mesh\":{");
			stream.writeStringFormat(false, "\"vertices\":%i,", (int)vertices);
			stream.writeStringFormat(false, "\"indices\":%i", (int)indices);
			stream.writeStringFormat(false, "}");
			stats.vertices += (int)vertices;
			stats.indices += (int)indices;
		}
	}
	if ((flags & JSONEXPORTER_CHILDREN) && !node.children().empty()) {
		stream.writeStringFormat(false, ",\"children\":[");
		for (size_t i = 0; i < node.children().size(); ++i) {
			const int childId = node.children()[i];
			stats += sceneGraphNodeJson(sceneGraph, childId, stream, flags);
			if (i + 1 < node.children().size()) {
				stream.writeStringFormat(false, ",");
			}
		}
		stream.writeStringFormat(false, "]");
	}
	stream.writeStringFormat(false, "}");
	return stats;
}

void sceneGraphNodeStatsJson(const NodeStats &stats, io::WriteStream &stream, uint32_t flags) {
	stream.writeStringFormat(false, "{\"voxel_count\":%i", stats.voxels);
	if (flags & JSONEXPORTER_MESHDETAILS) {
		stream.writeStringFormat(false, ",\"vertex_count\":%i", stats.vertices);
		stream.writeStringFormat(false, ",\"index_count\":%i", stats.indices);
	}
	stream.writeStringFormat(false, "}");
}

void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, io::WriteStream &stream, uint32_t flags) {
	stream.writeStringFormat(false, "{");
	stream.writeStringFormat(false, "\"root\":");
	NodeStats stats = sceneGraphNodeJson(sceneGraph, sceneGraph.root().id(), stream, flags);
	stream.writeStringFormat(false, ",\"stats\":");
	sceneGraphNodeStatsJson(stats, stream, flags);
	stream.writeStringFormat(false, "}");
}

} // namespace scenegraph
