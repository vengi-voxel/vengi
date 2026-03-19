/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "memento/IMementoStateListener.h"
#include "voxedit-util/network/protocol/ClientInfo.h"
#include "core/Function.h"

namespace voxedit {

class SceneManager;
class ClientNetwork;

struct ChatEntry {
	core::String sender;
	core::String message;
	double timestamp = 0.0;
	bool system = false;
};

using ChatCallback = core::Function<void(const ChatEntry &)>;

class Client : public core::IComponent, public memento::IMementoStateListener {
protected:
	SceneManager *_sceneMgr = nullptr;
	ClientNetwork *_network;
	bool _locked = false;
	core::DynamicArray<ChatEntry> _chatLog;
	core::DynamicArray<ClientInfo> _connectedClients;
	ChatCallback _chatCallback;

public:
	Client(SceneManager *sceneMgr);
	virtual ~Client();

	ClientNetwork &network() {
		return *_network;
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
	const core::DynamicArray<ClientInfo> &connectedClients() const;
	void updateConnectedClients(const core::DynamicArray<ClientInfo> &clients);
	/**
	 * @return The display name for a connected client - appends #id if another
	 *         client with the same name exists in the list.
	 */
	core::String disambiguatedName(const ClientInfo &info) const;
	void setChatCallback(const ChatCallback &callback);
	void clearChat();

	void lockListener();
	void unlockListener();
	void onMementoStateAdded(const memento::MementoState &state) override;
	void onMementoStateSkipped(const memento::MementoState &state) override;
};


} // namespace voxedit
