/**
 * @file
 */

#pragma once

#include "ClientNetwork.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringSet.h"
#include "memento/IMementoStateListener.h"
#include <functional>

namespace voxedit {

class SceneManager;

struct ChatEntry {
	core::String sender;
	core::String message;
	double timestamp = 0.0;
	bool system = false;
};

using ChatCallback = std::function<void(const ChatEntry &)>;

class Client : public core::IComponent, public memento::IMementoStateListener {
protected:
	SceneManager *_sceneMgr = nullptr;
	ClientNetwork _network;
	bool _locked = false;
	core::DynamicArray<ChatEntry> _chatLog;
	core::StringSet _knownUsers;
	ChatCallback _chatCallback;

public:
	Client(SceneManager *sceneMgr) : _sceneMgr(sceneMgr), _network(sceneMgr) {
	}
	virtual ~Client();

	ClientNetwork &network() {
		return _network;
	}

	void construct() override;
	bool init() override;
	void shutdown() override;
	bool isConnected() const;
	bool connect(const core::String &hostname, uint16_t port = 10001, bool localServer = false);
	void disconnect();
	void update(double nowSeconds);

	void executeCommand(const core::String &command);
	void sendChat(const core::String &message);
	void sendSceneState();

	void addChatMessage(const core::String &sender, const core::String &message, bool system);
	const core::DynamicArray<ChatEntry> &chatLog() const;
	const core::StringSet &knownUsers() const;
	void setChatCallback(const ChatCallback &callback);
	void clearChat();

	void lockListener();
	void unlockListener();
	void onMementoStateAdded(const memento::MementoState &state) override;
	void onMementoStateSkipped(const memento::MementoState &state) override;
};


} // namespace voxedit
