/**
 * @file
 */

#define NK_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "Nuklear.h"
#undef NK_IMPLEMENTATION
#include "NuklearNode.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "core/Assert.h"
#include "video/Renderer.h"
#include "core/Color.h"

namespace ui {
namespace nuklear {

void nkc_model(struct nkc_context* cctx, struct nkc_model* model) {
	const int modelId = cctx->meshRenderer->addMesh(model->modelPath);
	if (modelId == -1) {
		return;
	}
	struct nk_context* ctx = cctx->ctx;
	core_assert(ctx->current);
	core_assert(ctx->current->layout);
	if (!ctx->current || !ctx->current->layout) {
		return;
	}

	struct nk_rect bounds;
	if (!nk_widget(&bounds, ctx)) {
		return;
	}

	const glm::ivec2 size(bounds.w, bounds.h);
	model->camera.init(glm::ivec2(0), size, size);
	model->camera.update(0.0);

	const nk_color& c = ctx->style.window.background;
	const glm::vec4 prevColor = video::currentClearColor();
	video::clearColor(core::Color::fromRGBA(c.r, c.g, c.b, c.a));

	constexpr glm::mat4 translate(1.0f);
	constexpr double tau = glm::two_pi<double>();
	const double orientation = glm::mod(model->omegaY * model->timeSeconds, tau);
	const glm::mat4 rot = glm::rotate(translate, (float)orientation, glm::up);
	const glm::mat4 modelMatrix = glm::scale(rot, glm::vec3(model->scale));

	const video::TextureAtlasRendererPtr& atlasRenderer = cctx->textureAtlasRenderer;
	const video::TextureAtlasData& atlas = atlasRenderer->beginRender(modelId, bounds.w, bounds.h);
	cctx->meshRenderer->setModelMatrix(modelId, modelMatrix);
	cctx->meshRenderer->render(modelId, model->camera);
	atlasRenderer->endRender();
	video::clearColor(prevColor);

	struct nk_image image;
	image.handle = nk_handle_id(atlas.handle);
	image.w = atlas.texWidth;
	image.h = atlas.texHeight;
	// TODO: y axis is flipped due to framebuffer y-flipped rendering
	image.region[0] = atlas.sx * (float)image.w;
	image.region[1] = atlas.sy * (float)image.h;
	image.region[2] = (atlas.tx - atlas.sx) * (float)image.w;
	image.region[3] = (atlas.ty - atlas.sy) * (float)image.h;
	static constexpr struct nk_color white{255, 255, 255, 255};
	struct nk_window *win = ctx->current;
	nk_draw_image(&win->buffer, bounds, &image, white);
}

void nkc_text(struct nkc_context* ctx, const char *string, nk_flags alignment, const struct nk_color &color, const struct nk_user_font *font) {
	struct nk_rect bounds;
	nk_panel_alloc_space(&bounds, ctx->ctx);

	const struct nk_style *style = &ctx->ctx->style;
	struct nk_text nktext;
	nktext.padding = style->text.padding;
	nktext.background = style->window.background;
	nktext.text = color;

	struct nk_window *win = ctx->ctx->current;
	nk_widget_text(&win->buffer, bounds, string, SDL_strlen(string), &nktext, alignment, font);
}

}
}

const struct nk_color* nkc_get_default_color_style(int* n) {
	if (n != nullptr) {
		*n = NK_COLOR_COUNT;
	}
	return nk_default_color_style;
}
