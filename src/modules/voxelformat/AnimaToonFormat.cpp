/**
 * @file
 */

#include "AnimaToonFormat.h"
#include "core/Log.h"
#include "core/Tokenizer.h"
#include "core/collection/DynamicArray.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "util/Base64.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load scn file: Not enough data in stream " CORE_STRINGIFY(read)); \
		error = true; \
		return; \
	}

bool AnimaToonFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
									 SceneGraph &sceneGraph, const voxel::Palette &palette, const LoadContext &ctx) {
	const int64_t size = stream.size();
	core::String str(size, ' ');
	if (!stream.readString((int)str.size(), str.c_str())) {
		Log::error("Failed to read string from stream");
		return false;
	}

	bool error = false;
	core::Tokenizer tokenizer(str, " \t\n,:", "{}[]");
	core::String name = "unknown";
	while (tokenizer.hasNext()) {
		const core::String &token = tokenizer.next();
		if (token == "SceneName") {
			name = tokenizer.next();
		} else if (token == "ModelSave") {
			if (!parseJsonArray(tokenizer, [&error, &sceneGraph, &palette, &name] (const core::String &token) {
				io::BufferedReadWriteStream base64Stream;
				if (!util::Base64::decode(base64Stream, token)) {
					error = true;
					Log::error("Failed to decode ModelSave array entry");
					return;
				}
				base64Stream.seek(0);
				io::ZipReadStream readStream(base64Stream);
				glm::uvec3 size(32);
				const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
				voxel::RawVolume *volume = new voxel::RawVolume(region);
				SceneGraphNode node;
				node.setVolume(volume, true);
				node.setName(name);
				node.setPalette(palette);
				for (uint32_t x = 0; x < size.x; ++x) {
					for (uint32_t y = 0; y < size.y; ++y) {
						for (uint32_t z = 0; z < size.z; ++z) {
							AnimaToonVoxel v;
							wrap(readStream.readUInt8((uint8_t&)v.state))
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
				sceneGraph.emplace(core::move(node));
			})) {
				Log::error("Failed to parse ModelSave json array");
				return false;
			}
			if (error) {
				Log::error("There was an error in decoding the volume data");
				return false;
			}
		} else if (token == "savedPositionsList") {
			// TODO: meshPositions, meshRotations json AnimaToonPosition
		}
	}

	if (error) {
		Log::error("Failed to load scn");
		return false;
	}
	return true;
}

#undef wrap

} // namespace voxelformat
