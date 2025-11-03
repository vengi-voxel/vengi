/**
 * @file
 */

#include "Client.h"
#include "memento/MementoHandler.h"
#include "protocol/NodeAddedMessage.h"
#include "protocol/NodeKeyFramesMessage.h"
#include "protocol/NodeMovedMessage.h"
#include "protocol/NodePaletteChangedMessage.h"
#include "protocol/NodePropertiesMessage.h"
#include "protocol/NodeRemovedMessage.h"
#include "protocol/NodeRenamedMessage.h"
#include "protocol/VoxelModificationMessage.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/protocol/InitSessionMessage.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"

namespace voxedit {

Client::~Client() {
	shutdown();
}

void Client::construct() {
	_network.construct();
}

bool Client::init() {
	return _network.init();
}

bool Client::connect(const core::String &hostname, uint16_t port, bool localServer) {
	if (_network.connect(hostname, port)) {
		InitSessionMessage initMsg(localServer);
		return _network.sendMessage(initMsg);
	}
	return false;
}

bool Client::isConnected() const {
	return _network.isConnected();
}

void Client::disconnect() {
	_network.disconnect();
}

void Client::update(double nowSeconds) {
	_network.update(nowSeconds);
}

void Client::shutdown() {
	_network.shutdown();
}

void Client::lockListener() {
	_locked = true;
}

void Client::unlockListener() {
	_locked = false;
}

void Client::onMementoStateSkipped(const memento::MementoState &state) {
	onMementoStateAdded(state);
}

void Client::onMementoStateAdded(const memento::MementoState &state) {
	if (_locked || !isConnected()) {
		return;
	}
	switch (state.type) {
	case memento::MementoType::Modification: {
		VoxelModificationMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeMove: {
		NodeMovedMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeAdded: {
		NodeAddedMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRemoved: {
		NodeRemovedMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRenamed: {
		NodeRenamedMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodePaletteChanged: {
		NodePaletteChangedMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeKeyFrames: {
		NodeKeyFramesMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeProperties: {
		NodePropertiesMessage msg(state);
		_network.sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeNormalPaletteChanged:
		// TODO:
		Log::warn("Unhandled memento state type: %d", (int)state.type);
		break;
	case memento::MementoType::SceneGraphAnimation:
	case memento::MementoType::Max:
		break;
	}
}

void Client::executeCommand(const core::String &command) {
	if (!isConnected()) {
		return;
	}
	const core::String rconPassword = core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal();
	CommandMessage msg(command, rconPassword);
	Log::info("Send command to server: %s", command.c_str());
	_network.sendMessage(msg);
}

void Client::sendSceneState() {
	if (!isConnected()) {
		return;
	}
	SceneStateMessage msg(_sceneMgr->sceneGraph());
	Log::info("Send scene state to server (%i bytes)", (int)msg.size());
	_network.sendMessage(msg);
}


} // namespace voxedit
