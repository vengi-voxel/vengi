/**
 * @file
 */

#include "Server.h"
#include "AIMessages_generated.h"
#include "SelectHandler.h"
#include "PauseHandler.h"
#include "ResetHandler.h"
#include "StepHandler.h"
#include "ChangeHandler.h"
#include "AddNodeHandler.h"
#include "DeleteNodeHandler.h"
#include "UpdateNodeHandler.h"

#include "attrib/ShadowAttributes.h"
#include "backend/entity/ai/condition/ConditionParser.h"
#include "backend/entity/ai/tree/TreeNodeParser.h"
#include "backend/entity/ai/zone/Zone.h"
#include "core/EventBus.h"
#include "core/SharedPtr.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "flatbuffers/flatbuffers.h"
#include "network/NetworkEvents.h"
#include "backend/network/ServerNetwork.h"
#include "backend/network/ServerMessageSender.h"
#include <memory>

namespace backend {

namespace {
const int SV_BROADCAST_CHRDETAILS = 1 << 0;
const int SV_BROADCAST_STATE      = 1 << 1;
}

Server::Server(AIRegistry& aiRegistry, const metric::MetricPtr& metric,
		short port, const core::String& hostname) :
		_aiRegistry(aiRegistry), _selectedCharacterId(AI_NOTHING_SELECTED),
		_time(0L), _pause(false), _zone(nullptr), _port(port), _hostname(hostname) {
	const network::ProtocolHandlerRegistryPtr& r = core::make_shared<network::ProtocolHandlerRegistry>();
	r->registerHandler(ai::MsgType::Select, std::make_shared<SelectHandler>(*this));
	r->registerHandler(ai::MsgType::Pause, std::make_shared<PauseHandler>(*this));
	r->registerHandler(ai::MsgType::Reset, std::make_shared<ResetHandler>(*this));
	r->registerHandler(ai::MsgType::Step, std::make_shared<StepHandler>(*this));
	r->registerHandler(ai::MsgType::Ping, std::make_shared<SelectHandler>(*this));
	r->registerHandler(ai::MsgType::ChangeZone, std::make_shared<ChangeHandler>(*this));
	r->registerHandler(ai::MsgType::AddNode, std::make_shared<AddNodeHandler>(*this));
	r->registerHandler(ai::MsgType::DeleteNode, std::make_shared<DeleteNodeHandler>(*this));
	r->registerHandler(ai::MsgType::UpdateNode, std::make_shared<UpdateNodeHandler>(*this));

	_eventBus = std::make_shared<core::EventBus>(2);
	_eventBus->subscribe<network::NewConnectionEvent>(*this);
	_eventBus->subscribe<network::DisconnectEvent>(*this);

	_network = std::make_shared<network::AIServerNetwork>(r, _eventBus, metric);
	_messageSender = std::make_shared<network::AIMessageSender>(_network, metric);
}

Server::~Server() {
	_network->shutdown();
}

void Server::enqueueEvent(const Event& event) {
	core::ScopedLock scopedLock(_lock);
	_events.push_back(event);
}

void Server::onEvent(const network::NewConnectionEvent& evt) {
	Event event;
	event.type = EV_NEWCONNECTION;
	event.data.peer = evt.get();
	enqueueEvent(event);
}

void Server::onEvent(const network::DisconnectEvent& event) {
	Log::info("remote debugger disconnect");
	Zone* zone = _zone;
	if (zone == nullptr) {
		return;
	}

	zone->setDebug(false);
	if (_zone.compare_exchange(zone, nullptr) != nullptr) {
		// restore the zone state if no player is left for debugging
		const bool pauseState = _pause;
		if (pauseState) {
			pause(false);
		}

		// only if noone else already started a new debug session
		resetSelection();
	}
}

bool Server::start() {
	if (!_network->init()) {
		return false;
	}

	return _network->bind(_port, _hostname, 1, 1);
}

void Server::broadcastState(const Zone* zone) {
	core_trace_scoped(AIServerBroadcastState);
	_broadcastMask |= SV_BROADCAST_STATE;
	core::DynamicArray<flatbuffers::Offset<ai::State>> offsets;
	offsets.reserve(zone->size());
	auto func = [&] (const AIPtr& ai) {
		const ICharacterPtr& chr = ai->getCharacter();
		const glm::vec3& chrPosition = chr->getPosition();
		const ai::Vec3 position(chrPosition.x, chrPosition.y, chrPosition.z);
		const ai::CharacterMetaAttributes& chrMetaAttributes = chr->getMetaAttributes();
		auto metaAttributeIter = chrMetaAttributes.begin();
		auto metaAttributes = _stateFBB.CreateVector<flatbuffers::Offset<ai::MapEntry>>(chrMetaAttributes.size(),
			[&] (size_t i) {
				const core::String& sname = metaAttributeIter->first;
				const core::String& svalue = metaAttributeIter->second;
				auto name = _stateFBB.CreateString(sname.c_str(), sname.size());
				auto value = _stateFBB.CreateString(svalue.c_str(), svalue.size());
				++metaAttributeIter;
				return ai::CreateMapEntry(_stateFBB, name, value);
			});
		const attrib::ShadowAttributes& chrShadowAttributes = chr->shadowAttributes();
		auto attributes = _stateFBB.CreateVector<flatbuffers::Offset<ai::AttribEntry>>((size_t)attrib::Type::MAX,
			[&] (size_t i) {
				attrib::Type attribType = (attrib::Type)i;
				return ai::CreateAttribEntry(_stateFBB, (int)i, chrShadowAttributes.current(attribType), chrShadowAttributes.max(attribType));
			});
		offsets.push_back(ai::CreateState(_stateFBB, chr->getId(), &position, chr->getOrientation(), metaAttributes, attributes));
	};
	zone->execute(func);
	auto states = _stateFBB.CreateVector(offsets.data(), offsets.size());
	_messageSender->broadcastServerMessage(_stateFBB, ai::MsgType::StateWorld,
		ai::CreateStateWorld(_stateFBB, states).Union());
}

void Server::addChildren(const TreeNodePtr& node, core::DynamicArray<flatbuffers::Offset<ai::StateNodeStatic>>& offsets) const {
	for (const TreeNodePtr& childNode : node->getChildren()) {
		const int32_t nodeId = childNode->getId();
		const core::String& snodename = childNode->getName();
		const core::String& snodetype = childNode->getType();
		const core::String& snodeparameters = childNode->getParameters();
		const core::String& sconditionname = childNode->getCondition()->getName();
		const core::String& sconditionparameters = childNode->getCondition()->getParameters();
		auto nodename = _staticCharacterDetailsFBB.CreateString(snodename.c_str(), snodename.size());
		auto nodetype = _staticCharacterDetailsFBB.CreateString(snodetype.c_str(), snodetype.size());
		auto nodeparameters = _staticCharacterDetailsFBB.CreateString(snodeparameters.c_str(), snodeparameters.size());
		auto conditionname = _staticCharacterDetailsFBB.CreateString(sconditionname.c_str(), sconditionname.size());
		auto conditionparameters = _staticCharacterDetailsFBB.CreateString(sconditionparameters.c_str(), sconditionparameters.size());
		offsets.push_back(ai::CreateStateNodeStatic(_staticCharacterDetailsFBB, nodeId, nodename, nodetype,
			 nodeparameters, conditionname, conditionparameters));
		addChildren(childNode, offsets);
	}
}

void Server::broadcastStaticCharacterDetails(const Zone* zone) {
	const ai::CharacterId id = _selectedCharacterId;
	if (id == AI_NOTHING_SELECTED) {
		return;
	}

	core::DynamicArray<flatbuffers::Offset<ai::StateNodeStatic>> offsets;
	offsets.reserve(zone->size());
	static const auto func = [&] (const AIPtr& ai) {
		if (!ai) {
			return false;
		}
		const TreeNodePtr& node = ai->getBehaviour();
		const int32_t nodeId = node->getId();
		const core::String& snodename = node->getName();
		const core::String& snodetype = node->getType();
		const core::String& snodeparameters = node->getParameters();
		const core::String& sconditionname = node->getCondition()->getName();
		const core::String& sconditionparameters = node->getCondition()->getParameters();
		auto nodename = _staticCharacterDetailsFBB.CreateString(snodename.c_str(), snodename.size());
		auto nodetype = _staticCharacterDetailsFBB.CreateString(snodetype.c_str(), snodetype.size());
		auto nodeparameters = _staticCharacterDetailsFBB.CreateString(snodeparameters.c_str(), snodeparameters.size());
		auto conditionname = _staticCharacterDetailsFBB.CreateString(sconditionname.c_str(), sconditionname.size());
		auto conditionparameters = _staticCharacterDetailsFBB.CreateString(sconditionparameters.c_str(), sconditionparameters.size());
		offsets.push_back(ai::CreateStateNodeStatic(_staticCharacterDetailsFBB, nodeId, nodename, nodetype,
			 nodeparameters, conditionname, conditionparameters));
		addChildren(node, offsets);
		_messageSender->broadcastServerMessage(_staticCharacterDetailsFBB, ai::MsgType::CharacterStatic,
			ai::CreateCharacterStatic(_staticCharacterDetailsFBB, ai->getId(), _staticCharacterDetailsFBB.CreateVector(offsets.data(), offsets.size())).Union());
		return true;
	};

	if (!zone->execute(id, func)) {
		resetSelection();
	}
}

flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ai::StateNode>>> Server::addChildren(const TreeNodePtr& node, const AIPtr& ai) const {
	const TreeNodes& children = node->getChildren();
	std::vector<bool> currentlyRunning(children.size());
	node->getRunningChildren(ai, currentlyRunning);
	const int64_t aiTime = ai->_time;
	const size_t length = children.size();
	core::DynamicArray<flatbuffers::Offset<ai::StateNode>> offsets;
	offsets.reserve(length);
	for (size_t i = 0u; i < length; ++i) {
		const TreeNodePtr& childNode = children[i];
		const int32_t nodeId = childNode->getId();
		const ConditionPtr& condition = childNode->getCondition();
		const core::String conditionStr = condition ? condition->getNameWithConditions(ai) : "";
		const int64_t lastRun = childNode->getLastExecMillis(ai);
		const int64_t delta = lastRun == -1 ? -1 : aiTime - lastRun;
		ai::TreeNodeStatus status = node->getLastStatus(ai);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ai::StateNode>>> childNodeChildren = addChildren(childNode, ai);

		flatbuffers::Offset<ai::StateNode> child = ai::CreateStateNode(_characterDetailsFBB,
			nodeId, _characterDetailsFBB.CreateString(conditionStr.c_str(), conditionStr.size()),
			childNodeChildren, delta, (int)status, true);
		offsets.push_back(child);
	}
	return _characterDetailsFBB.CreateVector(offsets.data(), offsets.size());
}

void Server::broadcastCharacterDetails(const Zone* zone) {
	core_trace_scoped(AIServerBroadcastCharacterDetails);
	_broadcastMask |= SV_BROADCAST_CHRDETAILS;
	const ai::CharacterId id = _selectedCharacterId;
	if (id == AI_NOTHING_SELECTED) {
		return;
	}
	static const auto func = [&] (const AIPtr& ai) {
		if (!ai) {
			return false;
		}
		const TreeNodePtr& node = ai->getBehaviour();
		const int32_t nodeId = node->getId();
		const ConditionPtr& condition = node->getCondition();
		const core::String conditionStr = condition ? condition->getNameWithConditions(ai) : "";
		ai::TreeNodeStatus status = node->getLastStatus(ai);
		const int64_t lastRun = _time - node->getLastExecMillis(ai);
		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ai::StateNode>>> children = addChildren(node, ai);
		flatbuffers::Offset<ai::StateNode> rootnode = ai::CreateStateNode(_characterDetailsFBB, nodeId,
			_characterDetailsFBB.CreateString(conditionStr.c_str(), conditionStr.size()), children, lastRun, (int)status, true);

		flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ai::StateAggroEntry>>> aggro;
		const AggroMgr::Entries& entries = ai->getAggroMgr().getEntries();
		core::DynamicArray<flatbuffers::Offset<ai::StateAggroEntry>> offsets;
		offsets.reserve(entries.size());
		for (const Entry& e : entries) {
			offsets.push_back(ai::CreateStateAggroEntry(_characterDetailsFBB, e.getCharacterId(), e.getAggro()));
		}

		_messageSender->broadcastServerMessage(_characterDetailsFBB, ai::MsgType::CharacterDetails,
			ai::CreateCharacterDetails(_characterDetailsFBB, ai->getId(),
				_characterDetailsFBB.CreateVector(offsets.data(), offsets.size()), rootnode).Union());
		return true;
	};
	if (!zone->execute(id, func)) {
		resetSelection();
	}
}

void Server::handleEvents(Zone* zone, bool pauseState) {
	std::vector<Event> events;
	{
		core::ScopedLock scopedLock(_lock);
		events = std::move(_events);
		_events.clear();
	}
	bool sendNames = false;
	for (Event& event : events) {
		switch (event.type) {
		case EV_SELECTION: {
			if (zone == nullptr || event.data.characterId == AI_NOTHING_SELECTED) {
				resetSelection();
			} else {
				_selectedCharacterId = event.data.characterId;
				broadcastStaticCharacterDetails(zone);
				if (pauseState) {
					broadcastState(zone);
					broadcastCharacterDetails(zone);
				}
			}
			break;
		}
		case EV_STEP: {
			const int64_t queuedStepMillis = event.data.stepMillis;
			auto func = [=] (const AIPtr& ai) {
				if (!ai->isPause())
					return;
				ai->setPause(false);
				ai->update(queuedStepMillis, true);
				ai->getBehaviour()->execute(ai, queuedStepMillis);
				ai->setPause(true);
			};
			if (zone != nullptr) {
				zone->executeParallel(func);
				broadcastState(zone);
				broadcastCharacterDetails(zone);
			}
			break;
		}
		case EV_RESET: {
			static auto func = [] (const AIPtr& ai) {
				ai->getBehaviour()->resetState(ai);
			};
			event.data.zone->executeParallel(func);
			break;
		}
		case EV_PAUSE: {
			const bool newPauseState = event.data.pauseState;
			_pause = newPauseState;
			if (zone != nullptr) {
				auto func = [=] (const AIPtr& ai) {
					ai->setPause(newPauseState);
				};
				zone->executeParallel(func);
				_messageSender->broadcastServerMessage(_pauseFBB, ai::MsgType::Pause,
					ai::CreatePause(_pauseFBB, _pause).Union());
				// send the last time the most recent state until we unpause
				if (newPauseState) {
					broadcastState(zone);
					broadcastCharacterDetails(zone);
				}
			}
			break;
		}
		case EV_UPDATESTATICCHRDETAILS: {
			broadcastStaticCharacterDetails(event.data.zone);
			break;
		}
		case EV_NEWCONNECTION: {
			_messageSender->broadcastServerMessage(_pauseFBB, ai::MsgType::Pause,
				ai::CreatePause(_pauseFBB, pauseState).Union());

			sendNames = true;
			Log::info("new remote debugger connection");
			break;
		}
		case EV_ZONEADD: {
			if (!_zones.insert(event.data.zone)) {
				return;
			}
			_names.clear();
			for (const auto& z : _zones) {
				_names.push_back(z->first->getName());
			}
			sendNames = true;
			break;
		}
		case EV_ZONEREMOVE: {
			_zone.compare_exchange(event.data.zone, nullptr);
			if (_zones.remove(event.data.zone) != 1) {
				return;
			}
			_names.clear();
			for (const auto& z : _zones) {
				_names.push_back(z->first->getName());
			}
			sendNames = true;
			break;
		}
		case EV_SETDEBUG: {
			if (_pause) {
				pause(false);
			}

			Zone* nullzone = nullptr;
			_zone = nullzone;
			resetSelection();

			for (const auto& iter : _zones) {
				Zone* z = iter->first;
				const bool debug = z->getName() == event.strData;
				if (!debug) {
					continue;
				}
				if (_zone.compare_exchange(nullzone, z)) {
					z->setDebug(debug);
				}
			}

			break;
		}
		case EV_MAX:
			break;
		}
	}

	if (sendNames) {
		auto names = _namesFBB.CreateVector<flatbuffers::Offset<flatbuffers::String>>(_names.size(),
			[&] (size_t i) {
				return _namesFBB.CreateString(_names[i].c_str(), _names[i].size());
			});
		_messageSender->broadcastServerMessage(_namesFBB, ai::MsgType::Names,
			ai::CreateNames(_namesFBB, names).Union());
	}
}

void Server::resetSelection() {
	_selectedCharacterId = AI_NOTHING_SELECTED;
}

bool Server::updateNode(const ai::CharacterId& characterId, int32_t nodeId, const core::String& name, const core::String& type, const core::String& condition) {
	Zone* zone = _zone;
	if (zone == nullptr) {
		return false;
	}
	const AIPtr& ai = zone->getAI(characterId);
	const TreeNodePtr& node = ai->getBehaviour()->getId() == nodeId ? ai->getBehaviour() : ai->getBehaviour()->getChild(nodeId);
	if (!node) {
		return false;
	}
	ConditionParser conditionParser(_aiRegistry, condition);
	const ConditionPtr& conditionPtr = conditionParser.getCondition();
	if (!conditionPtr) {
		Log::error("Failed to parse the condition '%s'", condition.c_str());
		return false;
	}
	TreeNodeParser treeNodeParser(_aiRegistry, type);
	TreeNodePtr newNode = treeNodeParser.getTreeNode(name);
	if (!newNode) {
		Log::error("Failed to parse the node '%s'", type.c_str());
		return false;
	}
	newNode->setCondition(conditionPtr);
	for (auto& child : node->getChildren()) {
		newNode->addChild(child);
	}

	const TreeNodePtr& root = ai->getBehaviour();
	if (node == root) {
		ai->setBehaviour(newNode);
	} else {
		const TreeNodePtr& parent = root->getParent(root, nodeId);
		if (!parent) {
			Log::error("No parent for non-root node '%i'", nodeId);
			return false;
		}
		parent->replaceChild(nodeId, newNode);
	}

	Event event;
	event.type = EV_UPDATESTATICCHRDETAILS;
	event.data.zone = zone;
	enqueueEvent(event);
	return true;
}

bool Server::addNode(const ai::CharacterId& characterId, int32_t parentNodeId, const core::String& name, const core::String& type, const core::String& condition) {
	Zone* zone = _zone;
	if (zone == nullptr) {
		return false;
	}
	const AIPtr& ai = zone->getAI(characterId);
	TreeNodePtr node = ai->getBehaviour();
	if (node->getId() != parentNodeId) {
		node = node->getChild(parentNodeId);
	}
	if (!node) {
		return false;
	}
	ConditionParser conditionParser(_aiRegistry, condition);
	const ConditionPtr& conditionPtr = conditionParser.getCondition();
	if (!conditionPtr) {
		Log::error("Failed to parse the condition '%s'", condition.c_str());
		return false;
	}
	TreeNodeParser treeNodeParser(_aiRegistry, type);
	TreeNodePtr newNode = treeNodeParser.getTreeNode(name);
	if (!newNode) {
		Log::error("Failed to parse the node '%s'", type.c_str());
		return false;
	}
	newNode->setCondition(conditionPtr);
	if (!node->addChild(newNode)) {
		return false;
	}

	Event event;
	event.type = EV_UPDATESTATICCHRDETAILS;
	event.data.zone = zone;
	enqueueEvent(event);
	return true;
}

bool Server::deleteNode(const ai::CharacterId& characterId, int32_t nodeId) {
	Zone* zone = _zone;
	if (zone == nullptr) {
		return false;
	}
	const AIPtr& ai = zone->getAI(characterId);
	// don't delete the root
	const TreeNodePtr& root = ai->getBehaviour();
	if (root->getId() == nodeId) {
		return false;
	}

	const TreeNodePtr& parent = root->getParent(root, nodeId);
	if (!parent) {
		Log::error("No parent for non-root node '%i'", nodeId);
		return false;
	}
	parent->replaceChild(nodeId, TreeNodePtr());
	Event event;
	event.type = EV_UPDATESTATICCHRDETAILS;
	event.data.zone = zone;
	enqueueEvent(event);
	return true;
}

void Server::addZone(Zone* zone) {
	Event event;
	event.type = EV_ZONEADD;
	event.data.zone = zone;
	enqueueEvent(event);
}

void Server::removeZone(Zone* zone) {
	Event event;
	event.type = EV_ZONEREMOVE;
	event.data.zone = zone;
	enqueueEvent(event);
}

void Server::setDebug(const core::String& zoneName) {
	Event event;
	event.type = EV_SETDEBUG;
	event.strData = zoneName;
	enqueueEvent(event);
}

void Server::reset() {
	Zone* zone = _zone;
	if (zone == nullptr) {
		return;
	}
	Event event;
	event.type = EV_RESET;
	event.data.zone = zone;
	enqueueEvent(event);
}

void Server::select(const ai::CharacterId& id) {
	Event event;
	event.type = EV_SELECTION;
	event.data.characterId = id;
	enqueueEvent(event);
}

void Server::pause(bool state) {
	Event event;
	event.type = EV_PAUSE;
	event.data.pauseState = state;
	enqueueEvent(event);
}

void Server::step(int64_t stepMillis) {
	Event event;
	event.type = EV_STEP;
	event.data.stepMillis = stepMillis;
	enqueueEvent(event);
}

void Server::update(int64_t deltaTime) {
	_eventBus->update();
	core_trace_scoped(AIServerUpdate);
	_time += deltaTime;
	Zone* zone = _zone;
	bool pauseState = _pause;
	_broadcastMask = 0u;

	handleEvents(zone, pauseState);

	if (zone != nullptr) {
		if (!pauseState) {
			if ((_broadcastMask & SV_BROADCAST_STATE) == 0) {
				broadcastState(zone);
			}
			if ((_broadcastMask & SV_BROADCAST_CHRDETAILS) == 0) {
				broadcastCharacterDetails(zone);
			}
		}
	} else if (pauseState) {
		pause(false);
		resetSelection();
	}
	_network->update();
}

}
