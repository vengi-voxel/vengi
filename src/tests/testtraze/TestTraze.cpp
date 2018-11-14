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

struct VertexData {
	struct AttributeData {
		glm::vec4 vertex;
		glm::vec3 color {core::Color::Red};
	};
	std::vector<AttributeData> data;

	inline void reserve(size_t amount) {
		data.resize(amount);
	}
};

}

TestTraze::TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _protocol(eventBus), _colorShader(shader::ColorShader::getInstance()) {
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
	if (!_colorShader.setup()) {
		Log::error("Failed to init color shader");
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

	_vertexBufferId = _vertexBuffer.create();
	_vertexBuffer.setMode(_vertexBufferId, video::BufferMode::Dynamic);
	_vertexBufferIndexId = _vertexBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);

	video::Attribute attribPos;
	attribPos.bufferIndex = _vertexBufferId;
	attribPos.location = _colorShader.enableVertexAttributeArray("a_pos");
	attribPos.stride = sizeof(VertexData::AttributeData);
	attribPos.size = _colorShader.getAttributeComponents(attribPos.location);
	attribPos.type = video::mapType<decltype(VertexData::AttributeData::vertex)::value_type>();
	attribPos.offset = offsetof(VertexData::AttributeData, vertex);
	_vertexBuffer.addAttribute(attribPos);

	video::Attribute attribColor;
	attribColor.bufferIndex = _vertexBufferId;
	attribColor.location = _colorShader.enableVertexAttributeArray("a_color");
	attribColor.stride = sizeof(VertexData::AttributeData);
	attribColor.size = _colorShader.getAttributeComponents(attribColor.location);
	attribColor.type = video::mapType<decltype(VertexData::AttributeData::color)::value_type>();
	attribColor.offset = offsetof(VertexData::AttributeData, color);
	_vertexBuffer.addAttribute(attribColor);

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
	const traze::Bike& bike = event.get();
	Log::debug("Received bike event for player %u", bike.playerId);
}

void TestTraze::onEvent(const traze::TickerEvent& event) {
	const traze::Ticker& ticker = event.get();
	switch (ticker.type) {
	case traze::TickerType::Frag:
		Log::info("Received frag event");
		break;
	case traze::TickerType::Suicide:
		Log::info("Received suicide event");
		break;
	default:
		break;
	}
}

void TestTraze::onEvent(const traze::SpawnEvent& event) {
	const glm::ivec2& position = event.get();
	Log::info("Spawn at position %i:%i", position.x, position.y);
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
	const voxel::Region& region = v->region();
	const glm::ivec3 namesOffset(-region.getCentreX(), 0, -region.getUpperZ());
	const glm::mat4& namesTranslate = glm::translate(glm::vec3(namesOffset));
	_namesModel = glm::scale(namesTranslate, glm::vec3(0.2f));
}

void TestTraze::onEvent(const traze::PlayerListEvent& event) {
	_players = event.get();

	std::vector<uint32_t> indices;

	VertexData data;
	char buf[4096] = "";
	core::string::append(buf, sizeof(buf), "Players\n");
	int yOffset = 0;
	for (const traze::Player& p : _players) {
		const std::string& line = core::string::format("%s - %i", p.name.c_str(), p.frags);
		_voxelFont.render(line.c_str(), data.data, indices, [&] (const voxel::VoxelVertex& vertex, std::vector<VertexData::AttributeData>& data, int x, int y) {
			const VertexData::AttributeData vp{glm::vec4(vertex.position.x + x, vertex.position.y + y + yOffset, vertex.position.z, 1.0f), p.color};
			data.push_back(vp);
		});
		yOffset -= _voxelFont.lineHeight();
	}
	// TODO: the vertices should only be uploaded once for the whole glyph set. only the ibo should be dynamic and re-uploaded
	_vertexBuffer.update(_vertexBufferId, data.data);
	_vertexBuffer.update(_vertexBufferIndexId, indices);
}

core::AppState TestTraze::onRunning() {
	const int remaining = _eventBus->update(2);
	if (remaining) {
		Log::debug("Remaining events in queue: %i", remaining);
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
	_colorShader.shutdown();
	const std::vector<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	_vertexBuffer.shutdown();
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
	ImGui::Checkbox("Render board", &_renderBoard);
	ImGui::Checkbox("Render player names", &_renderPlayerNames);
	Super::onRenderUI();
}

void TestTraze::doRender() {
	if (_renderBoard) {
		_rawVolumeRenderer.render(_camera);
	}

	if (_renderPlayerNames && !_players.empty()) {
		const int elements = _vertexBuffer.elements(_vertexBufferIndexId, 1, sizeof(uint32_t));
		if (elements <= 0) {
			Log::warn("No player names rendered");
		}
		video::ScopedShader scoped(_colorShader);
		_colorShader.setViewprojection(_camera.viewProjectionMatrix());
		_colorShader.setModel(_namesModel);

		video::ScopedBuffer scopedBuf(_vertexBuffer);
		video::drawElements<uint32_t>(video::Primitive::Triangles, elements);
	}
}

TEST_APP(TestTraze)
