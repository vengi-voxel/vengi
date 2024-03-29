/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/Panel.h"
#include "core/collection/Buffer.h"
#include "scenegraph/SceneGraphAnimation.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class AnimationTimeline : public ui::Panel {
private:
	bool _play = false;
	// modifications on the keyframes would result in incorrect selection in neo-sequencer - so let's ensure to reset
	// the selection after a modification
	bool _clearSelection = false;
	double _seconds = 0.0;
	int32_t _startFrame = 0;
	int32_t _endFrame = -1;
	int _lastActivedNodeId = InvalidNodeId;
	struct Selection {
		scenegraph::FrameIndex frameIdx;
		int nodeId;
	};
	core::Buffer<Selection> _selectionBuffer;
	SceneManagerPtr _sceneMgr;

public:
private:
	using Super = ui ::Panel;

public:
	AnimationTimeline(ui ::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app), _sceneMgr(sceneMgr) {
	}
	void header(scenegraph::FrameIndex currentFrame, scenegraph::FrameIndex maxFrame);
	void timelineEntry(scenegraph::FrameIndex currentFrame, core::Buffer<Selection> &selectionBuffer,
				   core::Buffer<scenegraph::FrameIndex> &selectedFrames, const scenegraph::SceneGraphNode &modelNode);
	void sequencer(scenegraph::FrameIndex &currentFrame);
	bool init();
	bool update(const char *sequencerTitle, double deltaFrameSeconds);
	void resetFrames();
};

inline void AnimationTimeline::resetFrames() {
	_startFrame = 0;
	_endFrame = -1;
}

} // namespace voxedit
