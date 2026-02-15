/**
 * @file
 */

#include "Panel.h"
#include "IMGUIApp.h"
#include "core/Log.h"
#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "FileDialog.h"
#include "core/Assert.h"
#endif

namespace ui {

Panel::Panel(IMGUIApp *app, const char *title) : _app(app), _title(title) {
	Log::debug("create panel %s", _title.c_str());
	_app->addPanel(this);
}

Panel::~Panel() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
	unregisterUITests(_app->imguiTestEngine());
#endif
	_app->removePanel(this);
}

core::String Panel::makeTitle(const char *icon, const char *title, const char *id) {
	core::String str;
	if (icon != nullptr) {
		str.append(icon);
		str.append(" ");
	}
	if (title != nullptr) {
		str.append(title);
	}
	if (id != nullptr) {
		str.append(id);
	}
	return str;
}

core::String Panel::makeTitle(const char *title, const char *id) {
	return makeTitle(nullptr, title, id);
}

#ifdef IMGUI_ENABLE_TEST_ENGINE
void Panel::registerUITests(ImGuiTestEngine *, const char *) {
	Log::warn("No tests registered for panel %s", _title.c_str());
}

bool Panel::changeSlider(ImGuiTestContext *ctx, const char *path, bool left) {
	ctx->MouseMove(path);
	ctx->MouseDown();
	ctx->MouseMove(path, left ? ImGuiTestOpFlags_MoveToEdgeL : ImGuiTestOpFlags_MoveToEdgeR);
	ctx->MouseUp();
	return true;
}

bool Panel::saveFile(ImGuiTestContext *ctx, const char *filename) {
	ImGuiContext& g = *ctx->UiContext;
	IM_CHECK_RETV(focusWindow(ctx, "Save file"), false);
	ctx->ItemInputValue("Filename", filename);
	ctx->Yield();
	const int currentPopupSize = g.OpenPopupStack.Size;
	IM_CHECK_RETV(currentPopupSize > 0, false);
	ctx->ItemClick("Save");
	ctx->Yield();

	// Handle overwrite popup if the file already exists
	ImGuiWindow* overwriteWindow = ImGui::FindWindowByName(FILE_ALREADY_EXISTS_POPUP);
	if (overwriteWindow != nullptr && overwriteWindow->Active) {
		IM_CHECK_RETV(focusWindow(ctx, FILE_ALREADY_EXISTS_POPUP), false);
		ctx->ItemClick("###Yes");
		ctx->Yield();
	}

	// Handle format options popup if it appeared
	ImGuiWindow* optionsWindow = ImGui::FindWindowByName(OPTIONS_POPUP);
	if (optionsWindow != nullptr && optionsWindow->Active) {
		IM_CHECK_RETV(focusWindow(ctx, OPTIONS_POPUP), false);
		ctx->ItemClick("###Ok");
		ctx->Yield();
	}

	return true;
}

bool Panel::cancelSaveFile(ImGuiTestContext *ctx) {
	IM_CHECK_RETV(focusWindow(ctx, "Save file"), false);
	ctx->ItemClick("###Cancel");
	ctx->Yield();
	return true;
}

bool Panel::focusWindow(ImGuiTestContext *ctx, const char *title) {
	IM_CHECK_SILENT_RETV(title != nullptr, false);
	ImGuiWindow* window = ImGui::FindWindowByName(title);
	if (window == nullptr) {
		ctx->LogError("Error: could not find window with title/id %s", title);
		IM_CHECK_SILENT_RETV(window != nullptr, false);
	}
	ctx->WindowFocus(window->ID);
	ctx->SetRef(window);
	return true;
}

void Panel::unregisterUITests(ImGuiTestEngine *engine) {
	if (engine == nullptr) {
		return;
	}
	while (ImGuiTest* test = ImGuiTestEngine_FindTestByName(engine, testCategory(), nullptr)) {
		ImGuiTestEngine_UnregisterTest(engine, test);
	}
}

#endif

} // namespace ui
