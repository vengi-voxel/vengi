/**
 * @file
 */
#include "TestTraze.h"
#include "io/Filesystem.h"
#include "core/command/Command.h"
#include "voxel/MaterialColor.h"
#include "voxel/polyvox/Region.h"

namespace {
const int PlayFieldVolume = 0;
const int PlayerNamesVolume = 1;
}

TestTraze::TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _protocol(eventBus) {
	init(ORGANISATION, "testtraze");
	setRenderAxis(false);
	_eventBus->subscribe<traze::NewGridEvent>(*this);
	_eventBus->subscribe<traze::NewGamesEvent>(*this);
	_eventBus->subscribe<traze::PlayerListEvent>(*this);
	_eventBus->subscribe<traze::TickerEvent>(*this);
	_eventBus->subscribe<traze::SpawnEvent>(*this);
	_eventBus->subscribe<traze::BikeEvent>(*this);
}

core::AppState TestTraze::onConstruct() {
	core::AppState state = Super::onConstruct();
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
	return state;
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
	if (!_rawVolumeRenderer.onResize(glm::ivec2(0), dimension())) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::InitFailure;
	}
	if (!_voxelFont.init("font.ttf", 24, 4, true, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
			"_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸"
			"¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ")) {
		Log::error("Failed to init voxel font");
		return core::AppState::InitFailure;
	}

	_camera.setPosition(glm::vec3(0.0f, 50.0f, 84.0f));

	_logLevelVar->setVal(std::to_string(SDL_LOG_PRIORITY_INFO));
	Log::init();

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
}

void TestTraze::onEvent(const traze::TickerEvent& event) {
}

void TestTraze::onEvent(const traze::SpawnEvent& event) {
}

void TestTraze::onEvent(const traze::NewGridEvent& event) {
	voxel::RawVolume* v = event.get();
	delete _rawVolumeRenderer.setVolume(PlayFieldVolume, v);
	const glm::mat4& translate = glm::translate(-v->region().getCentre());
	const glm::mat4& rotateY = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4& rotateX = glm::rotate(glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	_rawVolumeRenderer.setModelMatrix(PlayFieldVolume, rotateX * rotateY * translate);
	if (!_rawVolumeRenderer.extract(PlayFieldVolume)) {
		Log::error("Failed to extract the volume");
	}
}

void TestTraze::onEvent(const traze::PlayerListEvent& event) {
	_players = event.get();

	std::vector<voxel::VoxelVertex> vertices;
	std::vector<voxel::IndexType> indices;

	char buf[4096] = "";
	core::string::append(buf, sizeof(buf), "Players\n");
	for (const traze::Player& p : _players) {
		const std::string& line = core::string::format("%s - %i\n", p.name.c_str(), p.frags);
		core::string::append(buf, sizeof(buf), line.c_str());
	}

	_voxelFont.render(buf, vertices, indices);
	if (indices.empty() || vertices.empty()) {
		Log::error("Failed to render voxel font");
		return;
	}
	const voxel::RawVolume* volume = _rawVolumeRenderer.volume(PlayFieldVolume);
	if (volume == nullptr) {
		Log::error("No grid volume set yet");
		return;
	}
	const voxel::Region& region = volume->region();
	const glm::ivec3 offset(-region.getCentreX(), 0, -region.getUpperZ());
	const glm::mat4& translate = glm::translate(glm::vec3(offset));
	const glm::mat4& model = glm::scale(translate, glm::vec3(0.2f));
	_rawVolumeRenderer.setModelMatrix(PlayerNamesVolume, model);
	if (!_rawVolumeRenderer.update(PlayerNamesVolume, vertices, indices)) {
		Log::error("Failed to update mesh");
		return;
	}
}

core::AppState TestTraze::onRunning() {
	const int remaining = _eventBus->update(20);
	if (remaining) {
		Log::warn("Remaining events in queue: %i", remaining);
	}
	core::AppState state = Super::onRunning();
	if (_currentGameIndex != -1) {
		_protocol.subscribe(_games[_currentGameIndex]);
	}
	return state;
}

core::AppState TestTraze::onCleanup() {
	core::AppState state = Super::onCleanup();
	_voxelFont.shutdown();
	const std::vector<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	_protocol.shutdown();
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
	Super::onRenderUI();
}

void TestTraze::doRender() {
	_rawVolumeRenderer.render(_camera);
}

TEST_APP(TestTraze)
