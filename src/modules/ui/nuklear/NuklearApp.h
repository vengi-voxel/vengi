/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "video/Texture.h"
#include "Console.h"
#include "RenderShaders.h"
#include "NuklearNode.h"
#include <memory>

namespace video {
class TexturePool;
using TexturePoolPtr = std::shared_ptr<TexturePool>;

class TextureAtlasRenderer;
using TextureAtlasRendererPtr = core::SharedPtr<TextureAtlasRenderer>;
}

namespace voxelrender {
class CachedMeshRenderer;
using CachedMeshRendererPtr = core::SharedPtr<CachedMeshRenderer>;
}

namespace ui {
namespace nuklear {

/**
 * @ingroup UI
 */
class NuklearApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
public:
	struct Vertex {
		float x, y;
		float u, v;
		union {
			struct { uint8_t r, g, b, a; };
			uint32_t col;
		};
	};
protected:
	struct nk_context _ctx;
	struct nkc_context _cctx;
	struct nk_font_atlas _atlas;
	struct nk_draw_null_texture _null;
	struct nk_buffer _cmds;
	struct nk_convert_config _config;

	// first is the default font size
	enum {
		FONT_22, FONT_16, FONT_30, FONT_40, FONT_MAX
	};
	static constexpr int _fontSizes[FONT_MAX] { 16, 22, 30, 40 };
	struct nk_font *_fonts[FONT_MAX];

	Console _console;

	shader::TextureShader _shader;
	video::Camera _camera;
	video::Buffer _vbo;
	video::TexturePtr _fontTexture;
	video::TexturePtr _emptyTexture;
	video::TexturePoolPtr _texturePool;
	voxelrender::CachedMeshRendererPtr _meshRenderer;
	video::TextureAtlasRendererPtr _textureAtlasRenderer;

	int32_t _vertexBufferIndex = -1;
	int32_t _elementBufferIndex = -1;

	int loadModelFile(const char *filename);
	struct nk_font *loadFontFile(const char *filename, int fontSize);
	struct nk_image loadImageFile(const char *filename);

	bool onKeyEvent(int32_t sym, int16_t modifier, bool down);

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;

	/**
	 * @brief Fonts are baked into a texture atlas. Your only chance to use
	 * the default atlas is to add your fonts in this method.
	 * @sa initUIConfig()
	 * @sa initUISkin()
	 */
	virtual void initUIFonts() {}
	/**
	 * @brief Hook to change the nuklear config before it is used.
	 * @sa initUIFonts()
	 * @sa initUISkin()
	 */
	virtual void initUIConfig(struct nk_convert_config& config);
	/**
	 * @brief Allows you to modify the ui skin. All fonts are loaded
	 * and baked at this point.
	 * @sa initUIFonts()
	 * @sa initUIConfig()
	 */
	virtual void initUISkin();

	virtual void beforeUI() {}
	virtual bool onRenderUI() = 0;

public:
	NuklearApp(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer);
	virtual ~NuklearApp();

	/**
	 * @brief Find the best matching font for the given size
	 */
	struct nk_font* font(int size);

	virtual void onWindowResize(int windowWidth, int windowHeight) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onRunning() override;
	virtual core::AppState onCleanup() override;
};

}
}
