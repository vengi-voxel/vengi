/**
 * @file
 */

#include "voxedit-util/network/Client.h"
#include "app/tests/AbstractTest.h"
#include "core/String.h"
#include "core/tests/TestHelper.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/PingMessage.h"

namespace voxedit {

class ClientTest : public app::AbstractTest {
protected:
	SceneManagerPtr _sceneMgr;
	SceneManager *sceneMgr() {
		return _sceneMgr.get();
	}
	core::SharedPtr<Client> _client;

	class PingTestHandler : public ProtocolTypeHandler<PingMessage> {
	public:
		int _cnt = 0;
		void execute(const ClientId & /*clientId*/, PingMessage *message) override {
			++_cnt;
		}
	};
	PingTestHandler _pingHandler;

public:
	void SetUp() override {
		app::AbstractTest::SetUp();
		const auto timeProvider = core::make_shared<core::TimeProvider>();
		const auto sceneRenderer = core::make_shared<ISceneRenderer>();
		const auto modifierRenderer = core::make_shared<IModifierRenderer>();
		const auto selectionManager = core::make_shared<SelectionManager>();
		_sceneMgr = core::make_shared<SceneManager>(timeProvider, _testApp->filesystem(), sceneRenderer,
													modifierRenderer, selectionManager);

		_client = core::make_shared<Client>(_sceneMgr.get());
		EXPECT_TRUE(_client->init());
		const core::String hostname = "localhost";
		const uint16_t port = 10001;

		// Try to connect to the local voxedit server
		const bool connected =
			_client->connect(hostname, port); // Skip the test if we can't connect - the server is not running
		if (!connected) {
			GTEST_SKIP() << "Could not connect to voxedit server at " << hostname.c_str() << ":" << port
						 << " - server not running";
		}
		_client->network().protocolRegistry().registerHandler(PROTO_PING, &_pingHandler);
	}

	void TearDown() override {
		_client->shutdown();
		app::AbstractTest::TearDown();
	}
};

TEST_F(ClientTest, testConnectionToLocalhost) {
	EXPECT_TRUE(_client->isConnected());
	double startSeconds = _testApp->timeProvider()->tickSeconds();
	_client->update(startSeconds);
	while (_pingHandler._cnt == 0) {
		_testApp->timeProvider()->updateTickTime();
		const double nowSeconds = _testApp->timeProvider()->tickSeconds();
		_client->update(nowSeconds);
		double waitSeconds = nowSeconds - startSeconds;
		ASSERT_LT(waitSeconds, 6.0);
	}
	_client->disconnect();
	EXPECT_FALSE(_client->isConnected());
}

} // namespace voxedit
