/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"

namespace voxedit {

class SceneManager;

namespace network {

class VoxelModificationHandler : public network::ProtocolTypeHandler<network::VoxelModificationMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	VoxelModificationHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, network::VoxelModificationMessage *message) override;
};

} // namespace network
} // namespace voxedit
