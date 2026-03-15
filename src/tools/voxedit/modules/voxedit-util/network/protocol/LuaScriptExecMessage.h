/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Message to execute a lua script synchronously on the server side.
 * The server will run the script to completion (including all yields) before sending an ack.
 */
class LuaScriptExecMessage : public network::ProtocolMessage {
private:
	core::String _rconPassword;
	core::String _scriptName;
	core::String _args;

public:
	LuaScriptExecMessage(const core::String &scriptName, const core::String &args, const core::String &rconPassword)
		: ProtocolMessage(PROTO_LUA_SCRIPT_EXEC) {
		_rconPassword = rconPassword;
		_scriptName = scriptName;
		_args = args;

		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password in LuaScriptExecMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_scriptName)) {
			Log::error("Failed to write script name in LuaScriptExecMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_args)) {
			Log::error("Failed to write args in LuaScriptExecMessage ctor");
			return;
		}
		writeSize();
	}

	LuaScriptExecMessage(network::MessageStream &in) {
		_id = PROTO_LUA_SCRIPT_EXEC;
		if (!in.readPascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to read rcon password");
			return;
		}
		if (!in.readPascalStringUInt16LE(_scriptName)) {
			Log::error("Failed to read script name");
			return;
		}
		if (!in.readPascalStringUInt16LE(_args)) {
			Log::error("Failed to read args");
			return;
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in LuaScriptExecMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password in LuaScriptExecMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_scriptName)) {
			Log::error("Failed to write script name in LuaScriptExecMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_args)) {
			Log::error("Failed to write args in LuaScriptExecMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::String &rconPassword() const {
		return _rconPassword;
	}

	const core::String &scriptName() const {
		return _scriptName;
	}

	const core::String &args() const {
		return _args;
	}
};

} // namespace voxedit
