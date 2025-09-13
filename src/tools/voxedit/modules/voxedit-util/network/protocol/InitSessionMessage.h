/**
 * @file
 */

#pragma once

#include "core/ConfigVar.h"
#include "core/String.h"
#include "core/Var.h"
#include "voxedit-util/network/ProtocolMessage.h"
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
	core::String _username;
	bool _localServer = false;

public:
	InitSessionMessage(bool localServer) : ProtocolMessage(PROTO_INIT_SESSION) {
		_protocolVersion = PROTOCOL_VERSION;
		_applicationVersion = core::Var::getSafe(cfg::AppVersion)->strVal();
		_username = core::Var::getSafe(cfg::AppUserName)->strVal();

		writeUInt32(_protocolVersion);
		writePascalStringUInt16LE(_applicationVersion);
		writePascalStringUInt16LE(_username);
		writeBool(localServer);
		writeSize();
	}

	InitSessionMessage(MessageStream &in) {
		_id = PROTO_INIT_SESSION;
		in.readUInt32(_protocolVersion);
		in.readPascalStringUInt16LE(_applicationVersion);
		in.readPascalStringUInt16LE(_username);
		_localServer = in.readBool();
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUInt32(_protocolVersion);
		writePascalStringUInt16LE(_applicationVersion);
		writePascalStringUInt16LE(_username);
		writeBool(_localServer);
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
};

} // namespace network
} // namespace voxedit
