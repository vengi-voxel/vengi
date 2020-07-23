/**
 * @file
 */
#include "AIDebugger.h"
#include "AIDebuggerWidget.h"
#include "AINodeStaticResolver.h"
#include "MapView.h"
#include <QtCore>
#include <vector>
#include "Version.h"
#include "core/Trace.h"
#include "ai-shared/protocol/IProtocolHandler.h"
#include "ai-shared/protocol/AIStateMessage.h"
#include "ai-shared/protocol/AINamesMessage.h"
#include "ai-shared/protocol/AIPauseMessage.h"
#include "ai-shared/protocol/AISelectMessage.h"
#include "ai-shared/protocol/AIStepMessage.h"
#include "ai-shared/protocol/AIChangeMessage.h"
#include "ai-shared/protocol/AIAddNodeMessage.h"
#include "ai-shared/protocol/AIUpdateNodeMessage.h"
#include "ai-shared/protocol/AIDeleteNodeMessage.h"
#include "ai-shared/protocol/AINamesMessage.h"
#include "ai-shared/protocol/AIStubTypes.h"
#include "ai-shared/protocol/AICharacterDetailsMessage.h"
#include "ai-shared/protocol/AICharacterStaticMessage.h"
#include "ai-shared/protocol/ProtocolMessageFactory.h"
#include "ai-shared/protocol/ProtocolHandlerRegistry.h"

namespace ai {
namespace debug {

class StateHandler: public ai::ProtocolHandler<ai::AIStateMessage> {
private:
	AIDebugger& _aiDebugger;
public:
	StateHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void execute(const ai::ClientId& /*clientId*/, const ai::AIStateMessage* msg) override {
		_aiDebugger.setEntities(msg->getStates());
		emit _aiDebugger.onEntitiesUpdated();
	}
};

class CharacterHandler: public ai::ProtocolHandler<ai::AICharacterDetailsMessage> {
private:
	AIDebugger& _aiDebugger;
public:
	CharacterHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void execute(const ai::ClientId& /*clientId*/, const ai::AICharacterDetailsMessage* msg) override {
		_aiDebugger.setCharacterDetails(msg->getCharacterId(), msg->getAggro(), msg->getNode());
		emit _aiDebugger.onSelected();
	}
};

class CharacterStaticHandler: public ai::ProtocolHandler<ai::AICharacterStaticMessage> {
private:
	AIDebugger& _aiDebugger;
public:
	CharacterStaticHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void execute(const ai::ClientId& /*clientId*/, const ai::AICharacterStaticMessage* msg) override {
		_aiDebugger.addCharacterStaticData(*msg);
		emit _aiDebugger.onSelected();
	}
};

class NamesHandler: public ai::ProtocolHandler<ai::AINamesMessage> {
private:
	AIDebugger& _aiDebugger;
public:
	NamesHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void execute(const ai::ClientId& /*clientId*/, const ai::AINamesMessage* msg) override {
		_aiDebugger.setNames(msg->getNames());
		emit _aiDebugger.onNamesReceived();
	}
};

class PauseHandler: public ai::ProtocolHandler<ai::AIPauseMessage> {
private:
	AIDebugger& _aiDebugger;
public:
	PauseHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void execute(const ai::ClientId& /*clientId*/, const ai::AIPauseMessage* msg) override {
		const bool pause = msg->isPause();
		_aiDebugger._pause = pause;
		emit _aiDebugger.onPause(pause);
	}
};

AIDebugger::AIDebugger(AINodeStaticResolver& resolver) :
		_stateHandler(new StateHandler(*this)), _characterHandler(new CharacterHandler(*this)), _characterStaticHandler(
				new CharacterStaticHandler(*this)), _pauseHandler(new PauseHandler(*this)), _namesHandler(new NamesHandler(*this)), _nopHandler(
				new ai::NopHandler()), _selectedId(AI_NOTHING_SELECTED), _socket(this), _pause(false), _resolver(resolver) {
	connect(&_socket, SIGNAL(readyRead()), SLOT(readTcpData()));
	connect(&_socket, SIGNAL(disconnected()), SLOT(onDisconnect()));

	ai::ProtocolHandlerRegistry& r = ai::ProtocolHandlerRegistry::get();
	r.registerHandler(ai::PROTO_STATE, _stateHandler);
	r.registerHandler(ai::PROTO_CHARACTER_DETAILS, _characterHandler);
	r.registerHandler(ai::PROTO_CHARACTER_STATIC, _characterStaticHandler);
	r.registerHandler(ai::PROTO_PAUSE, _pauseHandler);
	r.registerHandler(ai::PROTO_NAMES, _namesHandler);
	r.registerHandler(ai::PROTO_PING, _nopHandler);
}

AIDebugger::~AIDebugger() {
	disconnectFromAIServer();
	delete _stateHandler;
	delete _characterHandler;
	delete _characterStaticHandler;
	delete _pauseHandler;
	delete _namesHandler;
}

bool AIDebugger::isSelected(const ai::AIStateWorld& ai) const {
	return _selectedId == ai.getId();
}

void AIDebugger::setCharacterDetails(const ai::CharacterId& id, const ai::AIStateAggro& aggro, const ai::AIStateNode& node) {
	_selectedId = id;
	_aggro = aggro.getAggro();
	_node = node;
	_attributes.clear();
	const ai::AIStateWorld& state = _entities.value(id);
	const ai::CharacterAttributes& attributes = state.getAttributes();
	for (auto i = attributes.begin(); i != attributes.end(); ++i) {
		_attributes[QString(i->first.c_str())] = QString(i->second.c_str());
	}
}

void AIDebugger::addCharacterStaticData(const ai::AICharacterStaticMessage& msg) {
	const std::vector<ai::AIStateNodeStatic>& data = msg.getStaticNodeData();
	_resolver.set(data);
}

const ai::CharacterId& AIDebugger::getSelected() const {
	return _selectedId;
}

void AIDebugger::togglePause() {
	const bool newPauseMode = !_pause;
	writeMessage(ai::AIPauseMessage(newPauseMode));
}

void AIDebugger::select(ai::CharacterId id) {
	qDebug() << "select " << id;
	writeMessage(ai::AISelectMessage(id));
}

void AIDebugger::select(const ai::AIStateWorld& ai) {
	const ai::CharacterId id = ai.getId();
	select(id);
}

bool AIDebugger::writeMessage(const ai::IProtocolMessage& msg) {
	if (_socket.state() != QAbstractSocket::ConnectedState) {
		return false;
	}
	// serialize into streamcontainer to get the final size
	ai::streamContainer out;
	msg.serialize(out);
	// now put the serialized message into the byte array
	QByteArray temp;
	QDataStream data(&temp, QIODevice::ReadWrite);
	// add the framing size int
	const uint32_t size = out.size();
	ai::streamContainer sizeC;
	ai::IProtocolMessage::addInt(sizeC, size);
	for (ai::streamContainer::iterator i = sizeC.begin(); i != sizeC.end(); ++i) {
		const uint8_t byte = *i;
		data << byte;
	}
	// add the real message
	for (ai::streamContainer::iterator i = out.begin(); i != out.end(); ++i) {
		const uint8_t byte = *i;
		data << byte;
	}
	// now write everything to the socket
	_socket.write(temp);
	_socket.flush();
	return true;
}

void AIDebugger::unselect() {
	writeMessage(ai::AISelectMessage(AI_NOTHING_SELECTED));
	_selectedId = AI_NOTHING_SELECTED;
	_aggro.clear();
	_node = ai::AIStateNode();
	_attributes.clear();
	emit onSelected();
	qDebug() << "unselect entity";
}

void AIDebugger::step() {
	writeMessage(ai::AIStepMessage(1L));
}

void AIDebugger::reset() {
	writeMessage(ai::AIResetMessage());
}

void AIDebugger::change(const QString& name) {
	writeMessage(ai::AIChangeMessage(name.toStdString().c_str()));
}

void AIDebugger::updateNode(int32_t nodeId, const QVariant& name, const QVariant& type, const QVariant& condition) {
	writeMessage(ai::AIUpdateNodeMessage(nodeId, _selectedId, name.toString().toStdString().c_str(), type.toString().toStdString().c_str(), condition.toString().toStdString().c_str()));
}

void AIDebugger::deleteNode(int32_t nodeId) {
	writeMessage(ai::AIDeleteNodeMessage(nodeId, _selectedId));
}

void AIDebugger::addNode(int32_t parentNodeId, const QVariant& name, const QVariant& type, const QVariant& condition) {
	writeMessage(ai::AIAddNodeMessage(parentNodeId, _selectedId, name.toString().toStdString().c_str(),
		type.toString().toStdString().c_str(),
		condition.toString().toStdString().c_str()));
}

bool AIDebugger::connectToAIServer(const QString& hostname, short port) {
	disconnectFromAIServer();
	qDebug() << "connect to server: " << hostname << ":" << port;
	_socket.connectToHost(hostname, port, QAbstractSocket::ReadWrite, QAbstractSocket::AnyIPProtocol);
	if (_socket.waitForConnected()) {
		qDebug() << "Connection established " << _socket.state();
		return true;
	}
	const QAbstractSocket::SocketError socketError = _socket.error();
	switch (socketError) {
	case QAbstractSocket::RemoteHostClosedError:
		qDebug() << "The connection was closed by the host";
		break;
	case QAbstractSocket::HostNotFoundError:
		qDebug() << "The host was not found. Please check the host name and port settings";
		break;
	case QAbstractSocket::ConnectionRefusedError:
		qDebug() << "The connection was refused by the peer";
		break;
	default:
		qDebug() << "Socket error: " << socketError;
		break;
	}
	return false;
}

bool AIDebugger::disconnectFromAIServer() {
	qDebug() << "disconnect from server";
	_socket.disconnectFromHost();
	_socket.waitForDisconnected();
	_socket.close();
	return true;
}

void AIDebugger::onDisconnect() {
	qDebug() << "disconnect from server: " << _socket.state();
	{
		_pause = false;
		emit onPause(_pause);
	}
	{
		_selectedId = AI_NOTHING_SELECTED;
		_aggro.clear();
		_attributes.clear();
		_node = ai::AIStateNode();
		emit onSelected();
	}
	if (!_names.empty()) {
		_names.clear();
		emit onNamesReceived();
	}
	if (!_entities.empty()) {
		_entities.clear();
		emit onEntitiesUpdated();
	}
}

void AIDebugger::readTcpData() {
	while (_socket.bytesAvailable() > 0) {
		const QByteArray& data = _socket.readAll();
		// read everything that is currently available from the socket
		// and store it in our buffer
		const int n = data.count();
		for (int i = 0; i < n; ++i) {
			_stream.push_back(data[i]);
		}
		ai::ProtocolMessageFactory& mf = ai::ProtocolMessageFactory::get();
		for (;;) {
			if (!mf.isNewMessageAvailable(_stream)) {
				break;
			}
			// don't free this - preallocated memory that is reused
			ai::IProtocolMessage* msg = mf.create(_stream);
			if (msg == nullptr) {
				qDebug() << "unknown server message - disconnecting";
				disconnectFromAIServer();
				break;
			}
			ai::ProtocolHandlerRegistry& r = ai::ProtocolHandlerRegistry::get();
			ai::IProtocolHandler* handler = r.getHandler(*msg);
			if (handler != nullptr) {
				handler->execute(1, *msg);
			} else {
				qDebug() << "no handler for " << msg->getId();
				disconnectFromAIServer();
				break;
			}
		}
	}
}

MapView* AIDebugger::createMapWidget() {
	return new MapView(*this);
}

void AIDebugger::setNames(const std::vector<core::String>& names) {
	core_trace_scoped(SetNames);
	_names.clear();
	_names.reserve(names.size());
	for (const core::String& name : names) {
		_names << QString(name.c_str());
	}
}

void AIDebugger::setEntities(const std::vector<ai::AIStateWorld>& entities) {
	core_trace_scoped(SetEntities);
	_entities.clear();
	for (const ai::AIStateWorld& state : entities) {
		_entities.insert(state.getId(), state);
	}
	if (_selectedId == AI_NOTHING_SELECTED) {
		return;
	}
	if (_entities.contains(_selectedId)) {
		return;
	}
	// TODO: this doesn't work for some reason
//	unselect();
}

}
}
