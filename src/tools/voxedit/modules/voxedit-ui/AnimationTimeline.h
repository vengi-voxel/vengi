/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

class AnimationTimeline {
private:
	bool _play = false;
	double _seconds = 0.0;
	int32_t _startFrame = 0;
	int32_t _endFrame = 64;
	voxelformat::KeyFrameIndex _deleteKeyFrameIdx = InvalidKeyFrame;
	int _deleteFrameNode = -1;

public:
	void header(voxelformat::FrameIndex &currentFrame, voxelformat::FrameIndex maxFrame);
	void sequencer(voxelformat::FrameIndex &currentFrame);
	bool update(const char *sequencerTitle, double deltaFrameSeconds);
};

} // namespace voxedit
