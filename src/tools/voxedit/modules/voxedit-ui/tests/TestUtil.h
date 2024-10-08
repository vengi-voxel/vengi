/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphNode.h" // InvalidNodeId
#include "ui/IMGUIApp.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

int voxelCount(const SceneManagerPtr &sceneMgr, int node = InvalidNodeId);
void executeViewportClick();
bool executeViewportClickArea(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, const ImVec2 &offset);

bool centerOnViewport(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, ImVec2 offset = {0.0f, 0.0f});
int viewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);
int viewportSceneMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);

} // namespace voxedit
