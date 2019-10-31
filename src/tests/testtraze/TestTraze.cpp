/**
 * @file
 */
#include "TestTraze.h"
#include "core/io/Filesystem.h"
#include "core/command/Command.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/RawVolumeWrapper.h"

namespace {
const int PlayFieldVolume = 0;
const int FloorVolume = 1;
const int FontSize = 48;
}

TestTraze::TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _protocol(eventBus), _voxelFontRender(FontSize, 4, voxel::VoxelFont::OriginUpperLeft), _soundMgr(filesystem) {
	init(ORGANISATION, "testtraze");
	setRenderAxis(false);
	setRelativeMouseMode(false);
	_allowRelativeMouseMode = false;
	_eventBus->subscribe<traze::NewGridEvent>(*this);
	_eventBus->subscribe<traze::NewGamesEvent>(*this);
	_eventBus->subscribe<traze::PlayerListEvent>(*this);
	_eventBus->subscribe<traze::TickerEvent>(*this);
	_eventBus->subscribe<traze::SpawnEvent>(*this);
	_eventBus->subscribe<traze::BikeEvent>(*this);
	_eventBus->subscribe<traze::ScoreEvent>(*this);
}

const std::string& TestTraze::playerName(traze::PlayerId playerId) const {
	return player(playerId).name;
}

const traze::Player& TestTraze::player(traze::PlayerId playerId) const {
	for (auto& p : _players) {
		if (p.id == playerId) {
			return p;
		}
	}
	static traze::Player player;
	return player;
}

core::AppState TestTraze::onConstruct() {
	core::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);
	core::Var::get("mosquitto_host", "traze.iteratec.de");
	core::Var::get("mosquitto_port", "1883");
	_name = core::Var::get("name", "noname_testtraze");
	core::Command::registerCommand("join", [&] (const core::CmdArgs& args) { _protocol.join(_name->strVal()); });
	core::Command::registerCommand("bail", [&] (const core::CmdArgs& args) { _protocol.bail(); });
	core::Command::registerCommand("left", [&] (const core::CmdArgs& args) { _protocol.steer(traze::BikeDirection::W); });
	core::Command::registerCommand("right", [&] (const core::CmdArgs& args) { _protocol.steer(traze::BikeDirection::E); });
	core::Command::registerCommand("forward", [&] (const core::CmdArgs& args) { _protocol.steer(traze::BikeDirection::N); });
	core::Command::registerCommand("backward", [&] (const core::CmdArgs& args) { _protocol.steer(traze::BikeDirection::S); });
	core::Command::registerCommand("players", [&] (const core::CmdArgs& args) {
		for (const auto& p : _players) {
			Log::info("%s", p.name.c_str());
		}
	});
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	_rawVolumeRenderer.construct();
	_messageQueue.construct();
	_soundMgr.construct();
	return state;
}

void TestTraze::sound(const char *soundId) {
	if (!_soundMgr.play(soundId, glm::ivec2(0), false)) {
		Log::warn("Failed to play sound %s", soundId);
	}
}

core::AppState TestTraze::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}
	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}
	if (!_protocol.init()) {
		Log::error("Failed to init protocol");
		return core::AppState::InitFailure;
	}
	if (!_rawVolumeRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::InitFailure;
	}
	if (!_messageQueue.init()) {
		Log::error("Failed to init message queue");
		return core::AppState::InitFailure;
	}
	if (!_voxelFontRender.init()) {
		Log::error("Failed to init voxel font");
		return core::AppState::InitFailure;
	}
	if (!_soundMgr.init()) {
		Log::error("Failed to init sound manager");
		return core::AppState::InitFailure;
	}

	_camera.setPosition(glm::vec3(0.0f, 50.0f, 84.0f));
	_logLevelVar->setVal(std::to_string(SDL_LOG_PRIORITY_INFO));
	Log::init();

	_textCamera.setMode(video::CameraMode::Orthogonal);
	_textCamera.setNearPlane(-10.0f);
	_textCamera.setFarPlane(10.0f);
	_textCamera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_textCamera.update(0L);

	_voxelFontRender.setViewProjectionMatrix(_textCamera.viewProjectionMatrix());

	return state;
}

void TestTraze::onEvent(const traze::NewGamesEvent& event) {
	_games = event.get();
	Log::debug("Got %i games", (int)_games.size());
	// there are some points were we assume a limited amount of games...
	if (_games.size() >= UCHAR_MAX) {
		Log::warn("Too many games found - reducing them");
		_games.resize(UCHAR_MAX - 1);
	}
	// TODO: this doesn't work if the instanceName changed (new game added, old game removed...)
	if (_games.empty() || _currentGameIndex > (int8_t)_games.size()) {
		_protocol.unsubscribe();
		_currentGameIndex = -1;
	} else if (!_games.empty() && _currentGameIndex == -1) {
		Log::info("Select first game");
		_currentGameIndex = 0;
	}
}

void TestTraze::onEvent(const traze::BikeEvent& event) {
	const traze::Bike& bike = event.get();
	Log::debug("Received bike event for player %u (%i:%i)",
			bike.playerId, bike.currentLocation.x, bike.currentLocation.y);
}

void TestTraze::onEvent(const traze::TickerEvent& event) {
	const traze::Ticker& ticker = event.get();
	const std::string& fraggerName = playerName(ticker.fragger);
	const std::string& casualtyName = playerName(ticker.casualty);
	switch (ticker.type) {
	case traze::TickerType::Frag:
		if (ticker.fragger == _protocol.playerId()) {
			sound("you_win");
			_messageQueue.message("You fragged %s", casualtyName.c_str());
		} else if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
			_messageQueue.message("You were fragged by %s", fraggerName.c_str());
		} else {
			_messageQueue.message("%s fragged %s", fraggerName.c_str(), casualtyName.c_str());
		}
		break;
	case traze::TickerType::Suicide:
		if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
		} else {
			sound("suicide");
		}
		_messageQueue.message("%s committed suicide", fraggerName.c_str());
		break;
	case traze::TickerType::Collision:
		if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
		} else if (ticker.fragger == _protocol.playerId()) {
			sound("you_win");
		} else {
			sound("collision");
		}
		_messageQueue.message("%s - collision with another player", fraggerName.c_str());
		break;
	default:
		break;
	}
}

void TestTraze::onEvent(const traze::ScoreEvent& event) {
	const traze::Score& score = event.get();
	Log::debug("Received score event with %i entries", (int)score.size());
}

void TestTraze::onEvent(const traze::SpawnEvent& event) {
	const traze::Spawn& spawn = event.get();
	Log::debug("Spawn at position %i:%i", spawn.position.x, spawn.position.y);
	if (spawn.own) {
		_spawnPosition = spawn.position;
		_spawnTime = _now;
		sound("join");
	}
}

void TestTraze::onEvent(const traze::NewGridEvent& event) {
	voxel::RawVolume* v = event.get();
	if (_spawnTime > 0 && _now - _spawnTime < 4000) {
		const voxel::Voxel voxel = voxel::createRandomColorVoxel(voxel::VoxelType::Generic);
		v->setVoxel(glm::ivec3(_spawnPosition.y, 0, _spawnPosition.x), voxel);
		v->setVoxel(glm::ivec3(_spawnPosition.y, 1, _spawnPosition.x), voxel);
	}
	voxel::RawVolume* volume = _rawVolumeRenderer.volume(PlayFieldVolume);
	voxel::Region dirtyRegion;
	if (volume == nullptr || volume->region() != v->region()) {
		delete _rawVolumeRenderer.setVolume(PlayFieldVolume, v);
		dirtyRegion = v->region();
		volume = v;

		voxel::RawVolume* floor = new voxel::RawVolume(dirtyRegion);
		const voxel::Region& region = floor->region();
		const voxel::Voxel voxel = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
		// ground
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				floor->setVoxel(x, region.getLowerY(), z, voxel);
			}
		}
		// walls
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			floor->setVoxel(region.getLowerX(), region.getLowerY() + 1, z, voxel);
			floor->setVoxel(region.getUpperX(), region.getLowerY() + 1, z, voxel);
			floor->setVoxel(region.getLowerX(), region.getLowerY() + 2, z, voxel);
			floor->setVoxel(region.getUpperX(), region.getLowerY() + 2, z, voxel);
		}
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			floor->setVoxel(x, region.getLowerY() + 1, region.getLowerZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 1, region.getUpperZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 2, region.getLowerZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 2, region.getUpperZ(), voxel);
		}
		delete _rawVolumeRenderer.setVolume(FloorVolume, floor);
		if (!_rawVolumeRenderer.extract(FloorVolume, region)) {
			Log::error("Failed to extract the volume");
		}
	} else {
		voxel::RawVolumeWrapper wrapper(volume);
		const voxel::Region& region = volume->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		glm::ivec3 p;
		for (int x = mins.x; x <= maxs.x; ++x) {
			p.x = x;
			for (int y = mins.y; y <= maxs.y; ++y) {
				p.y = y;
				for (int z = mins.z; z <= maxs.z; ++z) {
					p.z = z;
					wrapper.setVoxel(p, v->voxel(p));
				}
			}
		}
		delete v;
		dirtyRegion = wrapper.dirtyRegion();
	}
	const glm::mat4& translate = glm::translate(-volume->region().getCentre());
	const glm::mat4& rotateY = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4& rotateX = glm::rotate(glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::mat4 model = rotateX * rotateY * translate;
	_rawVolumeRenderer.setModelMatrix(PlayFieldVolume, model);
	_rawVolumeRenderer.setModelMatrix(FloorVolume, model);
	if (!_rawVolumeRenderer.extract(PlayFieldVolume, dirtyRegion)) {
		Log::error("Failed to extract the volume");
	}
}

void TestTraze::onEvent(const traze::PlayerListEvent& event) {
	_players = event.get();
	_maxLength = 200;
	for (const traze::Player& p : _players) {
		_maxLength = core_max(_maxLength, _voxelFontRender.stringWidth(p.name.c_str()) + 60);
	}
}

core::AppState TestTraze::onRunning() {
	const int remaining = _eventBus->update(2);
	if (remaining) {
		Log::debug("Remaining events in queue: %i", remaining);
	}
	core::AppState state = Super::onRunning();
	if (!_protocol.connected()) {
		const uint64_t current = lifetimeInSeconds();
		if (_nextConnectTime < current) {
			const uint64_t delaySeconds = 3;
			_nextConnectTime += delaySeconds;
			_protocol.connect();
		}
	} else if (_currentGameIndex != -1) {
		_protocol.subscribe(_games[_currentGameIndex]);
	}
	_messageQueue.update(_deltaFrameMillis);
	_soundMgr.update(_deltaFrameMillis);
	return state;
}

core::AppState TestTraze::onCleanup() {
	core::AppState state = Super::onCleanup();
	_voxelFontRender.shutdown();
	_soundMgr.shutdown();
	const std::vector<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	_protocol.shutdown();
	_messageQueue.shutdown();
	return state;
}

void TestTraze::onRenderUI() {
	if (ImGui::BeginCombo("GameInfo", _currentGameIndex == -1 ? "" : _games[_currentGameIndex].name.c_str(), 0)) {
		for (size_t i = 0u; i < (size_t)_games.size(); ++i) {
			const traze::GameInfo& game = _games[i];
			const bool selected = _currentGameIndex == (int)i;
			if (ImGui::Selectable(game.name.c_str(), selected)) {
				_currentGameIndex = i;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::InputVarString("Name", _name);
	if (!_protocol.joined() && ImGui::Button("Join")) {
		_protocol.join(_name->strVal());
	}
	if (_protocol.joined() && ImGui::Button("Leave")) {
		_protocol.bail();
	}
	ImGui::Checkbox("Render board", &_renderBoard);
	ImGui::Checkbox("Render player names", &_renderPlayerNames);
	Super::onRenderUI();
}

void TestTraze::doRender() {
	if (_renderBoard) {
		_rawVolumeRenderer.render(_camera);
	}

	const glm::ivec2& dim = frameBufferDimension();
	_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(dim.x / 3, 0.0f, 0.0f)));
	int messageOffset = 0;
	_messageQueue.visitMessages([&] (int64_t /*remainingMillis*/, const std::string& msg) {
		_voxelFontRender.text(glm::ivec3(0.0f, (float)messageOffset, 0.0f), core::Color::White, "%s", msg.c_str());
		messageOffset += _voxelFontRender.lineHeight();
	});
	_voxelFontRender.swapBuffers();
	_voxelFontRender.render();

	if (!_protocol.connected()) {
		const char* connecting = "Connecting";
		const int w = _voxelFontRender.stringWidth(connecting);
		_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(dim.x / 2 - w / 2, dim.y / 2 - _voxelFontRender.lineHeight() / 2, 0.0f)));
		const glm::ivec3 pos(0, 0, 0);
		_voxelFontRender.text(pos, core::Color::Red, "%s", connecting);
		const int offset = int((_now - _initMillis) / 75);
		_voxelFontRender.text(glm::ivec3(pos.x + (offset % w), pos.y + _voxelFontRender.lineHeight(), pos.z), core::Color::Red, ".");
	} else if (_renderPlayerNames) {
		_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(20.0f, 20.0f, 0.0f)));
		int yOffset = 0;
		_voxelFontRender.text(glm::ivec3(0, yOffset, 0), core::Color::White, "Players");
		yOffset += _voxelFontRender.lineHeight();
		for (const traze::Player& p : _players) {
			_voxelFontRender.text(glm::ivec3(0, yOffset, 0), p.color, "* %s", p.name.c_str());
			_voxelFontRender.text(glm::ivec3(_maxLength, yOffset, 0), p.color, "%i/%i", p.frags, p.owned);
			yOffset += _voxelFontRender.lineHeight();
		}
	}

	_voxelFontRender.swapBuffers();
	_voxelFontRender.render();
}

TEST_APP(TestTraze)
