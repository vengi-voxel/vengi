/**
 * @file
 */
#pragma once

#include "core/DeltaFrameSeconds.h"
#include "core/String.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/network/handler/client/NodeAddedHandler.h"
#include "voxedit-util/network/handler/client/NodeIKConstraintHandler.h"
#include "voxedit-util/network/handler/client/NodeKeyFramesHandler.h"
#include "voxedit-util/network/handler/client/NodeMovedHandler.h"
#include "voxedit-util/network/handler/client/NodePaletteChangedHandler.h"
#include "voxedit-util/network/handler/client/NodeNormalPaletteChangedHandler.h"
#include "voxedit-util/network/handler/client/NodePropertiesHandler.h"
#include "voxedit-util/network/handler/client/NodeRemovedHandler.h"
#include "voxedit-util/network/handler/client/NodeRenamedHandler.h"
#include "voxedit-util/network/handler/client/SceneStateHandlerClient.h"
#include "voxedit-util/network/handler/client/SceneStateRequestHandler.h"
#include "voxedit-util/network/handler/client/VoxelModificationHandler.h"
#include "voxedit-util/network/handler/client/LogMessageHandler.h"
#include "voxedit-util/network/handler/client/ChatMessageHandler.h"
#include "voxedit-util/network/handler/client/ClientListHandler.h"
#include "voxedit-util/network/handler/client/SceneGraphAnimationHandler.h"
#include <stdint.h>

namespace network {
struct NetworkImpl;
}

namespace voxedit {

class ClientNetwork : public core::DeltaFrameSeconds {
protected:
	network::NetworkImpl *_impl;
	network::ProtocolHandlerRegistry _protocolRegistry;
	network::NopHandler _nopHandler;
	VoxelModificationHandler _voxelModificationHandler;
	NodeAddedHandler _nodeAddedHandler;
	NodeKeyFramesHandler _nodeKeyFramesHandle;
	NodeMovedHandler _nodeMovedHandler;
	NodePaletteChangedHandler _nodePaletteChangedHandle;
	NodeNormalPaletteChangedHandler _nodeNormalPaletteChangedHandle;
	NodePropertiesHandler _nodePropertiesHandler;
	NodeIKConstraintHandler _nodeIKConstraintHandler;
	NodeRemovedHandler _nodeRemovedHandler;
	NodeRenamedHandler _nodeRenamedHandler;
	SceneStateRequestHandler _sceneStateRequestHandler;
	SceneStateHandlerClient _sceneStateHandler;
	SceneGraphAnimationHandler _sceneGraphAnimationHandler;
	LogMessageHandler _logMessageHandler;
	ChatMessageHandler _chatMessageHandler;
	ClientListHandler _clientListHandler;
	network::MessageStream in;
	/** Queued outbound bytes; flushed non-blocking in update()/sendMessage() */
	network::MessageStream _out;

	/**
	 * @brief Flush as many queued outbound bytes as the socket accepts
	 * @return @c false on hard socket errors (caller should disconnect)
	 * @note @c EAGAIN leaves remaining bytes in @_out and returns @c true
	 */
	bool flushOutgoing();

public:
	ClientNetwork(SceneManager *sceneMgr);
	virtual ~ClientNetwork();

	network::ProtocolHandlerRegistry &protocolRegistry() {
		return _protocolRegistry;
	}

	network::ProtocolHandler *protocolHandler(const network::ProtocolMessage &msg) {
		return _protocolRegistry.getHandler(msg);
	}

	bool isConnected() const;
	bool connect(const core::String &hostname, uint16_t port = 10001);
	void disconnect();
	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(double nowSeconds);

	/**
	 * @brief Queue a message and best-effort flush without blocking
	 * @return @c false if not connected or a hard send error occurred
	 */
	bool sendMessage(const network::ProtocolMessage &msg);

	/** Remaining queued outbound bytes (for tests) */
	int64_t pendingOutgoingBytes() const {
		return _out.size();
	}
};

} // namespace voxedit
