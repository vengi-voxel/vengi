/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Message to create/send a new lua script to the server
 */
class LuaScriptCreateMessage : public network::ProtocolMessage {
private:
	core::String _rconPassword;
	core::String _name;
	core::String _content;

public:
	LuaScriptCreateMessage(const core::String &name, const core::String &content, const core::String &rconPassword)
		: ProtocolMessage(PROTO_LUA_SCRIPT_CREATE) {
		_rconPassword = rconPassword;
		_name = name;
		_content = content;

		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password in LuaScriptCreateMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_name)) {
			Log::error("Failed to write name in LuaScriptCreateMessage ctor");
			return;
		}
		if (!writePascalStringUInt32LE(_content)) {
			Log::error("Failed to write content in LuaScriptCreateMessage ctor");
			return;
		}
		writeSize();
	}

	LuaScriptCreateMessage(network::MessageStream &in) {
		_id = PROTO_LUA_SCRIPT_CREATE;
		if (!in.readPascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to read rcon password");
			return;
		}
		if (!in.readPascalStringUInt16LE(_name)) {
			Log::error("Failed to read name");
			return;
		}
		if (!in.readPascalStringUInt32LE(_content)) {
			Log::error("Failed to read content");
			return;
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in LuaScriptCreateMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password in LuaScriptCreateMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_name)) {
			Log::error("Failed to write name in LuaScriptCreateMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt32LE(_content)) {
			Log::error("Failed to write content in LuaScriptCreateMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::String &rconPassword() const {
		return _rconPassword;
	}

	const core::String &name() const {
		return _name;
	}

	const core::String &content() const {
		return _content;
	}
};

} // namespace voxedit
