/**
 * @file
 */

#include "AnimaToonFormat.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include <json.hpp>

namespace voxelformat {

#ifdef GLM_FORCE_QUAT_DATA_WXYZ
#error "GLM_FORCE_QUAT_DATA_WXYZ is not supported here"
#endif

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load scn file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

bool AnimaToonFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
									 scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
									 const LoadContext &ctx) {
	const int64_t size = stream.size();
	core::String str(size, ' ');
	if (!stream.readString((int)str.size(), str.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	nlohmann::json json = nlohmann::json::parse(str);
	core::String name = json.value("SceneName", "unknown").c_str();
	core::DynamicArray<int> modelNodeIds;
	for (const auto &e : json["ModelSave"]) {
		scenegraph::SceneGraphNode node;
		node.setName(name);
		node.setPalette(palette);
		io::BufferedReadWriteStream base64Stream;
		const core::String str = e.get<std::string>().c_str();
		if (!io::Base64::decode(base64Stream, str)) {
			Log::error("Failed to decode ModelSave array entry");
			return false;
		}
		base64Stream.seek(0);
		io::ZipReadStream readStream(base64Stream);
		glm::uvec3 size(40);
		const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
		voxel::RawVolume *volume = new voxel::RawVolume(region);
		node.setVolume(volume, true);
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					AnimaToonVoxel v;
					wrap(readStream.readUInt8((uint8_t &)v.state))
					wrap(readStream.readUInt8(v.val))
					wrap(readStream.readUInt32(v.rgba))
					if (v.rgba == 0) {
						continue;
					}
					const uint8_t color = palette.getClosestMatch(v.rgba);
					const voxel::Voxel voxel = voxel::createVoxel(palette, color);
					volume->setVoxel((int)x, (int)y, (int)z, voxel);
				}
			}
		}
		modelNodeIds.push_back(sceneGraph.emplace(core::move(node)));
	}

	// TODO: use me
#if 0
	for (const auto &savedPos : json["savedPositionsList"]) {
		const std::string innerJson = savedPos.get<std::string>();
		nlohmann::json inner = nlohmann::json::parse(innerJson);
		AnimaToonPosition pos;
		pos.isModified = inner.value("isModified", false);
		pos.isLeftHandClosed = inner.value("isLeftHandClosed", false);
		pos.isRightHandClosed = inner.value("isRightHandClosed", false);
		for (const auto &meshPos : inner["meshPositions"]) {
			const float x = meshPos.value("x", 0.0f);
			const float y = meshPos.value("y", 0.0f);
			const float z = meshPos.value("z", 0.0f);
			pos.meshPositions.push_back(glm::vec3(x, y, z));
		}
		for (const auto &meshRot : inner["meshRotations"]) {
			const float x = meshRot.value("x", 0.0f);
			const float y = meshRot.value("y", 0.0f);
			const float z = meshRot.value("z", 0.0f);
			const float w = meshRot.value("w", 0.0f);
			pos.meshRotations.push_back(glm::quat(x, y, z, w));
		}
		for (const auto &ikPos : inner["IKPositions"]) {
			const float x = ikPos.value("x", 0.0f);
			const float y = ikPos.value("y", 0.0f);
			const float z = ikPos.value("z", 0.0f);
			pos.ikPositions.push_back(glm::vec3(x, y, z));
		}
		for (const auto &ikRot : inner["IKRotations"]) {
			const float x = ikRot.value("x", 0.0f);
			const float y = ikRot.value("y", 0.0f);
			const float z = ikRot.value("z", 0.0f);
			const float w = ikRot.value("w", 0.0f);
			pos.ikRotations.push_back(glm::quat(x, y, z, w));
		}
		for (const auto &ikMod : inner["IKModified"]) {
			pos.ikModified.push_back(ikMod);
		}
	}
	for (const auto &e : json["customColors"]) {
		const core::RGBA color(e["r"], e["g"], e["b"], e["a"]);
	}
	for (const auto &e : json["ImportModelSave"]) {
	}
	auto mainCamPosIter = json.find("MainCamPosition");
	if (mainCamPosIter != json.end()) {
		const glm::vec3 pos((*mainCamPosIter).value("x", 0.0f), (*mainCamPosIter).value("y", 0.0f),
							(*mainCamPosIter).value("z", 0.0f));
	}
	auto mainCamRotIter = json.find("MainCamRotation");
	if (mainCamRotIter != json.end()) {
		const glm::quat rot((*mainCamRotIter).value("x", 0.0f), (*mainCamRotIter).value("y", 0.0f),
							(*mainCamRotIter).value("z", 0.0f), (*mainCamRotIter)["w"]);
	}
	auto camTargetPosIter = json.find("CamTargetPostion");
	if (camTargetPosIter != json.end()) {
		const glm::vec3 pos((*camTargetPosIter).value("x", 0.0f), (*camTargetPosIter).value("y", 0.0f),
							(*camTargetPosIter).value("z", 0.0f));
	}
#endif
	return true;
}

#undef wrap

} // namespace voxelformat
