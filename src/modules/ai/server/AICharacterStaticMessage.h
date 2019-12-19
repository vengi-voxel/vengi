/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"
#include <vector>

namespace ai {

class AICharacterStaticMessage : public IProtocolMessage {
private:
	CharacterId _chrId;
	const std::vector<AIStateNodeStatic>* _nodeStaticDataPtr;
	std::vector<AIStateNodeStatic> _nodeStaticData;
public:
	/**
	 * Make sure that none of the given references is destroyed, for performance reasons we are only storing the
	 * pointers to those instances in this class. So they need to stay valid until they are serialized.
	 */
	AICharacterStaticMessage(const CharacterId& id, const std::vector<AIStateNodeStatic>& nodeStaticData);

	explicit AICharacterStaticMessage(streamContainer& in);

	void serialize(streamContainer& out) const override;

	inline const std::vector<AIStateNodeStatic>& getStaticNodeData() const {
		if (_nodeStaticDataPtr)
			return *_nodeStaticDataPtr;
		return _nodeStaticData;
	}
};

}
