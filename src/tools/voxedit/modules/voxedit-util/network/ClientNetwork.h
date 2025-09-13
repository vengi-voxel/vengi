/**
 * @file
 */
#pragma once

#include "ProtocolMessage.h"
#include "core/DeltaFrameSeconds.h"
#include "core/String.h"
#include "voxedit-util/network/ProtocolHandlerRegistry.h"
#include "voxedit-util/network/handler/client/NodeAddedHandler.h"
#include "voxedit-util/network/handler/client/NodeKeyFramesHandler.h"
#include "voxedit-util/network/handler/client/NodeMovedHandler.h"
#include "voxedit-util/network/handler/client/NodePaletteChangedHandler.h"
#include "voxedit-util/network/handler/client/NodePropertiesHandler.h"
#include "voxedit-util/network/handler/client/NodeRemovedHandler.h"
#include "voxedit-util/network/handler/client/NodeRenamedHandler.h"
#include "voxedit-util/network/handler/client/SceneStateHandlerClient.h"
#include "voxedit-util/network/handler/client/SceneStateRequestHandler.h"
#include "voxedit-util/network/handler/client/VoxelModificationHandler.h"
#include <stdint.h>

namespace voxedit {
namespace network {

struct NetworkImpl;

class ClientNetwork : public core::DeltaFrameSeconds {
protected:
	NetworkImpl *_impl;
	ProtocolHandlerRegistry _protocolRegistry;
	network::NopHandler _nopHandler;
	network::VoxelModificationHandler _voxelModificationHandler;
	network::NodeAddedHandler _nodeAddedHandler;
	network::NodeKeyFramesHandler _nodeKeyFramesHandle;
	network::NodeMovedHandler _nodeMovedHandler;
	network::NodePaletteChangedHandler _nodePaletteChangedHandle;
	network::NodePropertiesHandler _nodePropertiesHandler;
	network::NodeRemovedHandler _nodeRemovedHandler;
	network::NodeRenamedHandler _nodeRenamedHandler;
	network::SceneStateRequestHandler _sceneStateRequestHandler;
	network::SceneStateHandlerClient _sceneStateHandler;
	MessageStream in;

public:
	ClientNetwork(SceneManager *sceneMgr);
	virtual ~ClientNetwork();

	ProtocolHandlerRegistry &protocolRegistry() {
		return _protocolRegistry;
	}

	bool isConnected() const;
	bool connect(const core::String &hostname, uint16_t port = 10001);
	void disconnect();
	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(double nowSeconds);

	bool sendMessage(const ProtocolMessage &msg);
};

} // namespace network
} // namespace voxedit
