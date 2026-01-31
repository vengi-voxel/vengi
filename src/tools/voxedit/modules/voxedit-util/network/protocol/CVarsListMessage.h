/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Info about a single cvar
 */
struct CVarInfo {
	core::String name;
	core::String value;
	core::String description;
	uint32_t flags = 0;
};

/**
 * @brief Response containing the list of cvars
 */
class CVarsListMessage : public network::ProtocolMessage {
private:
	core::DynamicArray<CVarInfo> _cvars;

public:
	CVarsListMessage(const core::DynamicArray<CVarInfo> &cvars) : ProtocolMessage(PROTO_CVARS_LIST) {
		if (!writeUInt32((uint32_t)cvars.size())) {
			Log::error("Failed to write cvar count in CVarsListMessage ctor");
			return;
		}
		for (const auto &cvar : cvars) {
			if (!writePascalStringUInt16LE(cvar.name)) {
				Log::error("Failed to write cvar name in CVarsListMessage ctor");
				return;
			}
			if (!writePascalStringUInt16LE(cvar.value)) {
				Log::error("Failed to write cvar value in CVarsListMessage ctor");
				return;
			}
			if (!writePascalStringUInt16LE(cvar.description)) {
				Log::error("Failed to write cvar description in CVarsListMessage ctor");
				return;
			}
			if (!writeUInt32(cvar.flags)) {
				Log::error("Failed to write cvar flags in CVarsListMessage ctor");
				return;
			}
		}
		writeSize();
	}

	CVarsListMessage(network::MessageStream &in) {
		_id = PROTO_CVARS_LIST;
		uint32_t count = 0;
		if (in.readUInt32(count) == -1) {
			Log::error("Failed to read cvar count");
			return;
		}
		_cvars.reserve(count);
		for (uint32_t i = 0; i < count; ++i) {
			CVarInfo info;
			if (!in.readPascalStringUInt16LE(info.name)) {
				Log::error("Failed to read cvar name");
				return;
			}
			if (!in.readPascalStringUInt16LE(info.value)) {
				Log::error("Failed to read cvar value");
				return;
			}
			if (!in.readPascalStringUInt16LE(info.description)) {
				Log::error("Failed to read cvar description");
				return;
			}
			if (in.readUInt32(info.flags) == -1) {
				Log::error("Failed to read cvar flags");
				return;
			}
			_cvars.push_back(info);
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in CVarsListMessage::writeBack");
			return;
		}
		if (!writeUInt32((uint32_t)_cvars.size())) {
			Log::error("Failed to write cvar count in CVarsListMessage::writeBack");
			return;
		}
		for (const auto &cvar : _cvars) {
			if (!writePascalStringUInt16LE(cvar.name)) {
				Log::error("Failed to write cvar name in CVarsListMessage::writeBack");
				return;
			}
			if (!writePascalStringUInt16LE(cvar.value)) {
				Log::error("Failed to write cvar value in CVarsListMessage::writeBack");
				return;
			}
			if (!writePascalStringUInt16LE(cvar.description)) {
				Log::error("Failed to write cvar description in CVarsListMessage::writeBack");
				return;
			}
			if (!writeUInt32(cvar.flags)) {
				Log::error("Failed to write cvar flags in CVarsListMessage::writeBack");
				return;
			}
		}
		writeSize();
	}

	const core::DynamicArray<CVarInfo> &cvars() const {
		return _cvars;
	}
};

} // namespace voxedit
