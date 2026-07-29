/**
 * @file
 */

#include "BlockbenchFormat.h"
#include "LZUTF8.h"
#include "MeshMaterial.h"
#include "Polygon.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/UUID.h"
#include "image/Image.h"
#include "io/Base64ReadStream.h"
#include "io/Base64WriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "util/Version.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxelutil/ImportFace.h"

#include "json/JSON.h"
#include <glm/trigonometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>

namespace voxelformat {

namespace priv {

static bool versionLessThan(const util::Version &v, int major, int minor) {
	return v.majorVersion < major || (v.majorVersion == major && v.minorVersion < minor);
}

static json::Json parseBBModelJson(const core::String &raw) {
	const core::String text = lzutf8::decodeBBModelText(raw);
	json::Json json = json::Json::parse(text);
	if (json.isValid()) {
		return json;
	}
	const core::String stripped = lzutf8::stripJsonComments(text);
	json = json::Json::parse(stripped);
	if (!json.isValid()) {
		Log::error("Failed to parse bbmodel JSON");
	}
	return json;
}

static void applyFormatMigrations(json::Json &json, BlockbenchFormat::BBMeta &bbMeta) {
	// Missing model_format inference
	if (bbMeta.modelFormat.empty()) {
		const json::Json &metaJson = json.get("meta");
		if (metaJson.boolVal("bone_rig", false)) {
			bbMeta.modelFormat = "bedrock_old";
		} else {
			bbMeta.modelFormat = "java_block";
		}
	}

	// cubes -> elements
	if (!json.contains("elements") && json.contains("cubes")) {
		json.set("elements", json.get("cubes"));
	}

	// geometry_name -> model_identifier
	if (!json.contains("model_identifier") && json.contains("geometry_name")) {
		json.set("model_identifier", json.get("geometry_name"));
	}

	// Pre-4.5 + box_uv: shade=false -> mirror_uv=true on elements
	if (versionLessThan(bbMeta.version, 4, 5) && bbMeta.box_uv && json.contains("elements")) {
		json::Json elements = json.get("elements");
		if (elements.isArray()) {
			for (int i = 0; i < elements.size(); ++i) {
				json::Json el = elements.get(i);
				if (el.isObject() && el.contains("shade") && !el.boolVal("shade", true)) {
					el.set("mirror_uv", true);
				}
			}
		}
	}

	// Pre-4.10: ensure relative_path is usable (leading slash join handled at load time)
	(void)bbMeta;
}

struct KeyFrame {
	core::String channel; // "rotation", "position", "scale"
	core::Buffer<glm::vec3> dataPoints;
	core::UUID uuid;
	float time = 0.0f;
	int color; // none = -1, white = 0, black, red, green, blue, yellow, pink, purple, orange, brown, cyan, gray, lightgray
	scenegraph::InterpolationType interpolation = scenegraph::InterpolationType::Linear;
	bool bezierLinked = false;
	glm::vec3 bezierLeftTime{0.0f};
	glm::vec3 bezierLeftValue{0.0f};
	glm::vec3 bezierRightTime{0.0f};
	glm::vec3 bezierRightValue{0.0f};
};

struct Animator {
	core::UUID uuid;
	core::String name;
	core::String type; // "bone", "cube"
	bool rotationGlobal = false;
	core::DynamicArray<KeyFrame> keyframes;
};

struct Animation {
	core::UUID uuid;
	core::String name;
	core::String loop; // "loop", "once"
	bool overrideVal;
	bool selected;
	float length;
	int snapping;
	core::DynamicArray<Animator> animators;
};

#define BLOCKBENCH_ANIMATION 1
#if BLOCKBENCH_ANIMATION
static inline scenegraph::InterpolationType toInterpolationType(const json::Json &json, const char *key, const scenegraph::InterpolationType defaultValue = scenegraph::InterpolationType::Linear) {
	const core::String val = json.strVal(key, "");
	if (val.empty()) {
		return defaultValue;
	}
	if (val == "linear") {
		return scenegraph::InterpolationType::Linear;
	} else if (val == "ease_in") {
		return scenegraph::InterpolationType::QuadEaseIn;
	} else if (val == "ease_out") {
		return scenegraph::InterpolationType::QuadEaseOut;
	} else if (val == "ease_in_out") {
		return scenegraph::InterpolationType::QuadEaseInOut;
	} else if (val == "bezier") {
		return scenegraph::InterpolationType::CubicBezier;
	} else if (val == "catmullrom") {
		return scenegraph::InterpolationType::CatmullRom;
	} else if (val == "step") {
		return scenegraph::InterpolationType::Instant;
	}
	Log::warn("Unsupported interpolation type: %s", val.c_str());
	return defaultValue;
}
#endif

template<class T>
static T toNumber(const json::Json &json, const char *key, T defaultValue) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	json::Json child = json.get(key);
	if (child.isNull()) {
		return defaultValue;
	}
	if (!child.isNumber()) {
		Log::warn("Value is not a number: %s", key);
		return defaultValue;
	}
	return (T)child.doubleVal();
}

static const glm::vec3 toVec3(const json::Json &json, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	if (json.isArray() && json.size() == 3) {
		return glm::vec3(json.get(0).floatVal(), json.get(1).floatVal(), json.get(2).floatVal());
	}
	if (!json.contains("x") || !json.contains("y") || !json.contains("z")) {
		return defaultValue;
	}

	// Handle both string and numeric types in data_points
	// Blockbench can serialize values as strings (e.g., "0", "0\n") or numbers
	auto getFloatValue = [](const json::Json &val, float defaultVal) -> float {
		if (val.isNumber()) {
			return val.floatVal();
		} else if (val.isString()) {
			const core::String str = val.str();
			char *end = nullptr;
			const float result = strtof(str.c_str(), &end);
			if (end != str.c_str() && result != HUGE_VALF && result != -HUGE_VALF) {
				return result;
			}
			Log::debug("Failed to parse float from string: '%s'", str.c_str());
			return defaultVal;
		}
		return defaultVal;
	};

	const float x = getFloatValue(json.get("x"), defaultValue.x);
	const float y = getFloatValue(json.get("y"), defaultValue.y);
	const float z = getFloatValue(json.get("z"), defaultValue.z);
	return glm::vec3(x, y, z);
}

static glm::vec3 toVec3(const json::Json &json, const char *key, const glm::vec3 &defaultValue = glm::vec3(0.0f)) {
	if (!json.contains(key)) {
		return defaultValue;
	}
	return toVec3(json.get(key), defaultValue);
}

static BlockbenchFormat::BBElementType toType(const json::Json &json, const char *key) {
	const core::String &type = json::toStr(json, key);
	if (type == "cube") {
		return BlockbenchFormat::BBElementType::Cube;
	} else if (type == "mesh") {
		return BlockbenchFormat::BBElementType::Mesh;
	} else if (type == "locator") {
		return BlockbenchFormat::BBElementType::Locator;
	} else if (type == "null_object") {
		return BlockbenchFormat::BBElementType::NullObject;
	} else if (type == "camera") {
		return BlockbenchFormat::BBElementType::Camera;
	} else if (type == "billboard") {
		return BlockbenchFormat::BBElementType::Billboard;
	} else if (type == "spline") {
		return BlockbenchFormat::BBElementType::Spline;
	} else if (type == "texture_mesh") {
		return BlockbenchFormat::BBElementType::TextureMesh;
	} else if (type == "bounding_box") {
		return BlockbenchFormat::BBElementType::BoundingBox;
	} else if (type == "armature") {
		return BlockbenchFormat::BBElementType::Armature;
	} else if (type == "armature_bone") {
		return BlockbenchFormat::BBElementType::ArmatureBone;
	}
	Log::debug("Unsupported element type: %s", type.c_str());
	return BlockbenchFormat::BBElementType::Max;
}

static int resolveTextureIndex(const json::Json &faceData, const BlockbenchFormat::BBMeta &bbMeta) {
	if (!faceData.contains("texture")) {
		return -1;
	}
	const json::Json tex = faceData.get("texture");
	if (!tex.isValid() || tex.isNull()) {
		return -1;
	}
	if (tex.isNumber()) {
		return tex.intVal();
	}
	if (tex.isString()) {
		const core::String uuid = tex.str();
		if (uuid.empty() || uuid == "null" || uuid == "false") {
			return -1;
		}
		for (int i = 0; i < (int)bbMeta.textureUUIDs.size(); ++i) {
			if (bbMeta.textureUUIDs[i] == uuid) {
				return i;
			}
		}
		Log::debug("Texture UUID not found in textures[]: %s", uuid.c_str());
		return -1;
	}
	return -1;
}

// Blockbench uses ZYX euler angle order for rotations
static glm::quat eulerZYX(const glm::vec3 &degrees) {
	const glm::vec3 rad = glm::radians(degrees);
	return glm::angleAxis(rad.z, glm::vec3(0, 0, 1))
		 * glm::angleAxis(rad.y, glm::vec3(0, 1, 0))
		 * glm::angleAxis(rad.x, glm::vec3(1, 0, 0));
}

static bool isSupportModelFormat(const core::String &modelFormat) {
	// skin projects often omit elements; everything else is accepted (unknown formats fall back like free)
	return modelFormat != "skin";
}

static bool parseMesh(const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
					  const json::Json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (!elementJson.contains("vertices")) {
		Log::error("Element is missing vertices in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &vertices = elementJson.get("vertices");
	if (!vertices.isObject()) {
		Log::error("Vertices is not an object in json file: %s", filename.c_str());
		return false;
	}

	if (!elementJson.contains("faces")) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &faces = elementJson.get("faces");
	if (!faces.isObject()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (auto faceIt = faces.begin(); faceIt != faces.end(); ++faceIt) {
		const json::Json faceData = *faceIt;
		if (!faceData.contains("uv")) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &uv = faceData.get("uv");
		if (!uv.isObject()) {
			Log::error("UV is not an object in json file: %s", filename.c_str());
			return false;
		}

		if (!faceData.contains("vertices")) {
			Log::error("Face is missing vertices in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &faceVertices = faceData.get("vertices");
		if (!faceVertices.isArray()) {
			Log::error("Vertices is not an array in json file: %s", filename.c_str());
			return false;
		}

		const int materialIdx = resolveTextureIndex(faceData, bbMeta);
		const bool materialIdxValid =
			materialIdx >= 0 && materialIdx < (int)meshMaterialArray.size() && meshMaterialArray[materialIdx];
		Polygon polygon;
		if (materialIdxValid) {
			polygon.setMaterialIndex(meshMaterialArray[materialIdx]);
		}
		for (const auto &vertex : faceVertices) {
			const core::String vertexName = vertex.str();
			if (!vertices.contains(vertexName.c_str())) {
				Log::error("Vertex not found in json file: %s", filename.c_str());
				return false;
			}
			json::Json vertexData = vertices.get(vertexName.c_str());
			if (!vertexData.isArray() || vertexData.size() != 3) {
				Log::error("Vertex is not an array of size 3 in json file: %s", filename.c_str());
				return false;
			}
			if (!uv.contains(vertexName.c_str())) {
				Log::error("UV not found for vertex in json file: %s", filename.c_str());
				return false;
			}
			json::Json uvData = uv.get(vertexName.c_str());
			if (!uvData.isArray() || uvData.size() != 2) {
				Log::error("UV is not an array of size 2 in json file: %s", filename.c_str());
				return false;
			}
			const glm::vec3 pos(vertexData.get(0).floatVal(), vertexData.get(1).floatVal(), vertexData.get(2).floatVal());
			const int x = uvData.get(0).intVal();
			const int y = uvData.get(1).intVal();
			glm::vec2 uvCoords;
			if (materialIdxValid) {
				uvCoords = meshMaterialArray[materialIdx]->texture ? meshMaterialArray[materialIdx]->texture->uv(x, y) : glm::vec2(0.0f);
			} else {
				uvCoords = glm::vec2(0.0f);
			}
			polygon.addVertex(pos, uvCoords);
		}
		polygon.toTris(bbElement.mesh);
	}
	return true;
}

static bool parseCube(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
					  const json::Json &elementJson, const MeshMaterialArray &meshMaterialArray,
					  BlockbenchFormat::BBElement &bbElement) {
	if (!elementJson.contains("from") || !elementJson.contains("to")) {
		Log::error("Element is missing from or to in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &from = elementJson.get("from");
	const json::Json &to = elementJson.get("to");
	if (!from.isArray() || from.size() != 3 || !to.isArray() || to.size() != 3) {
		Log::error("From or to is not an array of size 3 in json file: %s", filename.c_str());
		return false;
	}

	bbElement.cube.from = scale * priv::toVec3(from);
	bbElement.cube.to = scale * priv::toVec3(to);

	// Apply stretch (non-uniform scale around cube center) then inflate
	if (glm::any(glm::epsilonNotEqual(bbElement.stretch, glm::vec3(1.0f), 0.0001f))) {
		const glm::vec3 center = (bbElement.cube.from + bbElement.cube.to) * 0.5f;
		const glm::vec3 half = (bbElement.cube.to - bbElement.cube.from) * 0.5f;
		const glm::vec3 stretchedHalf = half * bbElement.stretch;
		bbElement.cube.from = center - stretchedHalf;
		bbElement.cube.to = center + stretchedHalf;
	}

	// Apply inflate: expand geometry in all directions without changing UV mapping
	if (bbElement.inflate != 0.0f) {
		const glm::vec3 inf(bbElement.inflate);
		bbElement.cube.from -= inf;
		bbElement.cube.to += inf;
	}

	if (!elementJson.contains("faces")) {
		Log::error("Element is missing faces in json file: %s", filename.c_str());
		return false;
	}

	const json::Json &faces = elementJson.get("faces");
	if (!faces.isObject()) {
		Log::error("Faces is not an object in json file: %s", filename.c_str());
		return false;
	}

	for (auto faceIt = faces.begin(); faceIt != faces.end(); ++faceIt) {
		const core::String faceName = faceIt.key();
		voxel::FaceNames faceType = voxel::FaceNames::Max;
		if (faceName == "north") {
			faceType = voxel::FaceNames::NegativeZ;
		} else if (faceName == "east") {
			faceType = voxel::FaceNames::PositiveX;
		} else if (faceName == "south") {
			faceType = voxel::FaceNames::PositiveZ;
		} else if (faceName == "west") {
			faceType = voxel::FaceNames::NegativeX;
		} else if (faceName == "up") {
			faceType = voxel::FaceNames::PositiveY;
		} else if (faceName == "down") {
			faceType = voxel::FaceNames::NegativeY;
		} else {
			Log::error("Unsupported face name: %s", faceName.c_str());
			continue;
		}

		const json::Json faceData = *faceIt;
		if (faceData.contains("enabled") && !faceData.boolVal("enabled", true)) {
			continue;
		}
		if (!faceData.contains("uv")) {
			Log::error("Face is missing uv in json file: %s", filename.c_str());
			return false;
		}

		const json::Json &uv = faceData.get("uv");
		if (!uv.isArray() || uv.size() != 4) {
			Log::error("UV is not an array of size 4 in json file: %s", filename.c_str());
			return false;
		}

		int materialIdx = -1;
		if (!meshMaterialArray.empty()) {
			materialIdx = resolveTextureIndex(faceData, bbMeta);
			if (materialIdx >= (int)meshMaterialArray.size()) {
				Log::error("Invalid material index: %d", materialIdx);
				return false;
			}
		}

		Log::debug("faceName: %s, materialIdx: %d", faceName.c_str(), materialIdx);
		int uvs[4]{uv.get(0).intVal(), uv.get(1).intVal(), uv.get(2).intVal(), uv.get(3).intVal()};
		if (bbElement.mirror_uv) {
			const int tmp0 = uvs[0];
			const int tmp2 = uvs[2];
			uvs[0] = tmp2;
			uvs[2] = tmp0;
		}
		const int uvRotation = priv::toNumber(faceData, "rotation", 0);
		if (materialIdx >= 0 && meshMaterialArray[materialIdx] && meshMaterialArray[materialIdx]->texture) {
			// Use per-texture UV dimensions if available, otherwise fall back to image dimensions
			const int uvW = materialIdx < (int)bbMeta.textureUVDimensions.size() && bbMeta.textureUVDimensions[materialIdx].x > 0
				? bbMeta.textureUVDimensions[materialIdx].x
				: meshMaterialArray[materialIdx]->texture->width();
			const int uvH = materialIdx < (int)bbMeta.textureUVDimensions.size() && bbMeta.textureUVDimensions[materialIdx].y > 0
				? bbMeta.textureUVDimensions[materialIdx].y
				: meshMaterialArray[materialIdx]->texture->height();
			glm::vec2 uv0 = image::Image::uv(uvs[0], uvs[1], uvW, uvH);
			glm::vec2 uv1 = image::Image::uv(uvs[2] - 1, uvs[3] - 1, uvW, uvH);
			// Apply UV rotation (0, 90, 180, 270 degrees) around the UV rect center
			if (uvRotation == 90 || uvRotation == 270) {
				const glm::vec2 center = (uv0 + uv1) * 0.5f;
				const glm::vec2 half = (uv1 - uv0) * 0.5f;
				if (uvRotation == 90) {
					uv0 = center + glm::vec2(-half.y, half.x);
					uv1 = center + glm::vec2(half.y, -half.x);
				} else {
					uv0 = center + glm::vec2(half.y, -half.x);
					uv1 = center + glm::vec2(-half.y, half.x);
				}
			} else if (uvRotation == 180) {
				const glm::vec2 tmp = uv0;
				uv0 = uv1;
				uv1 = tmp;
			}
			bbElement.cube.faces[(int)faceType].uvs[0] = uv0;
			bbElement.cube.faces[(int)faceType].uvs[1] = uv1;
		}
		bbElement.cube.faces[(int)faceType].textureIndex = materialIdx;
		const int tint = faceData.intVal("tint", -1);
		bbElement.cube.faces[(int)faceType].color = faceData.intVal("color", tint);
	}
	return true;
}

static void computeElementsAABB(const json::Json &elementsJson, glm::vec3 &mins, glm::vec3 &maxs) {
	mins = glm::vec3(std::numeric_limits<float>::max());
	maxs = glm::vec3(std::numeric_limits<float>::lowest());
	for (const auto &elementJson : elementsJson) {
		if (elementJson.contains("from") && elementJson.contains("to")) {
			const glm::vec3 from = priv::toVec3(elementJson, "from");
			const glm::vec3 to = priv::toVec3(elementJson, "to");
			mins = glm::min(mins, glm::min(from, to));
			maxs = glm::max(maxs, glm::max(from, to));
		} else if (elementJson.contains("vertices")) {
			const json::Json &verticesJson = elementJson.get("vertices");
			if (verticesJson.isObject()) {
				for (auto vIt = verticesJson.begin(); vIt != verticesJson.end(); ++vIt) {
					json::Json entry = *vIt;
					if (entry.isArray() && entry.size() == 3) {
						const glm::vec3 pos(entry.get(0).floatVal(), entry.get(1).floatVal(), entry.get(2).floatVal());
						mins = glm::min(mins, pos);
						maxs = glm::max(maxs, pos);
					}
				}
			}
		}
	}
}

static bool parseBillboardAsCube(const glm::vec3 &scale, const json::Json &elementJson,
								 BlockbenchFormat::BBElement &bbElement) {
	// Approximate billboard as a thin cube in the XY plane
	glm::vec3 pos = priv::toVec3(elementJson, "position");
	if (elementJson.contains("origin") && !elementJson.contains("position")) {
		pos = priv::toVec3(elementJson, "origin");
	}
	glm::vec2 size(2.0f, 2.0f);
	if (elementJson.contains("size")) {
		const json::Json s = elementJson.get("size");
		if (s.isArray() && s.size() >= 2) {
			size = glm::vec2(s.get(0).floatVal(), s.get(1).floatVal());
		}
	}
	bbElement.origin = scale * pos;
	const glm::vec3 half(size.x * 0.5f, size.y * 0.5f, 0.5f);
	bbElement.cube.from = scale * (pos - half);
	bbElement.cube.to = scale * (pos + half);
	bbElement.type = BlockbenchFormat::BBElementType::Cube;
	return true;
}

static bool parseElements(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
						  const json::Json &elementsJson, const MeshMaterialArray &meshMaterialArray,
						  BlockbenchFormat::BBElementMap &bbElementMap, scenegraph::SceneGraph &sceneGraph) {
	for (const auto &elementJson : elementsJson) {
		BlockbenchFormat::BBElement bbElement;
		bbElement.uuid = core::UUID(json::toStr(elementJson, "uuid"));
		bbElement.name = json::toStr(elementJson, "name");
		bbElement.origin = scale * priv::toVec3(elementJson, "origin");
		bbElement.rotation = priv::toVec3(elementJson, "rotation");
		bbElement.rescale = elementJson.boolVal("rescale", false);
		bbElement.locked = elementJson.boolVal("locked", false);
		bbElement.visible = elementJson.boolVal("visibility", true);
		bbElement.box_uv = elementJson.boolVal("box_uv", false);
		bbElement.mirror_uv = elementJson.boolVal("mirror_uv", false);
		bbElement.inflate = elementJson.floatVal("inflate", 0.0f);
		bbElement.stretch = priv::toVec3(elementJson, "stretch", glm::vec3(1.0f));
		bbElement.color = priv::toNumber(elementJson, "color", 0);
		bbElement.type = priv::toType(elementJson, "type");

		if (bbElement.type == BlockbenchFormat::BBElementType::Cube) {
			if (!parseCube(scale, filename, bbMeta, elementJson, meshMaterialArray, bbElement)) {
				return false;
			}
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Mesh) {
			if (!parseMesh(filename, bbMeta, elementJson, meshMaterialArray, bbElement)) {
				return false;
			}
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Billboard) {
			if (!parseBillboardAsCube(scale, elementJson, bbElement)) {
				Log::warn("Failed to approximate billboard element: %s", bbElement.name.c_str());
				continue;
			}
			// Map faces.front onto all cube faces when present
			if (elementJson.contains("faces")) {
				const json::Json faces = elementJson.get("faces");
				if (faces.isObject() && faces.contains("front")) {
					json::Json front = faces.get("front");
					const int materialIdx = resolveTextureIndex(front, bbMeta);
					for (int fi = 0; fi < (int)voxel::FaceNames::Max; ++fi) {
						bbElement.cube.faces[fi].textureIndex = materialIdx;
						bbElement.cube.faces[fi].color = front.intVal("color", bbElement.color);
					}
				}
			}
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Locator ||
				   bbElement.type == BlockbenchFormat::BBElementType::NullObject ||
				   bbElement.type == BlockbenchFormat::BBElementType::Camera) {
			// Prefer position over origin for these types; legacy from[] also accepted
			if (elementJson.contains("position")) {
				bbElement.origin = scale * priv::toVec3(elementJson, "position");
			} else if (elementJson.contains("from")) {
				bbElement.origin = scale * priv::toVec3(elementJson, "from");
			}
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Spline ||
				   bbElement.type == BlockbenchFormat::BBElementType::TextureMesh ||
				   bbElement.type == BlockbenchFormat::BBElementType::BoundingBox ||
				   bbElement.type == BlockbenchFormat::BBElementType::Armature ||
				   bbElement.type == BlockbenchFormat::BBElementType::ArmatureBone) {
			Log::debug("Skipping unsupported element type for %s", bbElement.name.c_str());
			continue;
		} else if (bbElement.type == BlockbenchFormat::BBElementType::Max) {
			// Unknown type: try cube if from/to exist, otherwise skip
			if (elementJson.contains("from") && elementJson.contains("to")) {
				bbElement.type = BlockbenchFormat::BBElementType::Cube;
				if (!parseCube(scale, filename, bbMeta, elementJson, meshMaterialArray, bbElement)) {
					Log::warn("Skipping unknown element that failed cube parse: %s", bbElement.name.c_str());
					continue;
				}
			} else {
				Log::debug("Skipping unknown element type without cube bounds: %s", bbElement.name.c_str());
				continue;
			}
		}

		// make a copy here, the element is moved into the map
		const core::UUID uuidCopy = bbElement.uuid;
		bbElementMap.emplace(uuidCopy, core::move(bbElement));
	}
	return true;
}

static bool parseOutliner(const glm::vec3 &scale, const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta,
						  const json::Json &entryJson, BlockbenchFormat::BBNode &bbNode) {
	bbNode.name = json::toStr(entryJson, "name");
	bbNode.uuid = core::UUID(json::toStr(entryJson, "uuid"));
	bbNode.locked = entryJson.boolVal("locked", false);
	bbNode.visible = entryJson.boolVal("visibility", true);
	bbNode.mirror_uv = entryJson.boolVal("mirror_uv", false);
	bbNode.origin = scale * priv::toVec3(entryJson, "origin");
	bbNode.rotation = priv::toVec3(entryJson, "rotation");
	bbNode.color = priv::toNumber(entryJson, "color", 0);
	bbNode.size = priv::toVec3(entryJson, "size", glm::vec3(1.0f));

	Log::debug("Node name: %s (%i references)", bbNode.name.c_str(), (int)bbNode.referenced.size());

	if (!entryJson.contains("children")) {
		return true;
	}
	const json::Json &childrenJson = entryJson.get("children");
	if (childrenJson.empty()) {
		return true;
	}
	if (!childrenJson.isArray()) {
		Log::error("Children is not an array in json file: %s", filename.c_str());
		return false;
	}

	for (auto iter = childrenJson.begin(); iter != childrenJson.end(); ++iter) {
		json::Json child = *iter;
		if (child.isString()) {
			core::UUID uuid = core::UUID(json::toStr(child));
			bbNode.referenced.push_back(uuid);
			continue;
		}
		if (!child.isObject()) {
			Log::error("Child entry is not an object in json file: %s", filename.c_str());
			return false;
		}
		BlockbenchFormat::BBNode bbChildNode;
		if (!parseOutliner(scale, filename, bbMeta, child, bbChildNode)) {
			return false;
		}
		bbNode.children.push_back(bbChildNode);
	}
	return true;
}

} // namespace priv

// Merge properties from the flat groups array into the outliner tree nodes by UUID.
// In format 5.0+, the outliner tree may only have uuid/children, while the groups array
// has the full properties (name, origin, rotation, etc.).
static void mergeGroupProperties(BlockbenchFormat::BBNode &node,
								 const core::Map<core::UUID, BlockbenchFormat::BBNode, 64, core::UUIDHash> &groupMap) {
	auto iter = groupMap.find(node.uuid);
	if (iter != groupMap.end()) {
		const BlockbenchFormat::BBNode &g = iter->value;
		if (node.name.empty()) {
			node.name = g.name;
		}
		node.origin = g.origin;
		node.rotation = g.rotation;
		node.locked = g.locked;
		node.visible = g.visible;
		node.mirror_uv = g.mirror_uv;
		node.color = g.color;
	}
	for (BlockbenchFormat::BBNode &child : node.children) {
		mergeGroupProperties(child, groupMap);
	}
}

bool BlockbenchFormat::generateMesh(const BBNode &bbNode, BBElement &bbElement, const MeshMaterialArray &meshMaterialArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	Mesh &mesh = bbElement.mesh;
	mesh.materials = meshMaterialArray;
	const int nodeIdx = voxelizeMesh(bbElement.uuid, bbElement.name, sceneGraph, core::move(mesh), parent);
	if (nodeIdx == InvalidNodeId) {
		return false;
	}
	scenegraph::SceneGraphNode &model = sceneGraph.node(nodeIdx);
	model.setLocked(bbNode.locked);
	model.setVisible(bbNode.visible);
	sceneGraph.updateTransforms();
	model.setRotation(priv::eulerZYX(bbElement.rotation), true);
	model.setTranslation(bbElement.origin - bbNode.origin);
	return true;
}

bool BlockbenchFormat::generateCube(const BBNode &bbNode, const BBElement &bbElement, const MeshMaterialArray &meshMaterialArray,
									scenegraph::SceneGraph &sceneGraph, int parent) const {
	const BBCube &cube = bbElement.cube;

	// In Blockbench, 'from' and 'to' define opposite corners of a cube, but they might not be in min/max order.
	// We normalize them to ensure we have proper bounds. This doesn't affect UV coordinates - those are handled separately per face.
	glm::vec3 mins = glm::min(bbElement.cube.from, bbElement.cube.to);
	glm::vec3 maxs = glm::max(bbElement.cube.from, bbElement.cube.to);

	glm::vec3 size = maxs - mins;
	// even a plane is one voxel for us
	size.x = glm::clamp(size.x, 1.0f, 1.0f + size.x);
	size.y = glm::clamp(size.y, 1.0f, 1.0f + size.y);
	size.z = glm::clamp(size.z, 1.0f, 1.0f + size.z);

	mins = glm::round(mins);
	maxs = mins + size - 1.0f;
	voxel::Region region(mins, maxs);
	if (!region.isValid()) {
		Log::error("Invalid region for element: %s (node: %s): %f:%f:%f/%f:%f:%f", bbElement.name.c_str(), bbNode.name.c_str(), mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		return false;
	}
	scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model, bbElement.uuid);
	region.shift(-region.getLowerCorner());
	model.createVolume(region);
	model.setName(bbElement.name);
	model.setLocked(bbNode.locked);
	model.setVisible(bbNode.visible);

	// Calculate pivot
	const glm::vec3 pivot = (bbElement.origin - bbElement.cube.from) / size;
	model.setPivot(pivot);

	const voxel::FaceNames order[] = {voxel::FaceNames::NegativeX, voxel::FaceNames::PositiveX,
									  voxel::FaceNames::NegativeY, voxel::FaceNames::PositiveY,
									  voxel::FaceNames::NegativeZ, voxel::FaceNames::PositiveZ};
	for (int i = 0; i < lengthof(order); ++i) {
		const BBCubeFace &face = cube.faces[(int)order[i]];
		const voxel::FaceNames faceName = order[i];
		image::ImagePtr image;
		if (face.textureIndex >= 0 && meshMaterialArray[face.textureIndex] &&
			meshMaterialArray[face.textureIndex]->texture) {
			image = meshMaterialArray[face.textureIndex]->texture;
		}
		const int faceColor = face.color >= 0 ? face.color : bbElement.color;
		voxelutil::importFace(*model.volume(), model.region(), model.palette(), faceName, image, face.uvs[0], face.uvs[1],
				      faceColor);
	}
	model.volume()->translate(-region.getLowerCorner());
	// Compute position relative to parent group
	const glm::vec3 localFrom = bbElement.cube.from - bbNode.origin;
	const glm::vec3 regionsize = region.getDimensionsInVoxels();
	const int nodeId = sceneGraph.emplace(core::move(model), parent);
	if (nodeId == InvalidNodeId) {
		return false;
	}
	// Set transform after emplace
	scenegraph::SceneGraphNode &emplaced = sceneGraph.node(nodeId);
	emplaced.setTranslation(localFrom + pivot * regionsize);
	emplaced.setRotation(priv::eulerZYX(bbElement.rotation));
	return true;
}

bool BlockbenchFormat::addNode(const BBNode &bbNode, const BBElementMap &bbElementMap, scenegraph::SceneGraph &sceneGraph,
							   const MeshMaterialArray &meshMaterialArray, int parent) const {
	Log::debug("node: %s with %i children", bbNode.name.c_str(), (int)bbNode.children.size());
	for (const core::UUID &uuid : bbNode.referenced) {
		auto elementIter = bbElementMap.find(uuid);
		if (elementIter == bbElementMap.end()) {
			const core::String uuidStr = uuid.str();
			Log::warn("Could not find node with uuid: %s", uuidStr.c_str());
			continue;
		}
		BBElement &bbElement = elementIter->value;
		if (bbElement.type == BBElementType::Cube) {
			if (!generateCube(bbNode, bbElement, meshMaterialArray, sceneGraph, parent)) {
				return false;
			}
		} else if (bbElement.type == BBElementType::Mesh) {
			if (!generateMesh(bbNode, bbElement, meshMaterialArray, sceneGraph, parent)) {
				return false;
			}
		} else if (bbElement.type == BBElementType::Locator || bbElement.type == BBElementType::NullObject) {
			scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point, bbElement.uuid);
			point.setName(bbElement.name);
			point.setVisible(bbElement.visible);
			point.setLocked(bbElement.locked);
			const int pointId = sceneGraph.emplace(core::move(point), parent);
			if (pointId != InvalidNodeId) {
				scenegraph::SceneGraphNode &n = sceneGraph.node(pointId);
				n.setTranslation(bbElement.origin - bbNode.origin);
				n.setRotation(priv::eulerZYX(bbElement.rotation));
			}
		} else if (bbElement.type == BBElementType::Camera) {
			Log::debug("Skipping camera element: %s", bbElement.name.c_str());
		} else {
			Log::warn("Unsupported element type: %i", (int)bbElement.type);
		}
	}
	for (const BBNode &bbChild : bbNode.children) {
		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group, bbChild.uuid);
		group.setName(bbChild.name);
		group.setVisible(bbChild.visible);
		group.setLocked(bbChild.locked);
		const glm::vec3 localOrigin = bbChild.origin - bbNode.origin;
		int groupParent = sceneGraph.emplace(core::move(group), parent);
		if (groupParent == InvalidNodeId) {
			Log::error("Failed to add node: %s", bbChild.name.c_str());
			return false;
		}
		// Set transform after emplace
		scenegraph::SceneGraphNode &groupNode = sceneGraph.node(groupParent);
		groupNode.setTranslation(localOrigin);
		groupNode.setRotation(priv::eulerZYX(bbChild.rotation));
		groupNode.setScale(bbChild.size);
		if (!addNode(bbChild, bbElementMap, sceneGraph, meshMaterialArray, groupParent)) {
			return false;
		}
	}
	return true;
}

void BlockbenchFormat::fixNode(BBNode &n) const {
	n.rotation.z = -n.rotation.z;
	for (BBNode &c : n.children) {
		fixNode(c);
	}
}

void BlockbenchFormat::processCompatibility(const BBMeta &meta, BBElementMap &elementMap, BBNode &root) const {
	// Compatibility notes based on Blockbench processCompatibility migrations
	if (priv::versionLessThan(meta.version, 3, 2)) {
		// Pre-3.2: Z-axis rotation was inverted for outliner groups only (not elements)
		fixNode(root);
	}
	(void)elementMap;
}

static bool parseAnimations(const core::String &filename, const BlockbenchFormat::BBMeta &bbMeta, json::Json &json,
							scenegraph::SceneGraph &sceneGraph) {
	// no animations found
	if (!json.contains("animations")) {
		return true;
	}

	const json::Json &animationsJson = json.get("animations");
	if (!animationsJson.isArray()) {
		Log::error("Animations is not an array in json file: %s", filename.c_str());
		return false;
	}
	for (const auto &animationJson : animationsJson) {
		const core::String animationName = json::toStr(animationJson, "name");
		if (animationName.empty()) {
			continue;
		}
		sceneGraph.addAnimation(animationName);
		Log::debug("addAnimation(%s) result, setAnimation result: %s", animationName.c_str(),
				   sceneGraph.setAnimation(animationName) ? "true" : "false");
#if BLOCKBENCH_ANIMATION
		priv::Animation animation;
		animation.uuid = core::UUID(json::toStr(animationJson, "uuid"));
		animation.name = animationName;
		animation.loop = json::toStr(animationJson, "loop");
		animation.overrideVal = animationJson.boolVal("override", false);
		animation.selected = animationJson.boolVal("selected", false);
		animation.length = animationJson.floatVal("length", 0.0f);
		animation.snapping = animationJson.intVal("snapping", 0);
		const core::String animTimeUpdate = json::toStr(animationJson, "anim_time_update");
		const core::String blendWeight = json::toStr(animationJson, "blend_weight");
		const core::String startDelay = json::toStr(animationJson, "start_delay");
		const core::String loopDelay = json::toStr(animationJson, "loop_delay");
		if (!animationJson.contains("animators") && !animationJson.contains("bones")) {
			Log::debug("No animators found in json file: %s", filename.c_str());
			continue;
		}
		json::Json animatorsObject = animationJson.contains("animators") ? animationJson.get("animators")
																		: animationJson.get("bones");
		for (auto animIt = animatorsObject.begin(); animIt != animatorsObject.end(); ++animIt) {
			priv::Animator animator;
			animator.uuid = core::UUID(animIt.key());
			const json::Json animatorsJson = *animIt;
			// Legacy: bare keyframe arrays
			json::Json keyframesJson;
			if (animatorsJson.isArray()) {
				animator.name = animIt.key();
				animator.type = "bone";
				keyframesJson = animatorsJson;
			} else {
				animator.name = json::toStr(animatorsJson, "name");
				animator.type = json::toStr(animatorsJson, "type");
				animator.rotationGlobal = animatorsJson.boolVal("rotation_global", false);
				if (!animatorsJson.contains("keyframes")) {
					Log::debug("No keyframes found in json file: %s", filename.c_str());
					continue;
				}
				keyframesJson = animatorsJson.get("keyframes");
			}

			for (const auto &keyframeJson : keyframesJson) {
				priv::KeyFrame kf;
				kf.channel = json::toStr(keyframeJson, "channel");
				kf.interpolation = priv::toInterpolationType(keyframeJson, "interpolation");
				kf.uuid = core::UUID(json::toStr(keyframeJson, "uuid"));
				kf.time = keyframeJson.floatVal("time", 0.0f);
				kf.color = keyframeJson.intVal("color", 0);
				kf.bezierLinked = keyframeJson.boolVal("bezier_linked", false);
				kf.bezierRightValue = priv::toVec3(keyframeJson, "bezier_right_value");
				kf.bezierRightTime = priv::toVec3(keyframeJson, "bezier_right_time");
				kf.bezierLeftValue = priv::toVec3(keyframeJson, "bezier_left_value");
				kf.bezierLeftTime = priv::toVec3(keyframeJson, "bezier_left_time");

				// Parse data_points array
				if (keyframeJson.contains("data_points") && keyframeJson.get("data_points").isArray()) {
					for (const auto &dataPoint : keyframeJson.get("data_points")) {
						kf.dataPoints.push_back(priv::toVec3(dataPoint));
					}
				}
				// Pre-5.0 compatibility: invert X for position/rotation, Y for rotation
				if (priv::versionLessThan(bbMeta.version, 5, 0)) {
					for (glm::vec3 &dp : kf.dataPoints) {
						if (kf.channel == "position" || kf.channel == "rotation") {
							dp.x = -dp.x;
						}
						if (kf.channel == "rotation") {
							dp.y = -dp.y;
						}
					}
					if (kf.interpolation == scenegraph::InterpolationType::CubicBezier) {
						if (kf.channel == "position" || kf.channel == "rotation") {
							kf.bezierLeftValue.x *= -1;
							kf.bezierRightValue.x *= -1;
						}
						if (kf.channel == "rotation") {
							kf.bezierLeftValue.y *= -1;
							kf.bezierRightValue.y *= -1;
						}
					}
				} else {
					// v5+: negate X and Z for rotation and position
					for (glm::vec3 &dp : kf.dataPoints) {
						if (kf.channel == "position" || kf.channel == "rotation") {
							dp.x = -dp.x;
							dp.z = -dp.z;
						}
					}
					if (kf.interpolation == scenegraph::InterpolationType::CubicBezier) {
						if (kf.channel == "position" || kf.channel == "rotation") {
							kf.bezierLeftValue.x *= -1;
							kf.bezierLeftValue.z *= -1;
							kf.bezierRightValue.x *= -1;
							kf.bezierRightValue.z *= -1;
						}
					}
				}
				animator.keyframes.push_back(kf);
			}
			animation.animators.push_back(animator);
		}
		for (const priv::Animator &animator : animation.animators) {
			Log::debug("Animator: %s with %d keyframes", animator.name.c_str(), (int)animator.keyframes.size());
			// Skip non-node animators (e.g., "effects" for sound/particle timelines)
			if (!animator.uuid.isValid()) {
				Log::debug("Skipping non-node animator: %s", animator.name.c_str());
				continue;
			}
			scenegraph::SceneGraphNode *node = sceneGraph.findNodeByUUID(animator.uuid);
			if (!node) {
				const core::String &uuidStr = animator.uuid.str();
				Log::warn("Node not found for uuid: %s", uuidStr.c_str());
				continue;
			}

			const core::String &uuidStr = node->uuid().str();
			Log::debug("Found node: %s (uuid: %s)", node->name().c_str(), uuidStr.c_str());

			// Read base transform from Default animation (rest pose) without switching
			scenegraph::SceneGraphTransform baseTransform;
			{
				const scenegraph::SceneGraphKeyFrames &defaultKfs = node->keyFrames(DEFAULT_ANIMATION);
				if (!defaultKfs.empty()) {
					baseTransform = defaultKfs[0].transform();
				}
			}

			// Sort keyframes by time to ensure correct ordering
			core::DynamicArray<priv::KeyFrame> sortedKeyframes = animator.keyframes;
			core::sort(sortedKeyframes.begin(), sortedKeyframes.end(), [](const priv::KeyFrame &a, const priv::KeyFrame &b) {
				return a.time < b.time;
			});

			for (const priv::KeyFrame &keyframe : sortedKeyframes) {
				if (keyframe.dataPoints.empty()) {
					Log::debug("Keyframe has no data points: channel=%s, time=%f", keyframe.channel.c_str(), keyframe.time);
					continue;
				}

				Log::debug("Keyframe: channel=%s, time=%f, interpolation=%d, dataPoints=%d",
						   keyframe.channel.c_str(), keyframe.time, (int)keyframe.interpolation,
						   (int)keyframe.dataPoints.size());

				// Blockbench uses seconds, convert to frames using the animation's snapping value
				const float fps = animation.snapping > 0 ? (float)animation.snapping : 24.0f;
				const scenegraph::FrameIndex frameIdx = keyframe.time * fps;

				// Get or create keyframe at this frame
				scenegraph::KeyFrameIndex kfIdx;
				if (!node->hasKeyFrameForFrame(frameIdx, &kfIdx)) {
					kfIdx = node->addKeyFrame(frameIdx);
					if (kfIdx == (scenegraph::KeyFrameIndex)-1) {
						Log::warn("Failed to add keyframe at frame %i for node %s", (int)frameIdx, node->name().c_str());
						continue;
					}
				}

				scenegraph::SceneGraphKeyFrame &kf = node->keyFrame(kfIdx);
				kf.interpolation = keyframe.interpolation;

				// Initialize transform with base (rest pose) values, then apply animation channel
				scenegraph::SceneGraphTransform transform = kf.transform();
				// If this is a fresh keyframe, seed it with the base transform so
				// channels not animated by this keyframe keep their rest pose values
				if (glm::all(glm::epsilonEqual(transform.localTranslation(), glm::vec3(0.0f), 0.001f))
					&& glm::all(glm::epsilonEqual(glm::vec3(glm::eulerAngles(transform.localOrientation())), glm::vec3(0.0f), 0.001f))) {
					transform.setLocalTranslation(baseTransform.localTranslation());
					transform.setLocalOrientation(baseTransform.localOrientation());
					transform.setLocalScale(baseTransform.localScale());
				}
				const glm::vec3 &value = keyframe.dataPoints[0];

				// In Blockbench, animation values are additive to the node's base transform

				if (keyframe.channel == "rotation") {
					// Blockbench uses ZYX euler angle order (degrees), additive to base rotation
					const glm::quat animRot = priv::eulerZYX(value);
					const glm::quat baseRot = baseTransform.localOrientation();
					transform.setLocalOrientation(baseRot * animRot);
					Log::debug("  Rotation: %.2f, %.2f, %.2f degrees", value.x, value.y, value.z);
				} else if (keyframe.channel == "position") {
					const glm::vec3 basePos = baseTransform.localTranslation();
					transform.setLocalTranslation(basePos + value);
					Log::debug("  Position: %.2f, %.2f, %.2f", value.x, value.y, value.z);
				} else if (keyframe.channel == "scale") {
					const glm::vec3 baseScale = baseTransform.localScale();
					transform.setLocalScale(baseScale * value);
					Log::debug("  Scale: %.2f, %.2f, %.2f", value.x, value.y, value.z);
				} else {
					Log::warn("Unknown animation channel: %s", keyframe.channel.c_str());
					continue;
				}

				node->setTransform(kfIdx, transform);

				// Handle bezier curves for cubic interpolation
				if (keyframe.interpolation == scenegraph::InterpolationType::CubicBezier) {
					// TODO: VOXELFORMAT: Store bezier control points if the SceneGraphKeyFrame supports it
					// For now, the interpolation type is set but control points are not used
					Log::debug("  Bezier linked=%d, leftTime=(%.2f,%.2f,%.2f), rightTime=(%.2f,%.2f,%.2f)",
							   keyframe.bezierLinked,
							   keyframe.bezierLeftTime.x, keyframe.bezierLeftTime.y, keyframe.bezierLeftTime.z,
							   keyframe.bezierRightTime.x, keyframe.bezierRightTime.y, keyframe.bezierRightTime.z);
				}
			}
		}
#endif
	}
	// Propagate the Default (rest pose) base transform to all other animations.
	// Nodes without explicit animation data get identity transforms when their animation
	// is created by setAnimation(). We need to copy the rest pose so they stay in place.
	for (const auto &entry : sceneGraph.nodes()) {
		scenegraph::SceneGraphNode &n = entry->value;
		if (n.type() == scenegraph::SceneGraphNodeType::Root) {
			continue;
		}
		const scenegraph::SceneGraphKeyFrames &defaultKfs = n.keyFrames(DEFAULT_ANIMATION);
		if (defaultKfs.empty()) {
			continue;
		}
		const scenegraph::SceneGraphTransform &baseTransform = defaultKfs[0].transform();
		for (const core::String &anim : sceneGraph.animations()) {
			if (anim == DEFAULT_ANIMATION) {
				continue;
			}
			if (!n.allKeyFrames().hasKey(anim)) {
				continue;
			}
			scenegraph::SceneGraphKeyFrames &kfs = const_cast<scenegraph::SceneGraphKeyFrames &>(n.keyFrames(anim));
			if (!kfs.empty()) {
				scenegraph::SceneGraphTransform &t = kfs[0].transform();
				// Only set base if the first keyframe is at frame 0 and has identity translation
				if (kfs[0].frameIdx == 0 && glm::all(glm::epsilonEqual(t.localTranslation(), glm::vec3(0.0f), 0.001f))
					&& glm::all(glm::epsilonEqual(glm::vec3(glm::eulerAngles(t.localOrientation())), glm::vec3(0.0f), 0.001f))) {
					t.setLocalTranslation(baseTransform.localTranslation());
					t.setLocalOrientation(baseTransform.localOrientation());
					t.setLocalScale(baseTransform.localScale());
				}
			}
		}
	}

	// Keep the Default animation - it contains the rest pose (group origins, element positions)
	return true;
}

bool BlockbenchFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	core::String jsonString;
	stream->readString(stream->remaining(), jsonString);
	json::Json json = priv::parseBBModelJson(jsonString);
	if (!json.isValid()) {
		return false;
	}

	if (!json.contains("meta")) {
		Log::error("No meta found in json file: %s", filename.c_str());
		return false;
	}
	const json::Json &metaJson = json.get("meta");

	BBMeta bbMeta;
	bbMeta.formatVersion = json::toStr(metaJson, "format_version");
	if (bbMeta.formatVersion.empty()) {
		bbMeta.formatVersion = json::toStr(metaJson, "format"); // deprecated alias
	}
	if (bbMeta.formatVersion.empty()) {
		Log::warn("No format_version found in json file: %s - assuming 4.10", filename.c_str());
		bbMeta.formatVersion = "4.10";
	}
	bbMeta.version = util::parseVersion(bbMeta.formatVersion);
	bbMeta.modelFormat = json::toStr(metaJson, "model_format");
	if (bbMeta.modelFormat.empty()) {
		bbMeta.modelFormat = json::toStr(metaJson, "type"); // very old
	}
	bbMeta.creationTimestamp = priv::toNumber(metaJson, "creation_time", (uint64_t)0);
	bbMeta.box_uv = metaJson.boolVal("box_uv", false);
	bbMeta.backup = metaJson.boolVal("backup", false);

	priv::applyFormatMigrations(json, bbMeta);

	if (!priv::isSupportModelFormat(bbMeta.modelFormat)) {
		Log::error("Unsupported model format: %s", bbMeta.modelFormat.c_str());
		return false;
	}
	bbMeta.name = json::toStr(json, "name", core::string::extractFilename(filename));
	bbMeta.model_identifier = json::toStr(json, "model_identifier");
	if (bbMeta.model_identifier.empty()) {
		bbMeta.model_identifier = json::toStr(json, "geometry_name");
	}

	if (json.contains("resolution")) {
		const json::Json resolutionJson = json.get("resolution");
		if (resolutionJson.isObject()) {
			bbMeta.resolution.x = priv::toNumber(resolutionJson, "width", 16);
			bbMeta.resolution.y = priv::toNumber(resolutionJson, "height", 16);
		}
	} else {
		bbMeta.resolution = glm::ivec2(16, 16);
	}

	json::Json textures = json.contains("textures") ? json.get("textures") : json::Json::array();
	if (!textures.isArray()) {
		Log::error("Textures is not an array in json file: %s", filename.c_str());
		return false;
	}

	MeshMaterialArray meshMaterialArray;
	meshMaterialArray.reserve(textures.size());
	bbMeta.textureUUIDs.reserve(textures.size());
	bbMeta.textureUVDimensions.reserve(textures.size());

	const core::String projectDir = core::string::extractDir(filename);

	for (const auto &texture : textures) {
		const core::String &name = json::toStr(texture, "name");
		const core::String &source = json::toStr(texture, "source");
		const core::String &path = json::toStr(texture, "path");
		core::String relativePath = json::toStr(texture, "relative_path");
		const core::String texUuid = json::toStr(texture, "uuid");
		bbMeta.textureUUIDs.push_back(texUuid);

		// Pre-4.10 relative_path normalization: ensure path is relative to project dir
		if (priv::versionLessThan(bbMeta.version, 4, 10) && !relativePath.empty() && relativePath[0] != '/') {
			relativePath = core::String("/") + relativePath;
		}

		image::ImagePtr image;

		// Spec load priority: relative_path -> absolute path (unless backup) -> data URL source
		if (!relativePath.empty()) {
			core::String rel = relativePath;
			if (rel.first() == '/') {
				rel = rel.substr(1);
			}
			const core::String fullPath = core::string::path(projectDir, rel);
			Log::debug("Loading texture from relative path: %s", fullPath.c_str());
			core::ScopedPtr<io::SeekableReadStream> relPathStream(archive->readStream(fullPath));
			if (relPathStream) {
				image = image::loadImage(fullPath, *relPathStream);
				if (!image->isLoaded()) {
					Log::warn("Failed to load texture from relative path: %s", relativePath.c_str());
					image = image::ImagePtr{};
				}
			}
		}
		if ((!image || !image->isLoaded()) && !path.empty() && !bbMeta.backup) {
			Log::debug("Loading texture from path: %s", path.c_str());
			core::ScopedPtr<io::SeekableReadStream> pathStream(archive->readStream(path));
			if (pathStream) {
				image = image::loadImage(path, *pathStream);
				if (!image->isLoaded()) {
					Log::warn("Failed to load texture from path: %s", path.c_str());
					image = image::ImagePtr{};
				}
			} else {
				Log::warn("Could not open stream for texture path: %s", path.c_str());
			}
		}
		if ((!image || !image->isLoaded()) && core::string::startsWith(source, "data:")) {
			const size_t mimetypeEndPos = source.find(";");
			if (mimetypeEndPos == core::String::npos) {
				Log::warn("No mimetype found in source for texture: %s", name.c_str());
			} else {
				const core::String &mimetype = source.substr(5, mimetypeEndPos - 5);
				if (mimetype != "image/png" && mimetype != "image/jpeg" && mimetype != "image/webp") {
					Log::warn("Unsupported mimetype: %s for texture: %s", mimetype.c_str(), name.c_str());
				} else {
					const size_t encodingEnd = source.find(",");
					if (encodingEnd == core::String::npos) {
						Log::warn("No encoding found in source for texture: %s", name.c_str());
					} else {
						const core::String &encoding = source.substr(mimetypeEndPos + 1, encodingEnd - mimetypeEndPos - 1);
						if (encoding != "base64") {
							Log::warn("Unsupported encoding: %s for texture: %s", encoding.c_str(), name.c_str());
						} else {
							const core::String &data = source.substr(encodingEnd + 1);
							if (data.size() < 16) {
								Log::warn("Base64 data too short for texture: %s (%d bytes)", name.c_str(), (int)data.size());
							} else {
								Log::debug("Loading texture: %s with size: %d", name.c_str(), (int)data.size());
								io::MemoryReadStream dataStream(data.c_str(), data.size());
								io::Base64ReadStream base64Stream(dataStream);
								io::BufferedReadWriteStream bufferedStream(base64Stream, data.size());
								image = image::loadImage(name, bufferedStream);
								if (!image->isLoaded()) {
									Log::warn("Failed to load texture from base64: %s", name.c_str());
								}
							}
						}
					}
				}
			}
		}

		// Always add material to array (even if null) to preserve indices
		if (image && image->isLoaded()) {
			meshMaterialArray.push_back(createMaterial(image));
		} else {
			meshMaterialArray.push_back(MeshMaterialPtr{});
			Log::debug("Added null material at index %d for texture: %s", (int)meshMaterialArray.size() - 1, name.c_str());
		}
		const int uvWidth = priv::toNumber(texture, "uv_width", image && image->isLoaded() ? image->width() : bbMeta.resolution.x);
		const int uvHeight = priv::toNumber(texture, "uv_height", image && image->isLoaded() ? image->height() : bbMeta.resolution.y);
		bbMeta.textureUVDimensions.push_back(glm::ivec2(uvWidth, uvHeight));
	}
	json::Json elementsJson = json.contains("elements") ? json.get("elements") : json::Json::array();
	if (!elementsJson.isArray()) {
		Log::error("Elements is not an array in json file: %s", filename.c_str());
		return false;
	}

	glm::vec3 elementsMins, elementsMaxs;
	priv::computeElementsAABB(elementsJson, elementsMins, elementsMaxs);
	const glm::vec3 scale = getInputScale(elementsMins, elementsMaxs);
	BBElementMap bbElementMap;
	if (!priv::parseElements(scale, filename, bbMeta, elementsJson, meshMaterialArray, bbElementMap, sceneGraph)) {
		Log::error("Failed to parse elements");
		return false;
	}

	json::Json outlinerJson = json.contains("outliner") ? json.get("outliner") : json::Json::array();
	if (!outlinerJson.isArray()) {
		Log::error("Outliner is not an array in json file: %s", filename.c_str());
		return false;
	}

	// Ensure the Default animation exists and is active before creating nodes
	// (emplace calls setAnimation which creates keyframes under the active animation name)
	sceneGraph.addAnimation(DEFAULT_ANIMATION);
	sceneGraph.setAnimation(DEFAULT_ANIMATION);

	BBNode bbRoot;
	for (const auto &entry : outlinerJson) {
		if (entry.isObject()) {
			// Parse group as a child node
			BBNode bbChildNode;
			if (!priv::parseOutliner(scale, filename, bbMeta, entry, bbChildNode)) {
				Log::error("Failed to parse outliner");
				return false;
			}
			bbRoot.children.push_back(bbChildNode);
		} else if (entry.isString()) {
			// Direct element reference at root level
			core::String uuid = json::toStr(entry);
			bbRoot.referenced.push_back(core::UUID(uuid));
		}
	}

	// Apply group properties from the flat groups array (format 5.0+ stores full
	// properties there, while the outliner tree may only have uuid/children)
	if (json.contains("groups")) {
		const json::Json &groupsJson = json.get("groups");
		Log::debug("Found groups array with %d entries", (int)groupsJson.size());
		if (groupsJson.isArray()) {
			core::Map<core::UUID, BBNode, 64, core::UUIDHash> groupMap;
			for (const auto &g : groupsJson) {
				BBNode gn;
				gn.uuid = core::UUID(json::toStr(g, "uuid"));
				if (!gn.uuid.isValid()) {
					continue;
				}
				gn.name = json::toStr(g, "name");
				gn.origin = scale * priv::toVec3(g, "origin");
				gn.rotation = priv::toVec3(g, "rotation");
				gn.locked = g.boolVal("locked", false);
				gn.visible = g.boolVal("visibility", true);
				gn.mirror_uv = g.boolVal("mirror_uv", false);
				gn.color = priv::toNumber(g, "color", 0);
				const core::UUID uuidCopy = gn.uuid;
				groupMap.emplace(uuidCopy, core::move(gn));
			}
			for (BBNode &child : bbRoot.children) {
				mergeGroupProperties(child, groupMap);
			}
		}
	}

	// Apply compatibility fixes for older Blockbench versions before creating scene nodes
	processCompatibility(bbMeta, bbElementMap, bbRoot);

	if (!addNode(bbRoot, bbElementMap, sceneGraph, meshMaterialArray, 0)) {
		Log::error("Failed to add node");
		return false;
	}

	if (!parseAnimations(filename, bbMeta, json, sceneGraph)) {
		Log::error("Failed to parse animations");
		// don't abort because we can still load the model without animations
	}

	scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	rootNode.setProperty(scenegraph::PropVersion, bbMeta.formatVersion);
	rootNode.setProperty(scenegraph::PropTitle, bbMeta.name);
	rootNode.setProperty("model_format", bbMeta.modelFormat);
	rootNode.setProperty("model_identifier", bbMeta.model_identifier);

	return true;
}

bool BlockbenchFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	json::Json root = json::Json::object();

	// Meta - target current Blockbench 5.0 project shape
	json::Json meta = json::Json::object();
	meta.set("format_version", "5.0");
	meta.set("model_format", "free");
	meta.set("box_uv", false);

	const scenegraph::SceneGraphNode &rootNode = sceneGraph.node(sceneGraph.root().id());
	const core::String &modelFormat = rootNode.property(core::String("model_format"));
	if (!modelFormat.empty()) {
		meta.set("model_format", modelFormat);
	}
	root.set("meta", meta);

	const core::String &title = rootNode.property(scenegraph::PropTitle);
	root.set("name", title.empty() ? core::string::extractFilename(filename) : title);
	const core::String &modelId = rootNode.property(core::String("model_identifier"));
	root.set("model_identifier", modelId);

	json::Json resolution = json::Json::object();
	resolution.set("width", 16);
	resolution.set("height", 16);
	root.set("resolution", resolution);

	json::Json elements = json::Json::array();
	json::Json textures = json::Json::array();
	json::Json groups = json::Json::array();
	json::Json outliner = json::Json::array();

	auto vec3Array = [](const glm::vec3 &v) {
		json::Json arr = json::Json::array();
		arr.push((double)v.x);
		arr.push((double)v.y);
		arr.push((double)v.z);
		return arr;
	};

	const core::String paletteUuid = core::UUID::generate().str();

	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		const voxel::Region &region = node.region();
		const glm::vec3 dims = region.getDimensionsInVoxels();
		const scenegraph::SceneGraphTransform &t = node.transform(0);
		const glm::vec3 worldPos = t.worldTranslation();
		const glm::vec3 pivot = node.pivot();
		const glm::vec3 from = worldPos - pivot * dims;
		const glm::vec3 to = from + dims;
		const glm::vec3 origin = worldPos;

		json::Json element = json::Json::object();
		element.set("name", node.name());
		element.set("uuid", node.uuid().str());
		element.set("type", "cube");
		element.set("from", vec3Array(from));
		element.set("to", vec3Array(to));
		element.set("origin", vec3Array(origin));
		element.set("visibility", node.visible());
		element.set("locked", node.locked());

		const glm::vec3 euler = glm::degrees(glm::eulerAngles(t.localOrientation()));
		if (glm::any(glm::epsilonNotEqual(euler, glm::vec3(0.0f), 0.001f))) {
			element.set("rotation", vec3Array(euler));
		}

		json::Json faces = json::Json::object();
		const char *faceNames[] = {"north", "east", "south", "west", "up", "down"};
		for (int i = 0; i < 6; ++i) {
			json::Json face = json::Json::object();
			json::Json uv = json::Json::array();
			const voxel::RawVolume *vol = node.volume();
			int colorIdx = 0;
			if (vol) {
				const voxel::Voxel &v = vol->voxel(region.getLowerCorner());
				if (voxel::isBlocked(v.getMaterial())) {
					colorIdx = v.getColor();
				}
			}
			const int px = colorIdx % 16;
			const int py = colorIdx / 16;
			uv.push(px);
			uv.push(py);
			uv.push(px + 1);
			uv.push(py + 1);
			face.set("uv", uv);
			face.set("texture", 0);
			faces.set(faceNames[i], face);
		}
		element.set("faces", faces);
		elements.push(element);
	}

	// Point nodes (locators)
	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (node.type() != scenegraph::SceneGraphNodeType::Point) {
			continue;
		}
		json::Json element = json::Json::object();
		element.set("name", node.name());
		element.set("uuid", node.uuid().str());
		element.set("type", "locator");
		element.set("position", vec3Array(node.transform(0).worldTranslation()));
		element.set("visibility", node.visible());
		element.set("locked", node.locked());
		elements.push(element);
	}
	root.set("elements", elements);

	{
		const palette::Palette *pal = nullptr;
		for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
			pal = &(*iter).palette();
			break;
		}
		if (pal) {
			image::ImagePtr palImage = image::createEmptyImage("palette");
			const int palW = 16;
			const int palH = 16;
			core::DynamicArray<uint8_t> pixels(palW * palH * 4);
			for (int i = 0; i < palW * palH && i < pal->colorCount(); ++i) {
				const color::RGBA c = pal->color(i);
				pixels[i * 4 + 0] = c.r;
				pixels[i * 4 + 1] = c.g;
				pixels[i * 4 + 2] = c.b;
				pixels[i * 4 + 3] = c.a;
			}
			palImage->loadRGBA(pixels.data(), palW, palH);

			io::BufferedReadWriteStream pngStream;
			if (palImage->writePNG(pngStream)) {
				pngStream.seek(0);
				io::BufferedReadWriteStream base64Stream;
				io::Base64WriteStream b64Writer(base64Stream);
				uint8_t buf[4096];
				while (!pngStream.eos()) {
					const int read = pngStream.read(buf, sizeof(buf));
					if (read <= 0) {
						break;
					}
					b64Writer.write(buf, read);
				}
				b64Writer.flush();
				base64Stream.seek(0);
				core::String base64Data;
				base64Stream.readString(base64Stream.size(), base64Data);

				json::Json tex = json::Json::object();
				tex.set("uuid", paletteUuid);
				tex.set("name", "palette.png");
				tex.set("source", core::String("data:image/png;base64,") + base64Data);
				tex.set("internal", true);
				tex.set("width", palW);
				tex.set("height", palH);
				tex.set("uv_width", palW);
				tex.set("uv_height", palH);
				textures.push(tex);
			}
		}
	}
	root.set("textures", textures);

	// Format 5.0: flat groups[] + slim outliner
	struct HierarchyBuilder {
		const scenegraph::SceneGraph &sg;
		json::Json &groupsOut;
		decltype(vec3Array) &toVec3;

		void addGroupData(const scenegraph::SceneGraphNode &node) {
			json::Json group = json::Json::object();
			group.set("uuid", node.uuid().str());
			group.set("name", node.name());
			group.set("origin", toVec3(node.transform(0).worldTranslation()));
			group.set("visibility", node.visible());
			group.set("locked", node.locked());
			group.set("isOpen", true);
			const glm::vec3 euler = glm::degrees(glm::eulerAngles(node.transform(0).localOrientation()));
			if (glm::any(glm::epsilonNotEqual(euler, glm::vec3(0.0f), 0.001f))) {
				group.set("rotation", toVec3(euler));
			}
			groupsOut.push(group);
		}

		json::Json buildOutlinerNode(const scenegraph::SceneGraphNode &node) {
			addGroupData(node);
			json::Json entry = json::Json::object();
			entry.set("uuid", node.uuid().str());
			entry.set("isOpen", true);
			json::Json children = json::Json::array();
			for (const core::UUID &childUUID : node.children()) {
				const scenegraph::SceneGraphNode *child = sg.findNodeByUUID(childUUID);
				if (child == nullptr) {
					continue;
				}
				if ((child->isAnyModelNode() || child->type() == scenegraph::SceneGraphNodeType::Point) &&
					child->children().empty()) {
					children.push(child->uuid().str());
				} else if (child->isAnyModelNode() || child->type() == scenegraph::SceneGraphNodeType::Group ||
						   child->type() == scenegraph::SceneGraphNodeType::Point) {
					children.push(buildOutlinerNode(*child));
				}
			}
			entry.set("children", children);
			return entry;
		}
	};

	HierarchyBuilder builder{sceneGraph, groups, vec3Array};
	const scenegraph::SceneGraphNode &rootSGNode = sceneGraph.node(sceneGraph.root().id());
	for (const core::UUID &childUUID : rootSGNode.children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		if (child == nullptr) {
			continue;
		}
		if ((child->isAnyModelNode() || child->type() == scenegraph::SceneGraphNodeType::Point) &&
			child->children().empty()) {
			outliner.push(child->uuid().str());
		} else if (child->isAnyModelNode() || child->type() == scenegraph::SceneGraphNodeType::Group ||
				   child->type() == scenegraph::SceneGraphNodeType::Point) {
			outliner.push(builder.buildOutlinerNode(*child));
		}
	}
	root.set("groups", groups);
	root.set("outliner", outliner);

	json::Json animations = json::Json::array();
	for (const core::String &animName : sceneGraph.animations()) {
		if (animName == DEFAULT_ANIMATION) {
			continue;
		}

		json::Json anim = json::Json::object();
		anim.set("uuid", core::UUID::generate().str());
		anim.set("name", animName);
		anim.set("loop", "loop");
		anim.set("override", false);
		anim.set("snapping", 24);

		float maxTime = 0.0f;
		const float fps = 24.0f;
		json::Json animators = json::Json::object();

		for (const auto &entry : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = entry->second;
			if (node.type() == scenegraph::SceneGraphNodeType::Root) {
				continue;
			}

			const scenegraph::SceneGraphKeyFrames &kfs = node.keyFrames(animName);
			const scenegraph::SceneGraphKeyFrames &defaultKfs = node.keyFrames(DEFAULT_ANIMATION);
			if (kfs.size() <= 1) {
				continue;
			}

			scenegraph::SceneGraphTransform baseTransform;
			if (!defaultKfs.empty()) {
				baseTransform = defaultKfs[0].transform();
			}

			json::Json animator = json::Json::object();
			animator.set("name", node.name());
			animator.set("type", "bone");

			json::Json keyframes = json::Json::array();
			for (const scenegraph::SceneGraphKeyFrame &kf : kfs) {
				const float time = (float)kf.frameIdx / fps;
				maxTime = glm::max(maxTime, time);
				const scenegraph::SceneGraphTransform &t = kf.transform();

				const glm::quat baseRot = baseTransform.localOrientation();
				const glm::quat currentRot = t.localOrientation();
				const glm::quat deltaRot = glm::conjugate(baseRot) * currentRot;
				const float sinX = 2.0f * (deltaRot.w * deltaRot.x - deltaRot.y * deltaRot.z);
				const float x = glm::abs(sinX) >= 1.0f ? glm::sign(sinX) * glm::half_pi<float>() : glm::asin(sinX);
				const float sinYcosX = 2.0f * (deltaRot.w * deltaRot.y + deltaRot.x * deltaRot.z);
				const float cosYcosX = 1.0f - 2.0f * (deltaRot.x * deltaRot.x + deltaRot.y * deltaRot.y);
				const float y = glm::atan(sinYcosX, cosYcosX);
				const float sinZcosX = 2.0f * (deltaRot.w * deltaRot.z + deltaRot.x * deltaRot.y);
				const float cosZcosX = 1.0f - 2.0f * (deltaRot.x * deltaRot.x + deltaRot.z * deltaRot.z);
				const float z = glm::atan(sinZcosX, cosZcosX);
				const glm::vec3 deltaEuler = glm::degrees(glm::vec3(x, y, z));
				const glm::vec3 deltaPos = t.localTranslation() - baseTransform.localTranslation();
				const glm::vec3 baseScale = baseTransform.localScale();
				const glm::vec3 curScale = t.localScale();
				const glm::vec3 scaleVal(baseScale.x != 0.0f ? curScale.x / baseScale.x : curScale.x,
										baseScale.y != 0.0f ? curScale.y / baseScale.y : curScale.y,
										baseScale.z != 0.0f ? curScale.z / baseScale.z : curScale.z);

				// Inverse of v5+ load conversion: negate X and Z for position/rotation
				auto pushChannel = [&](const char *channel, const glm::vec3 &value, bool invertXZ) {
					json::Json channelKf = json::Json::object();
					channelKf.set("channel", channel);
					channelKf.set("time", (double)time);
					channelKf.set("interpolation", "linear");
					channelKf.set("uuid", core::UUID::generate().str());
					json::Json dp = json::Json::object();
					dp.set("x", invertXZ ? -(double)value.x : (double)value.x);
					dp.set("y", (double)value.y);
					dp.set("z", invertXZ ? -(double)value.z : (double)value.z);
					json::Json dps = json::Json::array();
					dps.push(dp);
					channelKf.set("data_points", dps);
					keyframes.push(channelKf);
				};

				if (glm::any(glm::epsilonNotEqual(deltaEuler, glm::vec3(0.0f), 0.1f))) {
					pushChannel("rotation", deltaEuler, true);
				}
				if (glm::any(glm::epsilonNotEqual(deltaPos, glm::vec3(0.0f), 0.01f))) {
					pushChannel("position", deltaPos, true);
				}
				if (glm::any(glm::epsilonNotEqual(scaleVal, glm::vec3(1.0f), 0.01f))) {
					pushChannel("scale", scaleVal, false);
				}
			}

			if (!keyframes.size()) {
				continue;
			}
			animator.set("keyframes", keyframes);
			animators.set(node.uuid().str().c_str(), animator);
		}

		anim.set("length", (double)maxTime);
		anim.set("animators", animators);
		animations.push(anim);
	}
	root.set("animations", animations);

	const core::String jsonStr = root.dump(1);
	if (stream->write(jsonStr.c_str(), jsonStr.size()) != (int)jsonStr.size()) {
		Log::error("Failed to write bbmodel file: %s", filename.c_str());
		return false;
	}

	return true;
}

} // namespace voxelformat
