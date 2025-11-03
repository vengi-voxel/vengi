/**
 * @file
 */
#pragma once

#include "ProtocolMessage.h"
#include "core/NonCopyable.h"

namespace voxedit {

class ProtocolMessageFactory : public core::NonCopyable {
public:
	/**
	 * @brief Checks whether a new message is available in the stream
	 */
	static bool isNewMessageAvailable(MessageStream &in);

	/**
	 * @brief Call this only if @c isNewMessageAvailable returned @c true on the same @c MessageStream before!
	 * @note Don't free this pointer, it reuses memory for each new protocol message
	 */
	static ProtocolMessage *create(MessageStream &in);
};


} // namespace voxedit
