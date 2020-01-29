/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/VoxelFontRenderer.h"
#include "core/EventBus.h"
#include "audio/SoundManager.h"

#include "TrazeEvents.h"
#include "TrazeProtocol.h"

#include "util/MessageQueue.h"

/**
 * @brief Example application that renders the state of a traze board. See https://traze.iteratec.de/ for more details.
 */
class TestTraze : public TestApp,
	public core::IEventBusHandler<traze::NewGridEvent>,
	public core::IEventBusHandler<traze::PlayerListEvent>,
	public core::IEventBusHandler<traze::TickerEvent>,
	public core::IEventBusHandler<traze::SpawnEvent>,
	public core::IEventBusHandler<traze::BikeEvent>,
	public core::IEventBusHandler<traze::ScoreEvent>,
	public core::IEventBusHandler<traze::NewGamesEvent> {
private:
	using Super = TestApp;

	core::VarPtr _name;

	traze::Protocol _protocol;
	voxelrender::RawVolumeRenderer _rawVolumeRenderer;
	voxelrender::VoxelFontRenderer _voxelFontRender;
	MessageQueue _messageQueue;
	audio::SoundManager _soundMgr;

	bool _renderBoard = true;
	bool _renderPlayerNames = true;

	glm::ivec2 _spawnPosition { 0 };
	uint64_t _spawnTime = 0u;

	int _maxLength = 200;
	video::Camera _textCamera;

	std::vector<traze::GameInfo> _games;
	std::vector<traze::Player> _players;
	int8_t _currentGameIndex = -1;
	uint64_t _nextConnectTime = 0;

	void doRender() override;

	void sound(const char *soundId);

	const core::String& playerName(traze::PlayerId playerId) const;
	const traze::Player& player(traze::PlayerId playerId) const;
public:
	TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void onEvent(const traze::BikeEvent& event) override;
	void onEvent(const traze::NewGamesEvent& event) override;
	void onEvent(const traze::TickerEvent& event) override;
	void onEvent(const traze::SpawnEvent& event) override;
	void onEvent(const traze::NewGridEvent& event) override;
	void onEvent(const traze::PlayerListEvent& event) override;
	void onEvent(const traze::ScoreEvent& event) override;

	virtual void onRenderUI() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};
