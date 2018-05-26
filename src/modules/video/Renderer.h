/**
 * @file
 * @defgroup Video
 * @{
 * The video subsystem implements rendering and window management.
 * @}
 */

#pragma once

#include "Types.h"
#include "core/Assert.h"
#include "image/Image.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "TextureConfig.h"
#include "FrameBufferConfig.h"
#include "StencilConfig.h"
#include "RenderBuffer.h"

namespace video {

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

/**
 * @brief Maps data types to GL enums
 */
template<class DATATYPE>
constexpr inline DataType mapType() {
	if (std::is_floating_point<DATATYPE>()) {
		if (sizeof(DATATYPE) == 4u) {
			return DataType::Float;
		}
		return DataType::Double;
	}

	if (sizeof(DATATYPE) == 1u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedByte;
		}
		return DataType::Byte;
	}

	if (sizeof(DATATYPE) == 2u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedShort;
		}
		return DataType::Short;
	}

	if (sizeof(DATATYPE) == 4u) {
		if (std::is_unsigned<DATATYPE>()) {
			return DataType::UnsignedInt;
		}
		return DataType::Int;
	}

	return DataType::Max;
}

/**
 * @brief Allows to profile a particular renderer action
 */
class ProfilerGPU {
private:
	Id _id = InvalidId;
	double _min = 0.0;
	double _max = 0.0;
	double _avg = 0.0;
	std::string _name;
	std::vector<double> _samples;
	const int16_t _maxSampleCount;
	int16_t _sampleCount = 0;
	uint8_t _state = 0;
public:
	ProfilerGPU(const std::string& name, uint16_t maxSamples = 1024u);
	~ProfilerGPU();

	const std::vector<double>& samples() const;
	bool init();
	void shutdown();
	void enter();
	void leave();
	double minimum() const;
	double maximum() const;
	double avg() const;
	const std::string& name() const;
};

struct RenderState {
	int limits[std::enum_value(video::Limit::Max)] = { };
	inline int limit(video::Limit limit) const {
		return limits[std::enum_value(limit)];
	}

	double specs[std::enum_value(video::Spec::Max)] = { };
	inline int specificationi(video::Spec spec) const {
		return (int)(specification(spec) + 0.5);
	}

	inline double specification(video::Spec spec) const {
		return specs[std::enum_value(spec)];
	}

	bool features[std::enum_value(video::Feature::Max)] = { };
	inline bool supports(video::Feature feature) const {
		return features[std::enum_value(feature)];
	}
};

extern RenderState& renderState();

inline ProfilerGPU::ProfilerGPU(const std::string& name, uint16_t maxSamples) :
		_name(name), _maxSampleCount(maxSamples) {
	core_assert(maxSamples > 0);
	_samples.reserve(_maxSampleCount);
}

inline ProfilerGPU::~ProfilerGPU() {
	core_assert_msg(_id == 0u, "Forgot to shutdown gpu profiler: %s", _name.c_str());
	shutdown();
}

inline const std::vector<double>& ProfilerGPU::samples() const {
	return _samples;
}

inline const std::string& ProfilerGPU::name() const {
	return _name;
}

inline double ProfilerGPU::avg() const {
	return _avg;
}

inline double ProfilerGPU::minimum() const {
	return _min;
}

inline double ProfilerGPU::maximum() const {
	return _max;
}

/**
 * @brief Prepare the renderer initialization
 * @sa init()
 */
extern void setup();
/**
 * @note setup() must be called before init()
 */
extern bool init();
extern void destroyContext(RendererContext& context);
extern RendererContext createContext(SDL_Window* window);
extern void startFrame(SDL_Window* window, RendererContext& context);
extern void endFrame(SDL_Window* window);
extern void checkError();
extern bool setupCubemap(Id handle, const image::ImagePtr images[6]);
extern void readBuffer(GBufferTextureType textureType);
extern bool bindFrameBufferAttachment(Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear);
extern bool setupGBuffer(Id fbo, const glm::ivec2& dimension, Id* textures, int texCount, Id depthTexture);
/**
 * @brief Change the renderer line width
 * @param width The new line width
 * @return The previous line width
 */
extern float lineWidth(float width);
extern bool clearColor(const glm::vec4& clearColor);
extern void clear(ClearFlag flag);
extern bool viewport(int x, int y, int w, int h);
extern void getScissor(int& x, int& y, int& w, int& h);
extern void getViewport(int& x, int& y, int& w, int& h);
/**
 * @brief Given in screen coordinates
 * @note viewport() must have seen called before
 */
extern bool scissor(int x, int y, int w, int h);
/**
 * @brief Enables a renderer state
 * @param state The State to change
 * @return The previous state value
 */
extern bool enable(State state);
/**
 * @brief Disables a renderer state
 * @param state The State to change
 * @return The previous state value
 */
extern bool disable(State state);
extern void colorMask(bool red, bool green, bool blue, bool alpha);
extern bool cullFace(Face face);
extern bool depthFunc(CompareFunc func);
extern bool setupStencil(const StencilConfig& config);
extern void getBlendState(bool& enabled, BlendMode& src, BlendMode& dest, BlendEquation& func);
extern bool blendFunc(BlendMode src, BlendMode dest);
extern bool blendEquation(BlendEquation func);
extern PolygonMode polygonMode(Face face, PolygonMode mode);
extern bool polygonOffset(const glm::vec2& offset);
extern bool activeTextureUnit(TextureUnit unit);
extern bool bindTexture(TextureUnit unit, TextureType type, Id handle);
extern bool useProgram(Id handle);
extern Id getProgram();
extern bool bindVertexArray(Id handle);
extern Id boundVertexArray();
extern Id boundBuffer(BufferType type);
extern void unmapBuffer(Id handle, BufferType type);
extern void* mapBuffer(Id handle, BufferType type, AccessMode mode);
extern bool bindBuffer(BufferType type, Id handle);
extern bool unbindBuffer(BufferType type);
extern bool bindBufferBase(BufferType type, Id handle, uint32_t index = 0u);
extern void genBuffers(uint8_t amount, Id* ids);
extern Id genBuffer();
extern void deleteBuffers(uint8_t amount, Id* ids);
extern void deleteBuffer(Id& id);
extern void genVertexArrays(uint8_t amount, Id* ids);
extern Id genVertexArray();
extern void deleteShader(Id& id);
extern Id genShader(ShaderType type);
extern void deleteProgram(Id& id);
extern Id genProgram();
extern void deleteVertexArrays(uint8_t amount, Id* ids);
extern void deleteVertexArray(Id& id);
extern void genTextures(uint8_t amount, Id* ids);
extern Id genTexture();
extern void deleteTextures(uint8_t amount, Id* ids);
extern void deleteTexture(Id& id);
extern void genFramebuffers(uint8_t amount, Id* ids);
extern Id genFramebuffer();
extern void deleteFramebuffers(uint8_t amount, Id* ids);
extern void deleteFramebuffer(Id& id);
extern void genRenderbuffers(uint8_t amount, Id* ids);
extern Id genRenderbuffer();
extern void deleteRenderbuffers(uint8_t amount, Id* ids);
extern void deleteRenderbuffer(Id& id);
extern void configureAttribute(const Attribute& a);
extern Id genOcclusionQuery();
extern void deleteOcclusionQuery(Id& id);
extern bool isOcclusionQuery(Id id);
extern bool beginOcclusionQuery(Id id);
extern bool endOcclusionQuery(Id id);
extern bool isOcclusionQueryAvailable(Id id);
/**
 * @return The value of the query object's passed samples counter. The initial value is 0, -1 is returned on error
 * or if no result is available yet - this call does not block the gpu
 */
extern int getOcclusionQueryResult(Id id, bool wait = false);
/**
 * Binds a new frame buffer
 * @param mode The FrameBufferMode to bind the frame buffer with
 * @param handle The Id that represents the handle of the frame buffer
 * @param textureHandle The Id that represents the texture that should be bound as frame buffer color attachment.
 * @return The previously bound frame buffer Id
 */
extern Id bindFramebuffer(Id handle, FrameBufferMode mode = FrameBufferMode::Default);
extern bool setupRenderBuffer(TextureFormat format, int w, int h, int samples);
extern Id bindRenderbuffer(Id handle);
extern void bufferData(Id handle, BufferType type, BufferMode mode, const void* data, size_t size);
extern void bufferSubData(Id handle, BufferType type, intptr_t offset, const void* data, size_t size);
/**
 * @return The size of the buffer object, measured in bytes.
 */
extern size_t bufferSize(BufferType type);
extern void setupDepthCompareTexture(video::TextureType type, CompareFunc func, TextureCompareMode mode);
extern const glm::vec4& framebufferUV();
extern bool setupFramebuffer(const std::map<FrameBufferAttachment, TexturePtr>& colorTextures, const std::map<FrameBufferAttachment, RenderBufferPtr>& bufferAttachments);
extern void setupTexture(const TextureConfig& config);
extern void uploadTexture(video::TextureType type, video::TextureFormat format, int width, int height, const uint8_t* data, int index);
extern void drawElements(Primitive mode, size_t numIndices, DataType type, void* offset = nullptr);
extern void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount);
extern void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex);
extern void drawArrays(Primitive mode, size_t count);
extern void disableDebug();
extern bool hasFeature(Feature feature);
extern void enableDebug(DebugSeverity severity);
extern bool compileShader(Id id, ShaderType shaderType, const std::string& source, const std::string& name);
extern bool linkShader(Id program, Id vert, Id frag, Id geom, const std::string& name);
extern bool linkComputeShader(Id program, Id comp, const std::string& name);
extern bool bindImage(Id handle, AccessMode mode, ImageFormat format);
extern bool runShader(Id program, const glm::uvec3& workGroups, bool wait = false);
extern int fetchUniforms(Id program, ShaderUniforms& uniforms, const std::string& name);
extern int fetchAttributes(Id program, ShaderAttributes& attributes, const std::string& name);
extern void flush();

extern bool beginTransformFeedback(Primitive primitive);
extern void pauseTransformFeedback();
extern void resumeTransformFeedback();
extern void endTransformFeedback();
extern bool bindTransforFeebackBuffer(int index, Id bufferId);
extern bool bindTransformFeedback(Id id);
extern void deleteTransformFeedback(Id& id);
extern Id genTransformFeedback();
extern bool bindTransformFeedbackVaryings(Id program, TransformFeedbackCaptureMode transformFormat, const std::vector<std::string>& transformVaryings);

template<class IndexType>
inline void drawElements(Primitive mode, size_t numIndices, void* offset = nullptr) {
	drawElements(mode, numIndices, mapType<IndexType>(), offset);
}

template<class IndexType>
inline void drawElementsInstanced(Primitive mode, size_t numIndices, size_t amount) {
	drawElementsInstanced(mode, numIndices, mapType<IndexType>(), amount);
}

template<class IndexType>
inline void drawElementsBaseVertex(Primitive mode, size_t numIndices, int baseIndex, int baseVertex) {
	drawElementsBaseVertex(mode, numIndices, mapType<IndexType>(), sizeof(IndexType), baseIndex, baseVertex);
}

inline bool hasFeature(Feature f) {
	return renderState().supports(f);
}

inline int limit(Limit l) {
	return renderState().limit(l);
}

inline int specificationi(Spec l) {
	return renderState().specificationi(l);
}

inline double specification(Spec l) {
	return renderState().specification(l);
}

/**
 * @brief Checks whether a given hardware limit is exceeded.
 */
inline bool checkLimit(int amount, Limit l) {
	const int v = renderState().limit(l);
	return v >= amount;
}

}
