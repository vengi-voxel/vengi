/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/EventBus.h"
#include "core/Password.h"
#include "ServerMessages_generated.h"
#include "network/ClientNetwork.h"
#include "network/ClientMessageSender.h"
#include "network/NetworkEvents.h"
#include "network/ProtocolHandlerRegistry.h"

#include "network/ServerNetwork.h"

namespace backend {

class ConnectTest:
		public core::AbstractTest,
		public core::IEventBusHandler<network::NewConnectionEvent>,
		public core::IEventBusHandler<network::DisconnectEvent> {
protected:
	core::EventBusPtr _clientEventBus;
	core::EventBusPtr _serverEventBus;
	network::ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	network::ClientNetworkPtr _clientNetwork;
	network::ClientMessageSenderPtr _clientMessageSender;

	network::ServerNetworkPtr _serverNetwork;

	const uint16_t _port;
	const std::string _host = "127.0.0.1";

	int _disconnectEvent = 0;
	int _connectEvent = 0;
	int _userConnectHandlerCalled = 0;
public:
	ConnectTest() :
			_clientEventBus(std::make_shared<core::EventBus>()),
			_serverEventBus(std::make_shared<core::EventBus>()),
			_protocolHandlerRegistry(std::make_shared<network::ProtocolHandlerRegistry>()),
			_clientNetwork(std::make_shared<network::ClientNetwork>(_protocolHandlerRegistry, _clientEventBus)),
			_clientMessageSender(std::make_shared<network::ClientMessageSender>(_clientNetwork)),
			_serverNetwork(std::make_shared<network::ServerNetwork>(_protocolHandlerRegistry, _serverEventBus)),
			_port((uint16_t)((uint32_t)(intptr_t)this) + 1025) {
	}

	void onEvent(const network::DisconnectEvent& event) override {
		++_disconnectEvent;
		Log::info("got disconnect event with reason %i", (int)event.reason());
	}

	void onEvent(const network::NewConnectionEvent& event) override {
		++_connectEvent;
		flatbuffers::FlatBufferBuilder fbb;
		Log::info("got new connection event");
		_clientMessageSender->sendClientMessage(fbb, network::ClientMsgType::UserConnect,
				network::CreateUserConnect(fbb, fbb.CreateString("a@b.c"), fbb.CreateString(core::pwhash("somepassword", "TODO"))).Union());
	}

	bool listen() {
		return _serverNetwork->bind(_port, _host);
	}

	bool connect() {
		ENetPeer* peer = _clientNetwork->connect(_port, _host);
		if (!peer) {
			Log::error("Failed to connect to server localhost:%i", _port);
			return false;
		}

		peer->data = this;
		return true;
	}

	void onCleanupApp() override {
		_clientEventBus->unsubscribe<network::NewConnectionEvent>(*this);
		_clientEventBus->unsubscribe<network::DisconnectEvent>(*this);
		_serverEventBus->unsubscribe<network::DisconnectEvent>(*this);

		_clientNetwork->shutdown();
		_serverNetwork->shutdown();
	}

	bool onInitApp() override {
		_clientEventBus->subscribe<network::NewConnectionEvent>(*this);
		_clientEventBus->subscribe<network::DisconnectEvent>(*this);
		_serverEventBus->subscribe<network::DisconnectEvent>(*this);

		class UserConnectHandler: public network::IProtocolHandler {
		private:
			int *_called;
		public:
			UserConnectHandler(int *called) : _called(called) {
			}

			void execute(ENetPeer* peer, const void* message) override {
				(*_called)++;
			}
		};

		_serverNetwork->init();
		const network::ProtocolHandlerRegistryPtr& r = _serverNetwork->registry();
		r->registerHandler(network::EnumNameClientMsgType(network::ClientMsgType::UserConnect), std::make_shared<UserConnectHandler>(&_userConnectHandlerCalled));
		_clientNetwork->init();

		_disconnectEvent = 0;
		_connectEvent = 0;
		_userConnectHandlerCalled = 0;

		return true;
	}

	void update() {
		_serverNetwork->update();
		_clientNetwork->update();
		_serverNetwork->update();
		_clientNetwork->update();
	}
};

TEST_F(ConnectTest, testConnect) {
	ASSERT_TRUE(listen()) << "Failed to bind to port " << _port;
	ASSERT_TRUE(connect()) << "Failed to connect to port " << _port;

	update();
	EXPECT_EQ(0, _disconnectEvent);
	EXPECT_EQ(1, _connectEvent);

	_clientNetwork->disconnect();
	update();
	//EXPECT_EQ(1, _disconnectEvent);
	EXPECT_EQ(1, _connectEvent);
	EXPECT_EQ(1, _userConnectHandlerCalled);
}

}
