/**
 * @file
 */

#include "MapFormat.h"
#include "core/ArrayLength.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/Polygon.h"
#include "voxelformat/private/mesh/TextureLookup.h"

namespace voxelformat {

static glm::vec3 parsePlane(core::Tokenizer &tok) {
	if (!tok.hasNext()) {
		Log::error("Invalid plane line - expected ( - but got nothing");
		return {};
	}
	const core::String &begin = tok.next();
	if (begin != "(") {
		Log::error("Invalid plane line - expected ( - got %s", begin.c_str());
		return {};
	}
	glm::vec3 v{0};
	int component = 0;
	while (tok.hasNext()) {
		const core::String &t = tok.next();
		if (t == ")") {
			Log::trace("plane(%f:%f:%f)", v.x, v.y, v.z);
			return v;
		}
		if (component > 2) {
			Log::error("Invalid plane line - components exceeded");
			return {};
		}
		v[component++] = t.toFloat();
	}
	Log::error("Invalid plane line - expected )");
	return {};
}

static bool parseFloat(core::Tokenizer &tok, float &val) {
	val = 0.0f;
	if (!tok.hasNext()) {
		Log::error("End of buffer in parseFloat");
		return false;
	}
	if (tok.peekNext() == "\n") {
		Log::error("Found end of line in parseFloat");
		return false;
	}
	const core::String &t = tok.next();
	val = t.toFloat();
	return true;
}

static bool parseInt(core::Tokenizer &tok, int &val) {
	val = 0;
	if (!tok.hasNext()) {
		Log::error("End of buffer in parseInt");
		return false;
	}
	if (tok.peekNext() == "\n") {
		Log::error("Found end of line in parseInt");
		return false;
	}
	const core::String &t = tok.next();
	val = t.toInt();
	return true;
}

static bool skipFace(const core::String &texture) {
	const char *skipTextures[] = {"NULL",	   "noshader", "nodraw", "clip",	"lightclip",
								  "actorclip", "hint",	   "skip",	 "trigger", "origin"};
	for (int i = 0; i < lengthof(skipTextures); ++i) {
		if (core::string::extractFilename(texture) == skipTextures[i]) {
			Log::debug("Skipping face with texture %s", texture.c_str());
			return true;
		}
	}
	return false;
}

bool MapFormat::parseBrush(const core::String &filename, const io::ArchivePtr &archive, core::Tokenizer &tok,
						   MeshMaterialMap &materials, MeshFormat::MeshTriCollection &tris,
						   const glm::vec3 &scale) const {
	while (tok.hasNext()) {
		core::String t = tok.next();
		if (t == "}") {
			break;
		}
		if (t == "\n") {
			continue;
		}
		if (t == "patchDef2") {
			Log::error("Quake3 is not yet supported");
			return false;
		}
		if (t != "(") {
			Log::error("Invalid brush line - expected ( - got %s", t.c_str());
			return false;
		}
		tok.prev();

		glm::vec3 planePoints[3];
		planePoints[0] = parsePlane(tok);
		planePoints[1] = parsePlane(tok);
		planePoints[2] = parsePlane(tok);
		core::String texture;
		if (tok.hasNext()) {
			texture = tok.next();
		}

		if (tok.peekNext() == "[") {
			Log::error("Valve format is not yet supported");
			return false;
		}

		glm::vec2 offset;
		if (!parseFloat(tok, offset.x)) {
			Log::error("Failed to parse xoffset");
			return false;
		}
		if (!parseFloat(tok, offset.y)) {
			Log::error("Failed to parse yoffset");
			return false;
		}
		Log::trace("offset: %f:%f", offset.x, offset.y);

		float rotation;
		if (!parseFloat(tok, rotation)) {
			Log::error("Failed to parse rotation");
			return false;
		}
		Log::trace("rotation: %f", rotation);

		glm::vec2 texscale;
		if (!parseFloat(tok, texscale.x)) {
			Log::error("Failed to parse xscale");
			return false;
		}
		if (!parseFloat(tok, texscale.y)) {
			Log::error("Failed to parse yscale");
			return false;
		}
		Log::trace("texscale: %f:%f", texscale.x, texscale.y);

		int contentFlags;
		parseInt(tok, contentFlags);
		Log::trace("Contentflags: %i", contentFlags);
		int surfaceFlags;
		parseInt(tok, surfaceFlags);
		Log::trace("SurfaceFlags: %i", surfaceFlags);
		int value;
		parseInt(tok, value);
		Log::trace("Value: %i", value);

		if (!tok.hasNext()) {
			Log::error("Invalid plane line end detected");
			return false;
		}

		if (skipFace(texture)) {
			continue;
		}

		// Generate vertices
		const glm::vec3 &edge1 = planePoints[1] - planePoints[0];
		const glm::vec3 &edge2 = planePoints[2] - planePoints[0];
		const glm::vec3 &normal = glm::normalize(glm::cross(edge1, edge2));

		// Generate a basis for the plane (u, v)
		const glm::vec3 &u = glm::normalize(edge1);
		const glm::vec3 &v = glm::normalize(glm::cross(normal, u));

		Polygon polygon;
		// Create vertices with UV mapping
		for (const glm::vec3 &point : planePoints) {
			const glm::vec3 localPos = point - planePoints[0]; // Translate to local space
			glm::vec2 uv;
			uv.x = glm::dot(localPos, u) / texscale.x + offset.x;
			uv.y = glm::dot(localPos, v) / texscale.y + offset.y;

			// Apply rotation to UV coordinates
			const float cosTheta = glm::cos(glm::radians(rotation));
			const float sinTheta = glm::sin(glm::radians(rotation));
			const float uRotated = uv.x * cosTheta - uv.y * sinTheta;
			const float vRotated = uv.x * sinTheta + uv.y * cosTheta;

			// Store the vertex
			const glm::vec3 converted(point.x * scale.x, point.z * scale.y, point.y * scale.z);
			polygon.addVertex(converted, glm::vec2(uRotated, vRotated));
		}
		auto iter = materials.find(texture);
		MeshMaterialPtr material;
		if (iter == materials.end()) {
			const core::String &imageName = lookupTexture(filename, texture, archive);
			const image::ImagePtr &image = image::loadImage(imageName);
			material = createMaterial(image);
			materials.put(texture, material);
		} else {
			material = iter->value;
		}
		MeshFormat::MeshTriCollection localtris;
		polygon.toTris(localtris);
		for (auto &tri : localtris) {
			tri.material = material;
		}
		tris.append(localtris);
	}
	return true;
}

bool MapFormat::parseEntity(const core::String &filename, const io::ArchivePtr &archive, core::Tokenizer &tok,
							MeshMaterialMap &materials, MeshFormat::MeshTriCollection &tris,
							core::StringMap<core::String> &props, const glm::vec3 &scale) const {
	while (tok.hasNext()) {
		const core::String &t = tok.next();
		if (t == "}") {
			return true;
		}
		if (t == "\n") {
			continue;
		}
		if (t == "{") {
			Log::debug("Found brush");
			if (!parseBrush(filename, archive, tok, materials, tris, scale)) {
				Log::error("Failed to parse brush");
				return false;
			}
		} else {
			const core::String key = t;
			if (!tok.hasNext()) {
				Log::error("Missing value for key %s", key.c_str());
				return false;
			}
			const core::String value = tok.next();
			props.put(key, value);
			Log::debug("Key: %s, Value: %s", key.c_str(), value.c_str());
		}
	}
	return true;
}

bool MapFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	core::String map;
	if (!stream->readString(stream->size(), map, false)) {
		Log::error("Failed to read file %s", filename.c_str());
		return false;
	}

	const glm::vec3 &scale = getInputScale();

	core::TokenizerConfig cfg;
	cfg.skipComments = true;
	Log::debug("Tokenizing");
	core::Tokenizer tok(cfg, map, " \t\r");
	MeshMaterialMap materials;
	int entity = 0;
	while (tok.hasNext()) {
		const core::String &t = tok.next();
		if (t == "{") {
			MeshFormat::MeshTriCollection tris;
			core::StringMap<core::String> props;
			if (!parseEntity(filename, archive, tok, materials, tris, props, scale)) {
				Log::error("Failed to parse entity");
				return false;
			}
			if (tris.empty()) {
				continue;
			}
			core::String classname;
			props.get("classname", classname);
			const core::String name = core::string::format("%s brush %i", classname.c_str(), entity);
			const int nodeId = voxelizeNode(name, sceneGraph, tris);
			if (nodeId == InvalidNodeId) {
				Log::error("Voxelization failed");
				return false;
			}
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			for (const auto &entry : props) {
				node.setProperty(entry->key, entry->value);
			}
			++entity;
		}
	}

	return !sceneGraph.empty();
}

} // namespace voxelformat
