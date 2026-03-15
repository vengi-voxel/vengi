/**
 * @file
 */

#include "Client.h"
#include "core/TimeProvider.h"
#include "memento/MementoHandler.h"
#include "protocol/ChatMessage.h"
#include "protocol/InitSessionMessage.h"
#include "protocol/NodeAddedMessage.h"
#include "protocol/NodeKeyFramesMessage.h"
#include "protocol/NodeMovedMessage.h"
#include "protocol/NodePaletteChangedMessage.h"
#include "protocol/NodePropertiesMessage.h"
#include "protocol/NodeRemovedMessage.h"
#include "protocol/NodeRenamedMessage.h"
#include "protocol/NodeIKConstraintMessage.h"
#include "protocol/SceneStateMessage.h"
#include "protocol/VoxelModificationMessage.h"
#include "voxedit-util/SceneManager.h"
#include "protocol/NodeNormalPaletteChangedMessage.h"
#include "protocol/SceneGraphAnimationMessage.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/protocol/CommandMessage.h"

namespace voxedit {

Client::Client(SceneManager *sceneMgr) : _sceneMgr(sceneMgr), _network(new ClientNetwork(sceneMgr)) {
}

Client::~Client() {
	shutdown();
}

void Client::construct() {
	_network->construct();
}

bool Client::init() {
	return _network->init();
}

bool Client::connect(const core::String &hostname, uint16_t port, bool localServer) {
	if (_network->connect(hostname, port)) {
		InitSessionMessage initMsg(localServer);
		return _network->sendMessage(initMsg);
	}
	return false;
}

bool Client::isConnected() const {
	return _network->isConnected();
}

void Client::disconnect() {
	_network->disconnect();
}

void Client::update(double nowSeconds) {
	_network->update(nowSeconds);
}

void Client::shutdown() {
	_network->shutdown();
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
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeMove: {
		NodeMovedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeAdded: {
		NodeAddedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRemoved: {
		NodeRemovedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRenamed: {
		NodeRenamedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodePaletteChanged: {
		NodePaletteChangedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeKeyFrames: {
		NodeKeyFramesMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeProperties: {
		NodePropertiesMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeIKConstraint: {
		NodeIKConstraintMessage ikMsg(state, _sceneMgr->sceneGraph());
		_network->sendMessage(ikMsg);
		break;
	}
	case memento::MementoType::SceneNodeNormalPaletteChanged: {
		NodeNormalPaletteChangedMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::SceneGraphAnimation: {
		SceneGraphAnimationMessage msg(state);
		_network->sendMessage(msg);
		break;
	}
	case memento::MementoType::Max:
		break;
	}
}

void Client::executeCommand(const core::String &command) {
	if (!isConnected()) {
		return;
	}
	const core::String rconPassword = core::getVar(cfg::VoxEditNetRconPassword)->strVal();
	CommandMessage msg(command, rconPassword);
	Log::info("Send command to server: %s", command.c_str());
	_network->sendMessage(msg);
}

void Client::sendChat(const core::String &message) {
	if (!isConnected()) {
		return;
	}
	ChatMessage msg(message);
	_network->sendMessage(msg);
}

void Client::addChatMessage(const core::String &sender, const core::String &message, bool system) {
	ChatEntry entry;
	entry.sender = sender;
	entry.message = message;
	entry.timestamp = core::TimeProvider::systemMillis();
	entry.system = system;
	_chatLog.push_back(entry);
	if (_chatCallback) {
		_chatCallback(entry);
	}
}

const core::DynamicArray<ChatEntry> &Client::chatLog() const {
	return _chatLog;
}

const core::DynamicArray<ClientInfo> &Client::connectedClients() const {
	return _connectedClients;
}

void Client::updateConnectedClients(const core::DynamicArray<ClientInfo> &clients) {
	_connectedClients = clients;
	Log::debug("Updated connected clients list: %d clients", (int)_connectedClients.size());
}

core::String Client::disambiguatedName(const ClientInfo &info) const {
	for (const ClientInfo &other : _connectedClients) {
		if (other.id != info.id && other.name == info.name) {
			return core::String::format("%s#%u", info.name.c_str(), (uint32_t)info.id);
		}
	}
	return info.name;
}

void Client::setChatCallback(const ChatCallback &callback) {
	_chatCallback = callback;
}

void Client::clearChat() {
	_chatLog.clear();
}

void Client::sendSceneState() {
	if (!isConnected()) {
		return;
	}
	SceneStateMessage msg(_sceneMgr->sceneGraph());
	Log::info("Send scene state to server (%i bytes)", (int)msg.size());
	_network->sendMessage(msg);
}


} // namespace voxedit
