/**
 * @file
 */

#pragma once

#include "NuklearApp.h"
#include "commonlua/LUA.h"
#include "video/TexturePool.h"

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
	video::TexturePoolPtr _texturePool;
	bool _skipUntilReload = true;
public:
	LUAUIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::TexturePoolPtr& texturePool);
	virtual ~LUAUIApp();

	core::AppState onInit() override;
	core::AppState onCleanup() override;

	bool reload();
	bool onRenderUI() override;
};

}
}
