/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "../TrazeProtocol.h"
#include "../TrazeEvents.h"

namespace traze {

class TrazeProtocolTest: public app::AbstractTest,
	public core::IEventBusHandler<traze::NewGridEvent>,
	public core::IEventBusHandler<traze::PlayerListEvent>,
	public core::IEventBusHandler<traze::TickerEvent>,
	public core::IEventBusHandler<traze::SpawnEvent>,
	public core::IEventBusHandler<traze::BikeEvent>,
	public core::IEventBusHandler<traze::ScoreEvent>,
	public core::IEventBusHandler<traze::NewGamesEvent> {
private:
	using Super = app::AbstractTest;
protected:
	Protocol *_p;
	core::EventBusPtr _eventBus;
	Score _score;

public:
	void SetUp() override {
		Super::SetUp();
		_eventBus = app::App::getInstance()->eventBus();
		_eventBus->subscribe<traze::NewGridEvent>(*this);
		_eventBus->subscribe<traze::NewGamesEvent>(*this);
		_eventBus->subscribe<traze::PlayerListEvent>(*this);
		_eventBus->subscribe<traze::TickerEvent>(*this);
		_eventBus->subscribe<traze::SpawnEvent>(*this);
		_eventBus->subscribe<traze::BikeEvent>(*this);
		_eventBus->subscribe<traze::ScoreEvent>(*this);
		_p = new Protocol(_eventBus) ;
		ASSERT_TRUE(_p->init()) << "Initialization failed";
	}

	void TearDown() override {
		_eventBus->unsubscribe<traze::NewGridEvent>(*this);
		_eventBus->unsubscribe<traze::NewGamesEvent>(*this);
		_eventBus->unsubscribe<traze::PlayerListEvent>(*this);
		_eventBus->unsubscribe<traze::TickerEvent>(*this);
		_eventBus->unsubscribe<traze::SpawnEvent>(*this);
		_eventBus->unsubscribe<traze::BikeEvent>(*this);
		_eventBus->unsubscribe<traze::ScoreEvent>(*this);
		_p->shutdown();
		delete _p;
		_p = nullptr;
		Super::TearDown();
	}

	void onEvent(const traze::BikeEvent& event) override {
	}

	void onEvent(const traze::NewGamesEvent& event) override {
	}

	void onEvent(const traze::TickerEvent& event) override {
	}

	void onEvent(const traze::SpawnEvent& event) override {
	}

	void onEvent(const traze::NewGridEvent& event) override {
	}

	void onEvent(const traze::PlayerListEvent& event) override {
	}

	void onEvent(const traze::ScoreEvent& event) override {
		_score = event.get();
	}
};

TEST_F(TrazeProtocolTest, testParseScores) {
	const core::String json = R"(
		{
			"ingameNick1[id1]": 238,
			"ingameNick2[id2]": 235
		})";
	_p->parseScores(json);
	ASSERT_EQ(0, _eventBus->update());
	ASSERT_EQ(2, (int)_score.size());
	EXPECT_EQ("ingameNick2[id2]", _score[0]);
	EXPECT_EQ("ingameNick1[id1]", _score[1]);
}

}
