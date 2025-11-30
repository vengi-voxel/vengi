/**
 * @file
 */

#include "JsonExporter.h"
#include "core/Log.h"
#include "core/Var.h"
#include "palette/Material.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelutil/VolumeVisitor.h"

namespace scenegraph {

struct NodeStats {
	int voxels = 0;
	int vertices = 0;
	int indices = 0;

	NodeStats operator+(const NodeStats &other) const {
		NodeStats result;
		result.voxels = voxels + other.voxels;
		result.vertices = vertices + other.vertices;
		result.indices = indices + other.indices;
		return result;
	}

	NodeStats &operator+=(const NodeStats &other) {
		voxels += other.voxels;
		vertices += other.vertices;
		indices += other.indices;
		return *this;
	}
};

static NodeStats sceneGraphJsonNode_r(const scenegraph::SceneGraph &sceneGraph, int nodeId, bool printMeshDetails) {
	const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);

	const scenegraph::SceneGraphNodeType type = node.type();

	Log::printf("{");
	Log::printf("\"id\": %i,", nodeId);
	Log::printf("\"parent\": %i,", node.parent());
	Log::printf("\"name\": \"%s\",", node.name().c_str());
	Log::printf("\"type\": \"%s\",", scenegraph::SceneGraphNodeTypeStr[core::enumVal(type)]);
	const glm::vec3 &pivot = node.pivot();
	Log::printf("\"pivot\": \"%f:%f:%f\"", pivot.x, pivot.y, pivot.z);
	if (node.hasPalette()) {
		Log::printf(",\"palette\": {\n");
		const palette::Palette &palette = node.palette();
		Log::printf("\"name\": \"%s\"", palette.name().c_str());
		Log::printf(",\"color_count\": %i", palette.colorCount());
		Log::printf(",\"colors\": [");
		for (size_t i = 0; i < palette.size(); ++i) {
			if (i > 0) {
				Log::printf(",");
			}
			const color::RGBA &color = palette.color(i);
			Log::printf("{");
			Log::printf("\"r\": %i", color.r);
			Log::printf(",\"g\": %i", color.g);
			Log::printf(",\"b\": %i", color.b);
			Log::printf(",\"a\": %i", color.a);
			if (!palette.colorName(i).empty()) {
				Log::printf(",\"name\": \"%s\"", palette.colorName(i).c_str());
			}
			const palette::Material &mat = palette.material(i);
			Log::printf(",\"material\": {");
			Log::printf("\"type\": \"%i\"", (int)mat.type);
			for (int m = 0; m < (int)palette::MaterialProperty::MaterialMax; ++m) {
				if (m == palette::MaterialProperty::MaterialNone) {
					continue;
				}
				palette::MaterialProperty prop = (palette::MaterialProperty)m;
				if (!mat.has(prop)) {
					continue;
				}
				const float value = mat.value(prop);
				Log::printf(",\"%s\": %f", palette::MaterialPropertyName(prop), value);
			}
			Log::printf("}\n");
			Log::printf("}");
		}
		Log::printf("]");
		Log::printf("}");
	}
	NodeStats stats;
	if (type == scenegraph::SceneGraphNodeType::Model) {
		const voxel::RawVolume *v = node.volume();
		const voxel::Region &region = node.region();
		Log::printf(",\"volume\": {");
		Log::printf("\"region\": {");
		Log::printf("\"mins\": \"%i:%i:%i\",", region.getLowerX(), region.getLowerY(), region.getLowerZ());
		Log::printf("\"maxs\": \"%i:%i:%i\",", region.getUpperX(), region.getUpperY(), region.getUpperZ());
		Log::printf("\"size\": \"%i:%i:%i\"", region.getWidthInVoxels(), region.getHeightInVoxels(),
					region.getDepthInVoxels());
		Log::printf("},");
		if (v) {
			stats.voxels = voxelutil::countVoxels(*v);
		}
		Log::printf("\"voxels\": %i", stats.voxels);
		Log::printf("}");
	} else if (type == scenegraph::SceneGraphNodeType::Camera) {
		const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
		Log::printf(",\"camera\": {");
		Log::printf("\"field_of_view\": %i,", cameraNode.fieldOfView());
		Log::printf("\"nearplane\": %f,", cameraNode.nearPlane());
		Log::printf("\"farplane\": %f,", cameraNode.farPlane());
		Log::printf("\"mode\": \"%s\"", cameraNode.isOrthographic() ? "ortho" : "perspective");
		Log::printf("}");
	}
	if (!node.properties().empty()) {
		Log::printf(",\"properties\": {");
		auto piter = node.properties().begin();
		for (size_t i = 0; i < node.properties().size(); ++i) {
			const auto &entry = *piter;
			Log::printf("\"%s\": \"%s\"", entry->key.c_str(), entry->value.c_str());
			if (i + 1 < node.properties().size()) {
				Log::printf(",");
			}
			++piter;
		}
		Log::printf("}");
	}
	Log::printf(",\"animations\": [");
	for (size_t a = 0; a < sceneGraph.animations().size(); ++a) {
		Log::printf("{");
		Log::printf("\"name\": \"%s\",", sceneGraph.animations()[a].c_str());
		Log::printf("\"keyframes\": [");
		for (size_t i = 0; i < node.keyFrames().size(); ++i) {
			const scenegraph::SceneGraphKeyFrame &kf = node.keyFrames()[i];
			Log::printf("{");
			Log::printf("\"id\": %i,", kf.frameIdx);
			Log::printf("\"long_rotation\": %s,", kf.longRotation ? "true" : "false");
			Log::printf("\"interpolation\": \"%s\",",
						scenegraph::InterpolationTypeStr[core::enumVal(kf.interpolation)]);
			Log::printf("\"transform\": {");
			const scenegraph::SceneGraphTransform &transform = kf.transform();
			const glm::vec3 &tr = transform.worldTranslation();
			Log::printf("\"world_translation\": {");
			Log::printf("\"x\": %f,", tr.x);
			Log::printf("\"y\": %f,", tr.y);
			Log::printf("\"z\": %f", tr.z);
			Log::printf("},");
			const glm::vec3 &ltr = transform.localTranslation();
			Log::printf("\"local_translation\": {");
			Log::printf("\"x\": %f,", ltr.x);
			Log::printf("\"y\": %f,", ltr.y);
			Log::printf("\"z\": %f", ltr.z);
			Log::printf("},");
			const glm::quat &rt = transform.worldOrientation();
			const glm::vec3 &rtEuler = glm::degrees(glm::eulerAngles(rt));
			Log::printf("\"world_orientation\": {");
			Log::printf("\"x\": %f,", rt.x);
			Log::printf("\"y\": %f,", rt.y);
			Log::printf("\"z\": %f,", rt.z);
			Log::printf("\"w\": %f", rt.w);
			Log::printf("},");
			Log::printf("\"world_euler\": {");
			Log::printf("\"x\": %f,", rtEuler.x);
			Log::printf("\"y\": %f,", rtEuler.y);
			Log::printf("\"z\": %f", rtEuler.z);
			Log::printf("},");
			const glm::quat &lrt = transform.localOrientation();
			const glm::vec3 &lrtEuler = glm::degrees(glm::eulerAngles(lrt));
			Log::printf("\"local_orientation\": {");
			Log::printf("\"x\": %f,", lrt.x);
			Log::printf("\"y\": %f,", lrt.y);
			Log::printf("\"z\": %f,", lrt.z);
			Log::printf("\"w\": %f", lrt.w);
			Log::printf("},");
			Log::printf("\"local_euler\": {");
			Log::printf("\"x\": %f,", lrtEuler.x);
			Log::printf("\"y\": %f,", lrtEuler.y);
			Log::printf("\"z\": %f", lrtEuler.z);
			Log::printf("},");
			const glm::vec3 &sc = transform.worldScale();
			Log::printf("\"world_scale\": {");
			Log::printf("\"x\": %f,", sc.x);
			Log::printf("\"y\": %f,", sc.y);
			Log::printf("\"z\": %f", sc.z);
			Log::printf("},");
			const glm::vec3 &lsc = transform.localScale();
			Log::printf("\"local_scale\": {");
			Log::printf("\"x\": %f,", lsc.x);
			Log::printf("\"y\": %f,", lsc.y);
			Log::printf("\"z\": %f", lsc.z);
			Log::printf("}");
			Log::printf("}"); // transform
			Log::printf("}"); // keyframe
			if (i + 1 < node.keyFrames().size()) {
				Log::printf(",");
			}
		}
		Log::printf("]"); // keyframes
		Log::printf("}"); // animation
		if (a + 1 < sceneGraph.animations().size()) {
			Log::printf(",");
		}
	}
	Log::printf("]"); // animations

	if (printMeshDetails && node.isModelNode()) {
		const bool mergeQuads = core::Var::getSafe(cfg::VoxformatMergequads)->boolVal();
		const bool reuseVertices = core::Var::getSafe(cfg::VoxformatReusevertices)->boolVal();
		const bool ambientOcclusion = core::Var::getSafe(cfg::VoxformatAmbientocclusion)->boolVal();
		const voxel::SurfaceExtractionType meshType =
			(voxel::SurfaceExtractionType)core::Var::getSafe(cfg::VoxelMeshMode)->intVal();
		voxel::ChunkMesh mesh;
		voxel::SurfaceExtractionContext ctx =
			voxel::createContext(meshType, node.volume(), node.region(), node.palette(), mesh, {0, 0, 0}, mergeQuads,
								 reuseVertices, ambientOcclusion);

		voxel::extractSurface(ctx);
		const size_t vertices = mesh.mesh[0].getNoOfVertices() + mesh.mesh[1].getNoOfVertices();
		const size_t indices = mesh.mesh[0].getNoOfIndices() + mesh.mesh[1].getNoOfIndices();
		Log::printf(",\"mesh\": {");
		Log::printf("\"vertices\": %i,", (int)vertices);
		Log::printf("\"indices\": %i", (int)indices);
		Log::printf("}");
		stats.vertices += (int)vertices;
		stats.indices += (int)indices;
	}
	if (!node.children().empty()) {
		Log::printf(",\"children\": [");
		for (size_t i = 0; i < node.children().size(); ++i) {
			const int children = node.children()[i];
			stats += sceneGraphJsonNode_r(sceneGraph, children, printMeshDetails);
			if (i + 1 < node.children().size()) {
				Log::printf(",");
			}
		}
		Log::printf("]");
	}
	Log::printf("}");
	return stats;
}

void sceneGraphJson(const scenegraph::SceneGraph &sceneGraph, bool printMeshDetails) {
	Log::printf("{");
	Log::printf("\"root\": ");
	NodeStats stats = sceneGraphJsonNode_r(sceneGraph, sceneGraph.root().id(), printMeshDetails);
	Log::printf(",");
	Log::printf("\"stats\": {");
	Log::printf("\"voxel_count\": %i", stats.voxels);
	if (printMeshDetails) {
		Log::printf(",\"vertex_count\": %i,", stats.vertices);
		Log::printf("\"index_count\": %i", stats.indices);
	}
	Log::printf("}"); // stats
	Log::printf("}");
}

} // namespace scenegraph
