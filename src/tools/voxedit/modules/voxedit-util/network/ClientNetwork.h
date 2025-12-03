/**
 * @file
 */
#pragma once

#include "core/DeltaFrameSeconds.h"
#include "core/String.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/network/handler/client/NodeAddedHandler.h"
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
	NodeRemovedHandler _nodeRemovedHandler;
	NodeRenamedHandler _nodeRenamedHandler;
	SceneStateRequestHandler _sceneStateRequestHandler;
	SceneStateHandlerClient _sceneStateHandler;
	SceneGraphAnimationHandler _sceneGraphAnimationHandler;
	network::MessageStream in;

public:
	ClientNetwork(SceneManager *sceneMgr);
	virtual ~ClientNetwork();

	network::ProtocolHandlerRegistry &protocolRegistry() {
		return _protocolRegistry;
	}

	bool isConnected() const;
	bool connect(const core::String &hostname, uint16_t port = 10001);
	void disconnect();
	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(double nowSeconds);

	bool sendMessage(const network::ProtocolMessage &msg);
};

} // namespace voxedit
