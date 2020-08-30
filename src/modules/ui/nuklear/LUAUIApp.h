/**
 * @file
 */

#pragma once

#include "NuklearApp.h"
#include "commonlua/LUA.h"
#include "video/TexturePool.h"
#include "core/collection/Stack.h"
#include "core/String.h"

namespace ui {
namespace nuklear {

/**
 * @brief The lua UI application is using a lua script with in @code ui/$appname$.lua @endcode to assemble the
 * UI. This script is automatically reloaded if it is changed in the filesystem.
 * @ingroup UI
 */
class LUAUIApp : public NuklearApp {
private:
	using Super = NuklearApp;
protected:
	lua::LUA _lua;
	int _skipUntilReload = -1;
	struct WindowStackElement {
		inline WindowStackElement(const core::String& _id = "", const core::String& _parameter = "") :
			id(_id), parameter(_parameter) {
		}
		core::String id;			/**< lua function name */
		core::String parameter;		/**< optional lua function parameter */
	};
	using WindowStack = core::Stack<WindowStackElement, 64>;
	WindowStack _windowStack;
	core::String _rootWindow;
	core::String _uiScriptPath;

	virtual void configureLUA(lua::LUA&) {}
public:
	LUAUIApp(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer);
	virtual ~LUAUIApp();

	void rootWindow(const core::String& id);
	void pushWindow(const core::String& id, const core::String& parameter = "");
	void popWindow(int amount = 1);
	void popup(const core::String& message);
	void setGlobalAlpha(float alpha);

	app::AppState onInit() override;
	app::AppState onCleanup() override;

	bool reload();
	bool onRenderUI() override;
};

}
}
