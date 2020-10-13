/**
 * @file
 */

#pragma once

#include "AIMessages_generated.h"
#include "ai-shared/common/CharacterId.h"
#include "attrib/ShadowAttributes.h"
#include "core/collection/DynamicArray.h"
#include "network/AINetwork.h"
#include "network/MessageSender.h"
#include "network/NetworkEvents.h"
#include "ui/imgui/IMGUIApp.h"

/**
 * @ingroup Tools
 */
class AIDebug : public ui::imgui::IMGUIApp,
				public core::IEventBusHandler<network::NewConnectionEvent>,
				public core::IEventBusHandler<network::DisconnectEvent> {
public:
	const int _defaultPort = 11338;
private:
	using Super = ui::imgui::IMGUIApp;
	char _aiServer[1024] = "127.0.0.1";
	int _port = _defaultPort;
	ImVec2 _dbgMapOffset { 0.0f, 0.0f };

	int _dbgTreeIdAdd = -1;
	int _dbgTreeIdEdit = -1;

	enum class State {
		Connect,
		Debugging,
		Max
	};

	State _state = State::Connect;
	bool _pause = false;
	bool _centerOnSelection = false;
	float _zoom = 1.0f;
	char _entityListFilter[64] = "";
	size_t _stateWorldSize = 0u;
	size_t _characterDetailsSize = 0u;
	size_t _characterStaticSize = 0u;
	size_t _namesSize = 0u;
	bool _showStats = false;

	uint8_t _chrDetailsBuf[32768];
	ai::CharacterDetails *_chrDetailsMsg = nullptr;

	uint8_t _chrStaticBuf[32768];
	ai::CharacterStatic *_chrStaticMsg = nullptr;

	uint8_t _stateWorldBuf[262144];
	ai::StateWorld *_stateWorldMsg = nullptr;

	uint8_t _namesBuf[32768];
	ai::Names *_namesMsg = nullptr;

	core::Map<ai::CharacterId, const ai::State*> _entityStates;
	core::Map<int, const ai::StateNodeStatic*> _nodeStates;

	struct Server {
		Server(const core::String &host, int port) : _host(host), _port(port) {
		}
		core::String _host;
		int _port;
	};

	ENetPeer *_peer = nullptr;
	network::AINetworkPtr _aiNetwork;
	network::MessageSenderPtr _messageSender;
	core::VarPtr _serverList;
	core::DynamicArray<Server> _history;
	core::String _zoneId;

	void disconnect();
	void connect(const core::String& host, int port);
	bool addToHistory(const core::String& host, int port);

	void selectEntity(ai::CharacterId entityId);
	void changeZone(const char* zoneId);
	void executeCommand(const core::String& command) const;
	void togglePause() const;
	void step() const;
	void updateNode(int nodeId, ai::CharacterId entityId, const core::String& nodeName, const core::String& nodeType, const core::String& condition);
	void addNode(int parentNodeId, ai::CharacterId entityId, const core::String& nodeName, const core::String& nodeType, const core::String& condition);
	void deleteNode(int nodeId, ai::CharacterId entityId);

	bool hasDetails() const;
	const ai::State* entityState() const;
	bool isSelected(ai::CharacterId entityId) const;

	bool dbgConnect();
	void dbgBar();
	float dbgMapZoom() const;
	ImVec2 dbgMapConvertEntPos(float x, float y) const;
	ImVec2 dbgMapCalculateOffsetPos(float x, float y) const;
	bool dbgMapIsVisible(const ImVec2& pos, const ImVec2& mapMins, const ImVec2& mapMaxs) const;
	void dbgMap();
	void dbgStats();
	void dbgEntities();
	void dbgAttributes();
	void dbgMetaAttributes();
	void dbgAggro();
	void dbgTreeNode_r(const ai::StateNode *node, int level);
	void dbgTree();
	void dbgEditNode();
	void dbgAddNode();
public:
	AIDebug(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider,
		const network::AINetworkPtr& aiNetwork, const network::MessageSenderPtr& messageSender,
		const network::ProtocolHandlerRegistryPtr& protocolHandlerRegistry);

	void onRenderUI() override;

	void onEvent(const network::DisconnectEvent& event) override;
	void onEvent(const network::NewConnectionEvent& event) override;

	void onMessage(const ai::StateWorld* msg, const uint8_t* rawData, size_t rawDataLength);
	void onMessage(const ai::CharacterDetails* msg, const uint8_t* rawData, size_t rawDataLength);
	void onMessage(const ai::CharacterStatic* msg, const uint8_t* rawData, size_t rawDataLength);
	void onMessage(const ai::Names* msg, const uint8_t* rawData, size_t rawDataLength);
	void onMessage(const ai::Pause* msg);
	void onMessage(const ai::Ping* msg);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
