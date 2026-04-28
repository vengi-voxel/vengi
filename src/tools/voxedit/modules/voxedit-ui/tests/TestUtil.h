/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphNode.h" // InvalidNodeId
#include "ui/IMGUIApp.h"
#include "voxedit-ui/ViewMode.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
class Viewport;

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

/**
 * @brief Sets up a scene with a filled volume and returns the edit-mode viewport ID.
 *
 * Creates a new scene, fills it with voxels, activates edit mode, and centers
 * the mouse on the viewport. After this call the viewport is ready for brush
 * interaction via executeViewportClick() or executeViewportClickArea().
 *
 * @return The viewport ID (>= 0) on success, -1 on failure.
 */
int prepareBrushViewport(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, ui::IMGUIApp *app,
						 const char *sceneName, const voxel::Region &region = voxel::Region(0, 7));

/**
 * @brief Creates a new scene and fills it with voxels.
 */
bool newFilledScene(ImGuiTestContext *ctx, const SceneManagerPtr &sceneMgr, const char *sceneName,
					const voxel::Region &region = voxel::Region(0, 31));

/**
 * @brief Convenience to get the Viewport pointer for a given viewport ID.
 */
Viewport *viewportById(ui::IMGUIApp *app, int viewportId);

/**
 * @brief Simulates a drag-and-drop from one screen position to another.
 *
 * Uses MouseLiftDragThreshold() to ensure ImGui recognizes the drag, which is
 * required for BeginDragDropSource/BeginDragDropTarget to activate.
 */
void dragFromTo(ImGuiTestContext *ctx, const ImVec2 &from, const ImVec2 &to);

/**
 * @brief Toggles a menu checkbox and verifies the underlying cvar changed.
 * Clicks the menu item, yields, checks the var flipped, clicks again, checks it restored.
 */
bool toggleMenuCheckbox(ImGuiTestContext *ctx, const char *menuPath, const char *cvarName);

} // namespace voxedit
