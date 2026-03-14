/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

class SceneStateMessage : public network::ProtocolMessage {
private:
	scenegraph::SceneGraph _sceneGraph;

public:
	SceneStateMessage(scenegraph::SceneGraph &sceneGraph);
	SceneStateMessage(network::MessageStream &in, uint32_t size);
	void writeBack() override;

	scenegraph::SceneGraph &sceneGraph() {
		return _sceneGraph;
	}
};

} // namespace voxedit
