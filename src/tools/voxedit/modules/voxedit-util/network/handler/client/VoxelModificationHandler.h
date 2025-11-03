/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"

namespace voxedit {

class SceneManager;

class VoxelModificationHandler : public network::ProtocolTypeHandler<VoxelModificationMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	VoxelModificationHandler(SceneManager *sceneMgr);
	void execute(const network::ClientId &, VoxelModificationMessage *message) override;
};

} // namespace voxedit
