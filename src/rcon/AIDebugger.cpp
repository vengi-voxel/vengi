#include "AIDebugger.h"
#include "AIDebuggerWidget.h"
#include "AINodeStaticResolver.h"
#include "MapView.h"
#include <server/IProtocolMessage.h>
#include <server/AISelectMessage.h>
#include <server/AIUpdateNodeMessage.h>
#include <server/AIAddNodeMessage.h>
#include <server/AIUpdateNodeMessage.h>
#include <server/AICharacterDetailsMessage.h>
#include <server/AICharacterStaticMessage.h>
#include <server/AIChangeMessage.h>
#include <server/AINamesMessage.h>
#include <server/AIStepMessage.h>
#include <server/ProtocolMessageFactory.h>
#include <server/ProtocolHandlerRegistry.h>
#include <QtCore>
#include <vector>
#include "Version.h"

namespace ai {
namespace debug {

PROTOCOL_HANDLER(AIStateMessage);
PROTOCOL_HANDLER(AICharacterDetailsMessage);
PROTOCOL_HANDLER(AICharacterStaticMessage);
PROTOCOL_HANDLER(AIPauseMessage);
PROTOCOL_HANDLER(AINamesMessage);

class StateHandler: public AIStateMessageHandler {
private:
	AIDebugger& _aiDebugger;
public:
	StateHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void executeAIStateMessage(const ai::AIStateMessage& msg) override {
		_aiDebugger.setEntities(msg.getStates());
		emit _aiDebugger.onEntitiesUpdated();
	}
};

class CharacterHandler: public AICharacterDetailsMessageHandler {
private:
	AIDebugger& _aiDebugger;
public:
	CharacterHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void executeAICharacterDetailsMessage(const ai::AICharacterDetailsMessage& msg) override {
		_aiDebugger.setCharacterDetails(msg.getCharacterId(), msg.getAggro(), msg.getNode());
		emit _aiDebugger.onSelected();
	}
};

class CharacterStaticHandler: public AICharacterStaticMessageHandler {
private:
	AIDebugger& _aiDebugger;
public:
	CharacterStaticHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void executeAICharacterStaticMessage(const ai::AICharacterStaticMessage& msg) override {
		_aiDebugger.addCharacterStaticData(msg);
		emit _aiDebugger.onSelected();
	}
};

class NamesHandler: public AINamesMessageHandler {
private:
	AIDebugger& _aiDebugger;
public:
	NamesHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void executeAINamesMessage(const ai::AINamesMessage& msg) override {
		_aiDebugger.setNames(msg.getNames());
		emit _aiDebugger.onNamesReceived();
	}
};

class PauseHandler: public AIPauseMessageHandler {
private:
	AIDebugger& _aiDebugger;
public:
	PauseHandler (AIDebugger& aiDebugger) :
			_aiDebugger(aiDebugger) {
	}

	void executeAIPauseMessage(const ai::AIPauseMessage& msg) override {
		_aiDebugger._pause = msg.isPause();
		emit _aiDebugger.onPause(msg.isPause());
	}
};

AIDebugger::AIDebugger(AINodeStaticResolver& resolver) :
		QObject(), _stateHandler(new StateHandler(*this)), _characterHandler(new CharacterHandler(*this)), _characterStaticHandler(
				new CharacterStaticHandler(*this)), _pauseHandler(new PauseHandler(*this)), _namesHandler(new NamesHandler(*this)), _nopHandler(
				new NopHandler()), _selectedId(-1), _socket(this), _pause(false), _resolver(resolver) {
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
	_socket.close();
	delete _stateHandler;
	delete _characterHandler;
	delete _characterStaticHandler;
	delete _pauseHandler;
	delete _namesHandler;
}

bool AIDebugger::isSelected(const ai::AIStateWorld& ai) const {
	return _selectedId == ai.getId();
}

void AIDebugger::setCharacterDetails(const CharacterId& id, const AIStateAggro& aggro, const AIStateNode& node) {
	_selectedId = id;
	_aggro = std::move(aggro.getAggro());
	_node = std::move(node);
	_attributes.clear();
	const AIStateWorld& state = _entities.value(id);
	const CharacterAttributes& attributes = state.getAttributes();
	for (CharacterAttributes::const_iterator i = attributes.begin(); i != attributes.end(); ++i) {
		_attributes[QString::fromStdString(i->first)] = QString::fromStdString(i->second);
	}
}

void AIDebugger::addCharacterStaticData(const AICharacterStaticMessage& msg) {
	const std::vector<AIStateNodeStatic>& data = msg.getStaticNodeData();
	_resolver.set(data);
}

const CharacterId& AIDebugger::getSelected() const {
	return _selectedId;
}

void AIDebugger::togglePause() {
	const bool newPauseMode = !_pause;
	writeMessage(AIPauseMessage(newPauseMode));
}

void AIDebugger::select(const ai::AIStateWorld& ai) {
	writeMessage(AISelectMessage(ai.getId()));
}

bool AIDebugger::writeMessage(const IProtocolMessage& msg) {
	if (_socket.state() != QAbstractSocket::ConnectedState) {
		return false;
	}
	// serialize into streamcontainer to get the final size
	streamContainer out;
	msg.serialize(out);
	// now put the serialized message into the byte array
	QByteArray temp;
	QDataStream data(&temp, QIODevice::ReadWrite);
	// add the framing size int
	const uint32_t size = out.size();
	streamContainer sizeC;
	IProtocolMessage::addInt(sizeC, size);
	for (streamContainer::iterator i = sizeC.begin(); i != sizeC.end(); ++i) {
		const uint8_t byte = *i;
		data << byte;
	}
	// add the real message
	for (streamContainer::iterator i = out.begin(); i != out.end(); ++i) {
		const uint8_t byte = *i;
		data << byte;
	}
	// now write everything to the socket
	_socket.write(temp);
	return true;
}

void AIDebugger::unselect() {
	writeMessage(AISelectMessage(-1));
	_selectedId = -1;
	_aggro.clear();
	_node = AIStateNode();
	qDebug() << "unselect entity";
}

void AIDebugger::step() {
	writeMessage(AIStepMessage(1L));
}

void AIDebugger::reset() {
	writeMessage(AIResetMessage());
}

void AIDebugger::change(const QString& name) {
	writeMessage(AIChangeMessage(name.toStdString()));
}

void AIDebugger::updateNode(int32_t nodeId, const QVariant& name, const QVariant& type, const QVariant& condition) {
	writeMessage(AIUpdateNodeMessage(nodeId, _selectedId, name.toString().toStdString(), type.toString().toStdString(), condition.toString().toStdString()));
}

void AIDebugger::deleteNode(int32_t nodeId) {
	writeMessage(AIDeleteNodeMessage(nodeId, _selectedId));
}

void AIDebugger::addNode(int32_t parentNodeId, const QVariant& name, const QVariant& type, const QVariant& condition) {
	writeMessage(AIAddNodeMessage(parentNodeId, _selectedId, name.toString().toStdString(), type.toString().toStdString(), condition.toString().toStdString()));
}

bool AIDebugger::connectToAIServer(const QString& hostname, short port) {
	_socket.disconnectFromHost();
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

void AIDebugger::onDisconnect() {
	qDebug() << "disconnect from server: " << _socket.state();
	{
		_pause = false;
		emit onPause(_pause);
	}
	{
		_selectedId = -1;
		_aggro.clear();
		_attributes.clear();
		_node = AIStateNode();
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
	emit disconnect();
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
			if (!mf.isNewMessageAvailable(_stream))
				break;
			std::unique_ptr<ai::IProtocolMessage> msg(mf.create(_stream));
			if (!msg) {
				qDebug() << "unknown server message - disconnecting";
				_socket.disconnectFromHost();
				break;
			}
			ai::ProtocolHandlerRegistry& r = ai::ProtocolHandlerRegistry::get();
			ai::IProtocolHandler* handler = r.getHandler(*msg);
			if (handler) {
				handler->execute(1, *msg);
			} else {
				qDebug() << "no handler for " << msg->getId();
				_socket.disconnectFromHost();
				break;
			}
		}
	}
}

MapView* AIDebugger::createMapWidget() {
	return new MapView(*this);
}

void AIDebugger::setNames(const std::vector<std::string>& names) {
	_names.clear();
	for (std::vector<std::string>::const_iterator i = names.begin(); i != names.end(); ++i) {
		_names << QString::fromStdString(*i);
	}
}

void AIDebugger::setEntities(const std::vector<AIStateWorld>& entities) {
	_entities.clear();
	for (std::vector<AIStateWorld>::const_iterator i = entities.begin(); i != entities.end(); ++i) {
		_entities.insert(i->getId(), *i);
	}
}

}
}
