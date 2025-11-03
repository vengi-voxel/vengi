/**
 * @file
 */

#pragma once

#include "core/ConfigVar.h"
#include "core/String.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/ProtocolVersion.h"

namespace voxedit {
namespace network {

/**
 * @brief Initial session handshake message containing protocol version, application version, and username
 */
class InitSessionMessage : public ProtocolMessage {
private:
	uint32_t _protocolVersion;
	core::String _applicationVersion;
	core::String _password;
	core::String _username;
	bool _localServer = false;

public:
	InitSessionMessage(bool localServer) : ProtocolMessage(PROTO_INIT_SESSION) {
		_protocolVersion = PROTOCOL_VERSION;
		_applicationVersion = core::Var::getSafe(cfg::AppVersion)->strVal();
		_password = core::Var::getSafe(cfg::VoxEditNetPassword)->strVal();
		_username = core::Var::getSafe(cfg::AppUserName)->strVal();

		if (!writeUInt32(_protocolVersion)) {
			Log::error("Failed to write protocol version in InitSessionMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_applicationVersion)) {
			Log::error("Failed to write application version in InitSessionMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_username)) {
			Log::error("Failed to write username in InitSessionMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_password)) {
			Log::error("Failed to write password in InitSessionMessage ctor");
			return;
		}
		if (!writeBool(localServer)) {
			Log::error("Failed to write localServer flag in InitSessionMessage ctor");
			return;
		}
		writeSize();
	}

	InitSessionMessage(MessageStream &in) {
		_id = PROTO_INIT_SESSION;
		if (in.readUInt32(_protocolVersion) == -1) {
			Log::error("Failed to read protocol version for init session");
			return;
		}
		if (!in.readPascalStringUInt16LE(_applicationVersion)) {
			Log::error("Failed to read application version for init session");
			return;
		}
		if (!in.readPascalStringUInt16LE(_username)) {
			Log::error("Failed to read username for init session");
			return;
		}
		if (!in.readPascalStringUInt16LE(_password)) {
			Log::error("Failed to read password for init session");
			return;
		}
		_localServer = in.readBool();
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in InitSessionMessage::writeBack");
			return;
		}
		if (!writeUInt32(_protocolVersion)) {
			Log::error("Failed to write protocol version in InitSessionMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_applicationVersion)) {
			Log::error("Failed to write application version in InitSessionMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_username)) {
			Log::error("Failed to write username in InitSessionMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_password)) {
			Log::error("Failed to write password in InitSessionMessage::writeBack");
			return;
		}
		if (!writeBool(_localServer)) {
			Log::error("Failed to write localServer flag in InitSessionMessage::writeBack");
			return;
		}
		writeSize();
	}

	uint32_t protocolVersion() const {
		return _protocolVersion;
	}
	const core::String &applicationVersion() const {
		return _applicationVersion;
	}
	const core::String &username() const {
		return _username;
	}
	bool isLocalServer() const {
		return _localServer;
	}
	const core::String &password() const {
		return _password;
	}
};

} // namespace network
} // namespace voxedit
