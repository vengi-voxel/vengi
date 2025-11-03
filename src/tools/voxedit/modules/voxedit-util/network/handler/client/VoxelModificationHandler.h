/**
 * @file
 */

#pragma once

#include "voxedit-util/network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"

namespace voxedit {

class SceneManager;

class VoxelModificationHandler : public ProtocolTypeHandler<VoxelModificationMessage> {
private:
	voxedit::SceneManager *_sceneMgr;

public:
	VoxelModificationHandler(SceneManager *sceneMgr);
	void execute(const ClientId &, VoxelModificationMessage *message) override;
};

} // namespace voxedit
