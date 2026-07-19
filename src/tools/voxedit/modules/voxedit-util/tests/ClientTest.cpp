/**
 * @file
 */

#include "AbstractSceneManagerTest.h"
#include "core/String.h"
#include "network/ProtocolHandler.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/protocol/PingMessage.h"

namespace voxedit {

class ClientTest : public AbstractSceneManagerTest {
private:
	using Super = AbstractSceneManagerTest;

protected:
	static constexpr uint16_t TestPort = 19099;

	class PingTestHandler : public network::ProtocolTypeHandler<PingMessage> {
	public:
		int _cnt = 0;
		void execute(const network::ClientId & /*clientId*/, PingMessage *message) override {
			++_cnt;
		}
	};
	PingTestHandler _pingHandler;

	bool onInitApp() override {
		app::AbstractTest::onInitApp();
		_testApp->filesystem()->registerPath("brushes/");
		return _testApp->filesystem()->registerPath("selectionmodes/");
	}

	void SetUp() override {
		Super::SetUp();
		_sceneMgr->startLocalServer(TestPort, "127.0.0.1");
		ASSERT_TRUE(_sceneMgr->client().isConnected());
		_sceneMgr->client().network().protocolRegistry().registerHandler(PROTO_PING, &_pingHandler);
	}

	void TearDown() override {
		_sceneMgr->stopLocalServer();
		Super::TearDown();
	}

	void pumpNetwork(double maxSeconds = 2.0) {
		const double startSeconds = _testApp->timeProvider()->tickSeconds();
		while (_testApp->timeProvider()->tickSeconds() - startSeconds < maxSeconds) {
			_testApp->timeProvider()->updateTickTime();
			_sceneMgr->update(_testApp->timeProvider()->tickSeconds());
		}
	}
};

TEST_F(ClientTest, testSendMessageWhenDisconnected) {
	_sceneMgr->client().disconnect();
	EXPECT_FALSE(_sceneMgr->client().isConnected());
	PingMessage ping;
	EXPECT_FALSE(_sceneMgr->client().network().sendMessage(ping));
	EXPECT_EQ(0, _sceneMgr->client().network().pendingOutgoingBytes());
}

TEST_F(ClientTest, testSendMessageDoesNotBlock) {
	ClientNetwork &network = _sceneMgr->client().network();
	ASSERT_TRUE(_sceneMgr->client().isConnected());

	// Flood the outbound path with small messages. Previously a blocking send()
	// could hang forever if the peer stopped draining; queued non-blocking sends
	// must return promptly even if some bytes remain pending.
	for (int i = 0; i < 256; ++i) {
		PingMessage ping;
		ASSERT_TRUE(network.sendMessage(ping)) << "send failed at iteration " << i;
	}

	const double startSeconds = _testApp->timeProvider()->tickSeconds();
	while (network.pendingOutgoingBytes() > 0) {
		_testApp->timeProvider()->updateTickTime();
		const double nowSeconds = _testApp->timeProvider()->tickSeconds();
		_sceneMgr->update(nowSeconds);
		ASSERT_LT(nowSeconds - startSeconds, 6.0) << "outgoing queue did not drain";
	}
}

TEST_F(ClientTest, testLocalServerReceivesClientTraffic) {
	ClientNetwork &network = _sceneMgr->client().network();
	ASSERT_TRUE(_sceneMgr->client().isConnected());

	PingMessage ping;
	ASSERT_TRUE(network.sendMessage(ping));
	pumpNetwork();
	EXPECT_EQ(0, network.pendingOutgoingBytes());
}

} // namespace voxedit
