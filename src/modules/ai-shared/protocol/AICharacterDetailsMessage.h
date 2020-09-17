/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * If someone selected a character this message gets broadcasted.
 */
class AICharacterDetailsMessage: public IProtocolMessage {
private:
	CharacterId _chrId;
	const AIStateAggro* _aggroPtr;
	const AIStateNode* _rootPtr;
	AIStateAggro _aggro;
	AIStateNode _root;

	AIStateNode readNode (streamContainer& in);

	void writeNode (streamContainer& out, const AIStateNode& node) const;

	void writeAggro(streamContainer& out, const AIStateAggro& aggro) const;

	void readAggro(streamContainer& in, AIStateAggro& aggro) const;

public:
	/**
	 * Make sure that none of the given references is destroyed, for performance reasons we are only storing the
	 * pointers to those instances in this class. So they need to stay valid until they are serialized.
	 */
	AICharacterDetailsMessage(const CharacterId& id, const AIStateAggro& aggro, const AIStateNode& root);

	explicit AICharacterDetailsMessage(streamContainer& in);

	void serialize(streamContainer& out) const override;

	inline const CharacterId& getCharacterId() const {
		return _chrId;
	}

	inline const AIStateAggro& getAggro() const {
		if (_aggroPtr)
			return *_aggroPtr;
		return _aggro;
	}

	inline const AIStateNode& getNode() const {
		if (_rootPtr)
			return *_rootPtr;
		return _root;
	}
};

}
