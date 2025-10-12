/**
 * @file
 */

#include "SceneDebugPanel.h"
#include "IconsLucide.h"
#include "app/I18N.h"
#include "ui/IMGUIEx.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void SceneDebugPanel::update(const char *id) {
	core_trace_scoped(SceneDebugPanel);
	const core::String title = makeTitle(ICON_LC_BUG, _("Scene insights"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const ISceneRenderer::RendererStats stats = _sceneRenderer->rendererStats();
		ImGui::Text(_("Pending extractions: %i"), stats.pendingExtractions);
	}
	ImGui::End();
}

} // namespace voxedit
