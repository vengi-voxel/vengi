/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/collection/Buffer.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/Panel.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class AnimationTimeline : public ui::Panel {
private:
	using Super = ui::Panel;

	bool _play = false;
	// modifications on the keyframes would result in incorrect selection in neo-sequencer - so let's ensure to reset
	// the selection after a modification
	bool _clearSelection = false;
	bool _loop = true;
	double _frameTimeSeconds = 0.0;
	double _fps = 22.0;
	int32_t _startFrame = 0;
	int32_t _endFrame = -1;
	int _lastActivedNodeId = InvalidNodeId;
	struct Selection {
		scenegraph::FrameIndex frameIdx;
		int nodeId;
	};
	core::Buffer<Selection> _selectionBuffer;
	SceneManagerPtr _sceneMgr;

	void header(scenegraph::FrameIndex currentFrame, scenegraph::FrameIndex maxFrame);
	void timelineEntry(scenegraph::FrameIndex currentFrame, core::Buffer<Selection> &selectionBuffer,
					   core::Buffer<scenegraph::FrameIndex> &selectedFrames,
					   const scenegraph::SceneGraphNode &node);
	void sequencer(scenegraph::FrameIndex &currentFrame);

public:
	AnimationTimeline(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
		: Super(app, "animationtimeline"), _sceneMgr(sceneMgr) {
	}
	bool init();
	bool update(const char *id, double deltaFrameSeconds);
	void resetFrames();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline void AnimationTimeline::resetFrames() {
	_startFrame = 0;
	_endFrame = -1;
}

} // namespace voxedit
