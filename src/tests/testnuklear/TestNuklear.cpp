/**
 * @file
 */

#include "TestNuklear.h"
#include "testcore/TestAppMain.h"
#include "core/io/Filesystem.h"
#include "ui/nuklear/Nuklear.h"
#include "video/TexturePool.h"
#include "voxelformat/MeshCache.h"
#include "voxelrender/CachedMeshRenderer.h"
#include "video/TextureAtlasRenderer.h"
#include "overview.c"
#include "extended.c"
#include "node_editor.c"
#include "style.c"
#include "style_configurator.c"

static struct nk_color color_table[NK_COLOR_COUNT];

TestNuklear::TestNuklear(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer) :
		Super(metric, filesystem, eventBus, timeProvider, texturePool, meshRenderer, textureAtlasRenderer) {
	init(ORGANISATION, "testnuklear");
}

void TestNuklear::initUIFonts() {
	const char *fontFile = "extra_font/Roboto-Regular.ttf";
	_media.font_14 = loadFontFile(fontFile, 14.0f);
	_media.font_18 = loadFontFile(fontFile, 18.0f);
	_media.font_20 = loadFontFile(fontFile, 20.0f);
	_media.font_22 = loadFontFile(fontFile, 22.0f);
}

core::AppState TestNuklear::onInit() {
	const core::AppState state = Super::onInit();

	SDL_memcpy(color_table, nkc_get_default_color_style(nullptr), sizeof(color_table));

	_media.unchecked = loadImageFile("icon/unchecked.png");
	_media.checked = loadImageFile("icon/checked.png");
	_media.rocket = loadImageFile("icon/rocket.png");
	_media.cloud = loadImageFile("icon/cloud.png");
	_media.pen = loadImageFile("icon/pen.png");
	_media.play = loadImageFile("icon/play.png");
	_media.pause = loadImageFile("icon/pause.png");
	_media.stop = loadImageFile("icon/stop.png");
	_media.next = loadImageFile("icon/next.png");
	_media.prev = loadImageFile("icon/prev.png");
	_media.tools = loadImageFile("icon/tools.png");
	_media.dir = loadImageFile("icon/directory.png");
	_media.copy = loadImageFile("icon/copy.png");
	_media.convert = loadImageFile("icon/export.png");
	_media.del = loadImageFile("icon/delete.png");
	_media.edit = loadImageFile("icon/edit.png");
	_media.menu[0] = loadImageFile("icon/home.png");
	_media.menu[1] = loadImageFile("icon/phone.png");
	_media.menu[2] = loadImageFile("icon/plane.png");
	_media.menu[3] = loadImageFile("icon/wifi.png");
	_media.menu[4] = loadImageFile("icon/settings.png");
	_media.menu[5] = loadImageFile("icon/volume.png");

	for (int i = 0; i < 9; ++i) {
		char buffer[256];
		sprintf(buffer, "images/image%d.png", (i + 1));
		_media.images[i] = loadImageFile(buffer);
	}

	return state;
}

bool TestNuklear::onRenderUI() {
	overview(&_ctx);
	node_editor(&_ctx);
	static int mode = 0;
	static const char *options[] = {"THEME_BLACK", "THEME_WHITE", "THEME_RED", "THEME_BLUE", "THEME_DARK", "style configurator"};
	const struct nk_rect rect = nk_recti(980, 270, 275, 150);
	if (nk_begin_titled(&_ctx, "style_options", "Style options", rect, 0)) {
		nk_layout_row_dynamic(&_ctx, 25, 1);
		mode = nk_combo(&_ctx, options, SDL_arraysize(options), mode, 10, nk_vec2(100, 100));
	}
	nk_end(&_ctx);
	if (mode >= 0 && mode <= THEME_DARK) {
		set_style(&_ctx, (theme)mode);
	} else {
		style_configurator(&_ctx, color_table);
	}
	basic_demo(&_ctx, &_media);
	button_demo(&_ctx, &_media);
	grid_demo(&_ctx, &_media);

	return true;
}

int main(int argc, char *argv[]) {
	const voxelformat::MeshCachePtr& meshCache = std::make_shared<voxelformat::MeshCache>();
	const voxelrender::CachedMeshRendererPtr& meshRenderer = core::make_shared<voxelrender::CachedMeshRenderer>(meshCache);
	const video::TextureAtlasRendererPtr& textureAtlasRenderer = core::make_shared<video::TextureAtlasRenderer>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::TexturePoolPtr& texturePool = std::make_shared<video::TexturePool>(filesystem);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	TestNuklear app(metric, filesystem, eventBus, timeProvider, texturePool, meshRenderer, textureAtlasRenderer);
	return app.startMainLoop(argc, argv);
}
