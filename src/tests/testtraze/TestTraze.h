/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelfont/VoxelFont.h"
#include "core/EventBus.h"
#include "video/Buffer.h"
#include "RenderShaders.h"

#include "TrazeEvents.h"
#include "TrazeProtocol.h"

/**
 * @brief Example application that renders the state of a traze board. See https://traze.iteratec.de/ for more details.
 */
class TestTraze : public TestApp,
	public core::IEventBusHandler<traze::NewGridEvent>,
	public core::IEventBusHandler<traze::PlayerListEvent>,
	public core::IEventBusHandler<traze::TickerEvent>,
	public core::IEventBusHandler<traze::SpawnEvent>,
	public core::IEventBusHandler<traze::BikeEvent>,
	public core::IEventBusHandler<traze::NewGamesEvent> {
private:
	using Super = TestApp;

	core::VarPtr _name;

	traze::Protocol _protocol;
	voxelrender::RawVolumeRenderer _rawVolumeRenderer;
	voxel::VoxelFont _voxelFont;

	shader::ColorShader& _colorShader;
	video::Buffer _vertexBuffer;
	int32_t _vertexBufferId = -1;
	int32_t _vertexBufferIndexId = -1;
	glm::mat4 _namesModel { 1.0f };

	bool _renderBoard = true;
	bool _renderPlayerNames = true;

	std::vector<traze::GameInfo> _games;
	std::vector<traze::Player> _players;
	int8_t _currentGameIndex = -1;

	void doRender() override;
public:
	TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void onEvent(const traze::BikeEvent& event) override;
	void onEvent(const traze::NewGamesEvent& event) override;
	void onEvent(const traze::TickerEvent& event) override;
	void onEvent(const traze::SpawnEvent& event) override;
	void onEvent(const traze::NewGridEvent& event) override;
	void onEvent(const traze::PlayerListEvent& event) override;

	virtual void onRenderUI() override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};
