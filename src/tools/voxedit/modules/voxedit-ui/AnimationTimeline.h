/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

class AnimationTimeline {
private:
	bool _play = false;
	// modifications on the keyframes would result in incorrect selection in neo-sequencer - so let's ensure to reset
	// the selection after a modification
	bool _clearSelection = false;
	double _seconds = 0.0;
	int32_t _startFrame = 0;
	int32_t _endFrame = 64;
	struct Selection {
		scenegraph::FrameIndex frameIdx;
		int nodeId;
	};
	core::Buffer<Selection> _selectionBuffer;

public:
	void header(scenegraph::FrameIndex &currentFrame, scenegraph::FrameIndex maxFrame);
	void sequencer(scenegraph::FrameIndex &currentFrame);
	bool update(const char *sequencerTitle, double deltaFrameSeconds);
};

} // namespace voxedit
