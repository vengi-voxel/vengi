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
	RendererState &rs = rendererState();
	if (rs.pendingClearColor == clearColor) {
		return false;
	}
	rs.pendingClearColor = clearColor;
	return true;
}

bool viewport(int x, int y, int w, int h) {
	RendererState &rs = rendererState();
	if (rs.pendingViewportX == x && rs.pendingViewportY == y && rs.pendingViewportW == w && rs.pendingViewportH == h) {
		return false;
	}
	rs.pendingViewportX = x;
	rs.pendingViewportY = y;
	rs.pendingViewportW = w;
	rs.pendingViewportH = h;
	return true;
}

void getViewport(int &x, int &y, int &w, int &h) {
	RendererState &rs = rendererState();
	x = rs.pendingViewportX;
	y = rs.pendingViewportY;
	w = rs.pendingViewportW;
	h = rs.pendingViewportH;
}

void getScissor(int &x, int &y, int &w, int &h) {
	RendererState &rs = rendererState();
	x = rs.pendingScissorX;
	y = rs.pendingScissorY;
	w = rs.pendingScissorW;
	h = rs.pendingScissorH;
}

bool scissor(int x, int y, int w, int h) {
	if (w < 0) {
		w = 0;
	}
	if (h < 0) {
		h = 0;
	}

	RendererState &rs = rendererState();
	if (rs.pendingScissorX == x && rs.pendingScissorY == y && rs.pendingScissorW == w && rs.pendingScissorH == h) {
		return false;
	}
	rs.pendingScissorX = x;
	rs.pendingScissorY = y;
	rs.pendingScissorW = w;
	rs.pendingScissorH = h;
	return true;
}

void colorMask(bool red, bool green, bool blue, bool alpha) {
	RendererState &rs = rendererState();
	rs.pendingColorMask[0] = red;
	rs.pendingColorMask[1] = green;
	rs.pendingColorMask[2] = blue;
	rs.pendingColorMask[3] = alpha;
}

bool enable(State state) {
	RendererState &rs = rendererState();
	const int stateIndex = core::enumVal(state);
	if (rs.pendingStates[stateIndex]) {
		return true;
	}
	rs.pendingStates.set(stateIndex, true);
	return false;
}

bool disable(State state) {
	RendererState &rs = rendererState();
	const int stateIndex = core::enumVal(state);
	if (!rs.pendingStates[stateIndex]) {
		return false;
	}
	rs.pendingStates.set(stateIndex, false);
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
	RendererState &rs = rendererState();
	if (rs.pendingCullFace == face) {
		return false;
	}
	rs.pendingCullFace = face;
	return true;
}

Face currentCullFace() {
	return rendererState().pendingCullFace;
}

bool depthFunc(CompareFunc func) {
	RendererState &rs = rendererState();
	if (rs.pendingDepthFunc == func) {
		return false;
	}
	rs.pendingDepthFunc = func;
	return true;
}

CompareFunc getDepthFunc() {
	return rendererState().pendingDepthFunc;
}

bool blendEquation(BlendEquation func) {
	RendererState &rs = rendererState();
	if (rs.pendingBlendEquation == func) {
		return false;
	}
	rs.pendingBlendEquation = func;
	return true;
}

void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func) {
	const int stateIndex = core::enumVal(State::Blend);
	RendererState &rs = rendererState();
	enabled = rs.pendingStates[stateIndex];
	src = rs.pendingBlendSrcRGB;
	dest = rs.pendingBlendDestRGB;
	func = rs.pendingBlendEquation;
}

bool blendFunc(BlendMode src, BlendMode dest) {
	RendererState &rs = rendererState();
	if (rs.pendingBlendSrcRGB == src && rs.pendingBlendDestRGB == dest && rs.pendingBlendSrcAlpha == src &&
		rs.pendingBlendDestAlpha == dest) {
		return false;
	}
	rs.pendingBlendSrcRGB = src;
	rs.pendingBlendDestRGB = dest;
	rs.pendingBlendSrcAlpha = src;
	rs.pendingBlendDestAlpha = dest;
	return true;
}

bool blendFuncSeparate(BlendMode srcRGB, BlendMode destRGB, BlendMode srcAlpha, BlendMode destAlpha) {
	RendererState &rs = rendererState();
	if (rs.pendingBlendSrcRGB == srcRGB && rs.pendingBlendDestRGB == destRGB && rs.pendingBlendSrcAlpha == srcAlpha &&
		rs.pendingBlendDestAlpha == destAlpha) {
		return false;
	}
	rs.pendingBlendSrcRGB = srcRGB;
	rs.pendingBlendDestRGB = destRGB;
	rs.pendingBlendSrcAlpha = srcAlpha;
	rs.pendingBlendDestAlpha = destAlpha;
	return true;
}

PolygonMode polygonMode(Face face, PolygonMode mode) {
	RendererState &rs = rendererState();
	if (rs.pendingPolygonModeFace == face && rs.pendingPolygonMode == mode) {
		return rs.pendingPolygonMode;
	}
	rs.pendingPolygonModeFace = face;
	const PolygonMode old = rs.pendingPolygonMode;
	rs.pendingPolygonMode = mode;
	return old;
}

bool polygonOffset(const glm::vec2 &offset) {
	RendererState &rs = rendererState();
	if (rs.pendingPolygonOffset == offset) {
		return false;
	}
	rs.pendingPolygonOffset = offset;
	return true;
}

bool pointSize(float size) {
	RendererState &rs = rendererState();
	if (rs.pendingPointSize == size) {
		return false;
	}
	rs.pendingPointSize = size;
	return true;
}

Id currentTexture(TextureUnit unit) {
	if (TextureUnit::Max == unit) {
		return InvalidId;
	}
	return rendererState().textureHandle[core::enumVal(unit)];
}

const core::DynamicSet<Id> &textures() {
	return rendererState().textures;
}

Id boundVertexArray() {
	return rendererState().vertexArrayHandle;
}

Id boundBuffer(BufferType type) {
	const int typeIndex = core::enumVal(type);
	return rendererState().bufferHandle[typeIndex];
}

Id getProgram() {
	return rendererState().programHandle;
}

bool useProgram(Id handle) {
	if (rendererState().pendingProgramHandle == handle) {
		return false;
	}
	rendererState().pendingProgramHandle = handle;
	return true;
}

Id currentFramebuffer() {
	return rendererState().framebufferHandle;
}

int drawCalls() {
	return rendererState().drawCalls;
}

void resize(int windowWidth, int windowHeight, float scaleFactor) {
	rendererState().windowWidth = windowWidth;
	rendererState().windowHeight = windowHeight;
	rendererState().scaleFactor = scaleFactor;
}

glm::ivec2 getWindowSize() {
	return glm::ivec2(rendererState().windowWidth, rendererState().windowHeight);
}

float getScaleFactor() {
	return rendererState().scaleFactor;
}

} // namespace video
