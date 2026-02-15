/**
 *@file
 *
 * @brief Interface that defines the renderer functions that need to be implemented by a specific rendering backend
 */

#pragma once

#include "RenderBuffer.h"
#include "ShaderTypes.h"
#include "core/SharedPtr.h"
#include "video/RendererState.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct SDL_Window;

namespace image {
class Image;
typedef core::SharedPtr<Image> ImagePtr;
} // namespace image

namespace video {

RendererState &rendererState();

class Texture;
typedef core::SharedPtr<Texture> TexturePtr;

class TextureConfig;

/**
 * @brief Perform global setup required before a renderer backend is initialized.
 *
 * This function prepares any global state the renderer implementation needs
 * (loading function pointers, creating shared resources, etc.). It must be
 * called once before calling @c init().
 */
void setup();

/**
 * Flags used with mapBufferRange to request specific mapping behavior. These
 * are backend-agnostic hints; the GL backend maps them to the corresponding
 * GL_MAP_* bits when available. Use bitwise-or to combine flags.
 */
enum class MapBufferFlag : uint8_t {
	None = 0,
	/** Invalidate the specified range (GL_MAP_INVALIDATE_RANGE_BIT) */
	InvalidateRange = 1u << 0,
	/** Don't synchronize with GPU (GL_MAP_UNSYNCHRONIZED_BIT) */
	Unsynchronized = 1u << 1,
	/** Use explicit flush (GL_MAP_FLUSH_EXPLICIT_BIT) when available */
	ExplicitFlush = 1u << 2,
};
CORE_ENUM_BIT_OPERATIONS(MapBufferFlag)

/**
 * Map a buffer range. This is a thin, backend-agnostic wrapper that allows
 * requesting non-blocking mapping hints. The meaning of @p flags is defined
 * by the MapBufferFlag enum above. Implementations should return nullptr if
 * mapping failed.
 *
 * @param handle Buffer handle.
 * @param type Buffer type (array/index/uniform/etc.).
 * @param offset Byte offset into the buffer.
 * @param length Number of bytes to map.
 * @param mode Access mode (Read/Write/ReadWrite).
 * @param flags MapBufferFlag bitset (default: MapBufferFlag_None).
 * @return Pointer to mapped memory or nullptr on failure.
 */
void *mapBufferRange(Id handle, BufferType type, intptr_t offset, size_t length, AccessMode mode,
					 MapBufferFlag flags = MapBufferFlag::None);
/**
 * @brief Initialize the renderer backend for a window of the given size.
 *
 * Allocates and configures renderer resources required for rendering to a
 * window of @p windowWidth x @p windowHeight and applies the device pixel
 * @p scaleFactor used for high-DPI displays.
 *
 * @param windowWidth Width of the window in logical pixels.
 * @param windowHeight Height of the window in logical pixels.
 * @param scaleFactor Device pixel scale factor (e.g. 2.0 for Retina displays).
 * @return @c true on success, @c false on failure.
 * @note Callers must call @c setup() before this function.
 */
bool init(int windowWidth, int windowHeight, float scaleFactor);

/**
 * @brief Destroy a renderer context and free its resources.
 *
 * Cleans up any GPU and backend resources associated with the provided
 * @p context. After this call the context becomes invalid.
 *
 * @param[in,out] context The renderer context to destroy.
 */
void destroyContext(RendererContext &context);

/**
 * @brief Create a renderer context for an SDL window.
 *
 * Creates and returns a platform/backend-specific renderer context that can
 * be activated for rendering to the provided @p window.
 *
 * @param window Pointer to the SDL_Window to create the context for.
 * @return A RendererContext value representing the created context.
 */
RendererContext createContext(SDL_Window *window);

/**
 * @brief Activate a previously created renderer context for the given window.
 *
 * Makes @p context the current backend context for subsequent rendering
 * operations targeting @p window. Implementations should handle context
 * switching or no-op if the context is already active.
 *
 * @param window The SDL window to bind the context to.
 * @param context The renderer context to activate.
 */
void activateContext(SDL_Window *window, RendererContext &context);

/**
 * @brief End the current frame for @p window.
 *
 * Completes rendering for the current frame and performs any presentation or
 * flush required by the backend (e.g. swap buffers).
 *
 * @param window The SDL window whose frame is being finished.
 */
void endFrame(SDL_Window *window);
/**
 * @brief Checks the error state since the last call to this function.
 *
 * Queries the backend for any pending error state (for GL this calls glGetError
 * repeatedly) and logs or asserts depending on @p triggerAssert. The function
 * returns true if any error was detected, false otherwise.
 *
 * @param[in] triggerAssert When true (and in DEBUG builds) an assert will be
 * triggered on the first detected error. When false the function will log
 * detected errors instead of asserting.
 * @return @c true if an error was found, @c false if no error was found.
 */
bool checkError(bool triggerAssert = true);
/**
 * @brief Select which color attachment to read from for pixel read operations.
 *
 * On GL this maps to glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType).
 *
 * @param textureType Which G-buffer texture attachment to select as the read buffer.
 */
void readBuffer(GBufferTextureType textureType);
/**
 * @brief Change the renderer line width.
 *
 * Sets the requested line width and returns the previously requested value.
 * Note: wide lines (>1.0) are deprecated in modern core-profile GL and on
 * OpenGL ES. On such contexts the call may be a no-op; the value returned by
 * @c currentLineWidth() is the cached requested width, which may differ from
 * what the driver actually uses.
 *
 * @param width The new line width.
 * @return The previously requested line width.
 */
float lineWidth(float width);

/**
 * @brief Clear one or more buffers (color, depth, stencil).
 *
 * Performs a clear of the currently bound framebuffer according to @p flag.
 * The implementation may temporarily disable scissor testing to optimize
 * full-screen clears as seen in the GL backend.
 *
 * @param flag Bitfield of ClearFlag values to clear.
 */
void clear(ClearFlag flag);

/**
 * @brief Bind a texture handle to a texture unit and target.
 *
 * Uses DSA where available; otherwise activates the unit and binds the
 * texture to the appropriate target. Returns @c true on an actual change.
 */
bool bindTexture(TextureUnit unit, TextureType type, Id handle);

/**
 * @note The returned buffer should be freed with core_free/SDL_free by the caller.
 *
 * Reads pixel data from a texture handle into a newly allocated buffer.
 * The format and packing are chosen according to @p format and the width/height.
 *
 * @param unit Texture unit to use for non-DSA backends.
 * @param type Texture target type (2D, 3D, cube, ...).
 * @param format Pixel format to request for the read back.
 * @param handle Texture handle to read from.
 * @param w Width of the texture region to read.
 * @param h Height of the texture region to read.
 * @param[out] pixels Receives a pointer to the newly allocated pixel buffer on success.
 * @return @c true on success, @c false on failure (buffer is freed on failure).
 */
bool readTexture(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels);

/**
 * @brief Bind a vertex array object.
 *
 * Returns @c true if the bound VAO actually changed.
 */
bool bindVertexArray(Id handle);

/**
 * @brief Unmap a previously mapped buffer.
 *
 * @param handle Buffer handle returned from mapBuffer().
 * @param type The buffer binding type used for mapping.
 */
void unmapBuffer(Id handle, BufferType type);

/**
 * @brief Map a buffer object's data store into client memory for CPU access.
 *
 * Returns a pointer to the mapped memory region or nullptr on failure. The
 * pointer is valid until @c unmapBuffer() is called for the same @p handle.
 * Implementations may use direct-state-access (DSA) APIs where available.
 *
 * @param handle Buffer object id to map.
 * @param type Binding type used for mapping (Array, Uniform, etc.).
 * @param mode Access mode (Read, Write, ReadWrite).
 * @return Pointer to mapped memory or nullptr on failure.
 */
void *mapBuffer(Id handle, BufferType type, AccessMode mode);

/**
 * @brief Bind a buffer object for the specified buffer type.
 *
 * Returns @c true if the binding changed.
 */
bool bindBuffer(BufferType type, Id handle);

/**
 * @brief Unbind the buffer currently bound to @p type (bind InvalidId).
 *
 * Returns @c true if something was unbound.
 */
bool unbindBuffer(BufferType type);

/**
 * @brief Bind a buffer to an indexed binding point (e.g., UBO/SSBO binding).
 *
 * @param index Indexed binding point to bind to (default 0).
 * @return @c true if the binding changed.
 */
bool bindBufferBase(BufferType type, Id handle, uint32_t index = 0u);

/**
 * @brief Generate @p amount buffer object names and write them into @p ids.
 */
void genBuffers(uint8_t amount, Id *ids);

/**
 * @brief Delete @p amount buffers and mark ids as InvalidId.
 */
void deleteBuffers(uint8_t amount, Id *ids);

/**
 * @brief Generate @p amount vertex array objects and write them into @p ids.
 */
void genVertexArrays(uint8_t amount, Id *ids);

/**
 * @brief Delete a shader object and set @p id to InvalidId.
 */
void deleteShader(Id &id);

/**
 * @brief Create and return a new shader object for the given shader type.
 *
 * Returns InvalidId if shader creation is not available.
 */
Id genShader(ShaderType type);

/**
 * @brief Delete a shader program and set @p id to InvalidId.
 */
void deleteProgram(Id &id);

/**
 * @brief Create and return a new shader program object.
 */
Id genProgram();

/**
 * @brief Delete @p amount vertex arrays and invalidate the ids.
 */
void deleteVertexArrays(uint8_t amount, Id *ids);

/**
 * @brief Create @p amount textures appropriate for @p cfg and write them to @p ids.
 */
void genTextures(const TextureConfig &cfg, uint8_t amount, Id *ids);

/**
 * @brief Delete @p amount textures and invalidate the ids.
 */
void deleteTextures(uint8_t amount, Id *ids);

/**
 * @brief Generate framebuffer object names and store them into @p ids.
 */
void genFramebuffers(uint8_t amount, Id *ids);

/**
 * @brief Delete framebuffer objects and invalidate the provided ids.
 */
void deleteFramebuffers(uint8_t amount, Id *ids);

/**
 * @brief Generate renderbuffer object names and store them into @p ids.
 */
void genRenderbuffers(uint8_t amount, Id *ids);

/**
 * @brief Delete renderbuffer objects and invalidate the provided ids.
 */
void deleteRenderbuffers(uint8_t amount, Id *ids);

/**
 * @brief Configure a vertex attribute on the currently bound VAO/program.
 *
 * Enables the attribute, sets pointer/format and divisor as required.
 */
void configureAttribute(const Attribute &a);
/**
 * Binds a new frame buffer
 * @param mode The FrameBufferMode to bind the frame buffer with
 * @param handle The Id that represents the handle of the frame buffer
 * @return The previously bound frame buffer Id
 */
Id bindFramebuffer(Id handle, FrameBufferMode mode = FrameBufferMode::Default);
/**
 * @brief Blit (copy) from one framebuffer to another.
 *
 * Copies the specified buffers (color/depth/stencil as indicated by @p flag)
 * from @p handle to @p target with a full-size rectangle of @p width x @p height.
 * The backend may choose an appropriate filter for color blits.
 */
void blitFramebuffer(Id handle, Id target, ClearFlag flag, int width, int height);

/**
 * @brief Allocate and configure storage for a renderbuffer object.
 *
 * Typically used for depth/stencil or multisample attachments.
 *
 * @param rbo Renderbuffer id to configure (must be bound or DSA-capable).
 * @param format TextureFormat representing the internal format.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @param samples Number of samples for multisampling (0 for single-sampled).
 * @return @c true on success, @c false on failure.
 */
bool setupRenderBuffer(Id rbo, TextureFormat format, int w, int h, int samples);

/**
 * @brief Bind a renderbuffer object for subsequent renderbuffer operations.
 *
 * Returns the previously bound renderbuffer id.
 */
Id bindRenderbuffer(Id handle);

/**
 * @brief Upload or allocate buffer storage for @p handle.
 *
 * If @p data is null the backend will allocate storage of @p size. For
 * uniform buffers, some implementations may enforce a maximum size check.
 *
 * @param handle Buffer object id.
 * @param type The buffer binding type (Array, ElementArray, Uniform, etc.).
 * @param mode Usage hint (StaticDraw, DynamicDraw, StreamDraw, ...).
 * @param data Pointer to source data or nullptr to reserve uninitialized storage.
 * @param size Size in bytes of the data/storage.
 */
void bufferData(Id handle, BufferType type, BufferMode mode, const void *data, size_t size);

/**
 * @brief Update a sub-range of an existing buffer object's data store.
 *
 * @param handle Buffer object id.
 * @param type Buffer binding type.
 * @param offset Byte offset into the buffer to update.
 * @param data Pointer to source data.
 * @param size Number of bytes to update.
 */
void bufferSubData(Id handle, BufferType type, intptr_t offset, const void *data, size_t size);

/**
 * @brief Get the UV transform to map framebuffer coordinates to texture UVs.
 *
 * Some backends store framebuffer attachments flipped vertically; this
 * function returns a vec4 {u0, v0, u1, v1} suitable for sampling.
 */
const glm::vec4 &framebufferUV();

/**
 * @brief Attach a texture to a framebuffer's attachment point.
 *
 * Handles layered attachments and can optionally clear the attached buffer
 * after binding.
 *
 * @param fbo Framebuffer id to modify.
 * @param texture Texture id to attach.
 * @param attachment Attachment enum (color/depth/stencil/...).
 * @param layerIndex Layer or array slice index for 3D/array textures.
 * @param clear If true, clear the newly attached buffer according to its type.
 * @return @c true if the framebuffer is complete after the attachment.
 */
bool bindFrameBufferAttachment(Id fbo, Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear);

/**
 * @brief Configure a framebuffer with the provided color textures and renderbuffers.
 *
 * Attaches textures and renderbuffers as appropriate, sets draw buffers and
 * verifies framebuffer completeness.
 *
 * @return @c true if the framebuffer is complete and usable.
 */
bool setupFramebuffer(Id fbo, const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)],
					  const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]);

/**
 * @brief Apply texture parameter configuration to a texture object.
 *
 * Uses sampler/texture parameter settings from @p config (filtering, wrap,
 * compare mode, border color, alignment, etc.).
 */
void setupTexture(Id texture, const TextureConfig &config);

/**
 * @brief Upload pixel data into a texture object and allocate storage if needed.
 *
 * Supports 1D/2D/3D, multisample and array textures depending on @p type and
 * @p samples/@p index. The backend may use immutable storage APIs where
 * available.
 *
 * @param texture Texture id to upload into.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param data Optional pointer to pixel data (may be null to allocate storage).
 * @param index Layer/depth/array index for 3D/array textures.
 * @param cfg TextureConfig describing the texture format and type.
 */
void uploadTexture(Id texture, int width, int height, const uint8_t *data, int index, const TextureConfig &cfg);

/**
 * @brief Draw indexed geometry from the currently bound VAO/IBO.
 *
 * Issues an indexed draw call using the currently bound program and VAO.
 * @p offset is interpreted as a pointer/offset into the index buffer.
 */
void drawElements(Primitive mode, size_t numIndices, DataType type, void *offset = nullptr);

/**
 * @brief Draw non-indexed primitives.
 */
void drawArrays(Primitive mode, size_t count);

/**
 * @brief Enable backend debug output at the requested severity.
 *
 * If the backend exposes debug output (e.g., GL KHR_debug) this will enable
 * message callbacks and control which severities are reported.
 */
void enableDebug(DebugSeverity severity);

/**
 * @brief Compile GLSL/HLSL shader source for the given shader Id.
 *
 * The function uploads source to the shader object, compiles and checks the
 * compile status. On failure the GL backend may delete the shader object.
 *
 * @param id Shader object id (must be valid).
 * @param shaderType Type of shader (vertex/fragment/geometry/compute).
 * @param source Shader source code.
 * @param name Optional human-readable name for logging.
 * @return @c true if compilation succeeded, @c false otherwise.
 */
bool compileShader(Id id, ShaderType shaderType, const core::String &source, const core::String &name = "unknown-shader");

/**
 * @brief Link a shader program from attached shader objects.
 *
 * Attaches the provided shader objects, links the program and detaches the
 * shaders. On failure the backend will delete the program and return false.
 *
 * @param program Program id to link.
 * @param vert Vertex shader id.
 * @param frag Fragment shader id.
 * @param geom Optional geometry shader id or InvalidId.
 * @param name Human readable program name for logging.
 * @return @c true on successful link, @c false otherwise.
 */
bool linkShader(Id program, Id vert, Id frag, Id geom, const core::String &name = "unknown-shader");

/**
 * @brief Link a compute shader program.
 *
 * Similar to @c linkShader but for compute-only programs. On failure the
 * program will be deleted by the backend.
 */
bool linkComputeShader(Id program, Id comp, const core::String &name = "unknown-shader");

/**
 * @brief Bind a texture as an image unit for read/write access in shaders.
 *
 * @param handle Texture handle to bind as image unit 0.
 * @param mode AccessMode (Read, Write, ReadWrite).
 * @param format ImageFormat describing the view format.
 * @return @c true if the binding changed, @c false if it was already set.
 */
bool bindImage(Id handle, AccessMode mode, ImageFormat format);

/**
 * @brief Assign a human readable debug name to an object (if supported).
 *
 * The GL backend's implementation is currently disabled (it caused GL errors
 * on some drivers), but callers may use this function where supported to
 * attach object labels for external debuggers/profilers.
 */
void setObjectName(Id handle, ObjectNameType type, const core::String &name);

/**
 * @brief Execute a compute shader.
 *
 * Dispatches the compute shader described by @p program using the given
 * work group counts. The function returns true if the dispatch was issued
 * successfully (after validation and issuing the dispatch), false on
 * validation failure.
 */
bool runShader(Id program, const glm::uvec3 &workGroups, MemoryBarrierType wait = MemoryBarrierType::None);
/**
 * @brief Wait for the execution of a compute shader
 */
void waitShader(MemoryBarrierType wait);
/**
 * @brief Fetch all uniforms in a shader
 */
int fetchUniforms(Id program, ShaderUniforms &uniforms, const core::String &name = "unknown-shader");
/**
 * @brief Fetch all attributes in a shader
 */
int fetchAttributes(Id program, ShaderAttributes &attributes, const core::String &name = "unknown-shader");

/**
 * @brief Flush queued rendering commands to the GPU driver.
 *
 * Ensures that previously issued commands are submitted to the driver for
 * execution but does not block for completion. Useful for lowering latency
 * between CPU and GPU when required by the application.
 */
void flush();

/**
 * @brief Flush all pending renderer state changes to the backend.
 *
 * Applies any pending state (blend, depth, scissor, textures, etc.) to the
 * backend immediately, without requiring a draw call. This is used at frame
 * boundaries to ensure state changes queued by Scoped* destructors after the
 * last draw call are actually applied to the hardware.
 */
void syncPendingState();

/**
 * @brief Block until all previously submitted rendering commands have finished.
 *
 * This is a heavy synchronization point and should be used sparingly.
 * Typically implemented with glFinish() on OpenGL backends.
 */
void finish();

// shader stuff
void setUniformBufferBinding(Id program, uint32_t blockIndex, uint32_t blockBinding);
// only for validation purposes
int32_t getUniformBufferOffset(Id program, const char *name);

} // namespace video
