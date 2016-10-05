/**
 * @file
 */
#pragma once

#include "common/NonCopyable.h"
#include "IProtocolMessage.h"
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

class ProtocolMessageFactory : public NonCopyable {
private:
	uint8_t *_aiState;
	uint8_t *_aiSelect;
	uint8_t *_aiPause;
	uint8_t *_aiNames;
	uint8_t *_aiChange;
	uint8_t *_aiReset;
	uint8_t *_aiStep;
	uint8_t *_aiPing;
	uint8_t *_aiCharacterDetails;
	uint8_t *_aiCharacterStatic;
	uint8_t *_aiUpdateNode;
	uint8_t *_aiAddNode;
	uint8_t *_aiDeleteNode;

	ProtocolMessageFactory() :
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
public:
	~ProtocolMessageFactory() {
		delete[] _aiState;
		delete[] _aiSelect;
		delete[] _aiPause;
		delete[] _aiNames;
		delete[] _aiChange;
		delete[] _aiReset;
		delete[] _aiStep;
		delete[] _aiPing;
		delete[] _aiCharacterDetails;
		delete[] _aiCharacterStatic;
		delete[] _aiUpdateNode;
		delete[] _aiAddNode;
		delete[] _aiDeleteNode;
	}

	static ProtocolMessageFactory& get() {
		static ProtocolMessageFactory _instance;
		return _instance;
	}

	/**
	 * @brief Checks whether a new message is available in the stream
	 */
	inline bool isNewMessageAvailable(const streamContainer& in) const {
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

	/**
	 * @brief Call this only if @c isNewMessageAvailable returned @c true on the same @c streamContainer before!
	 * @note Don't free this pointer, it reuses memory for each new protocol message
	 */
	IProtocolMessage *create(streamContainer& in) {
		// remove the size from the stream
		in.erase(in.begin(), std::next(in.begin(), sizeof(int32_t)));
		// get the message type
		const uint8_t type = in.front();
		in.pop_front();
		if (type == PROTO_STATE) {
			return new (_aiState) AIStateMessage(in);
		} else if (type == PROTO_SELECT) {
			return new (_aiSelect) AISelectMessage(in);
		} else if (type == PROTO_PAUSE) {
			return new (_aiPause) AIPauseMessage(in);
		} else if (type == PROTO_NAMES) {
			return new (_aiNames) AINamesMessage(in);
		} else if (type == PROTO_CHANGE) {
			return new (_aiChange) AIChangeMessage(in);
		} else if (type == PROTO_RESET) {
			return new (_aiReset) AIResetMessage();
		} else if (type == PROTO_STEP) {
			return new (_aiStep) AIStepMessage(in);
		} else if (type == PROTO_PING) {
			return new (_aiPing) AIPingMessage();
		} else if (type == PROTO_CHARACTER_DETAILS) {
			return new (_aiCharacterDetails) AICharacterDetailsMessage(in);
		} else if (type == PROTO_CHARACTER_STATIC) {
			return new (_aiCharacterStatic) AICharacterStaticMessage(in);
		} else if (type == PROTO_UPDATENODE) {
			return new (_aiUpdateNode) AIUpdateNodeMessage(in);
		} else if (type == PROTO_ADDNODE) {
			return new (_aiAddNode) AIAddNodeMessage(in);
		} else if (type == PROTO_DELETENODE) {
			return new (_aiDeleteNode) AIDeleteNodeMessage(in);
		}

		return nullptr;
	}
};

}
