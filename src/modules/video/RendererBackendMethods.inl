#include "math/Vec4f.h"
#include "math/Vec3u.h"
// Included with RENDERER_METHOD and RENDERER_DEFAULT defined.
RENDERER_METHOD(RendererState &, rendererState, (), ())
RENDERER_METHOD(void *, mapBufferRange,
	(Id handle, BufferType type, intptr_t offset, size_t length, AccessMode mode, MapBufferFlag flags RENDERER_DEFAULT(MapBufferFlag::None)),
	(handle, type, offset, length, mode, flags))
RENDERER_METHOD(bool, init,
	(int windowWidth, int windowHeight, float scaleFactor),
	(windowWidth, windowHeight, scaleFactor))
RENDERER_METHOD(void, destroyContext, (RendererContext &context), (context))
RENDERER_METHOD(RendererContext, createContext, (SDL_Window *window), (window))
RENDERER_METHOD(void, activateContext, (SDL_Window *window, RendererContext &context), (window, context))
RENDERER_METHOD(void, endFrame, (SDL_Window *window), (window))
RENDERER_METHOD(void, waitDeviceIdle, (), ())
RENDERER_METHOD(Fence, createFence, (), ())
RENDERER_METHOD(bool, fenceSignaled, (Fence fence), (fence))
RENDERER_METHOD(void, deleteFence, (Fence &fence), (fence))
RENDERER_METHOD(bool, checkError, (bool triggerAssert RENDERER_DEFAULT(true)), (triggerAssert))
RENDERER_METHOD(void, readBuffer, (GBufferTextureType textureType), (textureType))
RENDERER_METHOD(float, lineWidth, (float width), (width))
RENDERER_METHOD(void, clear, (ClearFlag flag), (flag))
RENDERER_METHOD(bool, bindTexture, (TextureUnit unit, TextureType type, Id handle), (unit, type, handle))
RENDERER_METHOD(bool, readTexture,
	(TextureUnit unit, TextureType type, TextureFormat format, Id handle, int w, int h, uint8_t **pixels),
	(unit, type, format, handle, w, h, pixels))
RENDERER_METHOD(bool, readFramebuffer, (int x, int y, int w, int h, uint8_t **pixels), (x, y, w, h, pixels))
RENDERER_METHOD(bool, bindVertexArray, (Id handle), (handle))
RENDERER_METHOD(bool, unmapBuffer, (Id handle, BufferType type), (handle, type))
RENDERER_METHOD(void *, mapBuffer, (Id handle, BufferType type, AccessMode mode), (handle, type, mode))
RENDERER_METHOD(bool, bindBuffer, (BufferType type, Id handle), (type, handle))
RENDERER_METHOD(bool, unbindBuffer, (BufferType type), (type))
RENDERER_METHOD(bool, bindBufferBase,
	(BufferType type, Id handle, uint32_t index RENDERER_DEFAULT(0u)),
	(type, handle, index))
RENDERER_METHOD(void, genBuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, deleteBuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, genVertexArrays, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, deleteShader, (Id &id), (id))
RENDERER_METHOD(Id, genShader, (ShaderType type), (type))
RENDERER_METHOD(void, deleteProgram, (Id &id), (id))
RENDERER_METHOD(Id, genProgram, (), ())
RENDERER_METHOD(void, deleteVertexArrays, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, genTextures, (const TextureConfig &cfg, uint8_t amount, Id *ids), (cfg, amount, ids))
RENDERER_METHOD(void, deleteTextures, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, genFramebuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, deleteFramebuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, genRenderbuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, deleteRenderbuffers, (uint8_t amount, Id *ids), (amount, ids))
RENDERER_METHOD(void, configureAttribute, (const Attribute &a), (a))
RENDERER_METHOD(Id, bindFramebuffer,
	(Id handle, FrameBufferMode mode RENDERER_DEFAULT(FrameBufferMode::Default)),
	(handle, mode))
RENDERER_METHOD(void, blitFramebuffer,
	(Id handle, Id target, ClearFlag flag, int width, int height),
	(handle, target, flag, width, height))
RENDERER_METHOD(void, blitFramebuffer,
	(Id handle, Id target, FrameBufferAttachment attachment, int width, int height),
	(handle, target, attachment, width, height))
RENDERER_METHOD(void, drawBuffers, (uint8_t n, const FrameBufferAttachment *attachments), (n, attachments))
RENDERER_METHOD(bool, setupRenderBuffer,
	(Id rbo, TextureFormat format, int w, int h, int samples),
	(rbo, format, w, h, samples))
RENDERER_METHOD(Id, bindRenderbuffer, (Id handle), (handle))
RENDERER_METHOD(void, bufferData,
	(Id handle, BufferType type, BufferMode mode, const void *data, size_t size),
	(handle, type, mode, data, size))
RENDERER_METHOD(void, bufferSubData,
	(Id handle, BufferType type, intptr_t offset, const void *data, size_t size),
	(handle, type, offset, data, size))
RENDERER_METHOD(bool, bufferStorage,
	(Id handle, BufferType type, size_t size, BufferStorageFlag flags),
	(handle, type, size, flags))
RENDERER_METHOD(bool, copyBufferSubData,
	(Id readBuffer, intptr_t readOffset, Id writeBuffer, intptr_t writeOffset, size_t size),
	(readBuffer, readOffset, writeBuffer, writeOffset, size))
RENDERER_METHOD(const math::Vec4f &, framebufferUV, (), ())
RENDERER_METHOD(bool, bindFrameBufferAttachment,
	(Id fbo, Id texture, FrameBufferAttachment attachment, int layerIndex, bool clear),
	(fbo, texture, attachment, layerIndex, clear))
RENDERER_METHOD(bool, setupFramebuffer,
	(Id fbo, const TexturePtr (&colorTextures)[core::enumVal(FrameBufferAttachment::Max)], const RenderBufferPtr (&bufferAttachments)[core::enumVal(FrameBufferAttachment::Max)]),
	(fbo, colorTextures, bufferAttachments))
RENDERER_METHOD(void, setupTexture, (Id texture, const TextureConfig &config), (texture, config))
RENDERER_METHOD(void, uploadTexture,
	(Id texture, int width, int height, const uint8_t *data, int index, const TextureConfig &cfg),
	(texture, width, height, data, index, cfg))
RENDERER_METHOD(void, drawElements,
	(Primitive mode, size_t numIndices, DataType type, void *offset RENDERER_DEFAULT(nullptr)),
	(mode, numIndices, type, offset))
RENDERER_METHOD(void, drawElementsInstanced,
	(Primitive mode, size_t numIndices, DataType type, size_t instances, void *offset RENDERER_DEFAULT(nullptr)),
	(mode, numIndices, type, instances, offset))
RENDERER_METHOD(void, multiDrawElementsIndirect,
	(Primitive mode, DataType type, const void *indirect, int drawCount, int stride RENDERER_DEFAULT(0)),
	(mode, type, indirect, drawCount, stride))
RENDERER_METHOD(void, drawArrays, (Primitive mode, size_t count), (mode, count))
RENDERER_METHOD(void, enableDebug, (DebugSeverity severity), (severity))
RENDERER_METHOD(bool, compileShader,
	(Id id, ShaderType shaderType, const core::String &source, const core::String &name RENDERER_DEFAULT("unknown-shader")),
	(id, shaderType, source, name))
RENDERER_METHOD(bool, loadShaderSPIRV,
	(Id id, ShaderType shaderType, const uint8_t *spirv, size_t spirvSize, const core::String &name RENDERER_DEFAULT("unknown-shader")),
	(id, shaderType, spirv, spirvSize, name))
RENDERER_METHOD(bool, linkShader,
	(Id program, Id vert, Id frag, Id geom, const core::String &name RENDERER_DEFAULT("unknown-shader")),
	(program, vert, frag, geom, name))
RENDERER_METHOD(void, registerShaderBindings,
	(Id program, const ShaderResourceBinding *bindings, int count),
	(program, bindings, count))
RENDERER_METHOD(void, setProgramPushConstantSize, (Id program, uint32_t size), (program, size))
RENDERER_METHOD(void, setUniformBufferPushOverlay,
	(Id buffer, const UniformPushOverlayField *fields, int count),
	(buffer, fields, count))
RENDERER_METHOD(bool, linkComputeShader,
	(Id program, Id comp, const core::String &name RENDERER_DEFAULT("unknown-shader")),
	(program, comp, name))
RENDERER_METHOD(bool, bindImage, (Id handle, AccessMode mode, ImageFormat format), (handle, mode, format))
RENDERER_METHOD(void, setObjectName, (Id handle, ObjectNameType type, const core::String &name), (handle, type, name))
RENDERER_METHOD(bool, runShader,
	(Id program, const math::Vec3u &workGroups, MemoryBarrierType wait RENDERER_DEFAULT(MemoryBarrierType::None)),
	(program, workGroups, wait))
RENDERER_METHOD(bool, runShaderIndirect,
	(Id program, Id buffer, size_t offset, MemoryBarrierType wait RENDERER_DEFAULT(MemoryBarrierType::None)),
	(program, buffer, offset, wait))
RENDERER_METHOD(void, waitShader, (MemoryBarrierType wait), (wait))
RENDERER_METHOD(void, flush, (), ())
RENDERER_METHOD(void, syncPendingState, (), ())
RENDERER_METHOD(void, finish, (), ())
RENDERER_METHOD(Id, createTimerQuery, (), ())
RENDERER_METHOD(void, deleteTimerQuery, (Id &query), (query))
RENDERER_METHOD(void, beginTimerQuery, (Id query), (query))
RENDERER_METHOD(void, endTimerQuery, (), ())
RENDERER_METHOD(bool, timerQueryAvailable, (Id query), (query))
RENDERER_METHOD(uint64_t, timerQueryResult, (Id query), (query))
RENDERER_METHOD(Id, createAABBAccelerationStructure,
	(const AccelerationStructureAABB *aabbs, uint32_t count),
	(aabbs, count))
RENDERER_METHOD(void, deleteAccelerationStructure, (Id &handle), (handle))
RENDERER_METHOD(bool, bindAccelerationStructure, (Id handle, uint32_t binding RENDERER_DEFAULT(0u)), (handle, binding))
RENDERER_METHOD(void, traceVideoBegin, (const char *name), (name))
RENDERER_METHOD(void, traceVideoEnd, (), ())
