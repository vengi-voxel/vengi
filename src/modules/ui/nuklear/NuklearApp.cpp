/**
 * @file
 */

#include "NuklearApp.h"
#include "video/Renderer.h"
#include "video/ScopedViewPort.h"
#include "voxelrender/CachedMeshRenderer.h"
#include "video/TexturePool.h"
#include "video/TextureAtlasRenderer.h"
#include "core/UTF8.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "core/Color.h"
#include "io/Filesystem.h"
#include "core/Assert.h"
#include "voxel/MaterialColor.h"

#include <SDL.h>

namespace ui {
namespace nuklear {

static const int MAX_VERTEX_MEMORY = 32768 * sizeof(NuklearApp::Vertex);
static const int MAX_ELEMENT_MEMORY = 65536;

NuklearApp::NuklearApp(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer) :
		Super(metric, filesystem, eventBus, timeProvider), _console(this),
	_camera(video::CameraType::FirstPerson, video::CameraMode::Orthogonal),
	_texturePool(texturePool), _meshRenderer(meshRenderer), _textureAtlasRenderer(textureAtlasRenderer) {
	_cctx.ctx = &_ctx;
	_cctx.meshRenderer = _meshRenderer;
	_cctx.textureAtlasRenderer = _textureAtlasRenderer;
}

NuklearApp::~NuklearApp() {
}

bool NuklearApp::onMouseWheel(int32_t x, int32_t y) {
	if (_console.onMouseWheel(x, y)) {
		return true;
	}
	if (Super::onMouseWheel(x, y)) {
		return true;
	}
	_scrollDelta.x += x;
	_scrollDelta.y += y;
	return true;
}

void NuklearApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (_console.onMouseButtonPress(x, y, button)) {
		return;
	}
	Super::onMouseButtonPress(x, y, button, clicks);
}

void NuklearApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	if (_console.isActive()) {
		return;
	}
	Super::onMouseButtonRelease(x, y, button);
}

bool NuklearApp::onTextInput(const core::String& text) {
	if (_console.onTextInput(text)) {
		return true;
	}
	_textInput.append(text);
	return true;
}

bool NuklearApp::onKeyPress(int32_t key, int16_t modifier) {
	if (_console.onKeyPress(key, modifier)) {
		return true;
	}
	return Super::onKeyPress(key, modifier);
}

bool NuklearApp::onKeyRelease(int32_t key, int16_t modifier) {
	if (_console.isActive()) {
		return true;
	}
	return Super::onKeyRelease(key, modifier);
}

static void nk_sdl_clipbard_paste(nk_handle usr, struct nk_text_edit *edit) {
	if (!SDL_HasClipboardText()) {
		return;
	}
	const char *text = SDL_GetClipboardText();
	nk_textedit_paste(edit, text, nk_strlen(text));
}

static void nk_sdl_clipbard_copy(nk_handle usr, const char *text, int len) {
	if (len <= 0) {
		return;
	}
	char* str = new char[len + 1];
	core_memcpy(str, text, (size_t) len);
	str[len] = '\0';
	SDL_SetClipboardText(str);
	delete[] str;
}

static void* nk_core_alloc(nk_handle, void *, nk_size size) {
	return core_malloc(size);
}

static void nk_core_free(nk_handle, void *old) {
	return core_free(old);
}

struct nk_font *NuklearApp::loadFontFile(const char *filename, float fontSize) {
	const io::FilePtr& file = filesystem()->open(filename, io::FileMode::Read);
	if (!file) {
		Log::warn("Can't load font. Could not open '%s'", filename);
		return nullptr;
	}
	uint8_t *fontData = nullptr;
	const size_t fontDataSize = file->read((void **) &fontData);
	if (fontDataSize == 0) {
		Log::warn("Can't load font. Could not read '%s'", filename);
		return nullptr;
	}

	struct nk_font *font = nk_font_atlas_add_from_memory(&_atlas, fontData, fontDataSize, fontSize, nullptr);
	delete[] fontData;
	return font;
}

struct nk_image NuklearApp::loadImageFile(const char *filename) {
	video::TexturePtr tex = _texturePool->load(filename);
	if (!tex) {
		Log::warn("Could not load image: '%s'", filename);
		tex = _emptyTexture;
	}
	const video::Id handle = tex->handle();
	const int width = tex->width();
	const int height = tex->height();
	struct nk_image image;
	image.handle = nk_handle_id(handle);
	image.w = width;
	image.h = height;
	image.region[0] = 0;
	image.region[1] = 0;
	image.region[2] = image.region[0] + width;
	image.region[3] = image.region[1] + height;
	return image;
}

app::AppState NuklearApp::onInit() {
	const app::AppState state = Super::onInit();
	SDL_StartTextInput();
	showCursor(false);
	centerMousePosition();
	video::checkError();
	if (state != app::AppState::Running) {
		return state;
	}

	_emptyTexture = video::createEmptyTexture("**empty**");
	_fontTexture = video::createEmptyTexture("**font**");

	struct nk_allocator alloc;
	alloc.userdata.ptr = nullptr;
	alloc.alloc = nk_core_alloc;
	alloc.free = nk_core_free;
	if (nk_init(&_ctx, &alloc, nullptr) == 0) {
		Log::error("Could not init the ui");
		return app::AppState::InitFailure;
	};
	_ctx.clip.copy = nk_sdl_clipbard_copy;
	_ctx.clip.paste = nk_sdl_clipbard_paste;
	_ctx.clip.userdata = nk_handle_ptr(this);
	nk_font_atlas_init(&_atlas, &alloc);
	nk_font_atlas_begin(&_atlas);
	nk_buffer_init(&_cmds, &alloc, 4096);

	uint8_t *fontData = nullptr; // raw ttf data
	size_t fontDataSize = 0u;
	const io::FilePtr& file = filesystem()->open("font.ttf", io::FileMode::Read);
	if (file) {
		fontDataSize = file->read((void **) &fontData);
	} else {
		Log::warn("Failed to load font.ttf");
	}

	for (int i = 0; i < lengthof(_fonts); ++i) {
		const float fontSize = glm::round(_fontSizes[i]);
		if (fontDataSize > 0u) {
			_fonts[i] = nk_font_atlas_add_from_memory(&_atlas, fontData, fontDataSize, fontSize, nullptr);
		}
		if (_fonts[i] == nullptr) {
			_fonts[i] = nk_font_atlas_add_default(&_atlas, fontSize, nullptr);
		}
	}
	delete[] fontData;

	initUIFonts();

	int w, h;
	const void *image = nk_font_atlas_bake(&_atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	if (image == nullptr) {
		Log::error("Failed to bake font atlas");
		return app::AppState::InitFailure;
	}
	_fontTexture->upload(w, h, (const uint8_t*) image);
	nk_font_atlas_end(&_atlas, nk_handle_id((int)_fontTexture->handle()), &_null);
	nk_style_load_all_cursors(&_ctx, _atlas.cursors);

	if (_atlas.default_font) {
		nk_style_set_font(&_ctx, &_atlas.default_font->handle);
	} else if (_fonts[0]) {
		nk_style_set_font(&_ctx, &_fonts[0]->handle);
	}

	if (!_shader.setup()) {
		Log::error("Could not load the ui shader");
		return app::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the material colors");
		return app::AppState::InitFailure;
	}

	if (!_meshRenderer->init()) {
		Log::error("Could not initialize the mesh renderer");
		return app::AppState::InitFailure;
	}

	if (!_textureAtlasRenderer->init()) {
		Log::error("Could not initialize the texture atlas renderer");
		return app::AppState::InitFailure;
	}

	_vertexBufferIndex = _vbo.create();
	if (_vertexBufferIndex < 0) {
		Log::error("Failed to create ui vbo");
		return app::AppState::InitFailure;
	}
	_vbo.setMode(_vertexBufferIndex, video::BufferMode::Stream);

	_elementBufferIndex = _vbo.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_elementBufferIndex < 0) {
		Log::error("Failed to create ui ibo");
		return app::AppState::InitFailure;
	}

	_camera = video::uiCamera(glm::ivec2(0), frameBufferDimension(), windowDimension());

	_vbo.addAttribute(_shader.getColorAttribute(_vertexBufferIndex, &Vertex::r, true));
	_vbo.addAttribute(_shader.getTexcoordAttribute(_vertexBufferIndex, &Vertex::u));
	_vbo.addAttribute(_shader.getPosAttribute(_vertexBufferIndex, &Vertex::x));

	if (!_vbo.update(_vertexBufferIndex, nullptr, MAX_VERTEX_MEMORY)) {
		Log::error("Failed to upload vertex buffer data with %i bytes", MAX_VERTEX_MEMORY);
		return app::AppState::InitFailure;
	}
	if (!_vbo.update(_elementBufferIndex, nullptr, MAX_ELEMENT_MEMORY)) {
		Log::error("Failed to upload index buffer data with %i bytes", MAX_ELEMENT_MEMORY);
		return app::AppState::InitFailure;
	}

	static const struct nk_draw_vertex_layout_element vertexLayout[] = {
		{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, x)},
		{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, u)},
		{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Vertex, r)},
		{NK_VERTEX_LAYOUT_END}
	};
	core_memset(&_config, 0, sizeof(_config));
	_config.vertex_layout = vertexLayout;
	_config.vertex_size = sizeof(Vertex);
	_config.vertex_alignment = NK_ALIGNOF(Vertex);
	initUIConfig(_config);
	initUISkin();

	if (!_console.init()) {
		Log::error("Failed to initialize the console");
		return app::AppState::InitFailure;
	}

	return state;
}

void NuklearApp::initUISkin() {
}

void NuklearApp::initUIConfig(struct nk_convert_config& config) {
	config.null = _null;
	config.circle_segment_count = 22;
	config.curve_segment_count = 22;
	config.arc_segment_count = 22;
	config.global_alpha = 1.0f;
	config.shape_AA = NK_ANTI_ALIASING_ON;
	config.line_AA = NK_ANTI_ALIASING_ON;
}

void NuklearApp::onWindowResize(int windowWidth, int windowHeight) {
	Super::onWindowResize(windowWidth, windowHeight);
	_camera.init(glm::zero<glm::ivec2>(), frameBufferDimension(), windowDimension());
}

app::AppState NuklearApp::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_console.construct();
	return state;
}

struct nk_font* NuklearApp::font(float size) {
	float bestDelta = 10000.0f;
	int fontIndex = 0;
	for (int i = 0; i < lengthof(_fonts); ++i) {
		if (_fontSizes[i] == size) {
			return _fonts[i];
		}
		const float delta = _fontSizes[i] - size;
		if (delta < bestDelta) {
			fontIndex = i;
		}
	}
	return _fonts[fontIndex];
}

app::AppState NuklearApp::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	video::clear(video::ClearFlag::Color);
	_console.update(_deltaFrameSeconds);

	beforeUI();

	nk_input_begin(&_ctx);
	const char *c = _textInput.c_str();
	for (;;) {
		const int key = core::utf8::next(&c);
		if (key == -1) {
			break;
		}
		nk_glyph glyph;
		SDL_memcpy(glyph, &key, sizeof(glyph));
		nk_input_glyph(&_ctx, glyph);
	}
	_textInput.clear();

	const uint8_t* keys = SDL_GetKeyboardState(nullptr);

#define mod_input_key(nkkey, mod, sdlscancode) \
	nk_input_key(&_ctx, (nkkey), (mod) ? keys[(sdlscancode)] : 0)

	const uint32_t modState = SDL_GetModState();
	const bool shift = (modState & KMOD_SHIFT) != 0u;
	const bool ctrl = (modState & KMOD_CTRL) != 0u;

	nk_input_key(&_ctx, NK_KEY_SHIFT, (int)shift);
	nk_input_key(&_ctx, NK_KEY_CTRL, (int)ctrl);
	nk_input_key(&_ctx, NK_KEY_DEL, keys[SDL_SCANCODE_DELETE]);
	nk_input_key(&_ctx, NK_KEY_ENTER, keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER]);
	nk_input_key(&_ctx, NK_KEY_TAB, keys[SDL_SCANCODE_TAB]);
	nk_input_key(&_ctx, NK_KEY_BACKSPACE, keys[SDL_SCANCODE_BACKSPACE]);
	mod_input_key(NK_KEY_COPY, ctrl, SDL_SCANCODE_C);
	mod_input_key(NK_KEY_CUT, ctrl, SDL_SCANCODE_X);
	mod_input_key(NK_KEY_PASTE, ctrl, SDL_SCANCODE_V);
	nk_input_key(&_ctx, NK_KEY_UP, keys[SDL_SCANCODE_UP]);
	nk_input_key(&_ctx, NK_KEY_DOWN, keys[SDL_SCANCODE_DOWN]);
	nk_input_key(&_ctx, NK_KEY_LEFT, keys[SDL_SCANCODE_LEFT]);
	nk_input_key(&_ctx, NK_KEY_RIGHT, keys[SDL_SCANCODE_RIGHT]);
	nk_input_key(&_ctx, NK_KEY_TEXT_INSERT_MODE, keys[SDL_SCANCODE_INSERT]);
	nk_input_key(&_ctx, NK_KEY_TEXT_REPLACE_MODE, !keys[SDL_SCANCODE_INSERT]);
	nk_input_key(&_ctx, NK_KEY_TEXT_RESET_MODE, 0);
	mod_input_key(NK_KEY_TEXT_LINE_START, ctrl, SDL_SCANCODE_B);
	mod_input_key(NK_KEY_TEXT_LINE_END, ctrl, SDL_SCANCODE_E);
	nk_input_key(&_ctx, NK_KEY_TEXT_START, keys[SDL_SCANCODE_HOME]);
	nk_input_key(&_ctx, NK_KEY_TEXT_END, keys[SDL_SCANCODE_END]);
	mod_input_key(NK_KEY_TEXT_UNDO, ctrl, SDL_SCANCODE_Z);
	mod_input_key(NK_KEY_TEXT_REDO, ctrl, SDL_SCANCODE_Y);
	mod_input_key(NK_KEY_TEXT_SELECT_ALL, ctrl, SDL_SCANCODE_A);
	mod_input_key(NK_KEY_TEXT_WORD_LEFT, ctrl, SDL_SCANCODE_LEFT);
	mod_input_key(NK_KEY_TEXT_WORD_RIGHT, ctrl, SDL_SCANCODE_RIGHT);
	nk_input_key(&_ctx, NK_KEY_SCROLL_START, keys[SDL_SCANCODE_HOME]);
	nk_input_key(&_ctx, NK_KEY_SCROLL_END, keys[SDL_SCANCODE_END]);
	nk_input_key(&_ctx, NK_KEY_SCROLL_DOWN, keys[SDL_SCANCODE_PAGEDOWN]);
	nk_input_key(&_ctx, NK_KEY_SCROLL_UP, keys[SDL_SCANCODE_PAGEUP]);

#undef mod_input_key

	int x;
	int y;
	const uint32_t mouseState = SDL_GetMouseState(&x, &y);
	nk_input_motion(&_ctx, x, y);
	nk_input_scroll(&_ctx, _scrollDelta);
	nk_input_button(&_ctx, NK_BUTTON_LEFT, x, y, (int)(mouseState & SDL_BUTTON_LMASK));
	nk_input_button(&_ctx, NK_BUTTON_MIDDLE, x, y, (int)(mouseState & SDL_BUTTON_MMASK));
	nk_input_button(&_ctx, NK_BUTTON_RIGHT, x, y, (int)(mouseState & SDL_BUTTON_RMASK));
	nk_input_button(&_ctx, NK_BUTTON_DOUBLE, x, y, 0);
	_scrollDelta.x = _scrollDelta.y = 0.0f;

	nk_input_end(&_ctx);

	if (!_console.isActive()) {
		if (!onRenderUI()) {
			if (_ctx.current) {
				nk_end(&_ctx);
			}
			nk_clear(&_ctx);
			return state;
		}
	}

	const math::Rect<int> rect(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	_console.render(rect, _deltaFrameSeconds);

	video::ScopedShader scopedShader(_shader);
	_shader.setViewprojection(_camera.projectionMatrix());
	_shader.setModel(glm::mat4(1.0f));
	_shader.setTexture(video::TextureUnit::Zero);

	video::ScopedViewPort scopedViewPort(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
	video::enable(video::State::Blend);
	video::blendEquation(video::BlendEquation::Add);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);
	video::disable(video::State::CullFace);
	video::disable(video::State::DepthTest);
	video::enable(video::State::Scissor);

	void *vertices = _vbo.mapData(_vertexBufferIndex, video::AccessMode::Write);
	if (vertices == nullptr) {
		Log::warn("Failed to map vertices");
		return app::AppState::Cleanup;
	}
	void *elements = _vbo.mapData(_elementBufferIndex, video::AccessMode::Write);
	if (elements == nullptr) {
		Log::warn("Failed to map indices");
		return app::AppState::Cleanup;
	}

	struct nk_buffer vbuf;
	struct nk_buffer ebuf;
	nk_buffer_init_fixed(&vbuf, vertices, (nk_size) MAX_VERTEX_MEMORY);
	nk_buffer_init_fixed(&ebuf, elements, (nk_size) MAX_ELEMENT_MEMORY);

	const nk_flags convertRes = nk_convert(&_ctx, &_cmds, &vbuf, &ebuf, &_config);

	Log::trace("vertices buffer size: %i", (int)vbuf.size);
	Log::trace("index buffer size: %i", (int)ebuf.size);

	_vbo.unmapData(_vertexBufferIndex);
	_vbo.unmapData(_elementBufferIndex);

	if (convertRes == NK_CONVERT_SUCCESS) {
		const nk_draw_index *offset = nullptr;
		const struct nk_draw_command *cmd;
		int64_t drawCommands = 0;
		nk_draw_foreach(cmd, &_ctx, &_cmds) {
			if (!cmd->elem_count) {
				continue;
			}
			video::bindTexture(video::TextureUnit::Zero, video::TextureType::Texture2D, cmd->texture.id);
			video::scissor(cmd->clip_rect.x, cmd->clip_rect.y, cmd->clip_rect.w, cmd->clip_rect.h);
			video::drawElements(video::Primitive::Triangles, (size_t) cmd->elem_count, video::mapType<nk_draw_index>(), (void*) offset);
			offset += cmd->elem_count;
			++drawCommands;
		}
		core_trace_plot("UIDrawCommands", drawCommands);
	} else {
		 if (convertRes & NK_CONVERT_INVALID_PARAM) {
			Log::warn("An invalid argument was passed in the function call");
		}
		if (convertRes & NK_CONVERT_COMMAND_BUFFER_FULL) {
			Log::warn("The provided buffer for storing draw commands is full or failed to allocate more memory");
		}
		if (convertRes & NK_CONVERT_VERTEX_BUFFER_FULL) {
			Log::warn("The provided buffer for storing vertices is full or failed to allocate more memory");
		}
		if (convertRes & NK_CONVERT_ELEMENT_BUFFER_FULL) {
			Log::warn("The provided buffer for storing indicies is full or failed to allocate more memory");
		}
	}

	_vbo.unbind();

	nk_clear(&_ctx);
	nk_buffer_clear(&_cmds);
	nk_buffer_free(&vbuf);
	nk_buffer_free(&ebuf);
	return state;
}

app::AppState NuklearApp::onCleanup() {
	nk_font_atlas_clear(&_atlas);
	nk_buffer_free(&_cmds);
	nk_free(&_ctx);

	_console.shutdown();
	_shader.shutdown();
	_meshRenderer->shutdown();
	_textureAtlasRenderer->shutdown();
	_vbo.shutdown();
	if (_emptyTexture) {
		_emptyTexture->shutdown();
	}
	if (_fontTexture) {
		_fontTexture->shutdown();
	}

	return Super::onCleanup();
}

}
}
