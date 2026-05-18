/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include <imgui.h>

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

namespace brushhud {

/**
 * @brief Draw brush status and workflow hints over the viewport (edit mode).
 */
void render(const SceneManagerPtr &sceneMgr, const ImVec2 &windowPos, const ImVec2 &contentSize, float headerSize);

} // namespace brushhud
} // namespace voxedit
