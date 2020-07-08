/**
 * @file
 */

#include "ProtocolMessageFactory.h"
#include "AIStateMessage.h"
#include "AICharacterDetailsMessage.h"
#include "AICharacterStaticMessage.h"
#include "AIPauseMessage.h"
#include "AISelectMessage.h"
#include "AINamesMessage.h"
#include "AIChangeMessage.h"
#include "AIStepMessage.h"
#include "AIUpdateNodeMessage.h"
#include "AIAddNodeMessage.h"
#include "AIDeleteNodeMessage.h"

namespace ai {

ProtocolMessageFactory::ProtocolMessageFactory() :
	_aiState(new uint8_t[sizeof(AIStateMessage)]),
	_aiSelect(new uint8_t[sizeof(AISelectMessage)]),
	_aiPause(new uint8_t[sizeof(AIPauseMessage)]),
	_aiNames(new uint8_t[sizeof(AINamesMessage)]),
	_aiChange(new uint8_t[sizeof(AIChangeMessage)]),
	_aiReset(new uint8_t[sizeof(AIResetMessage)]),
	_aiStep(new uint8_t[sizeof(AIStepMessage)]),
	_aiPing(new uint8_t[sizeof(AIPingMessage)]),
	_aiCharacterDetails(new uint8_t[sizeof(AICharacterDetailsMessage)]),
	_aiCharacterStatic(new uint8_t[sizeof(AICharacterStaticMessage)]),
	_aiUpdateNode(new uint8_t[sizeof(AIUpdateNodeMessage)]),
	_aiAddNode(new uint8_t[sizeof(AIAddNodeMessage)]),
	_aiDeleteNode(new uint8_t[sizeof(AIDeleteNodeMessage)]) {
}

ProtocolMessageFactory::~ProtocolMessageFactory() {
	if (_usedAIState) {
		((AIStateMessage*)_aiState)->~AIStateMessage();
	}
	delete[] _aiState;
	if (_usedAISelect) {
		((AIStateMessage*)_aiSelect)->~AIStateMessage();
	}
	delete[] _aiSelect;
	if (_usedAIPause) {
		((AIStateMessage*)_aiPause)->~AIStateMessage();
	}
	delete[] _aiPause;
	if (_usedAINames) {
		((AIStateMessage*)_aiNames)->~AIStateMessage();
	}
	delete[] _aiNames;
	if (_usedAIChange) {
		((AIStateMessage*)_aiChange)->~AIStateMessage();
	}
	delete[] _aiChange;
	if (_usedAIReset) {
		((AIStateMessage*)_aiReset)->~AIStateMessage();
	}
	delete[] _aiReset;
	if (_usedAIStep) {
		((AIStateMessage*)_aiStep)->~AIStateMessage();
	}
	delete[] _aiStep;
	if (_usedAIPing) {
		((AIStateMessage*)_aiPing)->~AIStateMessage();
	}
	delete[] _aiPing;
	if (_usedAICharacterDetails) {
		((AIStateMessage*)_aiCharacterDetails)->~AIStateMessage();
	}
	delete[] _aiCharacterDetails;
	if (_usedAICharacterStatic) {
		((AIStateMessage*)_aiCharacterStatic)->~AIStateMessage();
	}
	delete[] _aiCharacterStatic;
	if (_usedAIUpdateNode) {
		((AIStateMessage*)_aiUpdateNode)->~AIStateMessage();
	}
	delete[] _aiUpdateNode;
	if (_usedAIAddNode) {
		((AIStateMessage*)_aiAddNode)->~AIStateMessage();
	}
	delete[] _aiAddNode;
	if (_usedAIDeleteNode) {
		((AIStateMessage*)_aiDeleteNode)->~AIStateMessage();
	}
	delete[] _aiDeleteNode;
}

bool ProtocolMessageFactory::isNewMessageAvailable(const streamContainer& in) const {
	const int32_t size = IProtocolMessage::peekInt(in);
	if (size == -1) {
		// not enough data yet, wait a little bit more
		return false;
	}
	const int streamSize = static_cast<int>(in.size() - sizeof(int32_t));
	if (size > streamSize) {
		// not enough data yet, wait a little bit more
		return false;
	}
	return true;
}

IProtocolMessage *ProtocolMessageFactory::create(streamContainer& in) {
	// remove the size from the stream
	in.erase(in.begin(), std::next(in.begin(), sizeof(int32_t)));
	// get the message type
	const uint8_t type = in.front();
	in.pop_front();
	if (type == PROTO_STATE) {
		_usedAIState = true;
		return new (_aiState) AIStateMessage(in);
	} else if (type == PROTO_SELECT) {
		_usedAISelect = true;
		return new (_aiSelect) AISelectMessage(in);
	} else if (type == PROTO_PAUSE) {
		_usedAIPause = true;
		return new (_aiPause) AIPauseMessage(in);
	} else if (type == PROTO_NAMES) {
		_usedAINames = true;
		return new (_aiNames) AINamesMessage(in);
	} else if (type == PROTO_CHANGE) {
		_usedAIChange = true;
		return new (_aiChange) AIChangeMessage(in);
	} else if (type == PROTO_RESET) {
		_usedAIReset = true;
		return new (_aiReset) AIResetMessage();
	} else if (type == PROTO_STEP) {
		_usedAIStep = true;
		return new (_aiStep) AIStepMessage(in);
	} else if (type == PROTO_PING) {
		_usedAIPing = true;
		return new (_aiPing) AIPingMessage();
	} else if (type == PROTO_CHARACTER_DETAILS) {
		_usedAICharacterDetails = true;
		return new (_aiCharacterDetails) AICharacterDetailsMessage(in);
	} else if (type == PROTO_CHARACTER_STATIC) {
		_usedAICharacterStatic = true;
		return new (_aiCharacterStatic) AICharacterStaticMessage(in);
	} else if (type == PROTO_UPDATENODE) {
		_usedAIUpdateNode = true;
		return new (_aiUpdateNode) AIUpdateNodeMessage(in);
	} else if (type == PROTO_ADDNODE) {
		_usedAIAddNode = true;
		return new (_aiAddNode) AIAddNodeMessage(in);
	} else if (type == PROTO_DELETENODE) {
		_usedAIDeleteNode = true;
		return new (_aiDeleteNode) AIDeleteNodeMessage(in);
	}

	return nullptr;
}

}
