/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphNode.h" // InvalidNodeId
#include "ui/IMGUIApp.h"
#include "voxedit-ui/ViewMode.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

int voxelCount(const SceneManagerPtr &sceneMgr, int node = InvalidNodeId);
void executeViewportClick();
bool executeViewportClickArea(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, const ImVec2 &offset);

bool newTemplateScene(ImGuiTestContext *ctx, const core::String &templateName);
bool changeViewMode(ImGuiTestContext *ctx, ViewMode viewMode);
bool centerOnViewport(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, int viewportId, ImVec2 offset = {0.0f, 0.0f});
int viewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);
int viewportSceneMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);
bool activateViewportSceneMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);
bool activateViewportEditMode(ImGuiTestContext *ctx, ui::IMGUIApp *app);
bool setVoxel(const SceneManagerPtr &sceneMgr, scenegraph::SceneGraphNode *node, const glm::ivec3 &pos, const voxel::Voxel &voxel);

} // namespace voxedit
