/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"
#include "core/collection/DynamicArray.h"

namespace ai {

class AICharacterStaticMessage : public IProtocolMessage {
private:
	CharacterId _chrId;
	const core::DynamicArray<AIStateNodeStatic>* _nodeStaticDataPtr;
	core::DynamicArray<AIStateNodeStatic> _nodeStaticData;
public:
	/**
	 * Make sure that none of the given references is destroyed, for performance reasons we are only storing the
	 * pointers to those instances in this class. So they need to stay valid until they are serialized.
	 */
	AICharacterStaticMessage(const CharacterId& id, const core::DynamicArray<AIStateNodeStatic>& nodeStaticData);

	explicit AICharacterStaticMessage(streamContainer& in);

	void serialize(streamContainer& out) const override;

	inline const core::DynamicArray<AIStateNodeStatic>& getStaticNodeData() const {
		if (_nodeStaticDataPtr)
			return *_nodeStaticDataPtr;
		return _nodeStaticData;
	}
};

}
