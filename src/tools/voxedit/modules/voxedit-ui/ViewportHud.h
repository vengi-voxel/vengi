/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include <imgui.h>

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

namespace viewporthud {

/**
 * @brief Draw viewport status and workflow hints (edit and scene mode).
 */
void render(const SceneManagerPtr &sceneMgr, bool sceneMode, const ImVec2 &windowPos, const ImVec2 &contentSize,
			float headerSize);

} // namespace viewporthud
} // namespace voxedit
