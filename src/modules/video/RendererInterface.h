/**
 *@file
 */

#pragma once

#include "RenderBuffer.h"
#include "ShaderTypes.h"
#include "core/SharedPtr.h"
#include "core/collection/List.h"
#include "core/collection/Set.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct SDL_Window;

namespace image {
class Image;
typedef core::SharedPtr<Image> ImagePtr;
} // namespace image

namespace video {

class Texture;
typedef core::SharedPtr<Texture> TexturePtr;

class TextureConfig;

/**
 * @brief Prepare the renderer initialization
 * @sa init()
 */
void setup();
/**
 * @note setup() must be called before init()
 */
bool init(int windowWidth, int windowHeight, float scaleFactor);
void resize(int windowWidth, int windowHeight, float scaleFactor);
float getScaleFactor();
glm::ivec2 getWindowSize();
void destroyContext(RendererContext &context);
RendererContext createContext(SDL_Window *window);
void activateContext(SDL_Window *window, RendererContext &context);
void startFrame(SDL_Window *window, RendererContext &context);
void endFrame(SDL_Window *window);
/**
 * @brief Checks the error state since the last call to this function.
 * @param[in] triggerAssert Triggers an assert whenever a failure happened. If @c false is given here, you might
 * just want to delete the error state for future checks or the previous state chnages are expected to fail.
 * @return @c false if no error was found, @c true if an error occurred
 */
bool checkError(bool triggerAssert = true);
void readBuffer(GBufferTextureType textureType);
/**
 * @brief Change the renderer line width
 * @param width The new line width
 * @return The previous line width
 */
float lineWidth(float width);
float currentLineWidth();
bool clearColor(const glm::vec4 &clearColor);
const glm::vec4 &currentClearColor();
void clear(ClearFlag flag);
bool viewport(int x, int y, int w, int h);
void getScissor(int &x, int &y, int &w, int &h);
void getViewport(int &x, int &y, int &w, int &h);
/**
 * @brief Given in screen coordinates
 * @note viewport() must have been called before
 */
bool scissor(int x, int y, int w, int h);
/**
 * @brief Enables a renderer state
 * @param state The State to change
 * @return The previous state value
 */
bool enable(State state);
bool currentState(State state);
/**
 * @brief Disables a renderer state
 * @param state The State to change
 * @return The previous state value
 */
bool disable(State state);
void colorMask(bool red, bool green, bool blue, bool alpha);
bool cullFace(Face face);
bool depthFunc(CompareFunc func);
CompareFunc getDepthFunc();
void getBlendState(bool &enabled, BlendMode &src, BlendMode &dest, BlendEquation &func);
bool blendFunc(BlendMode src, BlendMode dest);
bool blendEquation(BlendEquation func);
PolygonMode polygonMode(Face face, PolygonMode mode);
bool polygonOffset(const glm::vec2 &offset);
bool activateTextureUnit(TextureUnit unit);
Id currentTexture(TextureUnit unit);
bool bindTexture(TextureUnit unit, TextureType type, Id handle);
/**
 * @note The returned buffer should get freed with SDL_free
 */
bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels);
bool useProgram(Id handle);
Id getProgram();
bool bindVertexArray(Id handle);
Id boundVertexArray();
Id boundBuffer(BufferType type);
void unmapBuffer(Id handle, BufferType type);
bool bindBuffer(BufferType type, Id handle);
bool unbindBuffer(BufferType type);
bool bindBufferBase(BufferType type, Id handle, uint32_t index = 0u);
void genBuffers(uint8_t amount, Id *ids);
void deleteBuffers(uint8_t amount, Id *ids);
void genVertexArrays(uint8_t amount, Id *ids);
void deleteShader(Id &id);
Id genShader(ShaderType type);
void deleteProgram(Id &id);
Id genProgram();
void deleteVertexArrays(uint8_t amount, Id *ids);
void deleteVertexArray(Id &id);
void genTextures(const TextureConfig &cfg, uint8_t amount, Id *ids);
const core::Set<Id> &textures();
void deleteTextures(uint8_t amount, Id *ids);
Id currentFramebuffer();
void genFramebuffers(uint8_t amount, Id *ids);
void deleteFramebuffers(uint8_t amount, Id *ids);
/**
 * @note The returned buffer should get freed with SDL_free
 */
bool readFramebuffer(int x, int y, int w, int h, TextureFormat format, uint8_t **pixels);
void genRenderbuffers(uint8_t amount, Id *ids);
void deleteRenderbuffers(uint8_t amount, Id *ids);
void configureAttribute(const Attribute &a);
/**
 * Binds a new frame buffer
 * @param mode The FrameBufferMode to bind the frame buffer with
 * @param handle The Id that represents the handle of the frame buffer
 * @return The previously bound frame buffer Id
 */
Id bindFramebuffer(Id handle, FrameBufferMode mode = FrameBufferMode::Default);
void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height);
bool setupRenderBuffer(TextureFormat format, int w, int h, int samples);
Id bindRenderbuffer(Id handle);
void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size);
void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size);
const glm::vec4 &framebufferUV();
bool bindFrameBufferAttachment(Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear);
bool setupFramebuffer(const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)],
					  const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]);
void setupTexture(const TextureConfig &config);
void uploadTexture(video::TextureType type, video::TextureFormat format, int width, int height, const uint8_t *data,
				   int index, int samples);
void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset = nullptr);
void drawArrays(Primitive mode, size_t count);
void enableDebug(DebugSeverity severity);
bool compileShader(Id id, ShaderType shaderType, const core::String &source, const core::String &name = "unknown-shader");
bool linkShader(Id program, Id vert, Id frag, Id geom, const core::String &name = "unknown-shader");
bool linkComputeShader(Id program, Id comp, const core::String &name = "unknown-shader");
bool bindImage(Id handle, AccessMode mode, ImageFormat format);
/**
 * @brief Execute a compute shader
 */
bool runShader(Id program, const glm::uvec3 &workGroups, bool wait = false);
/**
 * @brief Fetch all uniforms in a shader
 */
int fetchUniforms(Id program, ShaderUniforms &uniforms, const core::String &name = "unknown-shader");
/**
 * @brief Fetch all attributes in a shader
 */
int fetchAttributes(Id program, ShaderAttributes &attributes, const core::String &name = "unknown-shader");
void flush();
void finish();

} // namespace video
