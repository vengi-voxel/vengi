/**
 * @file
 */

#include "Renderer.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "TextureConfig.h"
#include "core/Var.h"

namespace video {

static const char *featuresArray[] = {
	"r_texturecompressiondxt",	"r_texturecompressionpvrtc",
	"r_texturecompressionetc2", "r_texturecompressionatc",
	"r_texturefloat",			"r_texturehalffloat",
	"r_instancedarrays",		"r_debugoutput",
	"r_directstateaccess",		"r_bufferstorage",
	"r_multidrawindirect",		"r_computeshaders",
	"r_transformfeedback",		"r_shaderstoragebufferobject"
};
static_assert(core::enumVal(Feature::Max) == (int)lengthof(featuresArray), "Array sizes don't match with Feature enum");
static core::VarPtr featureVars[core::enumVal(Feature::Max)];

static RenderState s;

void construct() {
	for (int i = 0; i < core::enumVal(Feature::Max); ++i) {
		featureVars[i] = core::Var::get(featuresArray[i], "false", "Renderer feature cvar", core::Var::boolValidator);
	}
}

DataType mapIndexTypeBySize(size_t size) {
	if (size == 4u) {
		return DataType::UnsignedInt;
	}
	if (size == 2u) {
		return DataType::UnsignedShort;
	}
	core_assert(size == 1u);
	return DataType::UnsignedByte;
}

void deleteRenderbuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteRenderbuffers(1, &id);
	id = InvalidId;
}

void deleteFramebuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteFramebuffers(1, &id);
	id = InvalidId;
}

void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteTextures(1, &id);
	id = InvalidId;
}

Id genTexture(const TextureConfig &cfg) {
	Id id;
	genTextures(cfg, 1, &id);
	return id;
}

void deleteBuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteBuffers(1, &id);
	id = InvalidId;
}

Id genVertexArray() {
	Id id;
	genVertexArrays(1, &id);
	return id;
}

Id genBuffer() {
	Id id;
	genBuffers(1, &id);
	return id;
}

Id genRenderbuffer() {
	Id id;
	genRenderbuffers(1, &id);
	return id;
}

Id genFramebuffer() {
	Id id;
	genFramebuffers(1, &id);
	return id;
}

void disableDebug() {
	if (!hasFeature(Feature::DebugOutput)) {
		return;
	}
	disable(State::DebugOutput);
	Log::info("disable render debug messages");
}

bool checkLimit(int amount, Limit l) {
	const int v = renderState().limiti(l);
	if (v <= 0) {
		Log::trace("No limit found for %i", (int)l);
		return true;
	}
	return v >= amount;
}

bool useFeature(Feature feature) {
	if (!hasFeature(feature)) {
		return false;
	}
	const core::VarPtr& v = featureVars[core::enumVal(feature)];
	if (!v) {
		return true;
	}
	return v->boolVal();
}

RenderState& renderState() {
	return s;
}

float currentLineWidth() {
	return rendererState().pendingLineWidth;
}

const glm::vec4 &currentClearColor() {
	return rendererState().pendingClearColor;
}

bool clearColor(const glm::vec4 &clearColor) {
	if (rendererState().pendingClearColor == clearColor) {
		return false;
	}
	rendererState().pendingClearColor = clearColor;
	return true;
}

bool viewport(int x, int y, int w, int h) {
	if (rendererState().pendingViewportX == x && rendererState().pendingViewportY == y && rendererState().pendingViewportW == w && rendererState().pendingViewportH == h) {
		return false;
	}
	rendererState().pendingViewportX = x;
	rendererState().pendingViewportY = y;
	rendererState().pendingViewportW = w;
	rendererState().pendingViewportH = h;
	return true;
}

void getViewport(int &x, int &y, int &w, int &h) {
	x = rendererState().pendingViewportX;
	y = rendererState().pendingViewportY;
	w = rendererState().pendingViewportW;
	h = rendererState().pendingViewportH;
}

void getScissor(int &x, int &y, int &w, int &h) {
	x = rendererState().pendingScissorX;
	y = rendererState().pendingScissorY;
	w = rendererState().pendingScissorW;
	h = rendererState().pendingScissorH;
}

bool scissor(int x, int y, int w, int h) {
	if (w < 0) {
		w = 0;
	}
	if (h < 0) {
		h = 0;
	}

	if (rendererState().pendingScissorX == x && rendererState().pendingScissorY == y && rendererState().pendingScissorW == w && rendererState().pendingScissorH == h) {
		return false;
	}
	rendererState().pendingScissorX = x;
	rendererState().pendingScissorY = y;
	rendererState().pendingScissorW = w;
	rendererState().pendingScissorH = h;
	return true;
}

void colorMask(bool red, bool green, bool blue, bool alpha) {
	rendererState().pendingColorMask[0] = red;
	rendererState().pendingColorMask[1] = green;
	rendererState().pendingColorMask[2] = blue;
	rendererState().pendingColorMask[3] = alpha;
}

bool enable(State state) {
	const int stateIndex = core::enumVal(state);
	if (rendererState().pendingStates[stateIndex]) {
		return true;
	}
	rendererState().pendingStates.set(stateIndex, true);
	return false;
}

bool disable(State state) {
	const int stateIndex = core::enumVal(state);
	if (!rendererState().pendingStates[stateIndex]) {
		return false;
	}
	rendererState().pendingStates.set(stateIndex, false);
	return true;
}

bool currentState(State state) {
	const int stateIndex = core::enumVal(state);
	return rendererState().pendingStates[stateIndex];
}

bool cullFace(Face face) {
	if (face == Face::Max) {
		return false;
	}
	if (rendererState().pendingCullFace == face) {
		return false;
	}
	rendererState().pendingCullFace = face;
	return true;
}

Face currentCullFace() {
	return rendererState().pendingCullFace;
}

bool depthFunc(CompareFunc func) {
	if (rendererState().pendingDepthFunc == func) {
		return false;
	}
	rendererState().pendingDepthFunc = func;
	return true;
}

CompareFunc getDepthFunc() {
	return rendererState().pendingDepthFunc;
}

bool blendEquation(BlendEquation func) {
	if (rendererState().pendingBlendEquation == func) {
		return false;
	}
	rendererState().pendingBlendEquation = func;
	return true;
}

void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func) {
	const int stateIndex = core::enumVal(State::Blend);
	enabled = rendererState().pendingStates[stateIndex];
	src = rendererState().pendingBlendSrcRGB;
	dest = rendererState().pendingBlendDestRGB;
	func = rendererState().pendingBlendEquation;
}

bool blendFunc(BlendMode src, BlendMode dest) {
	if (rendererState().pendingBlendSrcRGB == src && rendererState().pendingBlendDestRGB == dest && rendererState().pendingBlendSrcAlpha == src &&
		rendererState().pendingBlendDestAlpha == dest) {
		return false;
	}
	rendererState().pendingBlendSrcRGB = src;
	rendererState().pendingBlendDestRGB = dest;
	rendererState().pendingBlendSrcAlpha = src;
	rendererState().pendingBlendDestAlpha = dest;
	return true;
}

bool blendFuncSeparate(BlendMode srcRGB, BlendMode destRGB, BlendMode srcAlpha, BlendMode destAlpha) {
	if (rendererState().pendingBlendSrcRGB == srcRGB && rendererState().pendingBlendDestRGB == destRGB && rendererState().pendingBlendSrcAlpha == srcAlpha &&
		rendererState().pendingBlendDestAlpha == destAlpha) {
		return false;
	}
	rendererState().pendingBlendSrcRGB = srcRGB;
	rendererState().pendingBlendDestRGB = destRGB;
	rendererState().pendingBlendSrcAlpha = srcAlpha;
	rendererState().pendingBlendDestAlpha = destAlpha;
	return true;
}

PolygonMode polygonMode(Face face, PolygonMode mode) {
	if (rendererState().pendingPolygonModeFace == face && rendererState().pendingPolygonMode == mode) {
		return rendererState().pendingPolygonMode;
	}
	rendererState().pendingPolygonModeFace = face;
	const PolygonMode old = rendererState().pendingPolygonMode;
	rendererState().pendingPolygonMode = mode;
	return old;
}

bool polygonOffset(const glm::vec2 &offset) {
	if (rendererState().pendingPolygonOffset == offset) {
		return false;
	}
	rendererState().pendingPolygonOffset = offset;
	return true;
}

bool pointSize(float size) {
	if (rendererState().pendingPointSize == size) {
		return false;
	}
	rendererState().pendingPointSize = size;
	return true;
}

Id currentTexture(TextureUnit unit) {
	if (TextureUnit::Max == unit) {
		return InvalidId;
	}
	return rendererState().textureHandle[core::enumVal(unit)];
}

}
