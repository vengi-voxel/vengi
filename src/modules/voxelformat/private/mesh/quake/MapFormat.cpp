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
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "math/Plane.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "voxelformat/private/mesh/Polygon.h"
#include "voxelformat/private/mesh/TextureLookup.h"

namespace voxelformat {

// Convert from Quake coordinates (X, Y, Z where Z is up) to Vengi coordinates (X, Y, Z where Y is up)
static inline glm::vec3 quakeToVengi(const glm::vec3 &quakePos) {
	return glm::vec3(quakePos.x, quakePos.z, -quakePos.y);
}

// clip plane
struct QFace {
	glm::vec3 planePoints[3];
	core::String texture;
	glm::vec2 offset;
	float rotation;
	glm::vec2 texscale;
	int contentFlags;
	int surfaceFlags;
	int value;

	glm::vec3 edge1;
	glm::vec3 edge2;
	glm::vec3 normal;
	float d;
	math::Plane plane;

	void finish() {
		edge1 = planePoints[1] - planePoints[0];
		edge2 = planePoints[2] - planePoints[0];
		normal = glm::normalize(glm::cross(edge1, edge2));
		d = glm::dot(normal, planePoints[0]);
		plane = math::Plane(normal, d);
	}
};

struct QBrush {
	core::DynamicArray<QFace> faces;
};

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

static glm::vec3 parsePlaneWithConversion(core::Tokenizer &tok) {
	const glm::vec3 quakePos = parsePlane(tok);
	return quakeToVengi(quakePos);
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

static void planeBasis(const glm::vec3 &n, glm::vec3 &u, glm::vec3 &v) {
	if (glm::abs(n.z) > 0.9f) {
		u = glm::normalize(glm::cross(n, glm::vec3(0, 1, 0)));
	} else {
		u = glm::normalize(glm::cross(n, glm::vec3(0, 0, 1)));
	}
	v = glm::normalize(glm::cross(u, n));
}

static core::DynamicArray<glm::vec3> createBasePolygon(const math::Plane &plane) {
	glm::vec3 u, v;
	planeBasis(plane.norm(), u, v);

	const float EXTENT = 8192.0f; // larger than any map
	const glm::vec3 origin = plane.norm() * -plane.dist();

	core::DynamicArray<glm::vec3> poly;
	poly.emplace_back(origin + u * EXTENT + v * EXTENT);
	poly.emplace_back(origin - u * EXTENT + v * EXTENT);
	poly.emplace_back(origin - u * EXTENT - v * EXTENT);
	poly.emplace_back(origin + u * EXTENT - v * EXTENT);

	return poly;
}

static core::DynamicArray<glm::vec3> clipPolygon(const core::DynamicArray<glm::vec3> &in, const math::Plane &plane) {
	core::DynamicArray<glm::vec3> out;
	if (in.empty()) {
		return out;
	}

	glm::vec3 prev = in.back();
	float prevDist = plane.distanceToPlane(prev);

	for (const glm::vec3 &curr : in) {
		float currDist = plane.distanceToPlane(curr);

		const bool currIn = currDist >= 0.0f;
		const bool prevIn = prevDist >= 0.0f;

		if (currIn ^ prevIn) {
			const float t = prevDist / (prevDist - currDist);
			out.emplace_back(prev + t * (curr - prev));
		}
		if (currIn) {
			out.emplace_back(curr);
		}

		prev = curr;
		prevDist = currDist;
	}

	return out;
}

static core::DynamicArray<glm::vec3> buildFacePolygon(const QFace &face, const QBrush &brush) {
	core::DynamicArray<glm::vec3> poly = createBasePolygon(face.plane);

	for (const QFace &clip : brush.faces) {
		if (&clip == &face) {
			continue;
		}
		poly = clipPolygon(poly, clip.plane);
		if (poly.size() < 3) {
			poly.clear();
			break;
		}
	}

	return poly;
}

static glm::vec2 computeUV(const QFace &face, const glm::vec3 &worldPos) {
	// 1 stable basis for the plane
	glm::vec3 u, v;
	planeBasis(face.normal, u, v);

	// 2 Projection onto basis
	float s = glm::dot(worldPos, u);
	float t = glm::dot(worldPos, v);

	// 3 rotation (Quake: degrees, CCW)
	if (face.rotation != 0.0f) {
		const float rad = glm::radians(face.rotation);
		const float cs = glm::cos(rad);
		const float sn = glm::sin(rad);

		const float sRot = cs * s - sn * t;
		const float tRot = sn * s + cs * t;

		s = sRot;
		t = tRot;
	}

	// 4 Scale
	const float scaleX = face.texscale.x != 0.0f ? face.texscale.x : 1.0f;
	const float scaleY = face.texscale.y != 0.0f ? face.texscale.y : 1.0f;

	s /= scaleX;
	t /= scaleY;

	// 5 Offset
	s += face.offset.x;
	t += face.offset.y;

	return glm::vec2(s, t);
}

bool MapFormat::parseBrush(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
						   MeshMaterialMap &meshMaterials, Mesh &mesh) const {
	QBrush qbrush;
	core::String line;
	while (stream.readLine(line)) {
		if (line.empty() || core::string::startsWith(line, "//")) {
			continue;
		}
		if (line == "}") {
			break;
		}

		core::Tokenizer tok(line, " ");
		core::String t = tok.peekNext();
		if (t == "patchDef2") {
			Log::error("Quake3 is not yet supported");
			return false;
		}

		QFace qface;
		qface.planePoints[0] = parsePlaneWithConversion(tok);
		qface.planePoints[1] = parsePlaneWithConversion(tok);
		qface.planePoints[2] = parsePlaneWithConversion(tok);
		qface.texture = tok.next();
		if (skipFace(qface.texture)) {
			continue;
		}

		if (tok.peekNext() == "[") {
			Log::error("Valve format is not yet supported");
			return false;
		}

		if (!parseFloat(tok, qface.offset.x)) {
			Log::error("Failed to parse xoffset");
			return false;
		}
		if (!parseFloat(tok, qface.offset.y)) {
			Log::error("Failed to parse yoffset");
			return false;
		}
		Log::trace("offset: %f:%f", qface.offset.x, qface.offset.y);

		if (!parseFloat(tok, qface.rotation)) {
			Log::error("Failed to parse rotation");
			return false;
		}
		Log::trace("rotation: %f", qface.rotation);

		if (!parseFloat(tok, qface.texscale.x)) {
			Log::error("Failed to parse xscale");
			return false;
		}
		if (!parseFloat(tok, qface.texscale.y)) {
			Log::error("Failed to parse yscale");
			return false;
		}
		Log::trace("texscale: %f:%f", qface.texscale.x, qface.texscale.y);

		if (tok.hasNext()) {
			parseInt(tok, qface.contentFlags);
			Log::trace("Contentflags: %i", qface.contentFlags);
			parseInt(tok, qface.surfaceFlags);
			Log::trace("SurfaceFlags: %i", qface.surfaceFlags);
			parseInt(tok, qface.value);
			Log::trace("Value: %i", qface.value);
		}

		qface.finish();
		qbrush.faces.emplace_back(core::move(qface));
	}

	core::DynamicArray<core::Path> additionPaths;
	additionPaths.push_back(core::Path("../textures/"));
	for (const QFace &qface : qbrush.faces) {
		auto iter = meshMaterials.find(qface.texture);
		MeshMaterialIndex materialIdx;
		if (iter == meshMaterials.end()) {
			const core::String &imageName = lookupTexture(filename, qface.texture, archive, additionPaths);
			const image::ImagePtr &image = image::loadImage(imageName);
			mesh.materials.push_back(createMaterial(image));
			materialIdx = mesh.materials.size() - 1;
			meshMaterials.put(qface.texture, materialIdx);
		} else {
			materialIdx = iter->value;
		}

		Polygon polygon;
		polygon.setMaterialIndex(materialIdx);

		// Create vertices with UV mapping
		const auto polyVerts = buildFacePolygon(qface, qbrush);

		if (polyVerts.empty()) {
			continue;
		}

		for (const glm::vec3 &p : polyVerts) {
			glm::vec2 uv = computeUV(qface, p);
			glm::vec3 snapped = p;
			const float epsilon = 0.001f;
			if (glm::abs(snapped.x - glm::round(snapped.x)) < epsilon) {
				snapped.x = glm::round(snapped.x);
			}
			if (glm::abs(snapped.y - glm::round(snapped.y)) < epsilon) {
				snapped.y = glm::round(snapped.y);
			}
			if (glm::abs(snapped.z - glm::round(snapped.z)) < epsilon) {
				snapped.z = glm::round(snapped.z);
			}
			polygon.addVertex(snapped, uv);
		}

		polygon.toTris(mesh);
	}
	return true;
}

bool MapFormat::parseEntity(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
							MeshMaterialMap &meshMaterials, Mesh &mesh,
							scenegraph::SceneGraphNodeProperties &props) const {
	core::String line;
	while (stream.readLine(line)) {
		if (line.empty() || core::string::startsWith(line, "//")) {
			continue;
		}
		Log::debug("Token in entity: '%s'", line.c_str());
		if (line == "}") {
			return true;
		}
		if (line == "{") {
			Log::debug("Found brush");
			if (!parseBrush(filename, archive, stream, meshMaterials, mesh)) {
				Log::error("Failed to parse brush");
				return false;
			}
		} else {
			core::Tokenizer tok(line, " ");
			if (tok.size() != 2) {
				Log::error("Invalid entity key/value pair: %s", line.c_str());
				return false;
			}
			const core::String key = tok.next();
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

	int entity = 0;
	core::String line;
	while (stream->readLine(line)) {
		if (line.empty() || core::string::startsWith(line, "//")) {
			continue;
		}
		Log::debug("Token in map: %s", line.c_str());
		if (line == "{") {
			Mesh mesh;
			MeshMaterialMap meshMaterials;
			scenegraph::SceneGraphNodeProperties props;
			if (!parseEntity(filename, archive, *stream, meshMaterials, mesh, props)) {
				Log::error("Failed to parse entity");
				return false;
			}
			if (mesh.vertices.empty()) {
				core::String classname;
				if (props.get("classname", classname)) {
					core::String originStr;
					glm::vec3 origin(0.0f);
					if (props.get("origin", originStr)) {
						core::Tokenizer tok(originStr, " ");
						float x = 0.0f;
						float y = 0.0f;
						float z = 0.0f;
						if (tok.hasNext()) {
							x = tok.next().toFloat();
						}
						if (tok.hasNext()) {
							y = tok.next().toFloat();
						}
						if (tok.hasNext()) {
							z = tok.next().toFloat();
						}
						origin = quakeToVengi(glm::vec3(x, y, z));
					}
					scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Point);
					node.setName(core::String::format("%s %i", classname.c_str(), entity));
					for (const auto &entry : props) {
						node.setProperty(entry->key, entry->value);
					}
					scenegraph::SceneGraphKeyFrame &frame = node.keyFrame(0);
					scenegraph::SceneGraphTransform &transform = frame.transform();
					transform.setWorldTranslation(origin);
					sceneGraph.emplace(core::move(node));
				}
			} else {
				core::String classname;
				props.get("classname", classname);
				const core::String name = core::String::format("%s brush %i", classname.c_str(), entity);
				const int nodeId = voxelizeMesh(name, sceneGraph, core::move(mesh));
				if (nodeId == InvalidNodeId) {
					Log::error("Voxelization failed");
					return false;
				}
				scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
				for (const auto &entry : props) {
					node.setProperty(entry->key, entry->value);
				}
			}
			++entity;
		}
	}

	return !sceneGraph.empty();
}

} // namespace voxelformat
