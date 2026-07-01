/**
 * @file
 */

#include "LXFFormat.h"
#include "LegoUtil.h"
#include "app/App.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "image/Image.h"
#include "io/CachingArchive.h"
#include "io/FilesystemArchive.h"
#include "io/ZipArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "tinyxml2.h"
#include <SDL3/SDL_stdinc.h>
#include <cmath>

namespace voxelformat {

namespace priv {

// LDD stud width is 0.8 (approx. cm); LDraw uses 20 LDU per stud -> 25 LDU per LDD unit
static const float kLddToLdu = 25.0f;

static bool matchElementName(const char *fullName, const char *localName) {
	if (fullName == nullptr || localName == nullptr) {
		return false;
	}
	if (SDL_strcmp(fullName, localName) == 0) {
		return true;
	}
	const char *colon = SDL_strchr(fullName, ':');
	if (colon != nullptr) {
		return SDL_strcmp(colon + 1, localName) == 0;
	}
	return false;
}

static const tinyxml2::XMLElement *findChildElement(const tinyxml2::XMLElement *parent, const char *localName) {
	if (parent == nullptr) {
		return nullptr;
	}
	for (const tinyxml2::XMLElement *child = parent->FirstChildElement(); child != nullptr;
		 child = child->NextSiblingElement()) {
		if (matchElementName(child->Name(), localName)) {
			return child;
		}
	}
	return nullptr;
}

static int parseDoubles(const char *text, double *values, int count) {
	if (text == nullptr) {
		return 0;
	}
	const char *ptr = text;
	int parsed = 0;
	for (int i = 0; i < count; ++i) {
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r' || *ptr == ',') {
			++ptr;
		}
		if (*ptr == '\0') {
			break;
		}
		char *endPtr = nullptr;
		values[i] = SDL_strtod(ptr, &endPtr);
		if (endPtr == ptr) {
			break;
		}
		ptr = endPtr;
		++parsed;
	}
	return parsed;
}

// Row-major 4x4 matrix matching lxf2ldr.js (m[row][col])
struct LxfMat4 {
	float m[4][4];
};

static LxfMat4 lxfMat4Identity() {
	LxfMat4 result{};
	for (int i = 0; i < 4; ++i) {
		result.m[i][i] = 1.0f;
	}
	return result;
}

// LXFML transformation="r00,r01,r02,r10,...,tx,ty,tz" is row-major 3x4 (Eurobricks schema,
// lxf2ldr.js). Bone transforms are world-space in LDD coordinates (Y-up).
static bool parseLxfTransformation(const char *text, LxfMat4 &out) {
	double values[12];
	if (parseDoubles(text, values, 12) != 12) {
		return false;
	}
	out = lxfMat4Identity();
	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 3; ++col) {
			out.m[row][col] = (float)values[row * 3 + col];
		}
	}
	out.m[3][0] = (float)values[9];
	out.m[3][1] = (float)values[10];
	out.m[3][2] = (float)values[11];
	return true;
}

static LxfMat4 matMul(const LxfMat4 &a, const LxfMat4 &b) {
	LxfMat4 result = lxfMat4Identity();
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.m[i][j] =
				a.m[0][j] * b.m[i][0] + a.m[1][j] * b.m[i][1] + a.m[2][j] * b.m[i][2] + a.m[3][j] * b.m[i][3];
		}
	}
	return result;
}

static glm::vec3 matMulVec(const LxfMat4 &matrix, const glm::vec3 &vec) {
	return glm::vec3(vec.x * matrix.m[0][0] + vec.y * matrix.m[1][0] + vec.z * matrix.m[2][0] + matrix.m[3][0],
					 vec.x * matrix.m[0][1] + vec.y * matrix.m[1][1] + vec.z * matrix.m[2][1] + matrix.m[3][1],
					 vec.x * matrix.m[0][2] + vec.y * matrix.m[1][2] + vec.z * matrix.m[2][2] + matrix.m[3][2]);
}

static glm::vec3 matMulDir(const LxfMat4 &matrix, const glm::vec3 &vec) {
	return glm::vec3(vec.x * matrix.m[0][0] + vec.y * matrix.m[1][0] + vec.z * matrix.m[2][0],
					 vec.x * matrix.m[0][1] + vec.y * matrix.m[1][1] + vec.z * matrix.m[2][1],
					 vec.x * matrix.m[0][2] + vec.y * matrix.m[1][2] + vec.z * matrix.m[2][2]);
}

static LxfMat4 axisAngleTransform(float tx, float ty, float tz, float ax, float ay, float az, float angle) {
	LxfMat4 tr = lxfMat4Identity();
	const float len = std::sqrt(ax * ax + ay * ay + az * az);
	if (len > 0.0f) {
		ax /= len;
		ay /= len;
		az /= len;
	}
	const float c = std::cos(angle);
	const float s = std::sin(angle);
	const float ic = 1.0f - c;
	// Keep the same matrix convention as lxf2ldr.js rot2mat()/mat_mul_vec().
	tr.m[0][0] = ax * ax * ic + c;
	tr.m[1][0] = ax * ay * ic - az * s;
	tr.m[2][0] = ax * az * ic + ay * s;
	tr.m[0][1] = ay * ax * ic + az * s;
	tr.m[1][1] = ay * ay * ic + c;
	tr.m[2][1] = ay * az * ic - ax * s;
	tr.m[0][2] = ax * az * ic - ay * s;
	tr.m[1][2] = ay * az * ic + ax * s;
	tr.m[2][2] = az * az * ic + c;
	tr.m[3][0] = tx;
	tr.m[3][1] = ty;
	tr.m[3][2] = tz;
	return tr;
}

static bool extractEscapedAttribute(const char *line, const char *key, core::String &value) {
	const char *start = SDL_strstr(line, key);
	if (start == nullptr) {
		return false;
	}
	start += SDL_strlen(key);
	const char *end = SDL_strstr(start, "\\\"");
	if (end == nullptr || end <= start) {
		return false;
	}
	value = core::String(start, (int)(end - start));
	return true;
}

static bool parseEscapedFloat(const char *line, const char *key, float &out) {
	core::String value;
	if (!extractEscapedAttribute(line, key, value)) {
		return false;
	}
	out = (float)SDL_strtod(value.c_str(), nullptr);
	return true;
}

static void parseLdrawXmlMapping(io::SeekableReadStream &stream, core::DynamicMap<int, core::String> &legoToLdraw,
								 core::DynamicStringMap<LxfMat4> &ldrawTransform) {
	char line[2048];
	while (stream.readLine(sizeof(line), line)) {
		if (SDL_strstr(line, "<Brick ") != nullptr) {
			core::String ldraw;
			core::String lego;
			if (extractEscapedAttribute(line, "ldraw=\\\"", ldraw) &&
				extractEscapedAttribute(line, "lego=\\\"", lego)) {
				legoToLdraw.put(SDL_atoi(lego.c_str()), ldraw.toLower());
			}
			continue;
		}
		if (SDL_strstr(line, "<Transformation ") == nullptr) {
			continue;
		}
		core::String ldraw;
		float tx = 0.0f, ty = 0.0f, tz = 0.0f;
		float ax = 0.0f, ay = 1.0f, az = 0.0f;
		float angle = 0.0f;
		if (!extractEscapedAttribute(line, "ldraw=\\\"", ldraw)) {
			continue;
		}
		if (!parseEscapedFloat(line, "tx=\\\"", tx) || !parseEscapedFloat(line, "ty=\\\"", ty) ||
			!parseEscapedFloat(line, "tz=\\\"", tz) || !parseEscapedFloat(line, "ax=\\\"", ax) ||
			!parseEscapedFloat(line, "ay=\\\"", ay) || !parseEscapedFloat(line, "az=\\\"", az) ||
			!parseEscapedFloat(line, "angle=\\\"", angle)) {
			continue;
		}
		ldrawTransform.put(ldraw.toLower(), axisAngleTransform(-tx, -ty, -tz, ax, ay, az, -angle));
	}
}

static const LxfMat4 &lddAxesMatrix() {
	static const LxfMat4 axes = {
		{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}};
	return axes;
}

// Same layout as LegoUtil::parseLine type-1: LDraw row vectors map to glm columns for R * v
static glm::mat3 ldrawRotationToGlm(const LxfMat4 &rotation) {
	return glm::mat3(rotation.m[0][0], rotation.m[1][0], rotation.m[2][0], rotation.m[0][1], rotation.m[1][1],
					 rotation.m[2][1], rotation.m[0][2], rotation.m[1][2], rotation.m[2][2]);
}

// Convert LDD bone matrix to LDraw sub-file placement (lxf2ldr.js ldr(), without part xml offsets)
static void lddTransformToLdrawSubFileRef(const LxfMat4 &ldd, glm::vec3 &pos, glm::mat3 &rot) {
	const LxfMat4 &axes = lddAxesMatrix();
	const LxfMat4 ldrRot = matMul(matMul(axes, ldd), axes);
	const glm::vec3 lddPos(ldd.m[3][0], ldd.m[3][1], ldd.m[3][2]);
	pos = kLddToLdu * matMulVec(axes, lddPos);
	rot = ldrawRotationToGlm(ldrRot);
}

// Convert LDD bone matrix with ldraw.xml part correction (x2l_tr in lxf2ldr.js).
static void lddTransformToLdrawSubFileRef(const LxfMat4 &ldd, const LxfMat4 &x2lTransform, glm::vec3 &pos,
										  glm::mat3 &rot) {
	LxfMat4 x2lRot = x2lTransform;
	x2lRot.m[3][0] = 0.0f;
	x2lRot.m[3][1] = 0.0f;
	x2lRot.m[3][2] = 0.0f;
	LxfMat4 adjusted = matMul(ldd, x2lRot);
	const glm::vec3 move(x2lTransform.m[3][0], x2lTransform.m[3][1], x2lTransform.m[3][2]);
	const glm::vec3 moved = matMulDir(adjusted, move);
	adjusted.m[3][0] = ldd.m[3][0] + moved.x;
	adjusted.m[3][1] = ldd.m[3][1] + moved.y;
	adjusted.m[3][2] = ldd.m[3][2] + moved.z;
	lddTransformToLdrawSubFileRef(adjusted, pos, rot);
}

static glm::mat4 lddToLdrawMatrix(const LxfMat4 &ldd) {
	glm::vec3 pos;
	glm::mat3 rot;
	lddTransformToLdrawSubFileRef(ldd, pos, rot);
	glm::mat4 result(1.0f);
	result[0] = glm::vec4(rot[0], 0.0f);
	result[1] = glm::vec4(rot[1], 0.0f);
	result[2] = glm::vec4(rot[2], 0.0f);
	result[3] = glm::vec4(pos, 1.0f);
	return result;
}

static glm::mat4 applyImportScale(const glm::mat4 &matrix, const glm::vec3 &scale) {
	glm::mat4 result = matrix;
	result[3][0] *= scale.x;
	result[3][1] *= scale.y;
	result[3][2] *= scale.z;
	return result;
}

static int parseFirstMaterialId(const char *materials) {
	if (materials == nullptr || *materials == '\0') {
		return 16;
	}
	return SDL_atoi(materials);
}

static core::String designIdToPartFile(const core::String &designId) {
	if (designId.empty()) {
		return {};
	}
	if (core::string::endsWith(designId, ".dat") || core::string::endsWith(designId, ".ldr")) {
		return designId;
	}
	return core::String::format("%s.dat", designId.c_str());
}

static io::ArchivePtr openLxfArchive(io::SeekableReadStream &stream) {
	if (io::ZipArchive::validStream(stream)) {
		return io::openZipArchive(&stream);
	}
	return {};
}

static bool readStreamContent(io::SeekableReadStream &stream, core::String &content) {
	const int64_t size = stream.size();
	if (size <= 0) {
		Log::error("Could not read LXFML stream");
		return false;
	}
	if (!stream.readString((int)size, content, false)) {
		Log::error("Failed to read LXFML content");
		return false;
	}
	return true;
}

static bool loadLxfmlContent(const core::String &filename, const io::ArchivePtr &archive, core::String &content) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	const io::ArchivePtr zipArchive = openLxfArchive(*stream);
	if (zipArchive) {
		io::ArchiveFiles files;
		zipArchive->list("*.lxfml", files);
		if (files.empty()) {
			zipArchive->list("IMAGE100.lxfml", files);
		}
		if (files.empty()) {
			Log::error("No LXFML file found in LXF archive %s", filename.c_str());
			return false;
		}
		core::ScopedPtr<io::SeekableReadStream> lxfmlStream(zipArchive->readStream(files[0].fullPath));
		if (!lxfmlStream) {
			Log::error("Could not read LXFML from %s", filename.c_str());
			return false;
		}
		return readStreamContent(*lxfmlStream, content);
	}
	if (stream->seek(0) == -1) {
		return false;
	}
	return readStreamContent(*stream, content);
}

struct PartInstance {
	core::String designId;
	int colorCode = 16;
	int boneRefId = -1;
	LxfMat4 partTransform = lxfMat4Identity();
	LxfMat4 boneTransform = lxfMat4Identity();
	LxfMat4 world = lxfMat4Identity();
};

struct LxfCamera {
	int refId = 0;
	int fieldOfView = 45;
	float distance = 100.0f;
	LxfMat4 lddTransform = lxfMat4Identity();
};

static void addXmlAttributes(const tinyxml2::XMLElement *element, const char *prefix,
							 core::DynamicStringMap<core::String> &properties) {
	if (element == nullptr) {
		return;
	}
	for (const tinyxml2::XMLAttribute *attr = element->FirstAttribute(); attr != nullptr; attr = attr->Next()) {
		properties.put(core::String::format("%s%s", prefix, attr->Name()), attr->Value());
	}
}

static void parseMeta(const tinyxml2::XMLElement *root, core::DynamicStringMap<core::String> &properties) {
	const tinyxml2::XMLElement *meta = findChildElement(root, "Meta");
	if (meta == nullptr) {
		return;
	}
	const tinyxml2::XMLElement *application = findChildElement(meta, "Application");
	if (application != nullptr) {
		addXmlAttributes(application, "ldd_application_", properties);
	}
	const tinyxml2::XMLElement *brand = findChildElement(meta, "Brand");
	if (brand != nullptr) {
		addXmlAttributes(brand, "ldd_brand_", properties);
	}
	const tinyxml2::XMLElement *brickSet = findChildElement(meta, "BrickSet");
	if (brickSet != nullptr) {
		addXmlAttributes(brickSet, "ldd_brickset_", properties);
	}
}

static void parseCameras(const tinyxml2::XMLElement *root, core::DynamicArray<LxfCamera> &cameras) {
	const tinyxml2::XMLElement *camerasElement = findChildElement(root, "Cameras");
	if (camerasElement == nullptr) {
		return;
	}
	for (const tinyxml2::XMLElement *camera = camerasElement->FirstChildElement(); camera != nullptr;
		 camera = camera->NextSiblingElement()) {
		if (!matchElementName(camera->Name(), "Camera")) {
			continue;
		}
		LxfCamera lxfCamera;
		const char *refId = camera->Attribute("refID");
		if (refId != nullptr) {
			lxfCamera.refId = SDL_atoi(refId);
		}
		const char *fieldOfView = camera->Attribute("fieldOfView");
		if (fieldOfView != nullptr) {
			lxfCamera.fieldOfView = (int)SDL_strtod(fieldOfView, nullptr);
		}
		const char *distance = camera->Attribute("distance");
		if (distance != nullptr) {
			lxfCamera.distance = (float)SDL_strtod(distance, nullptr);
		}
		const char *transformation = camera->Attribute("transformation");
		if (transformation != nullptr) {
			parseLxfTransformation(transformation, lxfCamera.lddTransform);
		}
		cameras.push_back(core::move(lxfCamera));
	}
}

static void applyRootProperties(scenegraph::SceneGraph &sceneGraph,
								const core::DynamicStringMap<core::String> &properties) {
	scenegraph::SceneGraphNode &root = sceneGraph.node(sceneGraph.root().id());
	for (auto iter = properties.begin(); iter != properties.end(); ++iter) {
		root.setProperty(iter->key, iter->value);
	}
}

static void addCameras(scenegraph::SceneGraph &sceneGraph, const core::DynamicArray<LxfCamera> &cameras,
					   int defaultCameraRef, const glm::vec3 &importScale) {
	const float farPlaneScale =
		core_max(core_max(glm::abs(importScale.x), glm::abs(importScale.y)), glm::abs(importScale.z));
	int activeCameraId = InvalidNodeId;
	for (const LxfCamera &lxfCamera : cameras) {
		scenegraph::SceneGraphNodeCamera camNode;
		camNode.setName(core::String::format("Camera %i", lxfCamera.refId));
		camNode.setPerspective();
		camNode.setFieldOfView(lxfCamera.fieldOfView);
		camNode.setFarPlane(lxfCamera.distance * farPlaneScale);
		camNode.setProperty("ldd_distance", core::string::toString(lxfCamera.distance));
		camNode.setProperty("ldd_camera_ref", core::string::toString(lxfCamera.refId));

		scenegraph::SceneGraphTransform transform;
		transform.setLocalMatrix(applyImportScale(lddToLdrawMatrix(lxfCamera.lddTransform), importScale));
		camNode.setTransform(0, transform);

		const int cameraId = sceneGraph.emplace(core::move(camNode), sceneGraph.root().id());
		if (cameraId != InvalidNodeId) {
			scenegraph::SceneGraphNode &cameraNode = sceneGraph.node(cameraId);
			cameraNode.transform(0).update(sceneGraph, cameraNode, 0, true);
		}
		if (activeCameraId == InvalidNodeId) {
			activeCameraId = cameraId;
		}
		if (lxfCamera.refId == defaultCameraRef) {
			activeCameraId = cameraId;
		}
	}
	if (activeCameraId != InvalidNodeId) {
		sceneGraph.setActiveNode(activeCameraId);
	}
}

static bool parseLxfml(const core::String &content, core::DynamicArray<PartInstance> &parts, core::String &modelName,
					   core::DynamicStringMap<core::String> &rootProperties, core::DynamicArray<LxfCamera> &cameras,
					   int &defaultCameraRef) {
	tinyxml2::XMLDocument doc;
	const tinyxml2::XMLError error = doc.Parse(content.c_str(), content.size());
	if (error != tinyxml2::XML_SUCCESS) {
		Log::error("Failed to parse LXFML: %s", doc.ErrorStr());
		return false;
	}
	const tinyxml2::XMLElement *root = doc.RootElement();
	if (root == nullptr || !matchElementName(root->Name(), "LXFML")) {
		Log::error("LXFML root element not found");
		return false;
	}
	const char *nameAttr = root->Attribute("name");
	if (nameAttr != nullptr) {
		modelName = nameAttr;
	}
	addXmlAttributes(root, "lxfml_", rootProperties);
	parseMeta(root, rootProperties);
	parseCameras(root, cameras);

	const tinyxml2::XMLElement *bricks = findChildElement(root, "Bricks");
	if (bricks != nullptr) {
		const char *cameraRef = bricks->Attribute("cameraRef");
		if (cameraRef != nullptr) {
			defaultCameraRef = SDL_atoi(cameraRef);
			rootProperties.put("lxfml_camera_ref", cameraRef);
		}
		for (const tinyxml2::XMLElement *brick = bricks->FirstChildElement(); brick != nullptr;
			 brick = brick->NextSiblingElement()) {
			if (!matchElementName(brick->Name(), "Brick")) {
				continue;
			}
			for (const tinyxml2::XMLElement *part = brick->FirstChildElement(); part != nullptr;
				 part = part->NextSiblingElement()) {
				if (!matchElementName(part->Name(), "Part")) {
					continue;
				}
				const char *designId = part->Attribute("designID");
				if (designId == nullptr) {
					continue;
				}
				const char *materials = part->Attribute("materials");
				if (materials == nullptr) {
					materials = part->Attribute("materialID");
				}
				const int materialId = parseFirstMaterialId(materials);

				PartInstance instance;
				instance.designId = designId;
				instance.colorCode = materialId;
				const char *tx = part->Attribute("tx");
				const char *ty = part->Attribute("ty");
				const char *tz = part->Attribute("tz");
				if (tx != nullptr && ty != nullptr && tz != nullptr) {
					LxfMat4 offset = lxfMat4Identity();
					offset.m[3][0] = (float)SDL_strtod(tx, nullptr);
					offset.m[3][1] = (float)SDL_strtod(ty, nullptr);
					offset.m[3][2] = (float)SDL_strtod(tz, nullptr);
					instance.partTransform = matMul(instance.partTransform, offset);
				}
				for (const tinyxml2::XMLElement *bone = part->FirstChildElement(); bone != nullptr;
					 bone = bone->NextSiblingElement()) {
					if (!matchElementName(bone->Name(), "Bone")) {
						continue;
					}
					const char *boneRef = bone->Attribute("refID");
					const char *transformation = bone->Attribute("transformation");
					if (transformation != nullptr) {
						parseLxfTransformation(transformation, instance.boneTransform);
					}
					if (boneRef != nullptr) {
						instance.boneRefId = SDL_atoi(boneRef);
					}
				}
				instance.world = matMul(instance.partTransform, instance.boneTransform);
				parts.push_back(core::move(instance));
			}
		}
	}

	return !parts.empty();
}

} // namespace priv

bool LXFFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::String content;
	if (!priv::loadLxfmlContent(filename, archive, content)) {
		return false;
	}

	core::DynamicArray<priv::PartInstance> parts;
	core::String modelName;
	core::DynamicStringMap<core::String> rootProperties;
	core::DynamicArray<priv::LxfCamera> cameras;
	int defaultCameraRef = -1;
	if (!priv::parseLxfml(content, parts, modelName, rootProperties, cameras, defaultCameraRef)) {
		Log::error("No parts found in LXFML file %s", filename.c_str());
		return false;
	}

	priv::applyRootProperties(sceneGraph, rootProperties);
	priv::addCameras(sceneGraph, cameras, defaultCameraRef, getScale());

	const core::String ldrawDir = core::getVar(cfg::VoxformatLDrawDir)->strVal();

	io::CachingArchive cachedArchive(legoutil::openLookupArchive(archive));
	legoutil::registerLdrawSearchPaths(cachedArchive);

	legoutil::ColorMap colors;
	legoutil::initColors(colors);
	core::DynamicMap<int, int> legoIdToLdrawCode;
	core::DynamicMap<int, core::String> legoIdToLdrawPart;
	core::DynamicStringMap<priv::LxfMat4> ldrawPartTransform;
	{
		core::ScopedPtr<io::SeekableReadStream> ldConfigStream(cachedArchive.findStream("LDConfig.ldr"));
		if (ldConfigStream) {
			legoutil::parseLegoIdMap(*ldConfigStream, colors, legoIdToLdrawCode);
		} else {
			legoutil::parseLDConfig(cachedArchive, colors);
		}
		core::ScopedPtr<io::SeekableReadStream> ldrawXmlStream(cachedArchive.findStream("ldraw.xml.p"));
		if (ldrawXmlStream) {
			priv::parseLdrawXmlMapping(*ldrawXmlStream, legoIdToLdrawPart, ldrawPartTransform);
		}
	}

	if (modelName.empty()) {
		modelName = core::string::extractFilenameWithExtension(filename);
	}

	int parentNodeId = sceneGraph.root().id();
	const bool createGroupNode = parts.size() > 1;
	if (createGroupNode) {
		scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
		groupNode.setName(modelName);
		parentNodeId = sceneGraph.emplace(core::move(groupNode), sceneGraph.root().id());
		if (parentNodeId == InvalidNodeId) {
			Log::error("Failed to create LXF group node for %s", filename.c_str());
			return false;
		}
	}

	int firstModelNodeId = InvalidNodeId;
	for (const priv::PartInstance &part : parts) {
		core::String partFile = priv::designIdToPartFile(part.designId);
		core::String mappedPartFile;
		const int legoDesignId = SDL_atoi(part.designId.c_str());
		if (legoIdToLdrawPart.get(legoDesignId, mappedPartFile)) {
			partFile = mappedPartFile;
		}
		const int colorCode = legoutil::lookupLdrawColor(legoIdToLdrawCode, part.colorCode);

		legoutil::SubFileRef ref;
		ref.colorCode = colorCode;
		ref.filename = partFile;
		const core::String transformKey = partFile.toLower();
		priv::LxfMat4 x2lTransform;
		if (ldrawPartTransform.get(transformKey, x2lTransform)) {
			priv::lddTransformToLdrawSubFileRef(part.world, x2lTransform, ref.pos, ref.transform);
		} else {
			priv::lddTransformToLdrawSubFileRef(part.world, ref.pos, ref.transform);
		}

		Mesh brickMesh;
		if (!legoutil::resolveSubFile(cachedArchive, ref, colors, brickMesh, 0) || brickMesh.vertices.empty()) {
			Log::warn("Could not resolve LXF part %s (%s)", part.designId.c_str(), partFile.c_str());
			continue;
		}
		const core::String nodeName = createGroupNode ? partFile : modelName;
		const int nodeId = voxelizeMesh(nodeName, sceneGraph, core::move(brickMesh), parentNodeId, true);
		if (firstModelNodeId == InvalidNodeId) {
			firstModelNodeId = nodeId;
		}
	}

	if (firstModelNodeId == InvalidNodeId) {
		Log::error("No LXF parts could be resolved from %s - check the LDraw library path ('%s' = '%s')",
				   filename.c_str(), cfg::VoxformatLDrawDir, ldrawDir.c_str());
		return false;
	}

	if (!createGroupNode) {
		scenegraph::SceneGraphNode &node = sceneGraph.node(firstModelNodeId);
		node.setName(modelName);
	}
	return true;
}

image::ImagePtr LXFFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return {};
	}
	const io::ArchivePtr zipArchive = priv::openLxfArchive(*stream);
	if (!zipArchive) {
		return {};
	}
	io::ArchiveFiles files;
	zipArchive->list("*.png", files);
	for (const io::FilesystemEntry &entry : files) {
		if (entry.name.toLower() != "image100.png") {
			continue;
		}
		core::ScopedPtr<io::SeekableReadStream> thumbnailStream(zipArchive->readStream(entry.fullPath));
		if (!thumbnailStream) {
			Log::error("Could not load file %s", entry.fullPath.c_str());
			return {};
		}
		return image::loadImage(entry.name, *thumbnailStream, thumbnailStream->size());
	}
	Log::debug("Could not find IMAGE100.png in the LXF archive");
	return {};
}

} // namespace voxelformat
