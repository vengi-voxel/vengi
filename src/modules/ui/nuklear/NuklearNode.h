/**
 * @file
 */

#include "Nuklear.h"
#include <stdint.h>
#include "video/Camera.h"
#include "video/TextureAtlasRenderer.h"
#include "voxelrender/CachedMeshRenderer.h"

namespace ui {
namespace nuklear {

struct nkc_context {
	nk_context* ctx = nullptr;
	voxelrender::CachedMeshRendererPtr meshRenderer;
	video::TextureAtlasRendererPtr textureAtlasRenderer;
};

struct nkc_model {
	const char* modelPath;
	double timeSeconds = 0.0f;
	float scale = 1.0f;
	float omegaY = 0.0f;
	video::Camera camera;
};

void nkc_model(struct nkc_context*, struct nkc_model* model);
void nkc_text(struct nkc_context* ctx, const char *string, nk_flags alignment, const struct nk_color &color, const struct nk_user_font *font);

}
}
