/**
 * @file
 */
#pragma once

#include <vector>
#include <utility>
#include "ai/SimpleAI.h"
#include "ai/server/IProtocolHandler.h"
#include "ai/server/AICharacterStaticMessage.h"
#include "ai/server/AICharacterDetailsMessage.h"
#include <QTcpSocket>
#include <QSettings>
#include <QFile>
#include <QStringList>
#include <QMap>
#include <QHash>

namespace ai {
namespace debug {

class MapView;
class AIDebuggerWidget;
class AINodeStaticResolver;
class PauseHandler;

/**
 * @brief This is the remote debugger for the ai entities.
 *
 * You can extend this class and override AIDebugger::createMapWidget to create our own @c MapView instance
 * to render additional details to your characters or even the map the entities are spawned on.
 */
class AIDebugger: public QObject {
	Q_OBJECT
friend class PauseHandler;
public:
	typedef QHash<CharacterId, AIStateWorld> Entities;
	typedef Entities::const_iterator EntitiesIter;
	typedef QMap<QString, QString> CharacterAttributesMap;
protected:
	typedef Entities::iterator Iter;
	// all the entities that are send by the ai debug server
	Entities _entities;

	// the network protocol message handlers
	ai::IProtocolHandler *_stateHandler;
	ai::IProtocolHandler *_characterHandler;
	ai::IProtocolHandler *_characterStaticHandler;
	ai::IProtocolHandler *_pauseHandler;
	ai::IProtocolHandler *_namesHandler;
	ai::IProtocolHandler *_nopHandler;

	// The buffer where we store our network data until we can read one complete protocol message.
	ai::streamContainer _stream;

	// the current selected entity id
	ai::CharacterId _selectedId;
	// the aggro list of the current selected entity
	std::vector<AIStateAggroEntry> _aggro;
	// the behaviour tree states of the current selected entity
	AIStateNode _node;
	// the attributes of the current selected entity
	CharacterAttributesMap _attributes;
	// the socket of the ai debug server
	QTcpSocket _socket;
	bool _pause;
	QStringList _names;
	AINodeStaticResolver& _resolver;

	bool writeMessage(const IProtocolMessage& msg);

private slots:
	void readTcpData();
	void onDisconnect();
public:
	AIDebugger(AINodeStaticResolver& resolver);
	virtual ~AIDebugger();

	/**
	 * @return The list of ai controlled entities
	 */
	const Entities& getEntities() const;
	void setEntities(const std::vector<AIStateWorld>& entities);
	void setCharacterDetails(const CharacterId& id, const AIStateAggro& aggro, const AIStateNode& node);
	void addCharacterStaticData(const AICharacterStaticMessage& msg);
	void setNames(const std::vector<std::string>& names);
	const QStringList& getNames() const;
	/**
	 * @return The behaviour tree node that is assigned to the selected entity
	 */
	const AIStateNode& getNode() const;
	/**
	 * @return The key/value pairs of attributes that are assigned on the server side to the selected ai entity
	 */
	const CharacterAttributesMap& getAttributes() const;
	const std::vector<AIStateAggroEntry>& getAggro() const;

	/**
	 * @brief Start the debugger - call this from your main method
	 */
	int run();
	bool connectToAIServer(const QString& hostname, short port);
	bool disconnectFromAIServer();

	bool isSelected(const ai::AIStateWorld& ai) const;
	const CharacterId& getSelected() const;
	void select(const ai::AIStateWorld& ai);
	void select(ai::CharacterId id);
	void togglePause();
	void unselect();
	void step();
	void reset();
	void change(const QString& name);
	void updateNode(int32_t nodeId, const QVariant& name, const QVariant& type, const QVariant& condition);
	void deleteNode(int32_t nodeId);
	void addNode(int32_t parentNodeId, const QVariant& name, const QVariant& type, const QVariant& condition);

	/**
	 * @brief override this if you would like to create your own @c MapView implementation that renders
	 * for example more details of your map.
	 */
	virtual MapView* createMapWidget();

signals:
	void onPause(bool pause);
	void disconnected();
	// signal that is triggered whenever the entity details for the current selected entity arrived
	void onSelected();
	// new names list was received
	void onNamesReceived();
	// entities on the map were updated
	void onEntitiesUpdated();
};

inline const std::vector<AIStateAggroEntry>& AIDebugger::getAggro() const {
	return _aggro;
}

inline const AIStateNode& AIDebugger::getNode() const {
	return _node;
}

inline const AIDebugger::CharacterAttributesMap& AIDebugger::getAttributes() const {
	return _attributes;
}

inline const AIDebugger::Entities& AIDebugger::getEntities() const {
	return _entities;
}

inline const QStringList& AIDebugger::getNames() const {
	return _names;
}

}
}
