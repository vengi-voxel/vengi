/**
 * @file
 *
 * @ingroup Compute
 */
#include "compute/cl/CLCompute.h"
#include "video/Buffer.h"
#include "video/gl/GLRenderer.h"
#include "video/Texture.h"
#include <CL/cl_gl.h>
#include <SDL.h>
#include <SDL_platform.h>

namespace computevideo {

bool init() {
	if (compute::_priv::_ctx.context) {
		Log::error("Init must happen before a context is created");
		return false;
	}
#if defined(__APPLE__)
	compute::_priv::_ctx.externalProperties.push_back(CL_CGL_SHAREGROUP_KHR);
	compute::_priv::_ctx.externalProperties.push_back((cl_context_properties)CGLGetShareGroup(CGLGetCurrentContext()));
#else
	SDL_GLContext glCtx = SDL_GL_GetCurrentContext();
	if (glCtx == nullptr) {
		return false;
	}
	compute::_priv::_ctx.externalProperties.push_back(CL_GL_CONTEXT_KHR);
	compute::_priv::_ctx.externalProperties.push_back((cl_context_properties)glCtx);
#ifdef __WINDOWS__
	intptr_t (*drawableFunc) (void) = (intptr_t(*)(void)) SDL_GL_GetProcAddress("wglGetCurrentDC");
	if (drawableFunc == nullptr) {
		return false;
	}
	compute::_priv::_ctx.externalProperties.push_back(CL_WGL_HDC_KHR);
	compute::_priv::_ctx.externalProperties.push_back((cl_context_properties)drawableFunc());
#endif
#ifdef __LINUX__
	intptr_t (*drawableFunc) (void) = (intptr_t(*)(void)) SDL_GL_GetProcAddress("glXGetCurrentDisplay");
	if (drawableFunc == nullptr) {
		return false;
	}
	compute::_priv::_ctx.externalProperties.push_back(CL_GLX_DISPLAY_KHR);
	compute::_priv::_ctx.externalProperties.push_back((cl_context_properties)drawableFunc());
#endif
#endif
	compute::_priv::_ctx.useGL = true;
	return true;
}

void shutdown() {
}

/**
 * @brief Creates an OpenCL buffer object from an OpenGL buffer object.
 *
 * @p Parameters
 *
 * @param context A valid OpenCL context created from an OpenGL context.
 * @param flags A bit-field that is used to specify usage information. Refer to the table for clCreateBuffer for a description of flags. Only CL_MEM_READ_ONLY, CL_MEM_WRITE_ONLY and CL_MEM_READ_WRITE values specified in the table at clCreateBuffer can be used.
 * @param bufobj The name of a GL buffer object. The data store of the GL buffer object must have have been previously created by calling OpenGL function glBufferData, although its contents need not be initialized. The size of the data store will be used to determine the size of the CL buffer object.
 * @param errcode_ret Returns an appropriate error code as described below. If errcode_ret is NULL, no error code is returned.
 *
 * @p Description
 *
 * The size of the GL buffer object data store at the time @c clCreateFromGLBuffer is called will be used as the size of buffer object returned by @c clCreateFromGLBuffer. If the
 * state of a GL buffer object is modified through the GL API (e.g. @c glBufferData) while there exists a corresponding CL buffer object, subsequent use of the CL buffer object
 * will result in undefined behavior.
 *
 * The @c clRetainMemObject and @c clReleaseMemObject functions can be used to retain and release the buffer object.
 * The CL buffer object created using @c clCreateFromGLBuffer can also be used to create a CL 1D image buffer object.
 *
 * @p Notes
 *
 * General information about GL sharing follows.
 *
 * The OpenCL specification in section 9.7 defines how to share data with texture and buffer objects in a parallel OpenGL implementation, but does not define how the association
 * between an OpenCL context and an OpenGL context or share group is established. This extension defines optional attributes to OpenCL context creation routines which associate
 * a GL context or share group object with a newly created OpenCL context. If this extension is supported by an implementation, the string "@c cl_khr_gl_sharing" will be present
 * in the @c CL_DEVICE_EXTENSIONS string described in the table of allowed values for param_name for @c clGetDeviceInfo or in the @c CL_PLATFORM_EXTENSIONS string described in the
 * table of allowed values for param_name for @c clGetPlatformInfo.
 *
 * This section discusses OpenCL functions that allow applications to use OpenGL buffer, texture, and renderbuffer objects as OpenCL memory objects. This allows efficient sharing
 * of data between OpenCL and OpenGL. The OpenCL API may be used to execute kernels that read and/or write memory objects that are also OpenGL objects.
 *
 * An OpenCL image object may be created from an OpenGL texture or renderbuffer object. An OpenCL buffer object may be created from an OpenGL buffer object.
 *
 * Any supported OpenGL object defined within the GL share group object, or the share group associated with the GL context from which the CL context is created, may be shared, with
 * the exception of the default OpenGL objects (i.e. objects named zero), which may not be shared.
 *
 * @p OpenGL and Corresponding OpenCL Image Formats
 *
 * The table below (Table 9.4) describes the list of GL texture internal formats and the corresponding CL image formats. If a GL texture object with an internal format from the
 * table below is successfully created by OpenGL, then there is guaranteed to be a mapping to one of the corresponding CL image format(s) in that table. Texture objects created
 * with other OpenGL internal formats may (but are not guaranteed to) have a mapping to a CL image format; if such mappings exist, they are guaranteed to preserve all color
 * components, data types, and at least the number of bits/component actually allocated by OpenGL for that format.
 * GL internal format 	CL image format (channel order, channel data type)
 * GL_RGBA8 	CL_RGBA, CL_UNORM_INT8 or CL_BGRA, CL_UNORM_INT8
 * GL_SRGBA8_ALPHA8 	CL_sRGBA, CL_UNORM_INT8
 * GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV 	CL_RGBA, CL_UNORM_INT8
 * GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV 	CL_BGRA, CL_UNORM_INT8
 * GL_RGBA8I, GL_RGBA8I_EXT 	CL_RGBA, CL_SIGNED_INT8
 * GL_RGBA16I, GL_RGBA16I_EXT 	CL_RGBA, CL_SIGNED_INT16
 * GL_RGBA32I, GL_RGBA32I_EXT 	CL_RGBA, CL_SIGNED_INT32
 * GL_RGBA8UI, GL_RGBA8UI_EXT 	CL_RGBA, CL_UNSIGNED_INT8
 * GL_RGBA16UI, GL_RGBA16UI_EXT 	CL_RGBA, CL_UNSIGNED_INT16
 * GL_RGBA32UI, GL_RGBA32UI_EXT 	CL_RGBA, CL_UNSIGNED_INT32
 * GL_RGBA8_SNORM 	CL_RGBA, CL_SNORM_INT8
 * GL_RGBA16 	CL_RGBA, CL_UNORM_INT16
 * GL_RGBA16_SNORM 	CL_RGBA, CL_SNORM_INT166
 * GL_RGBA16F, GL_RGBA16F_ARB 	CL_RGBA, CL_HALF_FLOAT
 * GL_RGBA32F, GL_RGBA32F_ARB 	CL_RGBA, CL_FLOAT
 * GL_R8 	CL_R, CL_UNORM_INT8
 * GL_R8_SNORM 	CL_R, CL_SNORM_INT8
 * GL_R16 	CL_R, CL_UNORM_INT16
 * GL_R16_SNORM 	CL_R, CL_SNORM_INT16
 * GL_R16F 	CL_R, CL_HALF_FLOAT
 * GL_R32F 	CL_R, CL_FLOAT
 * GL_R8I 	CL_R, CL_SIGNED_INT8
 * GL_R16I 	CL_R, CL_SIGNED_INT16
 * GL_R32I 	CL_R, CL_SIGNED_INT32
 * GL_R8UI 	CL_R, CL_UNSIGNED_INT8
 * GL_R16UI 	CL_R, CL_UNSIGNED_INT16
 * GL_R32UI 	CL_R, CL_UNSIGNED_INT32
 * GL_RG8 	CL_RG, CL_UNORM_INT8
 * GL_RG8_SNORM 	CL_RG, CL_SNORM_INT8
 * GL_RG16 	CL_RG, CL_UNORM_INT16
 * GL_RG16_SNORM 	CL_RG, CL_SNORM_INT16
 * GL_RG16F 	CL_RG, CL_HALF_FLOAT
 * GL_RG32F 	CL_RG, CL_FLOAT
 * GL_RG8I 	CL_RG, CL_SIGNED_INT8
 * GL_RG16I 	CL_RG, CL_SIGNED_INT16
 * GL_RG32I 	CL_RG, CL_SIGNED_INT32
 * GL_RG8UI 	CL_RG, CL_UNSIGNED_INT8
 * GL_RG16UI 	CL_RG, CL_UNSIGNED_INT16
 * GL_RG32UI 	CL_RG, CL_UNSIGNED_INT32
 *
 * If the @c cl_khr_gl_depth_images extension is enabled, the following new image formats are added to table 9.4 in section 9.6.3.1 of the OpenCL 2.0 extension specification. If
 * a GL texture object with an internal format from table 9.4 is successfully created by OpenGL, then there is guaranteed to be a mapping to one of the corresponding CL image
 * format(s) in that table.
 * GL internal format 	CL image format (channel order, channel data type)
 * GL_DEPTH_COMPONENT32F 	CL_DEPTH, CL_FLOAT
 * GL_DEPTH_COMPONENT16 	CL_DEPTH, CL_UNORM_INT16
 * GL_DEPTH24_STENCIL8 	CL_DEPTH_STENCIL, CL_UNORM_INT24
 * GL_DEPTH32F_STENCIL8 	CL_DEPTH_STENCIL, CL_FLOAT
 * Lifetime of [GL] Shared Objects
 *
 * An OpenCL memory object created from an OpenGL object (hereinafter refered to as a "shared CL/GL object") remains valid as long as the corresponding GL object has not been
 * deleted. If the GL object is deleted through the GL API (e.g. @c glDeleteBuffers, @c glDeleteTextures, or @c glDeleteRenderbuffers), subsequent use of the CL buffer or image
 * object will result in undefined behavior, including but not limited to possible CL errors and data corruption, but may not result in program termination.
 *
 * The CL context and corresponding command-queues are dependent on the existence of the GL share group object, or the share group associated with the GL context from which the
 * CL context is created. If the GL share group object or all GL contexts in the share group are destroyed, any use of the CL context or command-queue(s) will result in undefined
 * behavior, which may include program termination. Applications should destroy the CL command-queue(s) and CL context before destroying the corresponding GL share group or
 * contexts.
 *
 * @p Synchronizing OpenCL and OpenGL Access
 *
 * In order to ensure data integrity, the application is responsible for synchronizing access to shared CL/GL objects by their respective APIs. Failure to provide such
 * synchronization may result in race conditions and other undefined behavior including non-portability between implementations.
 *
 * Prior to calling @c clEnqueueAcquireGLObjects, the application must ensure that any pending GL operations which access the objects specified in mem_objects have completed.
 * This may be accomplished portably by issuing and waiting for completion of a @c glFinish command on all GL contexts with pending references to these objects. Implementations
 * may offer more efficient synchronization methods; for example on some platforms calling @c glFlush may be sufficient, or synchronization may be implicit within a thread, or
 * there may be vendor-specific extensions that enable placing a fence in the GL command stream and waiting for completion of that fence in the CL command queue. Note that no
 * synchronization methods other than @c glFinish are portable between OpenGL implementations at this time.
 *
 * When the extension cl_khr_egl_event is supported: Prior to calling clEnqueueAcquireGLObjects, the application must ensure that any pending EGL or EGL client API operations which
 * access the objects specified in mem_objects have completed. If the cl_khr_egl_event extension is supported and the EGL context in question supports fence sync objects, explicit
 * synchronisation can be achieved as set out in section 5.7.1. If the cl_khr_egl_event extension is not supported, completion of EGL client API commands may be determined by
 * issuing and waiting for completion of commands such as glFinish or vgFinish on all client API contexts with pending references to these objects. Some implementations may offer
 * other efficient synchronization methods. If such methods exist they will be described in platform-specific documentation. Note that no synchronization methods other than
 * @c glFinish and @c vgFinish are portable between all EGL client API implementations and all OpenCL implementations. While this is the only way to ensure completion that is
 * portable to all platforms, these are expensive operation and their use should be avoided if the @c cl_khr_egl_event extension is supported on a platform.
 *
 * Similarly, after calling clEnqueueReleaseGLObjects, the application is responsible for ensuring that any pending OpenCL operations which access the objects specified in
 * mem_objects have completed prior to executing subsequent GL commands which reference these objects. This may be accomplished portably by calling @c clWaitForEvents with the
 * event object returned by @c clEnqueueReleaseGLObjects, or by calling @c clFinish. As above, some implementations may offer more efficient methods.
 *
 * The application is responsible for maintaining the proper order of operations if the CL and GL contexts are in separate threads.
 *
 * If a GL context is bound to a thread other than the one in which @c clEnqueueReleaseGLObjects is called, changes to any of the objects in mem_objects may not be visible
 * to that context without additional steps being taken by the application. For an OpenGL 3.1 (or later) context, the requirements are described in Appendix D ("Shared Objects
 * and Multiple Contexts") of the OpenGL 3.1 Specification. For prior versions of OpenGL, the requirements are implementation-dependent.
 *
 * Attempting to access the data store of an OpenGL object after it has been acquired by OpenCL and before it has been released will result in undefined behavior. Similarly,
 * attempting to access a shared CL/GL object from OpenCL before it has been acquired by the OpenCL command queue, or after it has been released, will result in undefined
 * behavior.
 *
 * If the @c cl_khr_gl_event extension is supported, then the OpenCL implementation will ensure that any such pending OpenGL operations are complete for an OpenGL context bound
 * to the same thread as the OpenCL context. This is referred to as implicit synchronization.
 *
 * @p Errors
 *
 * Returns a valid non-zero OpenCL buffer object and errcode_ret is set to @c CL_SUCCESS if the buffer object is created successfully. Otherwise, it returns a NULL value with
 * one of the following error values returned in errcode_ret:
 *
 *     CL_INVALID_CONTEXT if context is not a valid context or was not created from a GL context.
 *     CL_INVALID_VALUE if values specified in flags are not valid.
 *     CL_INVALID_GL_OBJECT if bufobj is not a GL buffer object or is a GL buffer object but does not have an existing data store or the size of the buffer is 0.
 *     CL_OUT_OF_RESOURCES if there is a failure to allocate resources required by the OpenCL implementation on the device.
 *     CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required by the OpenCL implementation on the host.
 */
compute::Id createBuffer(compute::BufferFlag flags, video::Buffer& buffer, int idx) {
	const cl_mem_info clFlags = compute::_priv::convertFlags(flags);
	cl_int error;
	const video::Id bufferId = buffer.bufferHandle(idx);
	if (bufferId == video::InvalidId) {
		Log::debug("Invalid buffer handle");
		return compute::InvalidId;
	}
	compute::Id object = clCreateFromGLBuffer(compute::_priv::_ctx.context, clFlags, (cl_GLuint)bufferId, &error);
	if (error == CL_SUCCESS) {
		return object;
	}
	compute::_priv::checkError(error);
	return compute::InvalidId;
}

/**
 * @brief Creates an OpenCL image object, image array object, or image buffer object from an OpenGL texture object,
 * texture array object, texture buffer object, or a single face of an OpenGL cubemap texture object.
 *
 * @p Parameters
 *
 * @param context A valid OpenCL context created from an OpenGL context.
 *
 * @param flags A bit-field that is used to specify usage information. Refer to the table for clCreateBuffer for a description of flags. Only the values @c CL_MEM_READ_ONLY,
 * @c CL_MEM_WRITE_ONLY and @c CL_MEM_READ_WRITE can be used.
 *
 * @param texture_target This value must be one of @c GL_TEXTURE_1D, @c GL_TEXTURE_1D_ARRAY, @c GL_TEXTURE_BUFFER, @c GL_TEXTURE_2D, @c GL_TEXTURE_2D_ARRAY,
 * @c GL_TEXTURE_3D, @c GL_TEXTURE_CUBE_MAP_POSITIVE_X, @c GL_TEXTURE_CUBE_MAP_POSITIVE_Y, @c GL_TEXTURE_CUBE_MAP_POSITIVE_Z, @c GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
 * @c GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, @c GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, or @c GL_TEXTURE_RECTANGLE. (@c GL_TEXTURE_RECTANGLE requires OpenGL 3.1. Alternatively,
 * @c GL_TEXTURE_RECTANGLE_ARB may be specified if the OpenGL extension @c GL_ARB_texture_rectangle is supported.) texture_target is used only to define the image type
 * of texture. No reference to a bound GL texture object is made or implied by this parameter.
 * If the @c cl_khr_gl_msaa_sharing extension is enabled, texture_target may be @c GL_TEXTURE_2D_MULTISAMPLE or @c GL_TEXTURE_2D_MULTISAMPLE_ARRAY.
 * If texture_target is @c GL_TEXTURE_2D_MULTISAMPLE, @c clCreateFromGLTexture creates an OpenCL 2D multi-sample image object from an OpenGL 2D multi-sample texture
 * If texture_target is @c GL_TEXTURE_2D_MULTISAMPLE_ARRAY, @c clCreateFromGLTexture creates an OpenCL 2D multi-sample array image object from an OpenGL 2D multi-sample texture.
 *
 * @param miplevel The mipmap level to be used. If texture_target is @c GL_TEXTURE_BUFFER, miplevel must be 0. Implementations may return @c CL_INVALID_OPERATION for miplevel
 * values > 0
 *
 * @param texture The name of a GL 1D, 2D, 3D, 1D array, 2D array, cubemap, rectangle or buffer texture object. The texture object must be a complete texture as per
 * OpenGL rules on texture completeness. The texture format and dimensions defined by OpenGL for the specified miplevel of the texture will be used to create the OpenCL
 * image memory object. Only GL texture objects with an internal format that maps to appropriate image channel order and data type specified in tables 5.5 and
 * 5.6 (see cl_image_format) may be used to create the OpenCL image memory object.
 *
 * @param errcode_ret Returns an appropriate error code as described below. If errcode_ret is NULL, no error code is returned.
 *
 * @p Notes
 *
 * If the state of a GL texture object is modified through the GL API (e.g. @c glTexImage2D, @c glTexImage3D or the values of the texture parameters @c GL_TEXTURE_BASE_LEVEL
 * or @c GL_TEXTURE_MAX_LEVEL are modified) while there exists a corresponding CL image object, subsequent use of the CL image object will result in undefined behavior.
 *
 * The @c clRetainMemObject and @c clReleaseMemObject functions can be used to retain and release the image objects.
 *
 * @p General information about GL sharing follows.
 *
 * The OpenCL specification in section 9.7 defines how to share data with texture and buffer objects in a parallel OpenGL implementation, but does not define how the
 * association between an OpenCL context and an OpenGL context or share group is established. This extension defines optional attributes to OpenCL context creation
 * routines which associate a GL context or share group object with a newly created OpenCL context. If this extension is supported by an implementation, the string
 * "cl_khr_gl_sharing" will be present in the @c CL_DEVICE_EXTENSIONS string described in the table of allowed values for param_name for @c clGetDeviceInfo or in the
 * @c CL_PLATFORM_EXTENSIONS string described in the table of allowed values for param_name for @c clGetPlatformInfo.
 *
 * This section discusses OpenCL functions that allow applications to use OpenGL buffer, texture, and renderbuffer objects as OpenCL memory objects. This allows efficient
 * sharing of data between OpenCL and OpenGL. The OpenCL API may be used to execute kernels that read and/or write memory objects that are also OpenGL objects.
 *
 * An OpenCL image object may be created from an OpenGL texture or renderbuffer object. An OpenCL buffer object may be created from an OpenGL buffer object.
 *
 * Any supported OpenGL object defined within the GL share group object, or the share group associated with the GL context from which the CL context is created, may be
 * shared, with the exception of the default OpenGL objects (i.e. objects named zero), which may not be shared.
 *
 * @p OpenGL and Corresponding OpenCL Image Formats
 *
 * The table below (Table 9.4) describes the list of GL texture internal formats and the corresponding CL image formats. If a GL texture object with an internal format
 * from the table below is successfully created by OpenGL, then there is guaranteed to be a mapping to one of the corresponding CL image format(s) in that table. Texture
 * objects created with other OpenGL internal formats may (but are not guaranteed to) have a mapping to a CL image format; if such mappings exist, they are guaranteed to
 * preserve all color components, data types, and at least the number of bits/component actually allocated by OpenGL for that format.
 * GL internal format 	CL image format (channel order, channel data type)
 * GL_RGBA8 	CL_RGBA, CL_UNORM_INT8 or CL_BGRA, CL_UNORM_INT8
 * GL_SRGBA8_ALPHA8 	CL_sRGBA, CL_UNORM_INT8
 * GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV 	CL_RGBA, CL_UNORM_INT8
 * GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV 	CL_BGRA, CL_UNORM_INT8
 * GL_RGBA8I, GL_RGBA8I_EXT 	CL_RGBA, CL_SIGNED_INT8
 * GL_RGBA16I, GL_RGBA16I_EXT 	CL_RGBA, CL_SIGNED_INT16
 * GL_RGBA32I, GL_RGBA32I_EXT 	CL_RGBA, CL_SIGNED_INT32
 * GL_RGBA8UI, GL_RGBA8UI_EXT 	CL_RGBA, CL_UNSIGNED_INT8
 * GL_RGBA16UI, GL_RGBA16UI_EXT 	CL_RGBA, CL_UNSIGNED_INT16
 * GL_RGBA32UI, GL_RGBA32UI_EXT 	CL_RGBA, CL_UNSIGNED_INT32
 * GL_RGBA8_SNORM 	CL_RGBA, CL_SNORM_INT8
 * GL_RGBA16 	CL_RGBA, CL_UNORM_INT16
 * GL_RGBA16_SNORM 	CL_RGBA, CL_SNORM_INT166
 * GL_RGBA16F, GL_RGBA16F_ARB 	CL_RGBA, CL_HALF_FLOAT
 * GL_RGBA32F, GL_RGBA32F_ARB 	CL_RGBA, CL_FLOAT
 * GL_R8 	CL_R, CL_UNORM_INT8
 * GL_R8_SNORM 	CL_R, CL_SNORM_INT8
 * GL_R16 	CL_R, CL_UNORM_INT16
 * GL_R16_SNORM 	CL_R, CL_SNORM_INT16
 * GL_R16F 	CL_R, CL_HALF_FLOAT
 * GL_R32F 	CL_R, CL_FLOAT
 * GL_R8I 	CL_R, CL_SIGNED_INT8
 * GL_R16I 	CL_R, CL_SIGNED_INT16
 * GL_R32I 	CL_R, CL_SIGNED_INT32
 * GL_R8UI 	CL_R, CL_UNSIGNED_INT8
 * GL_R16UI 	CL_R, CL_UNSIGNED_INT16
 * GL_R32UI 	CL_R, CL_UNSIGNED_INT32
 * GL_RG8 	CL_RG, CL_UNORM_INT8
 * GL_RG8_SNORM 	CL_RG, CL_SNORM_INT8
 * GL_RG16 	CL_RG, CL_UNORM_INT16
 * GL_RG16_SNORM 	CL_RG, CL_SNORM_INT16
 * GL_RG16F 	CL_RG, CL_HALF_FLOAT
 * GL_RG32F 	CL_RG, CL_FLOAT
 * GL_RG8I 	CL_RG, CL_SIGNED_INT8
 * GL_RG16I 	CL_RG, CL_SIGNED_INT16
 * GL_RG32I 	CL_RG, CL_SIGNED_INT32
 * GL_RG8UI 	CL_RG, CL_UNSIGNED_INT8
 * GL_RG16UI 	CL_RG, CL_UNSIGNED_INT16
 * GL_RG32UI 	CL_RG, CL_UNSIGNED_INT32
 *
 * If the @c cl_khr_gl_depth_images extension is enabled, the following new image formats are added to table 9.4 in section 9.6.3.1 of the OpenCL 2.0 extension specification.
 * If a GL texture object with an internal format from table 9.4 is successfully created by OpenGL, then there is guaranteed to be a mapping to one of the corresponding CL
 * image format(s) in that table.
 * GL internal format 	CL image format (channel order, channel data type)
 * GL_DEPTH_COMPONENT32F 	CL_DEPTH, CL_FLOAT
 * GL_DEPTH_COMPONENT16 	CL_DEPTH, CL_UNORM_INT16
 * GL_DEPTH24_STENCIL8 	CL_DEPTH_STENCIL, CL_UNORM_INT24
 * GL_DEPTH32F_STENCIL8 	CL_DEPTH_STENCIL, CL_FLOAT
 * Lifetime of [GL] Shared Objects
 *
 * An OpenCL memory object created from an OpenGL object (hereinafter refered to as a "shared CL/GL object") remains valid as long as the corresponding GL object has not
 * been deleted. If the GL object is deleted through the GL API (e.g. @c glDeleteBuffers, @c glDeleteTextures, or @c glDeleteRenderbuffers), subsequent use of the CL buffer
 * or image object will result in undefined behavior, including but not limited to possible CL errors and data corruption, but may not result in program termination.
 *
 * The CL context and corresponding command-queues are dependent on the existence of the GL share group object, or the share group associated with the GL context from which
 * the CL context is created. If the GL share group object or all GL contexts in the share group are destroyed, any use of the CL context or command-queue(s) will result in undefined behavior, which may include program termination. Applications should destroy the CL command-queue(s) and CL context before destroying the corresponding GL share group or contexts.
 *
 * @p Synchronizing OpenCL and OpenGL Access
 *
 * In order to ensure data integrity, the application is responsible for synchronizing access to shared CL/GL objects by their respective APIs. Failure to provide such
 * synchronization may result in race conditions and other undefined behavior including non-portability between implementations.
 *
 * Prior to calling @c clEnqueueAcquireGLObjects, the application must ensure that any pending GL operations which access the objects specified in mem_objects have completed.
 * This may be accomplished portably by issuing and waiting for completion of a glFinish command on all GL contexts with pending references to these objects. Implementations
 * may offer more efficient synchronization methods; for example on some platforms calling glFlush may be sufficient, or synchronization may be implicit within a thread, or
 * there may be vendor-specific extensions that enable placing a fence in the GL command stream and waiting for completion of that fence in the CL command queue. Note that no
 * synchronization methods other than @c glFinish are portable between OpenGL implementations at this time.
 *
 * When the extension @c cl_khr_egl_event is supported: Prior to calling @c clEnqueueAcquireGLObjects, the application must ensure that any pending EGL or EGL client API
 * operations which access the objects specified in mem_objects have completed. If the @c cl_khr_egl_event extension is supported and the EGL context in question supports
 * fence sync objects, explicit synchronisation can be achieved as set out in section 5.7.1. If the @c cl_khr_egl_event extension is not supported, completion of EGL client
 * API commands may be determined by issuing and waiting for completion of commands such as @c glFinish or @c vgFinish on all client API contexts with pending references to
 * these objects. Some implementations may offer other efficient synchronization methods. If such methods exist they will be described in platform-specific documentation.
 * Note that no synchronization methods other than @c glFinish and @c vgFinish are portable between all EGL client API implementations and all OpenCL implementations. While
 * this is the only way to ensure completion that is portable to all platforms, these are expensive operation and their use should be avoided if the cl_khr_egl_event extension
 * is supported on a platform.
 *
 * Similarly, after calling @c clEnqueueReleaseGLObjects, the application is responsible for ensuring that any pending OpenCL operations which access the objects specified in
 * mem_objects have completed prior to executing subsequent GL commands which reference these objects. This may be accomplished portably by calling @c clWaitForEvents with
 * the event object returned by @c clEnqueueReleaseGLObjects, or by calling @c clFinish. As above, some implementations may offer more efficient methods.
 *
 * The application is responsible for maintaining the proper order of operations if the CL and GL contexts are in separate threads.
 *
 * If a GL context is bound to a thread other than the one in which @c clEnqueueReleaseGLObjects is called, changes to any of the objects in mem_objects may not be visible to
 * that context without additional steps being taken by the application. For an OpenGL 3.1 (or later) context, the requirements are described in Appendix D ("Shared Objects
 * and Multiple Contexts") of the OpenGL 3.1 Specification. For prior versions of OpenGL, the requirements are implementation-dependent.
 *
 * Attempting to access the data store of an OpenGL object after it has been acquired by OpenCL and before it has been released will result in undefined behavior.
 * Similarly, attempting to access a shared CL/GL object from OpenCL before it has been acquired by the OpenCL command queue, or after it has been released, will result in
 * undefined behavior.
 *
 * If the @c cl_khr_gl_event extension is supported, then the OpenCL implementation will ensure that any such pending OpenGL operations are complete for an OpenGL context bound
 * to the same thread as the OpenCL context. This is referred to as implicit synchronization.
 *
 * @p Errors
 *
 * Returns a valid non-zero OpenCL image object and errcode_ret is set to @c CL_SUCCESS if the image object is created successfully. Otherwise, it returns a @c NULL value with
 * one of the following error values returned in errcode_ret:
 *
 *     CL_INVALID_CONTEXT if context is not a valid context or was not created from a GL context.
 *     CL_INVALID_VALUE if values specified in flags are not valid or if value specified in texture_target is not one of the values specified in the description of texture_target.
 *     CL_INVALID_MIP_LEVEL if miplevel is less than the value of levelbase (for OpenGL implementations) or zero (for OpenGL ES implementations); or greater than the value of q (for both OpenGL and OpenGL ES). levelbase and q are defined for the texture in section 3.8.10 (Texture Completeness) of the OpenGL 2.1 specification and section 3.7.10 of the OpenGL ES 2.0.
 *     CL_INVALID_MIP_LEVEL if miplevel is greater than zero and the OpenGL implementation does not support creating from non-zero mipmap levels.
 *     CL_INVALID_GL_OBJECT if texture is not a GL texture object whose type matches texture_target, if the specified miplevel of texture is not defined, or if the width or height of the specified miplevel is zero or if the GL texture object is incomplete.
 *     CL_INVALID_IMAGE_FORMAT_DESCRIPTOR if the OpenGL texture internal format does not map to a supported OpenCL image format.
 *     CL_INVALID_OPERATION if texture is a GL texture object created with a border width value greater than zero.
 *     CL_OUT_OF_RESOURCES if there is a failure to allocate resources required by the OpenCL implementation on the device.
 *     CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required by the OpenCL implementation on the host.
 */
compute::Id createTexture(compute::BufferFlag flags, video::Texture& texture) {
	const cl_mem_info clFlags = compute::_priv::convertFlags(flags);
	cl_int error;
	const video::Id textureId = texture.handle();
	if (textureId == video::InvalidId) {
		Log::debug("Invalid texture handle");
		return compute::InvalidId;
	}
	compute::Id object = clCreateFromGLTexture(compute::_priv::_ctx.context, clFlags, video::_priv::TextureTypes[std::enum_value(texture.type())],
			0, (cl_GLuint)textureId, &error);
	if (error == CL_SUCCESS) {
		return object;
	}
	compute::_priv::checkError(error);
	return compute::InvalidId;
}

/**
 * Acquire OpenCL memory objects that have been created from OpenGL objects.
 * command_queue: A valid command-queue. All devices used to create the OpenCL context associated with command_queue must support acquiring shared CL/GL objects. This constraint is enforced at context creation time.
 * num_objects: The number of memory objects to be acquired in mem_objects.
 * mem_objects A pointer to a list of CL memory objects that correspond to GL objects.
 * event_wait_list , num_events_in_wait_list: Specifies events that need to complete before this particular command can be executed. If event_wait_list is NULL, then this particular command does not wait on any event to complete. If event_wait_list is NULL, num_events_in_wait_list must be 0. If event_wait_list is not NULL, the list of events pointed to by event_wait_list must be valid and num_events_in_wait_list must be greater than 0. The events specified in event_wait_list act as synchronization points.
 * event: Returns an event object that identifies this command and can be used to query or queue a wait for the command to complete. event can be NULL in which case it will not be possible for the application to query the status of this command or queue a wait for this command to complete.
 *
 * Description
 * These objects need to be acquired before they can be used by any OpenCL commands queued to a command-queue. The OpenGL objects are acquired by the OpenCL context associated with command_queue and can therefore be used by all command-queues associated with the OpenCL context.
 *
 * Errors
 * Returns CL_SUCCESS if the function is executed successfully. If num_objects is 0 and mem_objects is NULL the function does nothing and returns CL_SUCCESS. Otherwise, it returns one of the following errors:
 * CL_INVALID_VALUE if num_objects is zero and mem_objects is not a NULL value or if num_objects > 0 and mem_objects is NULL.
 * CL_INVALID_MEM_OBJECT if memory objects in mem_objects are not valid OpenCL memory objects.
 * CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
 * CL_INVALID_CONTEXT if context associated with command_queue was not created from an OpenGL context.
 * CL_INVALID_GL_OBJECT if memory objects in mem_objects have not been created from a GL object(s).
 * CL_INVALID_EVENT_WAIT_LIST if event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.
 * CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required by the OpenCL implementation on the host.
 */
bool enqueueAcquire(compute::Id id) {
	const cl_int error = clEnqueueAcquireGLObjects(compute::_priv::_ctx.commandQueue, 1, (cl_mem*)&id, 0, nullptr, nullptr);
	if (error == CL_SUCCESS) {
		return true;
	}
	compute::_priv::checkError(error);
	return false;
}

/**
 * Release OpenCL memory objects that have been created from OpenGL objects.
 * command_queue: A valid command-queue. All devices used to create the OpenCL context associated with command_queue must support acquiring shared CL/GL objects. This constraint is enforced at context creation time.
 * num_objects: The number of memory objects to be released in mem_objects.
 * mem_objects: A pointer to a list of CL memory objects that correspond to GL objects.
 * event_wait_list , num_events_in_wait_list: These parameters specify events that need to complete before this particular command can be executed. If event_wait_list is NULL, then this particular command does not wait on any event to complete. If event_wait_list is NULL, num_events_in_wait_list must be 0. If event_wait_list is not NULL, the list of events pointed to by event_wait_list must be valid and num_events_in_wait_list must be greater than 0. The events specified in event_wait_list act as synchronization points.
 * event: Returns an event object that identifies this particular read / write command and can be used to query or queue a wait for the command to complete. event can be NULL in which case it will not be possible for the application to query the status of this command or queue a wait for this command to complete.
 *
 * Description
 * These objects need to be released before they can be used by OpenGL. The OpenGL objects are released by the OpenCL context associated with command_queue.
 *
 * Errors
 * clEnqueueReleaseGLObjects returns CL_SUCCESS if the function is executed successfully. If num_objects is 0 and mem_objects is NULL the function does nothing and returns CL_SUCCESS. Otherwise, it returns one of the following errors:
 * CL_INVALID_VALUE if num_objects is zero and mem_objects is not a NULL value or if num_objects > 0 and mem_objects is NULL.
 * CL_INVALID_MEM_OBJECT if memory objects in mem_objects are not valid OpenCL memory objects.
 * CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
 * CL_INVALID_CONTEXT if context associated with command_queue was not created from an OpenGL context
 * CL_INVALID_GL_OBJECT if memory objects in mem_objects have not been created from a GL object(s).
 * CL_INVALID_EVENT_WAIT_LIST if event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.
 * CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required by the OpenCL implementation on the host.
 */
bool enqueueRelease(compute::Id id) {
	const cl_int error = clEnqueueReleaseGLObjects(compute::_priv::_ctx.commandQueue, 1, (cl_mem*)&id, 0, nullptr, nullptr);
	if (error == CL_SUCCESS) {
		return true;
	}
	compute::_priv::checkError(error);
	return false;
}

}
