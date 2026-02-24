/**
 * @file
 */

#include "SceneDebugPanel.h"
#include "app/I18N.h"
#include "core/collection/RingBuffer.h"
#include "scenegraph/Physics.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "video/Renderer.h"
#include "voxedit-ui/MainWindow.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

struct ContactState : public scenegraph::ContactListener {
	void onContact(const glm::vec3 &point) override {
		contactPoints.push_back(point);
	}
	core::RingBuffer<glm::vec3, 8> contactPoints;
};

ContactState g_contactState;

SceneDebugPanel::SceneDebugPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
								 const SceneRendererPtr &sceneRenderer, MainWindow *mainWindow)
	: Super(app, "scenedebug"), _sceneMgr(sceneMgr), _sceneRenderer(sceneRenderer), _mainWindow(mainWindow) {
	scenegraph::KinematicBody &body = _sceneMgr->cameraMovement().body();
	body.contactListener = &g_contactState;
}

void SceneDebugPanel::update(const char *id) {
	core_trace_scoped(SceneDebugPanel);
	const core::String title = makeTitle(ICON_LC_BUG, _("Scene insights"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		const ISceneRenderer::RendererStats stats = _sceneRenderer->rendererStats();
		ImGui::Text(_("Pending extractions: %i"), stats.pendingExtractions);
		ImGui::Text(_("Pending meshes: %i"), stats.pendingMeshes);
		ImGui::Text(_("Culled volumes: %i"), stats.culledVolumes);
		ImGui::CheckboxVar(cfg::RenderCullNodes);
		ImGui::CheckboxVar(cfg::RenderCullBuffers);
		ImGui::Text(_("Draw calls: %i"), video::drawCalls());

		scenegraph::KinematicBody &body = _sceneMgr->cameraMovement().body();
		ImGui::Text(_("Camera position: %.2f %.2f %.2f"), body.position.x, body.position.y, body.position.z);
		ImGui::Text(_("Camera velocity: %.2f %.2f %.2f"), body.velocity.x, body.velocity.y, body.velocity.z);
		ImGui::Text(_("Collided on x axis: %s"), body.collidedX ? "true" : "false");
		ImGui::Text(_("Collided on y axis: %s"), body.collidedY ? "true" : "false");
		ImGui::Text(_("Collided on z axis: %s"), body.collidedZ ? "true" : "false");
		ImGui::InputVec3(_("Camera extents"), body.extents);
		ImGui::InputFloat(_("Camera friction decay"), &body.frictionDecay);
		const Viewport *viewport = _mainWindow->hoveredViewport();
		for (const glm::vec3 &p : g_contactState.contactPoints) {
			ImGui::Text(_("Recent contact point: %.2f %.2f %.2f"), p.x, p.y, p.z);
			if (viewport == nullptr) {
				continue;
			}
			const video::Camera &camera = viewport->camera();
			if (!camera.isVisible(p)) {
				continue;
			}
			const glm::vec2 &screenPos = viewport->pos() + camera.worldToScreen(p);
			ImGui::GetForegroundDrawList()->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), 5.0f,
															IM_COL32(255, 0, 0, 255));
		}
	}
	ImGui::End();
}

} // namespace voxedit
