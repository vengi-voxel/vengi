/**
 * @file
 */

#include "TeardownFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "glm/fwd.hpp"
#include "io/StreamUtil.h"
#include "io/ZipReadStream.h"
#include "scenegraph/CoordinateSystemUtil.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "scenegraph/SceneGraphTransform.h"
#include "voxel/Voxel.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load teardown bin file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",   \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load teardown bin file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",   \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

static const int TDStringLength = 4096;

static bool readTransform(io::ReadStream &s, glm::vec3 &pos, glm::quat &rot) {
	wrapBool(io::readVec3(s, pos))
	wrapBool(io::readQuat(s, rot))
	return true;
}

// TODO: unsure and untested
static void setTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &pos, const glm::quat &quat) {
	// Teardown/MagicaVoxel uses Z-up coordinate system, vengi uses Y-up
	// We need to convert: Teardown (X right, Z up, Y forward) -> vengi (X right, Y up, Z back)
	// This means: swap Y and Z, and negate the new Z (was Y)
	glm::vec3 vengiPos(pos.x, pos.z, -pos.y);

	// For quaternion rotation: convert from Z-up to Y-up
	// Rotate the quaternion to match the coordinate system change
	glm::quat vengiQuat(quat.w, quat.x, quat.z, -quat.y);

	scenegraph::SceneGraphTransform transform;
	transform.setLocalOrientation(vengiQuat);
	transform.setLocalTranslation(vengiPos);
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
}

bool TeardownFormat::readEntity(const Header &header, scenegraph::SceneGraph &sceneGraph, io::ReadStream &s, int parent,
								int &nodeId) {
	EntityType entityType;
	wrap(s.readUInt8(*(uint8_t *)&entityType))
	uint32_t handle;
	wrap(s.readUInt32(handle))
	uint8_t tagCount;
	wrap(s.readUInt8(tagCount))
	scenegraph::SceneGraphNodeProperties properties;
	for (uint8_t t = 0; t < tagCount; ++t) {
		core::String tag;
		wrapBool(s.readString(TDStringLength, tag, true))
		core::String val;
		wrapBool(s.readString(TDStringLength * 10, val, true))
		Log::debug("Tag: '%s': '%s'", tag.c_str(), val.c_str());
		properties.put(tag, val);
	}
	core::String desc;
	wrapBool(s.readString(TDStringLength * 10, desc, true))
	Log::debug("Entity type: %u", (uint8_t)entityType);
	Log::debug("Description: '%s'", desc.c_str());
	Log::debug("Handle: %u", handle);
	Log::debug("Parent: %i", parent);
	switch (entityType) {
	case EntityType::Body:
		wrapBool(readBody(s))
		break;
	case EntityType::Shape: {
		int oldNodeId = nodeId;
		wrapBool(readShape(header, sceneGraph, s, parent, nodeId))
		if (oldNodeId != nodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			sceneGraph.node(nodeId).setProperty(scenegraph::PropDescription, desc);
			node.properties() = core::move(properties);
		}
		break;
	}
	case EntityType::Light:
		wrapBool(readLight(s))
		break;
	case EntityType::Location:
		wrapBool(readLocation(s))
		break;
	case EntityType::Water:
		wrapBool(readWater(s))
		break;
	case EntityType::Joint:
		wrapBool(readJoint(s))
		break;
	case EntityType::Vehicle:
		wrapBool(readVehicle(header, s))
		break;
	case EntityType::Wheel:
		wrapBool(readWheel(s))
		break;
	case EntityType::Screen:
		wrapBool(readScreen(s))
		break;
	case EntityType::Trigger:
		wrapBool(readTrigger(s))
		break;
	case EntityType::Script:
		wrapBool(readScript(s))
		break;
	case EntityType::Animator:
		wrapBool(readAnimator(header, s, parent, nodeId))
		break;
	default:
		Log::error("Invalid entity type: %d", (uint8_t)entityType);
		return false;
	}
	uint32_t children;
	wrap(s.readUInt32(children))
	// If this entity created a node, use it as the parent for children.
	// Otherwise fall back to the incoming parent id so children are attached to the correct node.
	int parentId = (nodeId != InvalidNodeId) ? nodeId : parent;
	for (uint32_t c = 0; c < children; ++c) {
		if (!readEntity(header, sceneGraph, s, parentId, nodeId)) {
			Log::error("Failed to read children %u/%u", c, children);
			return false;
		}
	}
	uint32_t sentinel;
	wrap(s.readUInt32(sentinel))
	if (sentinel != 0xBEEFBEEF) {
		Log::error("Could not load teardown bin file: Invalid entity sentinel");
		return false;
	}
	return true;
}

bool TeardownFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ZipReadStream s(*stream);
	uint8_t magic[5];
	for (int i = 0; i < lengthof(magic); ++i) {
		wrap(s.readUInt8(magic[i]))
	}
	if (magic[0] != 'T' || magic[1] != 'D' || magic[2] != 'B' || magic[3] != 'I' || magic[4] != 'N') {
		Log::error("Invalid teardown bin magic");
		return false;
	}

	uint8_t version[3];
	for (int i = 0; i < lengthof(version); ++i) {
		wrap(s.readUInt8(version[i]))
	}

	Header header;
	header.version = version[0] * 100 + version[1] * 10 + version[2];
	Log::debug("Teardown bin version: %u.%u.%u", version[0], version[1], version[2]);

	wrapBool(s.readString(TDStringLength, header.levelId, true))
	wrapBool(s.readString(TDStringLength, header.levelPath, true))
	wrapBool(s.readString(TDStringLength, header.layers, true))
	wrapBool(s.readString(TDStringLength, header.mod, true))

	uint32_t aaa1;
	wrap(s.readUInt32(aaa1))
	uint32_t enabledMods;
	wrap(s.readUInt32(enabledMods))
	for (uint32_t i = 0; i < enabledMods; ++i) {
		core::String key;
		core::String value;
		wrapBool(s.readString(TDStringLength, key, true))
		wrapBool(s.readString(TDStringLength, value, true))
		header.mods.put(key, value);
	}

	uint32_t spawnedMods;
	wrap(s.readUInt32(spawnedMods))
	for (uint32_t i = 0; i < spawnedMods; ++i) {
		core::String key;
		core::String value;
		wrapBool(s.readString(TDStringLength, key, true))
		wrapBool(s.readString(TDStringLength, value, true))
		header.spawnedMods.put(key, value);
	}

	uint32_t drivenVehicle;
	wrap(s.readUInt32(drivenVehicle))

	glm::vec3 shadowVolume;
	wrapBool(io::readVec3(s, shadowVolume))

	if (header.version >= 170) {
		glm::vec3 gravity;
		wrapBool(io::readVec3(s, gravity))
	}

	glm::vec3 spawnPos;
	wrapBool(io::readVec3(s, spawnPos))

	glm::quat spawnRot;
	wrapBool(io::readQuat(s, spawnRot))

	uint32_t worldBody;
	wrap(s.readUInt32(worldBody))

	uint32_t flashLight;
	wrap(s.readUInt32(flashLight))

	uint32_t explosionLua;
	wrap(s.readUInt32(explosionLua))

	uint32_t achievementLua;
	wrap(s.readUInt32(achievementLua))
	if (header.version >= 160) {
		uint32_t characterLua;
		wrap(s.readUInt32(characterLua))
	}

	// post processing
	float brightness;
	wrap(s.readFloat(brightness))

	glm::vec4 color;
	wrapBool(io::readVec4(s, color));

	float saturation;
	wrap(s.readFloat(saturation))

	float gamma;
	wrap(s.readFloat(gamma))

	float bloom;
	wrap(s.readFloat(bloom))

	glm::vec3 playerPos;
	wrapBool(io::readVec3(s, playerPos))

	glm::quat playerRot;
	wrapBool(io::readQuat(s, playerRot))

	float pitch;
	wrap(s.readFloat(pitch))

	float yaw;
	wrap(s.readFloat(yaw))

	if (header.version >= 170) {
		glm::quat orientation;
		wrapBool(io::readQuat(s, orientation))

		glm::quat cameraOrientation;
		wrapBool(io::readQuat(s, cameraOrientation))
	}

	glm::vec3 velocity;
	wrapBool(io::readVec3(s, velocity))

	float health;
	wrap(s.readFloat(health))

	float transitionTimer;
	wrap(s.readFloat(transitionTimer))

	float timeUnderwater;
	wrap(s.readFloat(timeUnderwater))

	float bluetideTimer;
	wrap(s.readFloat(bluetideTimer))

	float bluetidePower;
	wrap(s.readFloat(bluetidePower))

	if (header.version >= 160) {
		float animator;
		wrap(s.readFloat(animator))
	}

	// environment
	core::String skyBoxTexture;
	wrapBool(s.readString(TDStringLength, skyBoxTexture, true))
	Log::debug("Environment texture: %s", skyBoxTexture.c_str());

	glm::vec4 skyBoxTint;
	wrapBool(io::readVec4(s, skyBoxTint))

	float skyBoxBrightness;
	wrap(s.readFloat(skyBoxBrightness))

	float skyBoxRot;
	wrap(s.readFloat(skyBoxRot))

	glm::vec3 sunTintBrightness;
	wrapBool(io::readVec3(s, sunTintBrightness))

	glm::vec4 sunColorTint;
	wrapBool(io::readColor(s, sunColorTint))

	glm::vec3 sunDir;
	wrapBool(io::readVec3(s, sunDir))

	float sunBrightness;
	wrap(s.readFloat(sunBrightness))

	float sunSpread;
	wrap(s.readFloat(sunSpread))

	float sunLength;
	wrap(s.readFloat(sunLength))

	float sunFogScale;
	wrap(s.readFloat(sunFogScale))

	float sunGlare;
	wrap(s.readFloat(sunGlare))

	/*bool autoDir =*/s.readBool();

	glm::vec4 skyBoxConstant;
	wrapBool(io::readVec4(s, skyBoxConstant))

	float skyBoxAmbient;
	wrap(s.readFloat(skyBoxAmbient))

	float skyBoxAmbientExponent;
	wrap(s.readFloat(skyBoxAmbientExponent))

	glm::vec2 envExposure;
	wrapBool(io::readVec2(s, envExposure))

	float envBrightness;
	wrap(s.readFloat(envBrightness))

	uint8_t type = 0;
	if (header.version >= 160) {
		wrap(s.readUInt8(type))
	}
	glm::vec4 fogColor;
	wrapBool(io::readColor(s, fogColor))

	glm::vec4 fogParameters;
	wrapBool(io::readVec4(s, fogParameters))
	float fogHeightOffset = 0.0f;
	if (header.version >= 160) {
		wrap(s.readFloat(fogHeightOffset))
	}

	float waterWetness;
	wrap(s.readFloat(waterWetness))

	float waterPuddleAmount;
	wrap(s.readFloat(waterPuddleAmount))

	float waterPuddleSize;
	wrap(s.readFloat(waterPuddleSize))

	float waterRain;
	wrap(s.readFloat(waterRain))

	/*bool envHighlight =*/s.readBool();

	core::String envAmbientPath;
	wrapBool(s.readString(TDStringLength, envAmbientPath, true))
	Log::debug("Env ambient path: %s", envAmbientPath.c_str());

	float envAmbientVolume;
	wrap(s.readFloat(envAmbientVolume))

	float envSlippery;
	wrap(s.readFloat(envSlippery))

	float envFogScale;
	wrap(s.readFloat(envFogScale))

	glm::vec4 snowDir;
	wrapBool(io::readVec4(s, snowDir))

	glm::vec2 snowAmount;
	wrapBool(io::readVec2(s, snowAmount))

	/* bool snowOnGround = */ s.readBool();

	glm::vec3 envWind;
	wrapBool(io::readVec3(s, envWind))

	float envWaterHurt;
	wrap(s.readFloat(envWaterHurt))

	if (header.version >= 163) {
		core::String envLensDirt;
		wrapBool(s.readString(TDStringLength, envLensDirt, true))
		Log::debug("Env lens dirt: %s", envLensDirt.c_str());
	}

	// boundary
	uint32_t vertexCount;
	wrap(s.readUInt32(vertexCount))
	for (uint32_t i = 0; i < vertexCount; ++i) {
		glm::vec2 vertex;
		wrapBool(io::readVec2(s, vertex))
	}
	float boundaryPadLeft;
	wrap(s.readFloat(boundaryPadLeft))
	float boundaryPadTop;
	wrap(s.readFloat(boundaryPadTop))
	float boundaryPadRight;
	wrap(s.readFloat(boundaryPadRight))
	float boundaryPadBottom;
	wrap(s.readFloat(boundaryPadBottom))
	float boundaryMaxHeight;
	wrap(s.readFloat(boundaryMaxHeight))

	uint32_t fireCount;
	wrap(s.readUInt32(fireCount))
	for (uint32_t i = 0; i < fireCount; ++i) {
		uint32_t fireShape;
		wrap(s.readUInt32(fireShape))
		glm::vec3 firePos;
		wrapBool(io::readVec3(s, firePos))
		float fireMaxTime;
		wrap(s.readFloat(fireMaxTime))
		float fireTime;
		wrap(s.readFloat(fireTime))
		/*bool firePainted =*/s.readBool();
		/*bool fireBroken =*/s.readBool();
		uint32_t fireSpawnCount;
		wrap(s.readUInt32(fireSpawnCount))
	}

	uint32_t paletteCount;
	wrap(s.readUInt32(paletteCount))
	header.palettes.resize(paletteCount);
	for (uint32_t i = 0; i < paletteCount; ++i) {
		palette::Palette &palette = header.palettes[i];
		palette.setSize(256);
		for (int j = 0; j < 256; j++) {
			uint8_t mattype;
			wrap(s.readUInt8(mattype))
			glm::vec4 rgba;
			wrapBool(io::readVec4(s, rgba))
			palette.setColor(j, core::Color::getRGBA(rgba));
			float reflectivity;
			wrap(s.readFloat(reflectivity))
			float shinyness;
			wrap(s.readFloat(shinyness))
			float metalness;
			wrap(s.readFloat(metalness))
			float emissive;
			wrap(s.readFloat(emissive))
			/*bool isTint =*/s.readBool();

			palette::Material material;
			if (rgba.a < 1.0f) {
				material.type = palette::MaterialType::Glass;
				material.roughness = 1.0f - shinyness;
				material.indexOfRefraction = 1.5f; // typical glass IOR
				material.setValue(palette::MaterialProperty::MaterialRoughness, material.roughness);
				material.setValue(palette::MaterialProperty::MaterialIndexOfRefraction, material.indexOfRefraction);
			} else if (emissive > 0.0f) {
				material.type = palette::MaterialType::Emit;
				int flux = 0;
				if (emissive > 100.0f)
					flux = 4;
				else if (emissive > 10.0f)
					flux = 3;
				else if (emissive > 1.0f)
					flux = 2;
				else if (emissive > 0.1f)
					flux = 1;
				float emission = emissive / glm::pow(10.0f, (float)(flux - 1));
				material.emit = glm::clamp(emission, 0.0f, 1.0f);
				material.flux = (float)flux;
				material.setValue(palette::MaterialProperty::MaterialEmit, material.emit);
				material.setValue(palette::MaterialProperty::MaterialFlux, material.flux);
			} else if (reflectivity > 0.0f || shinyness > 0.0f || metalness > 0.0f) {
				// Metal material
				material.type = palette::MaterialType::Metal;
				material.roughness = 1.0f - shinyness;
				material.specular = glm::clamp(1.0f + reflectivity, 0.0f, 1.0f);
				material.metal = metalness;
				material.setValue(palette::MaterialProperty::MaterialRoughness, material.roughness);
				material.setValue(palette::MaterialProperty::MaterialSpecular, material.specular);
				material.setValue(palette::MaterialProperty::MaterialMetal, material.metal);
			} else {
				// Default diffuse material
				material.type = palette::MaterialType::Diffuse;
			}
			palette.setMaterial(j, material);
		}
		/*bool hasTransparent =*/s.readBool();
#if 1
		s.skip(256 * 3 * sizeof(core::RGBA));
#else
		core::RGBA blackTint[256];
		for (int k = 0; k < 256; ++k) {
			if (!io::readColor(s, blackTint[k])) {
				Log::error("Could not read black tint for palette %u index %u", i, k);
				return false;
			}
		}
		core::RGBA yellowTint[256];
		for (int k = 0; k < 256; ++k) {
			if (!io::readColor(s, yellowTint[k])) {
				Log::error("Could not read yellow tint for palette %u index %u", i, k);
				return false;
			}
		}
		core::RGBA rgbaTint[256];
		for (int k = 0; k < 256; ++k) {
			if (!io::readColor(s, rgbaTint[k])) {
				Log::error("Could not read RGBA tint for palette %u index %u", i, k);
				return false;
			}
		}
#endif
	}

	uint32_t registryCount;
	wrap(s.readUInt32(registryCount))
	scenegraph::SceneGraphNode &root = sceneGraph.node(0);
	for (uint32_t i = 0; i < registryCount; ++i) {
		core::String key;
		wrapBool(s.readString(TDStringLength, key, true))
		core::String value;
		wrapBool(s.readString(TDStringLength * 10, value, true))
		root.setProperty(key, value);
	}

	uint32_t topEntityCount;
	wrap(s.readUInt32(topEntityCount))
	Log::debug("%u top entities", topEntityCount);
	for (uint32_t i = 0; i < topEntityCount; ++i) {
		int nodeId = InvalidNodeId;
		if (!readEntity(header, sceneGraph, s, sceneGraph.root().id(), nodeId)) {
			Log::error("Failed to read top entity %u/%u", i, topEntityCount);
			return false;
		}
	}

	// TODO: VOXELFORMAT: parsing fails after this point - need to investigate
#if 0
	uint32_t projectileCount;
	wrap(s.readUInt32(projectileCount))
	Log::debug("Projectile count: %u", projectileCount);
	for (uint32_t i = 0; i < projectileCount; ++i) {
		glm::vec3 origin;
		wrapBool(io::readVec3(s, origin))

		glm::vec3 direction;
		wrapBool(io::readVec3(s, direction))

		float dist;
		wrap(s.readFloat(dist))

		float maxDist;
		wrap(s.readFloat(maxDist))

		uint32_t projectileType;
		wrap(s.readUInt32(projectileType))

		float strength;
		wrap(s.readFloat(strength))
	}

	/* bool hasSnow = */ s.readBool();
	uint32_t assetCount;
	wrap(s.readUInt32(assetCount))
	Log::debug("Asset count: %u", assetCount);
	for (uint32_t i = 0; i < assetCount; ++i) {
		core::String folder;
		wrapBool(s.readString(TDStringLength, folder, true))
		/* bool doOverride = */ s.readBool();
	}
#endif

	sceneGraph.updateTransforms();

	return true;
}

bool TeardownFormat::readBody(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	glm::vec3 velocity;
	wrapBool(io::readVec3(s, velocity))
	glm::vec3 angularVelocity;
	wrapBool(io::readVec3(s, angularVelocity))
	/*bool dynamic =*/s.readBool();
	uint8_t active;
	wrap(s.readUInt8(active))
	float friction;
	wrap(s.readFloat(friction))
	uint8_t frictionMode;
	wrap(s.readUInt8(frictionMode))
	float restitution;
	wrap(s.readFloat(restitution))
	uint8_t restitutionMode;
	wrap(s.readUInt8(restitutionMode))
	return true;
}

bool TeardownFormat::readVoxels(const Header &header, scenegraph::SceneGraphNode &node, io::ReadStream &s) {
	uint32_t sx, sy, sz;
	wrap(s.readUInt32(sx))
	wrap(s.readUInt32(sy))
	wrap(s.readUInt32(sz))
	const uint64_t voxelCnt = (uint64_t)sx * (uint64_t)sy * (uint64_t)sz;
	if (voxelCnt > 0) {
		// Teardown (Z-up): X, Y, Z -> Vengi (Y-up): X, Z, -Y (swap Y and Z, negate new Z)
		voxel::Region region(0, 0, 0, sx - 1, sz - 1, sy - 1);
		if (!region.isValid()) {
			Log::error("The region is invalid: %i:%i:%i", sx - 1, sz - 1, sy - 1);
			return false;
		}
		voxel::RawVolume *v = new voxel::RawVolume(region);
		node.setVolume(v, true);
		uint32_t encoded;
		wrap(s.readUInt32(encoded))
		// Run-length encoding: pairs of (run_length, palette_index)
		// Voxels are stored in XYZ order (X changes fastest)
		uint32_t voxelIndex = 0;
		for (uint32_t i = 0; i < encoded / 2; ++i) {
			uint8_t rl, idx;
			wrap(s.readUInt8(rl))
			wrap(s.readUInt8(idx))

			// Decode run-length encoded voxels
			const uint32_t runLength = (uint32_t)rl + 1; // run length is stored as n-1
			for (uint32_t r = 0; r < runLength && voxelIndex < voxelCnt; ++r) {
				// Convert linear index to 3D coordinates (Teardown XYZ order where Z is up)
				const uint32_t x = voxelIndex % sx;
				const uint32_t y = (voxelIndex / sx) % sy;
				const uint32_t z = voxelIndex / (sx * sy);

				// Create voxel with palette index
				if (idx > 0) { // 0 is empty/air
					const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, idx);
					// Map Teardown coordinates (X, Y, Z) to Vengi coordinates (X, Z, -Y)
					// Teardown: Z-up, Y-forward -> Vengi: Y-up, Z-back
					v->setVoxel((int)x, (int)z, (int)(sy - 1 - y), voxel);
				}
				++voxelIndex;
			}
		}
	}
	uint32_t paletteId;
	wrap(s.readUInt32(paletteId))
	if (paletteId < header.palettes.size()) {
		node.setPalette(header.palettes[paletteId]);
	}
	float scale;
	wrap(s.readFloat(scale))
	for (int i = 0; i < 8; ++i) {
		uint8_t lightMask;
		wrap(s.readUInt8(lightMask))
	}
	/*bool isDisconnected =*/s.readBool();
	return true;
}

bool TeardownFormat::readShape(const Header &header, scenegraph::SceneGraph &sceneGraph, io::ReadStream &s, int parent,
							   int &nodeId) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	uint16_t shapeFlags;
	wrap(s.readUInt16(shapeFlags))
	uint8_t collisionLayer;
	wrap(s.readUInt8(collisionLayer))
	uint8_t collisionMask;
	wrap(s.readUInt8(collisionMask))
	float density;
	wrap(s.readFloat(density))
	float strength;
	wrap(s.readFloat(strength))

	uint16_t texTile;
	wrap(s.readUInt16(texTile))
	uint16_t blendTile;
	wrap(s.readUInt16(blendTile))
	float texWeight;
	wrap(s.readFloat(texWeight))
	float blendWeight;
	wrap(s.readFloat(blendWeight))
	glm::vec3 textureOffset;
	wrapBool(io::readVec3(s, textureOffset))

	float emissiveScale;
	wrap(s.readFloat(emissiveScale))
	/*bool isBroken =*/s.readBool();
	uint8_t hasVoxels;
	wrap(s.readUInt8(hasVoxels))
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	wrapBool(readVoxels(header, node, s))
	if (hasVoxels && node.volume() != nullptr) {
		setTransform(node, pos, rot);
		node.setName("Shape");
		nodeId = sceneGraph.emplace(core::move(node), parent);
	}

	uint8_t origin;
	wrap(s.readUInt8(origin))
	if (header.version >= 160) {
		uint32_t animator;
		wrap(s.readUInt32(animator))
	}
	return true;
}

bool TeardownFormat::readLight(io::ReadStream &s) {
	/*bool enabled =*/s.readBool();
	uint8_t type;
	wrap(s.readUInt8(type))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	glm::vec4 color;
	wrapBool(io::readColor(s, color))
	float scale;
	wrap(s.readFloat(scale))
	float reach;
	wrap(s.readFloat(reach))
	float size;
	wrap(s.readFloat(size))
	float unshadowed;
	wrap(s.readFloat(unshadowed))
	float angle;
	wrap(s.readFloat(angle))
	float penumbra;
	wrap(s.readFloat(penumbra))
	float fogiter;
	wrap(s.readFloat(fogiter))
	float fogscale;
	wrap(s.readFloat(fogscale))
	float areaSize0;
	wrap(s.readFloat(areaSize0))
	float areaSize1;
	wrap(s.readFloat(areaSize1))
	float capsule;
	wrap(s.readFloat(capsule))
	glm::vec3 position;
	wrapBool(io::readVec3(s, position))
	uint8_t index;
	wrap(s.readUInt8(index))
	float flickering;
	wrap(s.readFloat(flickering))
	core::String soundPath;
	wrapBool(s.readString(TDStringLength, soundPath, true))
	float soundVol;
	wrap(s.readFloat(soundVol))
	float glare;
	wrap(s.readFloat(glare))
	return true;
}

bool TeardownFormat::readLocation(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	return true;
}

bool TeardownFormat::readWater(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	float depth;
	wrap(s.readFloat(depth))
	float wave;
	wrap(s.readFloat(wave))
	float ripple;
	wrap(s.readFloat(ripple))
	float motion;
	wrap(s.readFloat(motion))
	float foam;
	wrap(s.readFloat(foam))
	glm::vec4 color;
	wrapBool(io::readColor(s, color))
	float visibility;
	wrap(s.readFloat(visibility))
	uint32_t vertexCount;
	wrap(s.readUInt32(vertexCount))
	for (uint32_t i = 0; i < vertexCount; ++i) {
		glm::vec2 v;
		wrapBool(io::readVec2(s, v))
	}
	return true;
}

bool TeardownFormat::readRope(io::ReadStream &s) {
	glm::vec4 color;
	wrapBool(io::readColor(s, color))
	float zero;
	wrap(s.readFloat(zero))
	float strength;
	wrap(s.readFloat(strength))
	float maxStretch;
	wrap(s.readFloat(maxStretch))
	float slack;
	wrap(s.readFloat(slack))
	float segLen;
	wrap(s.readFloat(segLen))
	uint8_t active;
	wrap(s.readUInt8(active))
	uint32_t segments;
	wrap(s.readUInt32(segments))
	for (uint32_t i = 0; i < segments; ++i) {
		glm::vec3 from;
		wrapBool(io::readVec3(s, from))
		glm::vec3 to;
		wrapBool(io::readVec3(s, to))
	}
	return true;
}

bool TeardownFormat::readJoint(io::ReadStream &s) {
	uint32_t type;
	wrap(s.readUInt32(type))
	uint32_t shape0, shape1;
	wrap(s.readUInt32(shape0))
	wrap(s.readUInt32(shape1))
	glm::vec3 pos0;
	wrapBool(io::readVec3(s, pos0))
	glm::vec3 pos1;
	wrapBool(io::readVec3(s, pos1))
	glm::vec3 axis0;
	wrapBool(io::readVec3(s, axis0))
	glm::vec3 axis1;
	wrapBool(io::readVec3(s, axis1))
	/*bool connected =*/s.readBool();
	/*bool collide =*/s.readBool();
	float rotstrength;
	wrap(s.readFloat(rotstrength))
	float rotspring;
	wrap(s.readFloat(rotspring))
	glm::quat hingeRot;
	wrapBool(io::readQuat(s, hingeRot))
	glm::vec2 limits;
	wrapBool(io::readVec2(s, limits))
	float maxvel;
	wrap(s.readFloat(maxvel))
	float strength;
	wrap(s.readFloat(strength))
	float size;
	wrap(s.readFloat(size))
	/*bool sound =*/s.readBool();
	/*bool autodisable =*/s.readBool();
	float connStrength;
	wrap(s.readFloat(connStrength))
	float disconnect;
	wrap(s.readFloat(disconnect))
	if (type == 4 /*Rope*/) {
		wrapBool(readRope(s))
	}
	return true;
}

bool TeardownFormat::readVehicle(const Header &header, io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	uint32_t body;
	wrap(s.readUInt32(body))
	glm::vec3 pos1;
	glm::quat rot1;
	wrapBool(readTransform(s, pos1, rot1))
	glm::vec3 pos2;
	glm::quat rot2;
	wrapBool(readTransform(s, pos2, rot2))
	uint32_t wheelCount;
	wrap(s.readUInt32(wheelCount))
	for (uint32_t i = 0; i < wheelCount; ++i) {
		uint32_t tmp;
		wrap(s.readUInt32(tmp))
	}
	float topspeed;
	wrap(s.readFloat(topspeed))
	float topClamp;
	wrap(s.readFloat(topClamp))
	float spring;
	wrap(s.readFloat(spring))
	float damping;
	wrap(s.readFloat(damping))
	float accel;
	wrap(s.readFloat(accel))
	float strength;
	wrap(s.readFloat(strength))
	float friction;
	wrap(s.readFloat(friction))
	float maxSteer;
	wrap(s.readFloat(maxSteer))
	/* bool handbrake = */ s.readBool();
	float antispin;
	wrap(s.readFloat(antispin))
	float steerassist;
	wrap(s.readFloat(steerassist))
	float assistmul;
	wrap(s.readFloat(assistmul))
	float antiroll;
	wrap(s.readFloat(antiroll))
	core::String sndPath;
	wrapBool(s.readString(TDStringLength, sndPath, true))
	float sndVol;
	wrap(s.readFloat(sndVol))
	glm::vec3 camera;
	wrapBool(io::readVec3(s, camera))
	glm::vec3 player;
	wrapBool(io::readVec3(s, player))
	glm::vec3 exit;
	wrapBool(io::readVec3(s, exit))
	glm::vec3 propeller;
	wrapBool(io::readVec3(s, propeller))
	float difflock;
	wrap(s.readFloat(difflock))
	float health;
	wrap(s.readFloat(health))
	uint32_t mainVoxel;
	wrap(s.readUInt32(mainVoxel))
	/* braking */ s.readBool();
	float passiveBrake;
	wrap(s.readFloat(passiveBrake))
	uint32_t refCount;
	wrap(s.readUInt32(refCount))
	for (uint32_t i = 0; i < refCount; ++i) {
		uint32_t ref;
		wrap(s.readUInt32(ref))
	}
	uint32_t exhaustCount;
	wrap(s.readUInt32(exhaustCount))
	for (uint32_t i = 0; i < exhaustCount; ++i) {
		glm::vec3 pos;
		glm::quat rot;
		wrapBool(readTransform(s, pos, rot))
		float str;
		wrap(s.readFloat(str))
	}
	uint32_t vitalCount;
	wrap(s.readUInt32(vitalCount))
	for (uint32_t i = 0; i < vitalCount; ++i) {
		uint32_t b;
		wrap(s.readUInt32(b));
		glm::vec3 pos;
		wrapBool(io::readVec3(s, pos))
		float r;
		wrap(s.readFloat(r));
		uint32_t nv;
		wrap(s.readUInt32(nv));
	}
	if (header.version >= 160) {
		uint32_t animCount;
		wrap(s.readUInt32(animCount))
		for (uint32_t i = 0; i < animCount; ++i) {
			core::String name;
			wrapBool(s.readString(TDStringLength, name, true))
			glm::vec3 pos;
			glm::quat rot;
			wrapBool(readTransform(s, pos, rot))
			uint32_t h;
			wrap(s.readUInt32(h))
		}
	}
	float bounds;
	wrap(s.readFloat(bounds))
	/* bool noroll = */ s.readBool();
	float breakth;
	wrap(s.readFloat(breakth))
	float smoke;
	wrap(s.readFloat(smoke))
	return true;
}

bool TeardownFormat::readWheel(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	uint32_t vehicle;
	wrap(s.readUInt32(vehicle))
	uint32_t vehicleBody;
	wrap(s.readUInt32(vehicleBody))
	uint32_t body;
	wrap(s.readUInt32(body))
	uint32_t shape;
	wrap(s.readUInt32(shape))
	uint32_t groundShape;
	wrap(s.readUInt32(groundShape))
	for (int i = 0; i < 3; ++i) {
		uint32_t v;
		wrap(s.readUInt32(v))
	}
	/* bool onGround = */ s.readBool();
	glm::vec3 pos1;
	glm::quat rot1;
	wrapBool(readTransform(s, pos1, rot1))
	glm::vec3 pos2;
	glm::quat rot2;
	wrapBool(readTransform(s, pos2, rot2))
	float steer;
	wrap(s.readFloat(steer))
	float drive;
	wrap(s.readFloat(drive))
	glm::vec2 travel;
	wrapBool(io::readVec2(s, travel))
	float radius;
	wrap(s.readFloat(radius))
	float width;
	wrap(s.readFloat(width))
	float angularSpeed;
	wrap(s.readFloat(angularSpeed))
	float stance;
	wrap(s.readFloat(stance))
	float verticalOffset;
	wrap(s.readFloat(verticalOffset))
	return true;
}

bool TeardownFormat::readScreen(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	glm::vec2 size;
	wrapBool(io::readVec2(s, size))
	float bulge;
	wrap(s.readFloat(bulge))
	uint32_t resX, resY;
	wrap(s.readUInt32(resX))
	wrap(s.readUInt32(resY))
	core::String script;
	wrapBool(s.readString(TDStringLength, script, true))
	/*bool enabled =*/s.readBool();
	/*bool interactive =*/s.readBool();
	float emissive;
	wrap(s.readFloat(emissive))
	float fxraster;
	wrap(s.readFloat(fxraster))
	float fxca;
	wrap(s.readFloat(fxca))
	float fxnoise;
	wrap(s.readFloat(fxnoise))
	float fxglitch;
	wrap(s.readFloat(fxglitch))
	return true;
}

bool TeardownFormat::readTrigger(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	uint32_t type;
	wrap(s.readUInt32(type))
	float sphere;
	wrap(s.readFloat(sphere))
	glm::vec3 boxSize;
	wrapBool(io::readVec3(s, boxSize))
	float polygon;
	wrap(s.readFloat(polygon))
	uint32_t vertexCount;
	wrap(s.readUInt32(vertexCount))
	for (uint32_t i = 0; i < vertexCount; ++i) {
		glm::vec2 v;
		wrapBool(io::readVec2(s, v))
	}
	core::String path;
	wrapBool(s.readString(TDStringLength, path, true))
	float ramp;
	wrap(s.readFloat(ramp))
	uint8_t stype;
	wrap(s.readUInt8(stype))
	float volume;
	wrap(s.readFloat(volume))
	return true;
}

bool TeardownFormat::readScript(io::ReadStream &s) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	core::String file;
	wrapBool(s.readString(TDStringLength, file, true))
	Log::debug("Script file %s", file.c_str());

	uint32_t entries;
	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		core::String k, v;
		wrapBool(s.readString(TDStringLength, k, true))
		wrapBool(s.readString(TDStringLength * 10, v, true))
		Log::debug("Key: '%s': '%s'", k.c_str(), v.c_str());
	}

	float tick;
	wrap(s.readFloat(tick))
	float update;
	wrap(s.readFloat(update))

	uint32_t varCount;
	wrap(s.readUInt32(varCount))
	wrapBool(readLuaTable(s))

	uint32_t entityCount;
	wrap(s.readUInt32(entityCount))
	for (uint32_t i = 0; i < entityCount; ++i) {
		uint32_t e;
		wrap(s.readUInt32(e))
	}

	uint32_t soundCount;
	wrap(s.readUInt32(soundCount))
	for (uint32_t i = 0; i < soundCount; ++i) {
		uint32_t t;
		wrap(s.readUInt32(t));
		core::String n;
		wrapBool(s.readString(TDStringLength, n, true))
	}

	uint32_t transitionCount;
	wrap(s.readUInt32(transitionCount))
	for (uint32_t i = 0; i < transitionCount; ++i) {
		core::String var;
		wrapBool(s.readString(TDStringLength, var, true))
		uint8_t trans;
		wrap(s.readUInt8(trans));
		float tt, ct, cv, tv;
		wrap(s.readFloat(tt))
		wrap(s.readFloat(ct))
		wrap(s.readFloat(cv))
		wrap(s.readFloat(tv))
	}
	return true;
}

bool TeardownFormat::readLuaValue(io::ReadStream &s, int typeId) {
	// lua_table.h defines the numeric values as:
	// NIL = 0, Boolean = 1, Number = 3, String = 4, Table = 5, Reference = 0xFFFFFFFB
	const uint32_t refType = 0xFFFFFFFBu;
	switch ((uint32_t)typeId) {
	case 1u: // Boolean
		(void)s.readBool();
		return true;
	case 3u: { // Number (double)
		double d;
		return s.readDouble(d) == 0;
	}
	case 4u: { // String
		core::String v;
		return s.readString(TDStringLength * 10, v, true);
	}
	case 5u: // Table
		return readLuaTable(s);
	case refType: { // Reference (signed -5 stored as 0xFFFFFFFB)
		uint32_t ref;
		return s.readUInt32(ref) == 0;
	}
	case 0u: // NIL
	default:
		return true;
	}
}

bool TeardownFormat::readLuaTable(io::ReadStream &s) {
	Log::debug("Read Lua table");
	// sequence of (key_type, key, value_type, value) terminated by key_type == NIL (0)
	uint32_t entryIdx = 0;
	const uint32_t refType = 0xFFFFFFFBu;
	for (;;) {
		uint32_t keyType;
		wrap(s.readUInt32(keyType))
		if (keyType == 0u) {
			// NIL marks end
			Log::debug("readLuaTable: terminated after %u entries", entryIdx);
			return true;
		}
		// Accept the expected LuaType values: 0,1,3,4,5 and the special Reference value 0xFFFFFFFB
		if (!(keyType == 0u || keyType == 1u || keyType == 3u || keyType == 4u || keyType == 5u ||
			  keyType == refType)) {
			Log::error("readLuaTable: invalid keyType %u at entry %u", keyType, entryIdx);
			return false;
		}

		wrapBool(readLuaValue(s, (int)keyType))

		uint32_t valueType;
		wrap(s.readUInt32(valueType))
		if (!(valueType == 0u || valueType == 1u || valueType == 3u || valueType == 4u || valueType == 5u ||
			  valueType == refType)) {
			Log::error("readLuaTable: invalid valueType %u at entry %u", valueType, entryIdx);
			return false;
		}

		wrapBool(readLuaValue(s, (int)valueType))

		entryIdx++;
	}
	return true;
}

bool TeardownFormat::readAnimator(const Header &header, io::ReadStream &s, int parent, int &nodeId) {
	uint16_t flags;
	wrap(s.readUInt16(flags))
	glm::vec3 pos;
	glm::quat rot;
	wrapBool(readTransform(s, pos, rot))
	core::String path;
	wrapBool(s.readString(TDStringLength, path, true))
	s.readBool();

	uint32_t entries;
	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		uint32_t a, b, c;
		wrap(s.readUInt32(a))
		wrap(s.readUInt32(b))
		wrap(s.readUInt32(c));
		core::String str;
		wrapBool(s.readString(TDStringLength, str, true))
	}

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		glm::vec3 p;
		glm::quat r;
		wrapBool(readTransform(s, p, r))
		glm::vec2 v1;
		wrapBool(io::readVec2(s, v1))
		glm::vec2 v2;
		wrapBool(io::readVec2(s, v2))
		float f1, f2;
		wrap(s.readFloat(f1))
		wrap(s.readFloat(f2))
		uint8_t b1, b2;
		wrap(s.readUInt8(b1))
		wrap(s.readUInt8(b2))
		uint32_t i1, i2, i3, i4;
		wrap(s.readUInt32(i1))
		wrap(s.readUInt32(i2))
		wrap(s.readUInt32(i3))
		wrap(s.readUInt32(i4))
		glm::quat q;
		wrapBool(io::readQuat(s, q))
		glm::vec3 v3_1, v3_2, v3_3, v3_4;
		wrapBool(io::readVec3(s, v3_1))
		wrapBool(io::readVec3(s, v3_2))
		wrapBool(io::readVec3(s, v3_3))
		wrapBool(io::readVec3(s, v3_4))
		uint32_t i5, i6;
		wrap(s.readUInt32(i5))
		wrap(s.readUInt32(i6))
	}

	wrap(s.readUInt32(entries))
	// scenegraph::SceneGraphNode groupNode(scenegraph::SceneGraphNodeType::Group);
	// groupNode.setName(path);
	// nodeId = sceneGraph.emplace(core::move(groupNode), parent);
	for (uint32_t i = 0; i < entries; ++i) {
		uint32_t i1;
		wrap(s.readUInt32(i1))
		glm::vec3 p2;
		glm::quat r2;
		wrapBool(readTransform(s, p2, r2))
		float f1, f2;
		wrap(s.readFloat(f1))
		wrap(s.readFloat(f2))
		uint32_t i2, i3;
		wrap(s.readUInt32(i2))
		wrap(s.readUInt32(i3))
		uint8_t b1, b2, b3, b4;
		wrap(s.readUInt8(b1))
		wrap(s.readUInt8(b2))
		wrap(s.readUInt8(b3))
		wrap(s.readUInt8(b4))
		(void)s.readBool();
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		wrapBool(readVoxels(header, node, s))
		// setTransform(node, p2, r2);
		// sceneGraph.emplace(core::move(node), nodeId);
	}

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		core::String str;
		wrapBool(s.readString(TDStringLength, str, true))
		uint8_t tmp[56];
		if (s.read(tmp, sizeof(tmp)) != (int)sizeof(tmp)) {
			return false;
		}
	}

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		core::String str;
		wrapBool(s.readString(TDStringLength, str, true))
		uint8_t tmp[128];
		if (s.read(tmp, sizeof(tmp)) != (int)sizeof(tmp)) {
			return false;
		}
	}

	uint32_t dummy;
	wrap(s.readUInt32(dummy))

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		uint8_t tmp[72];
		if (s.read(tmp, sizeof(tmp)) != (int)sizeof(tmp)) {
			return false;
		}
	}

	wrap(s.readUInt32(dummy))

	wrap(s.readUInt32(entries))
	if (entries > 0) {
		const size_t bytes = (size_t)entries * 8;
		if (s.skipDelta((int64_t)bytes) == -1) {
			return false;
		}
	}

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		core::String str;
		wrapBool(s.readString(TDStringLength, str, true))
	}

	wrap(s.readUInt32(entries))
	if (entries > 0) {
		const size_t bytes = (size_t)entries * 28;
		if (s.skipDelta((int64_t)bytes) == -1) {
			return false;
		}
	}

	wrap(s.readUInt32(entries))
	if (entries > 0) {
		const size_t bytes = (size_t)entries * 28;
		if (s.skipDelta((int64_t)bytes) == -1) {
			return false;
		}
	}

	wrap(s.readUInt32(entries))
	if (entries > 0) {
		const size_t bytes = (size_t)entries * 4;
		if (s.skipDelta((int64_t)bytes) == -1) {
			return false;
		}
	}

	wrap(s.readUInt32(entries))
	for (uint32_t i = 0; i < entries; ++i) {
		core::String str;
		wrapBool(s.readString(TDStringLength, str, true))
		glm::vec3 p3;
		glm::quat r3;
		wrapBool(readTransform(s, p3, r3))
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
