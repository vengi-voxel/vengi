/**
 * @file
 */

#pragma once

#include "core/EventBus.h"
#include "voxel/RawVolume.h"
#include <mosquitto.h>
#include "core/String.h"
#include "TrazeTypes.h"
#include <unordered_map>

namespace traze {

class Protocol {
private:
	core::EventBusPtr _eventBus;

	core::String _instanceName;
	core::String _playerToken;
	core::String _clientToken;
	PlayerId _playerId = 0u;
	struct mosquitto *_mosquitto = nullptr;
	bool _connected = false;
	bool _subscribed = false;
	std::unordered_map<uint32_t, Player> _players;;

	enum ConnectState : uint8_t {
		Success = 0,
		UnacceptableProtocolVersion = 1,
		IdentifierRejected = 2,
		BrokerUnavailable = 3,
		MaxKnown = BrokerUnavailable,
		Max = 255,
		Unknown = 255
	};
	void onMessage(const struct mosquitto_message *msg);
	bool send(const core::String& topic, const core::String& json) const;

public:
	Protocol(const core::EventBusPtr& eventBus);

	bool init();
	void shutdown();

	bool connect();

	PlayerId playerId() const;

	bool joined() const;
	bool connected() const;

	bool unsubscribe();
	bool subscribe(const GameInfo& games);

	void onConnect(ConnectState status);

	/**
	 * @brief A high score table is published every 10 seconds at the scores topic.
	 * @code
	 * {
	 *   "ingameNick1[id1]": 238,
	 *   "ingameNick2[id2]": 235
	 * }
	 * @endcode
	 */
	void parseScores(const core::String& json);

	/**
	 * @brief The player topic is published every 5 seconds.
	 * @code
	 * [
	 *   {
	 *     "id": 1,
	 *     "name": "player1",
	 *     "color": "#28BA3C",
	 *     "frags": 1,
	 *     "owned": 2
	 *   },
	 *   {
	 *     "id": 2,
	 *     "name": "player2",
	 *     "color": "#0A94FF",
	 *     "frags": 2,
	 *     "owned": 1
	 *   }
	 * ]
	 * @endcode
	 */
	void parsePlayers(const core::String& json);

	/**
	 * @code
	 * {
	 *  "id": 1337,
	 *  "name": "myIngameNick",
	 *  "secretUserToken":"de37c1bc-d0e6-4c66-aaa3-911511f43d54",
	 *  "position": [15,3]
	 *}
	 * @endcode
	 */
	void parseOwnPlayer(const core::String& json);

	/**
	 * @brief The ticker topic is published whenever a death of a player occurs.
	 * @code
	 * {
	 *  "type": "frag", (or suicide)
	 *  "casualty": 2,
	 *  "fragger": 4
	 * }
	 * @endcode
	 */
	void parseTicker(const core::String& json) const;

	/**
	 * @code
	 * [
	 *   {
	 *     "name": "instanceName",
	 *     "activePlayers": 5
	 *   }
	 * ]
	 * @endcode
	 */
	void parseGames(const core::String& json) const;

	/**
	 * @brief The grid topic is published on every server tick. (4 times a Second)
	 * @code
	 * {
	 *  "height":3,
	 *  "width":3,
	 *  "tiles":[
	 *        [ 1, 0, 0 ],
	 *        [ 1, 1, 2 ],
	 *        [ 0, 2, 2 ]
	 *    ],
	 *  "bikes":[
	 *      {
	 *        "playerId":2,
	 *        "currentLocation":[1,0],
	 *        "direction":"W",
	 *        "trail":[[2,0],[2,1]]
	 *        }
	 *   ],
	 *   "spawns":[[2,2]]
	 *}
	 * @endcode
	 */
	void parseGridAndUpdateVolume(const core::String& json);

	/**
	 * @brief Client Registration
	 * You send a request to join the game. In return you’ll get a user token that allows you to control
	 * your bike. The Response will be sent to your private MQTT topic.
	 *
	 * traze/{instanceName}/join
	 * @code
	 * {
	 *   "name": "myIngameNick",
	 *   "mqttClientName": "myClientName"
	 * }
	 * @endcode
	 * If the server accepts your request you’ll receive a message communicating your initial position. Once you give your first direction command your game starts.
	 * You have to provide a unique MQTT client name in order to receive your session token on your clients own topic. It is important that you specify this very
	 * client name in the MQTT connect call to the broker, otherwise you will not be able to receive messages on the traze/{instanceName}/player/{myClientName} topic
	 * due to the brokers access control list settings. In order to not be subject to a MQTT deauthentication attack you should choose a client name that can not be
	 * guessed. UUIDs are a good solution.
	 *
	 * traze/{instanceName}/player/{myClientName}
	 * @code
	 * {
	 *   "id": 1337,
	 *   "name": "myIngameNick",
	 *   "secretUserToken":"de37c1bc-d0e6-4c66-aaa3-911511f43d54",
	 *   "position": [15,3]
	 * }
	 * @endcode
	 * Because the ingame nick is part of the topic your nickname may not include #, +, /.
	 */
	bool join(const core::String& name);

	/**
	 * @brief Steering your Light Cycle
	 * You steer by giving the directions for your next turn via an MQTT message. If you don’t commit a course correction within the specified timeframe your light cycle will continue on it’s previous path.
	 *
	 * traze/{instanceName}/{playerId}/steer
	 * @code
	 * {
	 *   "course":"N",
	 *   "playerToken": "de37c1bc-d0e6-4c66-aaa3-911511f43d54"
	 * }
	 * @endcode
	 * The options for a course Change are North, South, East or West.
	 */
	bool steer(BikeDirection direction) const;

	/**
	 * @brief Leaving a Game
	 *
	 * You may leave the game at any time.
	 *
	 * traze/{instanceName}/{playerId}/bail
	 * @code
	 * "playerToken": "yourSecretToken"
	 * @endcode
	 */
	bool bail();
};

inline PlayerId Protocol::playerId() const {
	return _playerId;
}

inline bool Protocol::joined() const {
	return _playerId != 0u;
}

inline bool Protocol::connected() const {
	return _connected;
}

}
