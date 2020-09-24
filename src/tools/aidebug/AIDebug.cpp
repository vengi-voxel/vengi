/**
 * @file
 */

#include "AIDebug.h"
#include "AIMessages_generated.h"
#include "ai-shared/common/CharacterId.h"
#include "ai-shared/common/CharacterMetaAttributes.h"
#include "app/App.h"
#include "attrib/AttributeType.h"
#include "attrib/ContainerValues.h"
#include "attrib/ShadowAttributes.h"
#include "command/Command.h"
#include "core/Color.h"
#include "core/Enum.h"
#include "core/EventBus.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include "network/IMsgProtocolHandler.h"
#include "network/ProtocolHandlerRegistry.h"
#include "IconsFontAwesome5.h"

namespace priv {

static const uint32_t TableFlags =
	ImGuiTableFlags_Scroll | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Resizable |
	ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg;

}

struct StateWorldHandler : public network::IMsgProtocolHandler<ai::StateWorld, void> {
	AIDebug &_aiDebug;
	StateWorldHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::StateWorld *msg, const uint8_t *rawData, size_t rawDataLength) override {
		_aiDebug.onMessage(msg, rawData, rawDataLength);
	}
};

struct CharacterDetailsHandler : public network::IMsgProtocolHandler<ai::CharacterDetails, void> {
	AIDebug &_aiDebug;
	CharacterDetailsHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::CharacterDetails *msg, const uint8_t *rawData,
						size_t rawDataLength) override {
		_aiDebug.onMessage(msg, rawData, rawDataLength);
	}
};

struct CharacterStaticHandler : public network::IMsgProtocolHandler<ai::CharacterStatic, void> {
	AIDebug &_aiDebug;
	CharacterStaticHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::CharacterStatic *msg, const uint8_t *rawData, size_t rawDataLength) override {
		_aiDebug.onMessage(msg, rawData, rawDataLength);
	}
};

struct NamesHandler : public network::IMsgProtocolHandler<ai::Names, void> {
	AIDebug &_aiDebug;
	NamesHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::Names *msg, const uint8_t *rawData, size_t rawDataLength) override {
		_aiDebug.onMessage(msg, rawData, rawDataLength);
	}
};

struct PauseHandler : public network::IMsgProtocolHandler<ai::Pause, void> {
	AIDebug &_aiDebug;
	PauseHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::Pause *msg, const uint8_t *rawData, size_t rawDataLength) override {
		_aiDebug.onMessage(msg);
	}
};

struct PingHandler : public network::IMsgProtocolHandler<ai::Ping, void> {
	AIDebug &_aiDebug;
	PingHandler(AIDebug &aiDebug) : _aiDebug(aiDebug) {
	}
	void executeWithRaw(void *, const ai::Ping *msg, const uint8_t *rawData, size_t rawDataLength) override {
		_aiDebug.onMessage(msg);
	}
};

AIDebug::AIDebug(const metric::MetricPtr &metric, const io::FilesystemPtr &filesystem,
				 const core::EventBusPtr &eventBus, const core::TimeProviderPtr &timeProvider,
				 const network::AINetworkPtr &aiNetwork, const network::MessageSenderPtr &messageSender,
				 const network::ProtocolHandlerRegistryPtr &protocolHandlerRegistry)
	: Super(metric, filesystem, eventBus, timeProvider), _aiNetwork(aiNetwork), _messageSender(messageSender) {

	protocolHandlerRegistry->registerHandler(ai::MsgType::StateWorld, std::make_shared<StateWorldHandler>(*this));
	protocolHandlerRegistry->registerHandler(ai::MsgType::CharacterDetails,
											 std::make_shared<CharacterDetailsHandler>(*this));
	protocolHandlerRegistry->registerHandler(ai::MsgType::CharacterStatic,
											 std::make_shared<CharacterStaticHandler>(*this));
	protocolHandlerRegistry->registerHandler(ai::MsgType::Names, std::make_shared<NamesHandler>(*this));
	protocolHandlerRegistry->registerHandler(ai::MsgType::Pause, std::make_shared<PauseHandler>(*this));
	protocolHandlerRegistry->registerHandler(ai::MsgType::Ping, std::make_shared<PingHandler>(*this));

	init(ORGANISATION, "aidebug");
	_allowRelativeMouseMode = false;
}

void AIDebug::disconnect() {
	_aiNetwork->disconnect();
	_peer = nullptr;
}

void AIDebug::connect(const core::String& host, int port) {
	_peer = _aiNetwork->connect(port, host);
}

bool AIDebug::addToHistory(const core::String& host, int port) {
	if (_history.empty()) {
		_history.emplace_back(host, port);
		return true;
	}
	bool found = false;
	for (const Server& s : _history) {
		if (s._host == host && s._port == port) {
			found = true;
			break;
		}
	}
	if (!found) {
		_history.emplace_back(host, port);
		return true;
	}

	return false;
}

void AIDebug::onMessage(const ai::StateWorld *, const uint8_t *rawData, size_t rawDataLength) {
	core_assert(lengthof(_stateWorldBuf) > rawDataLength);
	core_memcpy(_stateWorldBuf, rawData, rawDataLength);
	const ai::Message *rootMsg = ai::GetMessage(_stateWorldBuf);
	_stateWorldMsg = (ai::StateWorld *)rootMsg->data();
	_entityStates.clear();
	_stateWorldSize += rawDataLength;
	if (!_stateWorldMsg->states()) {
		return;
	}
	for (const auto &s : *_stateWorldMsg->states()) {
		_entityStates.put(s->character_id(), s);
	}
}

void AIDebug::onMessage(const ai::CharacterDetails *, const uint8_t *rawData, size_t rawDataLength) {
	const auto oldCharacterId = _chrDetailsMsg == nullptr ? 0 : _chrDetailsMsg->character_id();
	core_assert(lengthof(_chrDetailsBuf) > rawDataLength);
	core_memcpy(_chrDetailsBuf, rawData, rawDataLength);
	const ai::Message *rootMsg = ai::GetMessage(_chrDetailsBuf);
	ai::CharacterDetails *chrDetailsMsg = (ai::CharacterDetails *)rootMsg->data();
	if (_chrDetailsMsg == nullptr || chrDetailsMsg->character_id() != oldCharacterId) {
		_centerOnSelection = true;
	}
	_chrDetailsMsg = chrDetailsMsg;
	_characterDetailsSize += rawDataLength;
}

void AIDebug::onMessage(const ai::CharacterStatic *, const uint8_t *rawData, size_t rawDataLength) {
	core_assert(lengthof(_chrStaticBuf) > rawDataLength);
	core_memcpy(_chrStaticBuf, rawData, rawDataLength);
	const ai::Message *rootMsg = ai::GetMessage(_chrStaticBuf);
	_chrStaticMsg = (ai::CharacterStatic *)rootMsg->data();
	_nodeStates.clear();
	_characterStaticSize += rawDataLength;
	if (!_chrStaticMsg->node_statics()) {
		return;
	}
	for (const auto &s : *_chrStaticMsg->node_statics()) {
		_nodeStates.put(s->node_id(), s);
	}
}

void AIDebug::onMessage(const ai::Names *, const uint8_t *rawData, size_t rawDataLength) {
	core_assert(lengthof(_namesBuf) > rawDataLength);
	core_memcpy(_namesBuf, rawData, rawDataLength);
	const ai::Message *rootMsg = ai::GetMessage(_namesBuf);
	_namesMsg = (ai::Names *)rootMsg->data();
	_state = State::Debugging;
	_namesSize += rawDataLength;
	if (_stateWorldMsg != nullptr) {
		return;
	}
	if (_namesMsg->names()->size() > 0) {
		changeZone(_namesMsg->names()->Get(0)->c_str());
	}
}

void AIDebug::onMessage(const ai::Pause *msg) {
	_pause = msg->pause();
}

void AIDebug::onMessage(const ai::Ping *msg) {
	Log::debug("Ping received");
}

bool AIDebug::hasDetails() const {
	return _chrDetailsMsg != nullptr;
}

void AIDebug::togglePause() const {
	static flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendMessage(fbb, ai::MsgType::Pause, ai::CreatePause(fbb, !_pause).Union());
}

void AIDebug::executeCommand(const core::String& command) const {
	static flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendMessage(fbb, ai::MsgType::ExecuteCommand, ai::CreateExecuteCommand(fbb,
								fbb.CreateString(command.c_str(), command.size())).Union());
}

void AIDebug::step() const {
	static flatbuffers::FlatBufferBuilder fbb;
	int64_t millis = 1;
	_messageSender->sendMessage(fbb, ai::MsgType::Step, ai::CreateStep(fbb, millis).Union());
}

void AIDebug::changeZone(const char *zoneId) {
	Log::info("Change zone to %s", zoneId);
	static flatbuffers::FlatBufferBuilder fbb;
	_chrDetailsMsg = nullptr;
	_chrStaticMsg = nullptr;
	_stateWorldMsg = nullptr;
	_zoneId = zoneId;
	_messageSender->sendMessage(fbb, ai::MsgType::ChangeZone,
								ai::CreateChangeZone(fbb, fbb.CreateString(zoneId)).Union());
}

void AIDebug::selectEntity(ai::CharacterId entityId) {
	Log::info("Select entity %" PRIChrId, entityId);
	static flatbuffers::FlatBufferBuilder fbb;
	_chrDetailsMsg = nullptr;
	_chrStaticMsg = nullptr;
	_messageSender->sendMessage(fbb, ai::MsgType::Select, ai::CreateSelect(fbb, entityId).Union());
}

void AIDebug::updateNode(int nodeId, ai::CharacterId entityId, const core::String& nodeName, const core::String& nodeType, const core::String& condition) {
	static flatbuffers::FlatBufferBuilder fbb;
	const auto& snodeName = fbb.CreateString(nodeName.c_str());
	const auto& snodeType = fbb.CreateString(nodeType.c_str());
	const auto& scondition = fbb.CreateString(condition.c_str());
	_messageSender->sendMessage(fbb, ai::MsgType::UpdateNode,
			ai::CreateUpdateNode(fbb, nodeId, entityId, snodeName, snodeType, scondition).Union());
}

void AIDebug::addNode(int parentNodeId, ai::CharacterId entityId, const core::String& nodeName, const core::String& nodeType, const core::String& condition) {
	static flatbuffers::FlatBufferBuilder fbb;
	const auto& snodeName = fbb.CreateString(nodeName.c_str());
	const auto& snodeType = fbb.CreateString(nodeType.c_str());
	const auto& scondition = fbb.CreateString(condition.c_str());
	_messageSender->sendMessage(fbb, ai::MsgType::AddNode,
			ai::CreateAddNode(fbb, parentNodeId, entityId, snodeName, snodeType, scondition).Union());
}

void AIDebug::deleteNode(int nodeId, ai::CharacterId entityId) {
	static flatbuffers::FlatBufferBuilder fbb;
	_messageSender->sendMessage(fbb, ai::MsgType::DeleteNode,
			ai::CreateDeleteNode(fbb, nodeId, entityId).Union());
}

bool AIDebug::isSelected(ai::CharacterId entityId) const {
	if (_chrDetailsMsg == nullptr) {
		return false;
	}
	return _chrDetailsMsg->character_id() == entityId;
}

const ai::State *AIDebug::entityState() const {
	if (!hasDetails()) {
		return nullptr;
	}

	const ai::State *state;
	_entityStates.get(_chrDetailsMsg->character_id(), state);
	return state;
}

bool AIDebug::dbgConnect() {
	static bool keepRunning = true;
	ImGui::Begin("Connect", &keepRunning, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::PushFont(_bigFont);
	ImGui::Text("AI Remote Debugger");
	ImGui::PopFont();
	ImGui::Separator();
	ImGui::TextUnformatted("AI debug server address");
	bool connectClicked = false;
	connectClicked |= ImGui::InputText("Hostname", _aiServer, sizeof(_aiServer), ImGuiInputTextFlags_EnterReturnsTrue);
	connectClicked |= ImGui::InputInt("Port", &_port, ImGuiInputTextFlags_EnterReturnsTrue);
	connectClicked |= ImGui::Button(ICON_FA_WIFI " Connect");

	ImGui::Separator();

	if (connectClicked) {
		addToHistory(_aiServer, _port);
	}

	ImGui::TextUnformatted(ICON_FA_SERVER " Servers");
	if (ImGui::BeginTable("##serverlist", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Hostname", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
		ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
		ImGui::TableHeadersRow();
		for (const Server& s : _history) {
			ImGui::TableNextColumn();
			if (ImGui::Selectable(s._host.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
				core_memcpy(_aiServer, s._host.c_str(), sizeof(_aiServer));
				_aiServer[sizeof(_aiServer) - 1] = '\0';
				_port = s._port;
			}
			ImGui::TableNextColumn();
			ImGui::Text("%i", s._port);
		}
		ImGui::EndTable();
	}

	ImGui::End();

	if (!keepRunning) {
		requestQuit();
		return false;
	}

	return connectClicked && _aiServer[0] != '\0';
}

void AIDebug::dbgAttributes() {
	const ai::State *state = entityState();
	if (state == nullptr) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Attributes")) {
		if (ImGui::BeginTable("##attributeslist", 3, priv::TableFlags)) {
			ImGui::TableSetupColumn("Attribute", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableHeadersRow();
			for (const auto &a : *state->attrib()) {
				ImGui::TableNextColumn();
				const attrib::Type attribType = (attrib::Type)a->key();
				ImGui::TextUnformatted(network::EnumNameAttribType(attribType));
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", a->current());
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", a->max());
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void AIDebug::dbgMetaAttributes() {
	const ai::State *state = entityState();
	if (state == nullptr) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Properties")) {
		if (ImGui::BeginTable("##metaattributeslist", 2, priv::TableFlags)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableHeadersRow();

			for (const auto &a : *state->meta_attributes()) {
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(a->key()->c_str());
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(a->value()->c_str());
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void AIDebug::dbgAggro() {
	if (!hasDetails()) {
		return;
	}
	auto *aggrolist = _chrDetailsMsg->aggro();
	if (aggrolist == nullptr) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Aggro")) {
		if (ImGui::BeginTable("##aggrolist", 2, priv::TableFlags)) {
			ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Aggro", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableHeadersRow();
			for (const auto &e : *aggrolist) {
				ImGui::TableNextColumn();
				ImGui::Text("%" PRIChrId, e->character_id());
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", e->aggro());
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

static core::String humanSize(uint64_t bytes) {
	static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
	static const char length = lengthof(units);

	int unitIdx = 0;
	double dblBytes = bytes;
	for (; bytes / 1024 > 0 && unitIdx < length - 1; ++unitIdx, bytes /= 1024) {
		dblBytes = bytes / 1024.0;
	}

	return core::string::format("%.02lf%s", dblBytes, units[unitIdx]);
}

void AIDebug::dbgStats() {
	if (!_showStats) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Stats", &_showStats)) {
		if (_namesMsg != nullptr) {
			ImGui::Text("Zones: %i", (int)_namesMsg->names()->size());
		}
		if (_stateWorldMsg != nullptr) {
			ImGui::Text("Entities: %i", (int)_stateWorldMsg->states()->size());
		}
		ImGui::Separator();
		if (ImGui::BeginTable("Network traffic", 2, priv::TableFlags)) {
			ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableHeadersRow();
			ImGui::TableKeyValue("StateWorld", humanSize(_stateWorldSize));
			ImGui::TableKeyValue("CharacterStatic", humanSize(_characterStaticSize));
			ImGui::TableKeyValue("CharacterDetails", humanSize(_characterDetailsSize));
			ImGui::TableKeyValue("Names", humanSize(_namesSize));
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void AIDebug::dbgEntities() {
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Entities")) {
		if (_namesMsg != nullptr) {
			if (ImGui::BeginCombo(ICON_FA_MAP " Zone", _zoneId.c_str(), 0)) {
				for (size_t i = 0u; i < (size_t)_namesMsg->names()->size(); ++i) {
					const char *name = _namesMsg->names()->Get(i)->c_str();
					if (ImGui::Selectable(name, false)) {
						changeZone(name);
					}
				}
				ImGui::EndCombo();
			}
		}
		if (_stateWorldMsg != nullptr) {
			ImGui::InputText(ICON_FA_SEARCH_LOCATION " Filter", _entityListFilter, sizeof(_entityListFilter));
			if (ImGui::BeginTable("##entitylist", 2, priv::TableFlags)) {
				ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
				ImGui::TableHeadersRow();
				for (const auto &e : *_stateWorldMsg->states()) {
					core::String name = "Unknown";
					for (const auto &a : *e->meta_attributes()) {
						if (!SDL_strcmp(ai::attributes::NAME, a->key()->c_str())) {
							name = a->value()->c_str();
							break;
						}
					}
					if (_entityListFilter[0] != '\0') {
						if (SDL_strstr(name.c_str(), _entityListFilter) == nullptr) {
							char buf[32];
							SDL_snprintf(buf, sizeof(buf), "%" PRIChrId, e->character_id());
							if (SDL_strstr(buf, _entityListFilter) == nullptr) {
								continue;
							}
						}
					}
					ImGui::TableNextColumn();
					ImGui::Text("%" PRIChrId, e->character_id());
					ImGui::TableNextColumn();
					if (ImGui::Selectable(name.c_str(), isSelected(e->character_id()),
										  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
						selectEntity(e->character_id());
					}
				}
				ImGui::EndTable();
			}
		}
	}
	ImGui::End();
}

void AIDebug::dbgTreeNode_r(const ai::StateNode *node, int level) {
	ImGui::TableNextColumn();
	const ai::StateNodeStatic *staticNodeDetails = nullptr;
	if (!_nodeStates.get(node->node_id(), staticNodeDetails)) {
		Log::warn("Could not get static node details for node %i", node->node_id());
		return;
	}
	const char *nodeName = staticNodeDetails->name()->c_str();
	const char *nodeType = staticNodeDetails->type()->c_str();
	const char *nodeParameters = staticNodeDetails->parameters()->c_str();
	const char *conditionName = node->condition()->c_str();
	const bool hasChildren = (node->children()->size() > 0);

	bool open = false;
	if (hasChildren) {
		open = ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen, ICON_FA_FOLDER " %s", nodeName);
	} else {
		ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth);
	}

	if (ImGui::BeginPopupContextItem("Edit")) {
		ImGui::Text("%s", nodeName);
		if (ImGui::Selectable(ICON_FA_EDIT " Edit")) {
			_dbgTreeIdEdit = node->node_id();
			Log::info("Edit node %i", _dbgTreeIdEdit);
		}
		if (ImGui::Selectable(ICON_FA_MINUS " Delete")) {
			deleteNode(node->node_id(), _chrDetailsMsg->character_id());
		}
		if (ImGui::Selectable(ICON_FA_PLUS " Add")) {
			_dbgTreeIdAdd = node->node_id();
		}
		ImGui::Separator();
		ImGui::EndPopup();
	}

	ImGui::TableNextColumn();

	ImGui::TextUnformatted(nodeParameters); ImGui::TableNextColumn();
	ImGui::TextUnformatted(nodeType); ImGui::TableNextColumn();
	if (node->condition_state()) {
		ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Green);
	} else {
		ImGui::PushStyleColor(ImGuiCol_Text, core::Color::Red);
	}
	ImGui::TextUnformatted(conditionName); ImGui::TableNextColumn();
	ImGui::PopStyleColor(1);
	ImGui::Text("%s", ai::EnumNameTreeNodeStatus(node->status())); ImGui::TableNextColumn();
	ImGui::Text("%li", node->last_run());
	if (open) {
		for (const auto &c : *node->children()) {
			dbgTreeNode_r(c, level + 1);
		}
		ImGui::TreePop();
	}
}

void AIDebug::dbgTree() {
	if (!hasDetails()) {
		return;
	}
	const ImVec2 size(frameBufferWidth(), 200);
	const ImVec2 pos(0, frameBufferHeight() - size.y);
	ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Behaviourtree")) {
		if (ImGui::BeginTable("##nodelist", 6, priv::TableFlags)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableSetupColumn(ICON_FA_CLOCK " Last run", ImGuiTableColumnFlags_WidthAlwaysAutoResize);
			ImGui::TableHeadersRow();
			dbgTreeNode_r(_chrDetailsMsg->root(), 0);
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

bool AIDebug::dbgMapIsVisible(const ImVec2& pos, const ImVec2& mapMins, const ImVec2& mapMaxs) const {
	return pos.x > mapMins.x && pos.y > mapMins.y && pos.x < mapMaxs.x && pos.y < mapMaxs.y;
}

float AIDebug::dbgMapZoom() const {
	return _zoom;
}

ImVec2 AIDebug::dbgMapConvertEntPos(float x, float y) const {
	return ImVec2(dbgMapZoom() * (_dbgMapOffset.x + x), dbgMapZoom() * (_dbgMapOffset.y + y));
}

void AIDebug::dbgMap() {
	if (_stateWorldMsg == nullptr || _stateWorldMsg->states() == nullptr) {
		return;
	}
	if (_centerOnSelection) {
		for (const auto &e : *_stateWorldMsg->states()) {
			const bool selected = isSelected(e->character_id());
			if (selected) {
				_dbgMapOffset = ImVec2(-e->position()->x() * dbgMapZoom() + _frameBufferDimension.x / 2.0f, -e->position()->z() * dbgMapZoom() + _frameBufferDimension.y / 2.0f);
				break;
			}
		}
		_centerOnSelection = false;
	}

	const uint32_t viewRadiusColor = ImGui::GetColorU32(core::Color::Yellow);
	const uint32_t attackRadiusColor = ImGui::GetColorU32(core::Color::Red);
	const uint32_t entityColor = ImGui::GetColorU32(core::Color::White);
	const uint32_t selectedEntityColor = ImGui::GetColorU32(core::Color::Green);
	const uint32_t hoveredEntityColor = ImGui::GetColorU32(core::Color::DarkGray);
	const uint32_t healthColor = ImGui::GetColorU32(core::Color::DarkGreen);
	const uint32_t damageColor = ImGui::GetColorU32(core::Color::DarkRed);
	const uint32_t homecol = ImGui::GetColorU32(core::Color::Blue);
	const uint32_t targetcol = ImGui::GetColorU32(core::Color::Cyan);
	ImGui::SetNextWindowSize(ImVec2(_frameBufferDimension.x, _frameBufferDimension.y));
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	const ImVec2 mapMins(0.0f, 0.0f);
	const ImVec2 mapMaxs(_frameBufferDimension.x, _frameBufferDimension.y);
	if (ImGui::Begin("##map", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings)) {
		dbgBar();

		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0)) {
			_dbgMapOffset.x += _mouseRelativePos.x;
			_dbgMapOffset.y += _mouseRelativePos.y;
		}

		const float radius = 10.0f * dbgMapZoom();
		const ImVec2 entSize(radius * 2.0f);
		ImDrawList *draw = ImGui::GetWindowDrawList();
		const ImVec2& clipRectMins = ImGui::GetCursorPos();
		ImVec2 clipRectMaxs = ImGui::GetContentRegionAvail();
		clipRectMaxs.x += clipRectMins.x;
		clipRectMaxs.y += clipRectMins.y;
		draw->PushClipRect(clipRectMins, clipRectMaxs, true);
		for (const auto &e : *_stateWorldMsg->states()) {
			const ImVec2& entPos = dbgMapConvertEntPos(e->position()->x(), e->position()->z());
			if (!dbgMapIsVisible(entPos, mapMins, mapMaxs)) {
				continue;
			}
			const float orientation = e->orientation();
			const glm::vec2 dir(glm::cos(orientation), glm::sin(orientation));
			ImGui::SetCursorScreenPos({entPos.x - radius, entPos.y - radius});
			const bool selected = isSelected(e->character_id());
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetColorU32(ImGuiCol_HeaderActive, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(ImGuiCol_HeaderHovered, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_Header, 0.0f));
			if (ImGui::Selectable("##ent", selected, ImGuiSelectableFlags_AllowDoubleClick, entSize)) {
				selectEntity(e->character_id());
			}
			ImGui::PopStyleColor(3);

			attrib::Values attribCurrent;
			attrib::Values attribMax;
			attribCurrent.fill(0.0);
			attribMax.fill(0.0);
			for (const auto& a : *e->attrib()) {
				attribCurrent[a->key()] = a->current();
				attribMax[a->key()] = a->max();
			}

			const bool hover = ImGui::TooltipText(
					"ID: %" PRIChrId "\n"
					"Pos: %f:%f:%f\n"
					"Home: %f:%f:%f\n"
					"Target: %f:%f:%f\n"
					"Strength: %.2f/%.2f",
					e->character_id(),
					e->position()->x(), e->position()->y(), e->position()->z(),
					e->home_position()->x(), e->home_position()->y(), e->home_position()->z(),
					e->target_position()->x(), e->target_position()->y(), e->target_position()->z(),
					attribCurrent[core::enumVal(attrib::Type::STRENGTH)], attribMax[core::enumVal(attrib::Type::STRENGTH)]);

			uint32_t col = entityColor;
			if (selected) {
				col = selectedEntityColor;
			} else if (hover) {
				col = hoveredEntityColor;
			}
			draw->AddCircle(entPos, radius, col, 12, 1.0f);
			draw->AddLine(entPos, {entPos.x + dir.x * radius * 2.0f, entPos.y + dir.y * radius * 2.0f}, col, 1.0f);
			if (selected) {
				const ImVec2& homePos = dbgMapConvertEntPos(e->home_position()->x(), e->home_position()->z());
				const ImVec2& targetPos = dbgMapConvertEntPos(e->target_position()->x(), e->target_position()->z());
				draw->AddLine(entPos, homePos, homecol, 1.0f);
				draw->AddLine(entPos, targetPos, targetcol, 1.0f);
			}

			const float viewRadius = (float)attribCurrent[core::enumVal(attrib::Type::VIEWDISTANCE)] * dbgMapZoom();
			if (viewRadius > radius) {
				draw->AddCircle(entPos, (float)viewRadius, viewRadiusColor, 18, 1.0f);
			}

			const float attackRadius = (float)attribCurrent[core::enumVal(attrib::Type::ATTACKRANGE)] * dbgMapZoom();
			if (attackRadius > 0.0) {
				draw->AddCircle(entPos, (float)attackRadius, attackRadiusColor, 12, 1.0f);
			}

			const float barHeight = 4.0;
			const float healthCurrent = attribCurrent[core::enumVal(attrib::Type::ATTACKRANGE)];
			const float healthMax = attribMax[core::enumVal(attrib::Type::ATTACKRANGE)];
			if (healthMax > 0.0f) {
				const float healthWidth = healthCurrent * 100.0f / healthMax;
				const ImVec2 minsHealth(entPos.x - radius, entPos.y + radius);
				const ImVec2 maxsHealth(minsHealth.x + radius * 2.0f * healthWidth / 100.0f, minsHealth.y + barHeight);
				draw->AddRectFilled(minsHealth, maxsHealth, healthColor);

				const float damageWidth = 100.0f - healthWidth;
				if (damageWidth > 0.0f) {
					const ImVec2 minsDamage(maxsHealth.x, minsHealth.y);
					const ImVec2 maxsDamage(entPos.x + radius, maxsHealth.y);
					draw->AddRectFilled(minsDamage, maxsDamage, damageColor);
				}
			}
		}
		draw->PopClipRect();
	}
	if (ImGui::IsWindowHovered()) {
		_zoom = core_max(0.01f, dbgMapZoom() + ImGui::GetIO().MouseWheel * 0.1f);
		// TODO: don't center on selection - but on the relative position in the map view
		//_centerOnSelection = true;
	}
	ImGui::End();
}

void AIDebug::dbgBar() {
	if (ImGui::Button("Disconnect")) {
		disconnect();
	}
	ImGui::SameLine();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
	ImGui::SameLine();
	bool pausePressed;
	if (_pause) {
		pausePressed = ImGui::ToggleButton(ICON_FA_PLAY " Pause", _pause);
	} else {
		pausePressed = ImGui::ToggleButton(ICON_FA_PAUSE " Pause", _pause);
	}
	if (pausePressed) {
		togglePause();
	}
	ImGui::SameLine();
	if (ImGui::DisabledButton(ICON_FA_STEP_FORWARD " Step", !_pause)) {
		step();
	}
	ImGui::SameLine();
	if (ImGui::ToggleButton(ICON_FA_CHART_BAR " Stats", _showStats)) {
		_showStats ^= true;
	}
	ImGui::SameLine();
	static char command[1024];
	if (ImGui::InputText("Command", command, sizeof(command))) {
		executeCommand(command);
	}
}

void AIDebug::dbgAddNode() {
	bool addActive = _dbgTreeIdAdd != -1;
	if (!addActive) {
		return;
	}
	const ImVec2 pos(_frameBufferDimension.x / 2.0f, _frameBufferDimension.y / 2.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Add", &addActive, ImGuiWindowFlags_AlwaysAutoResize)) {
		// TODO: implement adding new child node
	}
	if (!addActive) {
		_dbgTreeIdAdd = -1;
	}
	ImGui::End();
}

void AIDebug::dbgEditNode() {
	bool editActive = _dbgTreeIdEdit != -1;
	if (!editActive) {
		return;
	}
	const ImVec2 pos(_frameBufferDimension.x / 2.0f, _frameBufferDimension.y / 2.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Edit", &editActive, ImGuiWindowFlags_AlwaysAutoResize)) {
		const ai::StateNodeStatic* staticNode;
		if (_nodeStates.get(_dbgTreeIdEdit, staticNode)) {
			// TODO: implement edit
			ImGui::LabelText("Name", "%s", staticNode->name()->c_str());
			ImGui::LabelText("Type", "%s", staticNode->type()->c_str());
		} else {
			editActive = false;
		}
	}
	if (!editActive) {
		_dbgTreeIdEdit = -1;
	}
	ImGui::End();
}

void AIDebug::onEvent(const network::DisconnectEvent &event) {
	Log::info("Received disconnect event");
	_aiNetwork->destroy();
	_state = State::Connect;
	_entityStates.clear();
	_nodeStates.clear();
	_chrDetailsMsg = nullptr;
	_chrStaticMsg = nullptr;
	_stateWorldMsg = nullptr;
	_namesMsg = nullptr;
	_dbgMapOffset = { 0.0f, 0.0f };
	_pause = false;
	_centerOnSelection = false;
	_zoneId = "";
	_zoom = 1.0f;
	_entityListFilter[0] = '\0';
	_stateWorldSize = 0u;
	_characterDetailsSize = 0u;
	_characterStaticSize = 0u;
	_namesSize = 0u;
	_dbgTreeIdAdd = -1;
	_dbgTreeIdEdit = -1;
}

void AIDebug::onEvent(const network::NewConnectionEvent &event) {
	Log::debug("Received connection event");
	_state = State::Debugging;
}

app::AppState AIDebug::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_serverList = core::Var::get("dbg_serverlist", "");

	command::Command::registerCommand("connect", [this] (const command::CmdArgs &args) {
		const core::String host = args.empty() ? _aiServer : args[0];
		const int port = args.size() >= 2 ? core::string::toInt(args[1]) : _port;
		connect(host, port);
	});

	command::Command::registerCommand("disconnect", [this] (const command::CmdArgs &args) {
		disconnect();
	});

	return state;
}

app::AppState AIDebug::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!_aiNetwork->init()) {
		Log::error("Failed to initialize the network layer");
		return app::AppState::InitFailure;
	}

	const core::String& servers = _serverList->strVal();
	if (!servers.empty()) {
		core::Tokenizer tokenizer(servers, ";");
		while (tokenizer.hasNext()) {
			core::String hostAndPort = tokenizer.next();
			const size_t i = hostAndPort.find(":");
			if (i == core::String::npos) {
				addToHistory(hostAndPort, _defaultPort);
				continue;
			}
			const core::String& host = hostAndPort.substr(0, i);
			const core::String& portStr = hostAndPort.substr(i + 1, hostAndPort.size() - (i + 1));
			addToHistory(host, core::string::toInt(portStr));
		}
	}

	eventBus()->subscribe<network::NewConnectionEvent>(*this);
	eventBus()->subscribe<network::DisconnectEvent>(*this);
	return state;
}

app::AppState AIDebug::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	_aiNetwork->update();
	return state;
}

void AIDebug::onRenderUI() {
	switch (_state) {
	case State::Connect:
		if (dbgConnect()) {
			connect(_aiServer, _port);
		}
		break;
	case State::Debugging:
		dbgMap();
		dbgEntities();
		dbgStats();
		dbgTree();
		dbgAttributes();
		dbgMetaAttributes();
		dbgAggro();
		dbgEditNode();
		dbgAddNode();
		break;
	default:
		break;
	}
}

app::AppState AIDebug::onCleanup() {
	eventBus()->unsubscribe<network::NewConnectionEvent>(*this);
	eventBus()->unsubscribe<network::DisconnectEvent>(*this);
	_aiNetwork->shutdown();

	core::String servers;
	for (const Server& s : _history) {
		if (servers.contains(s._host)) {
			continue;
		}
		if (!servers.empty()) {
			servers.append(";");
		}
		servers.append(s._host);
		servers.append(":");
		servers.append(core::string::toString(s._port));
	}
	_serverList->setVal(servers);

	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr &eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr &filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr &timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr &metric = std::make_shared<metric::Metric>();
	const network::ProtocolHandlerRegistryPtr &protocolHandlerRegistry =
		core::make_shared<network::ProtocolHandlerRegistry>();
	const network::AINetworkPtr aiNetwork = core::make_shared<network::AINetwork>(protocolHandlerRegistry, eventBus);
	const network::MessageSenderPtr &messageSender = core::make_shared<network::MessageSender>(aiNetwork);
	AIDebug app(metric, filesystem, eventBus, timeProvider, aiNetwork, messageSender, protocolHandlerRegistry);
	return app.startMainLoop(argc, argv);
}
