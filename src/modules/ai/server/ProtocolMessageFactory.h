/**
 * @file
 */
#pragma once

#include "common/NonCopyable.h"
#include "IProtocolMessage.h"

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

	ProtocolMessageFactory();
public:
	~ProtocolMessageFactory();

	static ProtocolMessageFactory& get() {
		static ProtocolMessageFactory _instance;
		return _instance;
	}

	/**
	 * @brief Checks whether a new message is available in the stream
	 */
	bool isNewMessageAvailable(const streamContainer& in) const;

	/**
	 * @brief Call this only if @c isNewMessageAvailable returned @c true on the same @c streamContainer before!
	 * @note Don't free this pointer, it reuses memory for each new protocol message
	 */
	IProtocolMessage *create(streamContainer& in);
};

}
