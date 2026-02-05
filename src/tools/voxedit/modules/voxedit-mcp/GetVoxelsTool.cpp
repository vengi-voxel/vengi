/**
 * @file
 */

#include "GetVoxelsTool.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

GetVoxelsTool::GetVoxelsTool() : Tool("voxedit_get_voxels") {
	_tool["description"] = "Get voxel data from a model node. Returns compact binary data (base64) "
						   "in either SPARSE or RLE format, whichever is smaller.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

static void encodeSparse(const voxel::RawVolume *volume, io::BufferedReadWriteStream &stream) {
	const voxel::Region &region = volume->region();
	auto func = [&](int32_t x, int32_t y, int32_t z, const voxel::Voxel &voxel) {
		stream.writeUInt8((uint8_t)(x - region.getLowerX()));
		stream.writeUInt8((uint8_t)(y - region.getLowerY()));
		stream.writeUInt8((uint8_t)(z - region.getLowerZ()));
		stream.writeUInt8(voxel.getColor());
	};
	voxelutil::visitVolume(*volume, func);
}

static void encodeRLE(const voxel::RawVolume *volume, io::BufferedReadWriteStream &stream) {
	auto flushRun = [&stream](uint8_t color, uint16_t cnt) {
		if (cnt == 0) {
			return;
		}
		stream.writeUInt8(color);
		if (cnt < 128) {
			stream.writeUInt8((uint8_t)cnt);
		} else {
			stream.writeUInt8((uint8_t)(0x80 | (cnt & 0x7F)));
			stream.writeUInt8((uint8_t)(cnt >> 7));
		}
	};

	uint8_t currentColor = 0;
	uint16_t count = 0;
	auto func = [&](int32_t x, int32_t y, int32_t z, const voxel::Voxel &voxel) {
		uint8_t color = voxel::isAir(voxel.getMaterial()) ? 0 : voxel.getColor();
		if (color == currentColor && count < 0x3FFF) {
			++count;
		} else {
			flushRun(currentColor, count);
			currentColor = color;
			count = 1;
		}
	};
	voxelutil::visitVolume(*volume, func, voxelutil::VisitAll());
	flushRun(currentColor, count);
}

static core::String encodeVolumeToResponse(const voxel::RawVolume *volume) {
	const voxel::Region &region = volume->region();

	io::BufferedReadWriteStream sparseStream, rleStream;
	encodeSparse(volume, sparseStream);
	encodeRLE(volume, rleStream);

	const bool useRLE = rleStream.size() <= sparseStream.size();
	io::BufferedReadWriteStream &dataStream = useRLE ? rleStream : sparseStream;
	dataStream.seek(0);

	core::String result =
		core::String::format("FORMAT: %s\nDIMS: %d,%d,%d\nORIGIN: %d,%d,%d\nDATA: ", useRLE ? "RLE" : "SPARSE",
							 region.getWidthInVoxels(), region.getHeightInVoxels(), region.getDepthInVoxels(),
							 region.getLowerX(), region.getLowerY(), region.getLowerZ());
	result += io::Base64::encode(dataStream);
	result += "\n\n";

	if (useRLE) {
		result += "Decoding: RLE pairs (colorIndex:u8, count:varint). count<128 is 1 byte, "
				  "else 2 bytes (0x80|low7, high8). Color 0=air. Order: X->Y->Z.";
	} else {
		result += "Decoding: Each voxel is (x:u8, y:u8, z:u8, colorIndex:u8). "
				  "Coordinates relative to ORIGIN. Only non-air voxels.";
	}
	return result;
}

bool GetVoxelsTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}
	if (!node->isModelNode()) {
		return ctx.result(id, "Node is not a model node", true);
	}
	const voxel::RawVolume *volume = node->volume();
	if (volume == nullptr) {
		return ctx.result(id, "Node has no volume", true);
	}
	return ctx.result(id, encodeVolumeToResponse(volume), false);
}

} // namespace voxedit
