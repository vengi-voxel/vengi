/**
 * @file
 *
 * @ingroup Compute
 */
#include "CLCompute.h"
#include "core/Log.h"
#include "core/App.h"
#include "core/String.h"
#include "core/Assert.h"
#include "io/Filesystem.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/gtc/round.hpp>
#include <string>
#include <unordered_map>

namespace compute {

namespace _priv {

static std::unordered_map<void*, size_t> _sizes;

Context _ctx;

cl_mem_flags convertFlags(BufferFlag flags) {
	cl_mem_flags clValue = (cl_mem_flags)0;
	if ((flags & BufferFlag::ReadWrite) != BufferFlag::None) {
		clValue |= CL_MEM_READ_WRITE;
	}
	if ((flags & BufferFlag::WriteOnly) != BufferFlag::None) {
		clValue |= CL_MEM_WRITE_ONLY;
	}
	if ((flags & BufferFlag::ReadOnly) != BufferFlag::None) {
		clValue |= CL_MEM_READ_ONLY;
	}
	if ((flags & BufferFlag::UseHostPointer) != BufferFlag::None) {
		clValue |= CL_MEM_USE_HOST_PTR;
	}
	if ((flags & BufferFlag::AllocHostPointer) != BufferFlag::None) {
		clValue |= CL_MEM_ALLOC_HOST_PTR;
	}
	if ((flags & BufferFlag::CopyHostPointer) != BufferFlag::None) {
		clValue |= CL_MEM_COPY_HOST_PTR;
	}
	return clValue;
}

#define CLCOMPUTEERR(x) case x: return #x
#define CLCOMPUTEERRCTX(ctx, x) case x: return ctx ": " #x

static const char *convertCLError(cl_int err) {
	switch (err) {
	#ifdef CL_VERSION_1_0
	CLCOMPUTEERR(CL_SUCCESS);
	CLCOMPUTEERR(CL_DEVICE_NOT_FOUND);
	CLCOMPUTEERR(CL_DEVICE_NOT_AVAILABLE);
	CLCOMPUTEERR(CL_COMPILER_NOT_AVAILABLE);
	CLCOMPUTEERR(CL_MEM_OBJECT_ALLOCATION_FAILURE);
	CLCOMPUTEERR(CL_OUT_OF_RESOURCES);
	CLCOMPUTEERR(CL_OUT_OF_HOST_MEMORY);
	CLCOMPUTEERR(CL_PROFILING_INFO_NOT_AVAILABLE);
	CLCOMPUTEERR(CL_MEM_COPY_OVERLAP);
	CLCOMPUTEERR(CL_IMAGE_FORMAT_MISMATCH);
	CLCOMPUTEERR(CL_IMAGE_FORMAT_NOT_SUPPORTED);
	CLCOMPUTEERR(CL_BUILD_PROGRAM_FAILURE);
	CLCOMPUTEERR(CL_MAP_FAILURE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_VALUE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_DEVICE_TYPE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_PLATFORM);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_DEVICE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_CONTEXT);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_QUEUE_PROPERTIES);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_COMMAND_QUEUE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_HOST_PTR);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_MEM_OBJECT);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_IMAGE_SIZE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_SAMPLER);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_BINARY);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_BUILD_OPTIONS);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_PROGRAM);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_PROGRAM_EXECUTABLE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_KERNEL_NAME);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_KERNEL_DEFINITION);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_KERNEL);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_ARG_INDEX);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_ARG_VALUE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_ARG_SIZE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_KERNEL_ARGS);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_WORK_DIMENSION);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_WORK_GROUP_SIZE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_WORK_ITEM_SIZE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_GLOBAL_OFFSET);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_EVENT_WAIT_LIST);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_EVENT);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_OPERATION);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_GL_OBJECT);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_BUFFER_SIZE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_MIP_LEVEL);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_GLOBAL_WORK_SIZE);
	#endif

	#ifdef CL_VERSION_1_1
	CLCOMPUTEERR(CL_MISALIGNED_SUB_BUFFER_OFFSET);
	CLCOMPUTEERR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_PROPERTY);
	#endif

	#ifdef CL_VERSION_1_2
	CLCOMPUTEERR(CL_COMPILE_PROGRAM_FAILURE);
	CLCOMPUTEERR(CL_LINKER_NOT_AVAILABLE);
	CLCOMPUTEERR(CL_LINK_PROGRAM_FAILURE);
	CLCOMPUTEERR(CL_DEVICE_PARTITION_FAILED);
	CLCOMPUTEERR(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_IMAGE_DESCRIPTOR);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_COMPILER_OPTIONS);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_LINKER_OPTIONS);
	CLCOMPUTEERRCTX("Compile error", CL_INVALID_DEVICE_PARTITION_COUNT);
	#endif

	#if defined(CL_VERSION_2_0)
	CLCOMPUTEERR(CL_INVALID_PIPE_SIZE);
	CLCOMPUTEERR(CL_INVALID_DEVICE_QUEUE);
	#endif

	#if defined(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR)
	CLCOMPUTEERR(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
	#endif

	#if defined(__CL_EXT_H) && defined (cl_khr_icd)
	CLCOMPUTEERR(CL_PLATFORM_NOT_FOUND_KHR);
	#else
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	#endif

	#if defined(__OPENCL_CL_D3D10_H)
	CLCOMPUTEERR(CL_INVALID_D3D10_DEVICE_KHR);
	CLCOMPUTEERR(CL_INVALID_D3D10_RESOURCE_KHR);
	CLCOMPUTEERR(CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR);
	CLCOMPUTEERR(CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR);
	#endif

	#if defined(__OPENCL_CL_D3D11_H)
	CLCOMPUTEERR(CL_INVALID_D3D11_DEVICE_KHR);
	CLCOMPUTEERR(CL_INVALID_D3D11_RESOURCE_KHR);
	CLCOMPUTEERR(CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR);
	CLCOMPUTEERR(CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR);
	#endif

	#if defined(__OPENCL_CL_DX9_MEDIA_SHARING_H)
	CLCOMPUTEERR(CL_INVALID_DX9_MEDIA_ADAPTER_KHR);
	CLCOMPUTEERR(CL_INVALID_DX9_MEDIA_SURFACE_KHR);
	CLCOMPUTEERR(CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR);
	CLCOMPUTEERR(CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR);
	#endif

	#if defined(__CL_EXT_H) && defined(cl_ext_device_fission)
	CLCOMPUTEERR(CL_DEVICE_PARTITION_FAILED_EXT);
	CLCOMPUTEERR(CL_INVALID_PARTITION_COUNT_EXT);
	CLCOMPUTEERR(CL_INVALID_PARTITION_NAME_EXT);
	#endif

	#if defined(__OPENCL_CL_EGL_H)
	CLCOMPUTEERR(CL_EGL_RESOURCE_NOT_ACQUIRED_KHR);
	CLCOMPUTEERR(CL_INVALID_EGL_OBJECT_KHR);
	#endif

	#if defined(__CL_EXT_H) && defined(cl_intel_accelerator)
	CLCOMPUTEERR(CL_INVALID_ACCELERATOR_INTEL);
	CLCOMPUTEERR(CL_INVALID_ACCELERATOR_TYPE_INTEL);
	CLCOMPUTEERR(CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL);
	CLCOMPUTEERR(CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL);
	#endif
	default:
		return "Unknown error";
	}
}
#undef CLCOMPUTEERR
#undef CLCOMPUTEERRCTX

bool checkError(cl_int clError, bool triggerAssert) {
	if (triggerAssert) {
		core_assert_msg(clError == CL_SUCCESS, "CL err: %s => %i", convertCLError(clError), clError);
	}
	return clError == CL_SUCCESS;
}

} // end _priv namespace

static std::string getPlatformName(cl_platform_id id) {
	size_t size = 0u;
	cl_int error;

	error = clGetPlatformInfo(id, CL_PLATFORM_NAME, 0, nullptr, &size);
	_priv::checkError(error);

	std::string result;
	result.resize(size);
	error = clGetPlatformInfo(id, CL_PLATFORM_NAME, size,
			const_cast<char*>(result.data()), nullptr);
	_priv::checkError(error);

	return result;
}

static std::string getDeviceInfo(cl_device_id id, cl_device_info param) {
	size_t size = 0u;
	cl_int error;

	error = clGetDeviceInfo(id, param, 0, nullptr, &size);
	_priv::checkError(error);

	std::string result;
	result.resize(size);
	error = clGetDeviceInfo(id, param, size,
			const_cast<char*>(result.data()), nullptr);
	_priv::checkError(error);

	return result;
}

size_t requiredAlignment() {
	return _priv::_ctx.alignment;
}

// https://www.khronos.org/registry/OpenCL/sdk/1.0/docs/man/xhtml/clBuildProgram.html
bool configureProgram(Id program) {
	const cl_int error = clBuildProgram((cl_program)program,
		0,
		nullptr,
		"-cl-no-signed-zeros -cl-denorms-are-zero -cl-fast-relaxed-math -cl-finite-math-only -Werror",
		nullptr,
		nullptr);
	if (error == CL_BUILD_PROGRAM_FAILURE) {
		char buf[4096];
		const cl_int infoError = clGetProgramBuildInfo((cl_program) program,
				_priv::_ctx.deviceId,
				CL_PROGRAM_BUILD_LOG, sizeof(buf), buf, nullptr);
		if (infoError == CL_SUCCESS) {
			Log::error("Failed to build program: %s", buf);
		} else {
			Log::error("Failed to build program, but couldn't query the reason");
		}
	}
	_priv::checkError(error);
	return error == CL_SUCCESS;
}

bool deleteProgram(Id& program) {
	if (program == InvalidId) {
		return true;
	}
	const cl_int error = clReleaseProgram((cl_program)program);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		program = InvalidId;
		return true;
	}
	return false;
}

Id createBuffer(BufferFlag flags, size_t size, void* data) {
	if (_priv::_ctx.context == nullptr) {
		return InvalidId;
	}
	core_assert(size > 0);

	const cl_mem_flags clValue = _priv::convertFlags(flags);
	const bool useHostPtr = (flags & BufferFlag::UseHostPointer) != BufferFlag::None;

	cl_int error;
	cl_mem bufferObject = clCreateBuffer(_priv::_ctx.context, clValue,
			size, useHostPtr ? data : nullptr, &error);
	_priv::checkError(error);
	if (error != CL_SUCCESS) {
		return InvalidId;
	}
	if (!useHostPtr && data != nullptr) {
		void *target = clEnqueueMapBuffer(_priv::_ctx.commandQueue, bufferObject, CL_TRUE, CL_MAP_WRITE,
				0, size, 0, nullptr, nullptr, &error);
		_priv::checkError(error);
		if (target == nullptr) {
			clReleaseMemObject(bufferObject);
			return InvalidId;
		}
		memcpy(target, data, size);
		cl_event event;
		error = clEnqueueUnmapMemObject(_priv::_ctx.commandQueue, bufferObject, target, 0, nullptr, &event);
		_priv::checkError(error);
		if (error != CL_SUCCESS) {
			clReleaseMemObject(bufferObject);
			return InvalidId;
		}
		error = clWaitForEvents(1, &event);
		_priv::checkError(error);
	}
	_priv::_sizes[bufferObject] = size;
	return (Id)bufferObject;
}

bool deleteBuffer(Id& buffer) {
	if (buffer == InvalidId) {
		return true;
	}
	const cl_int error = clReleaseMemObject((cl_mem)buffer);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		_priv::_sizes.erase(buffer);
		buffer = InvalidId;
		return true;
	}
	return false;
}

bool updateBuffer(Id buffer, size_t size, const void* data, bool blockingWrite) {
	if (buffer == InvalidId) {
		return false;
	}
	if (_priv::_ctx.commandQueue == nullptr) {
		return false;
	}
	const cl_int error = clEnqueueWriteBuffer(_priv::_ctx.commandQueue,
			(cl_mem) buffer, blockingWrite ? CL_TRUE : CL_FALSE, 0, size, data,
			0, nullptr, nullptr);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		_priv::_sizes[buffer] = size;
		return true;
	}
	return false;
}

bool readBuffer(Id buffer, size_t size, void* data) {
	if (buffer == InvalidId) {
		return false;
	}
	if (_priv::_ctx.commandQueue == nullptr) {
		return false;
	}
	if (size <= 0) {
		return false;
	}
	if (data == nullptr) {
		return false;
	}
	const auto i = _priv::_sizes.find(buffer);
	core_assert_always(i != _priv::_sizes.end());
	core_assert_msg(i->second == size, "Expected to read %i bytes, but was asked to read %i", (int)i->second, (int)size);
	const cl_int error = clEnqueueReadBuffer(_priv::_ctx.commandQueue,
			(cl_mem) buffer, CL_TRUE, 0, size, data, 0, nullptr, nullptr);
	_priv::checkError(error);
	return error == CL_SUCCESS;
}

/**
 * @param[in] data A pointer to the image data that may already be allocated by the application. The size of the buffer that host_ptr points to must be greater
 * than or equal to image_slice_pitch * image_depth. The size of each element in bytes must be a power of 2. The image data specified by host_ptr is stored as
 * a linear sequence of adjacent 2D slices. Each 2D slice is a linear sequence of adjacent scanlines. Each scanline is a linear sequence of image elements.
 *
 * @li <a href="https://www.khronos.org/registry/OpenCL/sdk/1.0/docs/man/xhtml/clCreateImage2D.html">clCreateImage2D</a>
 * @li <a href="https://www.khronos.org/registry/OpenCL/sdk/1.0/docs/man/xhtml/clCreateImage3D.html">clCreateImage3D</a>
 */
// TODO: data alignment
// TODO: support flags like read-only
Id createTexture(const Texture& texture, const uint8_t* data) {
	if (!_priv::_ctx.imageSupport) {
		Log::warn("No image support for the selected device");
		return InvalidId;
	}
	cl_int error = CL_SUCCESS;
	cl_mem id;

	/**
	 * A pointer to a structure that describes format properties of the image to be allocated. See cl_image_format for a detailed description
	 * of the image format descriptor.
	 */
	cl_image_format fmt;
	memset(&fmt, 0, sizeof(fmt));
	fmt.image_channel_order = _priv::TextureFormats[std::enum_value(texture.format())];
	fmt.image_channel_data_type = _priv::TextureDataFormats[std::enum_value(texture.dataformat())];
	const size_t channelSize = _priv::TextureDataFormatSizes[std::enum_value(texture.dataformat())];
	const size_t components = _priv::TextureFormatComponents[std::enum_value(texture.format())];

	cl_image_desc desc;
	memset(&desc, 0, sizeof(desc));
	desc.image_width = texture.width();
	desc.image_height = texture.height();

	if (desc.image_width == 0) {
		Log::error("Texture width is 0");
		return InvalidId;
	}
	if (desc.image_height == 0) {
		Log::error("Texture height is 0");
		return InvalidId;
	}

	/**
	 * A bit-field that is used to specify allocation and usage information about the image memory object being created and is described in the table List of
	 * supported cl_mem_flags values for clCreateBuffer.
	 */
	cl_mem_flags flags = (cl_mem_flags)0;
	if (data != nullptr) {
		//flags |= CL_MEM_READ_ONLY;
		flags |= CL_MEM_COPY_HOST_PTR;
	}

	if (texture.type() == TextureType::Texture3D) {
		if (desc.image_width > _priv::_ctx.image3DSize[0]) {
			Log::error("Max 3d texture width exceeded");
			return InvalidId;
		}
		if (desc.image_height > _priv::_ctx.image3DSize[1]) {
			Log::error("Max 3d texture height exceeded");
			return InvalidId;
		}
		/**
		 * The width and height of the image in pixels. These must be values greater than or equal to 1.
		 * The depth of the image in pixels. This must be a value greater than 1.
		 */
		desc.image_depth = texture.layers();
		if (desc.image_depth > _priv::_ctx.image3DSize[2]) {
			Log::error("Max 3d texture depth exceeded");
			return InvalidId;
		}
		if (desc.image_depth < 1) {
			Log::error("There must be more than 1 layer in a 3d texture");
			return InvalidId;
		}
		/**
		 * The scan-line pitch in bytes. This must be 0 if host_ptr is NULL and can be either 0 or greater than or equal to image_width * size of element in
		 * bytes if host_ptr is not NULL. If host_ptr is not NULL and image_row_pitch is equal to 0, image_row_pitch is calculated as image_width * size of
		 * element in bytes. If image_row_pitch is not 0, it must be a multiple of the image element size in bytes.
		 */
		desc.image_row_pitch = data == nullptr ? 0 : (desc.image_width * channelSize * components);
		/**
		 * The size in bytes of each 2D slice in the 3D image. This must be 0 if host_ptr is NULL and can be either 0 or greater than or equal to
		 * image_row_pitch * image_height if host_ptr is not NULL. If host_ptr is not NULL and image_slice_pitch equal to 0, image_slice_pitch is calculated
		 * as image_row_pitch * image_height. If image_slice_pitch is not 0, it must be a multiple of the image_row_pitch.
		 */
		desc.image_slice_pitch = data == nullptr ? 0 : (desc.image_row_pitch * desc.image_height);
		desc.image_type = CL_MEM_OBJECT_IMAGE3D;
	} else if (texture.type() == TextureType::Texture2D) {
		if (desc.image_width > _priv::_ctx.image2DSize[0]) {
			Log::error("Max 2d texture width exceeded");
			return InvalidId;
		}
		if (desc.image_height > _priv::_ctx.image2DSize[1]) {
			Log::error("Max 2d texture height exceeded");
			return InvalidId;
		}
		/**
		 * The scan-line pitch in bytes. This must be 0 if host_ptr is NULL and can be either 0 or greater than or equal to image_width * size of element in
		 * bytes if host_ptr is not NULL. If host_ptr is not NULL and image_row_pitch is equal to 0, image_row_pitch is calculated as image_width * size of
		 * element in bytes. If image_row_pitch is not 0, it must be a multiple of the image element size in bytes.
		 */
		desc.image_row_pitch = data == nullptr ? 0 : (desc.image_width * channelSize * components);
		desc.image_type = CL_MEM_OBJECT_IMAGE2D;
	} else if (texture.type() == TextureType::Texture1D) {
		if (desc.image_width > _priv::_ctx.image1DSize) {
			Log::error("Max 1d texture width exceeded");
			return InvalidId;
		}
		desc.image_type = CL_MEM_OBJECT_IMAGE1D;
	}
	id = clCreateImage(_priv::_ctx.context, flags, &fmt, &desc, const_cast<void*>((const void*)data), &error);
	_priv::checkError(error);
	return id;
}

void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	const cl_int error = clReleaseMemObject((cl_mem)id);
	id = InvalidId;
	_priv::checkError(error);
}

Id createSampler(const TextureConfig& config) {
	cl_int error = CL_SUCCESS;
	/* Specifies how out-of-range image coordinates are handled when reading from an image. This can be set to
	 * CL_ADDRESS_REPEAT, CL_ADDRESS_CLAMP_TO_EDGE, CL_ADDRESS_CLAMP, and CL_ADDRESS_NONE. */
	const cl_addressing_mode wrapMode = _priv::TextureWraps[std::enum_value(config.wrap())];
	/* Specifies the type of filter that must be applied when reading an image. This can be CL_FILTER_NEAREST or CL_FILTER_LINEAR. */
	const cl_filter_mode filterMode = _priv::TextureFilters[std::enum_value(config.filter())];
	/* Determines if the image coordinates specified are normalized (if normalized_coords is CL_TRUE) or not (if normalized_coords is CL_FALSE). */
	const cl_bool normalized = (cl_bool)config.normalizedCoordinates();
	const Id id = clCreateSampler(_priv::_ctx.context, normalized, wrapMode, filterMode, &error);
	_priv::checkError(error);
	return id;
}

void deleteSampler(Id& id) {
	if (id == InvalidId) {
		return;
	}
	const cl_int error = clReleaseSampler((cl_sampler)id);
	id = InvalidId;
	_priv::checkError(error);
}

/**
 * @brief Enqueues a command to read from a 2D or 3D image object to host memory.
 * @p Parameters
 * @param command_queue: Refers to the command-queue in which the read command will be queued. command_queue and image must be created with the same OpenCL contex
 * @param image: Refers to a valid 2D or 3D image object.
 * @param blocking_read: Indicates if the read operations are blocking or non-blocking.
 * If blocking_read is CL_TRUE i.e. the read command is blocking, clEnqueueReadImage does not return until the buffer data has been read and copied into memory pointed to by ptr.
 * If blocking_read is CL_FALSE i.e. map operation is non-blocking, clEnqueueReadImage queues a non-blocking read command and returns. The contents of the buffer that ptr points to cannot be used until the read command has completed. The event argument returns an event object which can be used to query the execution status of the read command. When the read command has completed, the contents of the buffer that ptr points to can be used by the application.
 * @param origin: Defines the (x, y, z) offset in pixels in the image from where to read. If image is a 2D image object, the z value given by origin[2] must be 0.
 * @param region: Defines the (width, height, depth) in pixels of the 2D or 3D rectangle being read. If image is a 2D image object, the depth value given by region[2] must be 1.
 * @param row_pitch: The length of each row in bytes. This value must be greater than or equal to the element size in bytes * width. If row_pitch is set to 0, the appropriate row pitch is calculated based on the size of each element in bytes multiplied by width.
 * @param slice_pitch: Size in bytes of the 2D slice of the 3D region of a 3D image being read. This must be 0 if image is a 2D image. This value must be greater than or equal to row_pitch * height. If slice_pitch is set to 0, the appropriate slice pitch is calculated based on the row_pitch * height.
 * @param ptr: The pointer to a buffer in host memory where image data is to be written to.
 * @param event_wait_list , num_events_in_wait_list: Specify events that need to complete before this particular command can be executed. If event_wait_list is NULL, then this particular command does not wait on any event to complete. If event_wait_list is NULL, num_events_in_wait_list must be 0. If event_wait_list is not NULL, the list of events pointed to by event_wait_list must be valid and num_events_in_wait_list must be greater than 0. The events specified in event_wait_list act as synchronization points. The context associated with events in event_wait_list and command_queue must be the same.
 * @param event: Returns an event object that identifies this particular read command and can be used to query or queue a wait for this particular command to complete. event can be NULL in which case it will not be possible for the application to query the status of this command or queue a wait for this command to complete.
 * @p Notes
 * Calling clEnqueueReadImage to read a region of the image object with the ptr argument value set to host_ptr + (origin[2] * image slice pitch + origin[1] * image row pitch + origin[0] * bytes per pixel), where host_ptr is a pointer to the memory region specified when the image object being read is created with CL_MEM_USE_HOST_PTR, must meet the following requirements in order to avoid undefined behavior:
 * All commands that use this image object have finished execution before the read command begins execution.
 * The row_pitch and slice_pitch argument values in clEnqueueReadImage must be set to the image row pitch and slice pitch.
 * The image object is not mapped.
 * The image object is not used by any command-queue until the read command has finished execution.
 * @p Errors
 * clEnqueueReadImage return CL_SUCCESS if the function is executed successfully. Otherwise, it returns one of the following errors.
 * CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
 * CL_INVALID_CONTEXT if the context associated with command_queue and image are not the same or if the context associated with command_queue and events in event_wait_list are not the same.
 * CL_INVALID_MEM_OBJECT if image is not a valid image object.
 * CL_INVALID_VALUE if the region being read specified by origin and region is out of bounds or if ptr is a NULL value.
 * CL_INVALID_VALUE if image is a 2D image object and origin[2] is not equal to 0 or region[2] is not equal to 1 or slice_pitch is not equal to 0.
 * CL_INVALID_EVENT_WAIT_LIST if event_wait_list is NULL and num_events_in_wait_list greater than 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events.
 * CL_MEM_OBJECT_ALLOCATION_FAILURE if there is a failure to allocate memory for data store associated with image.
 * CL_OUT_OF_HOST_MEMORY if there is a failure to allocate resources required by the OpenCL implementation on the host.
 */
bool readTexture(compute::Texture& texture, void *data, const glm::ivec3& origin, const glm::ivec3& region, bool blocking) {
	if (data == nullptr) {
		return false;
	}
	if (origin.x < 0 || origin.x >= texture.width()) {
		Log::debug("origin (%u:%u:%u) may not exceed the texture dimensions (%i:%i:%i)",
				origin.x, origin.y, origin.z, texture.width(), texture.height(), texture.layers());
		return false;
	}
	if (origin.y < 0 || origin.y >= texture.height()) {
		Log::debug("origin (%u:%u:%u) may not exceed the texture dimensions (%i:%i:%i)",
				origin.x, origin.y, origin.z, texture.width(), texture.height(), texture.layers());
		return false;
	}
	if (region.x <= 0 || region.y <= 0 || region.z <= 0) {
		Log::debug("Region must be bigger than 0 in every dimension");
		return false;
	}
	if (region.x > (texture.width() - origin.x)) {
		Log::debug("region (%u:%u:%u) and offset (%u:%u:%u) exceed the texture boundaries (%i,%i,%i)",
			region.x, region.y, region.z, origin.x, origin.y, origin.z, texture.width(), texture.height(), texture.layers());
		return false;
	}
	if (region.y > (texture.height() - origin.y)) {
		Log::debug("region (%u:%u:%u) and offset (%u:%u:%u) exceed the texture boundaries (%i,%i,%i)",
			region.x, region.y, region.z, origin.x, origin.y, origin.z, texture.width(), texture.height(), texture.layers());
		return false;
	}
	if (region.z > (texture.layers() - origin.z)) {
		Log::debug("region (%u:%u:%u) and offset (%u:%u:%u) exceed the texture boundaries (%i,%i,%i)",
			region.x, region.y, region.z, origin.x, origin.y, origin.z, texture.width(), texture.height(), texture.layers());
		return false;
	}

	const compute::Id textureId = texture.handle();
	if (textureId == compute::InvalidId) {
		Log::debug("Invalid texture given");
		return false;
	}
	const size_t clOrigin[] = {(size_t)origin.x, (size_t)origin.y, (size_t)origin.z};
	const size_t clRegion[] = {(size_t)region.x, (size_t)region.y, (size_t)region.z};
	const cl_int error = clEnqueueReadImage(
			_priv::_ctx.commandQueue, (cl_mem)textureId, blocking ? CL_TRUE : CL_FALSE,
			clOrigin, clRegion, 0, 0, data, 0, nullptr, nullptr);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		if (blocking) {
			return finish();
		}
		return true;
	}
	return false;
}

bool copyBufferToImage(compute::Id buffer, compute::Id image, size_t bufferOffset, const glm::ivec3& origin, const glm::ivec3& region) {
	if (origin.x < 0) {
		return false;
	}
	if (origin.y < 0) {
		return false;
	}
	if (region.x <= 0 || region.y <= 0 || region.z <= 0) {
		Log::debug("Region must be bigger than 0 in every dimension");
		return false;
	}
	const size_t clOrigin[] = {(size_t)origin.x, (size_t)origin.y, (size_t)origin.z};
	const size_t clRegion[] = {(size_t)region.x, (size_t)region.y, (size_t)region.z};
	const cl_int error = clEnqueueCopyBufferToImage(
			_priv::_ctx.commandQueue, (cl_mem)buffer, (cl_mem)image, bufferOffset, clOrigin, clRegion, 0u, nullptr, nullptr);
	_priv::checkError(error);
	return error == CL_SUCCESS;
}

Id createProgram(const std::string& source) {
	if (_priv::_ctx.context == nullptr) {
		return InvalidId;
	}
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateProgramWithSource.html
	const size_t lengths[] = { source.size () };
	const char* sources[] = { source.data () };

	cl_int error = CL_SUCCESS;
	cl_program program = clCreateProgramWithSource(_priv::_ctx.context,
			SDL_arraysize(sources),
			sources,
			lengths,
			&error);
	_priv::checkError(error);
	return program;
}

bool deleteKernel(Id& kernel) {
	if (kernel == InvalidId) {
		return false;
	}
	const cl_int error = clReleaseKernel((cl_kernel)kernel);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		kernel = InvalidId;
		return true;
	}
	return false;
}

bool kernelArg(Id kernel, uint32_t index, const Texture& texture, int32_t samplerIndex) {
	if (kernel == InvalidId) {
		return false;
	}
	Log::debug("Set kernel arg for index %u to texture %p", index, texture.handle());
	Id textureId = texture.handle();
	cl_int error = clSetKernelArg((cl_kernel)kernel, index, sizeof(cl_mem), &textureId);
	_priv::checkError(error);
	if (samplerIndex >= 0) {
		Id samplerId = texture.sampler();
		error = clSetKernelArg((cl_kernel)kernel, samplerIndex, sizeof(cl_sampler), &samplerId);
		_priv::checkError(error);
	}
	return error == CL_SUCCESS;
}


bool kernelArg(Id kernel, uint32_t index, size_t size, const void* data) {
	if (kernel == InvalidId) {
		return false;
	}
	Log::debug("Set kernel arg for index %u", index);
	const cl_int error = clSetKernelArg((cl_kernel)kernel, index, size, data);
	_priv::checkError(error);
	return error == CL_SUCCESS;
}

/**
 * Work-group instances are executed in parallel across multiple compute units or concurrently
 * on the same compute unit.
 *
 * Each work-item is uniquely identified by a global identifier. The global ID, which can be read
 * inside the kernel, is computed using the value given by global_work_size and global_work_offset.
 * In OpenCL 1.0, the starting global ID is always (0, 0, ... 0). In addition, a work-item is also
 * identified within a work-group by a unique local ID. The local ID, which can also be read by the
 * kernel, is computed using the value given by local_work_size. The starting local ID is
 * always (0, 0, ... 0).
 *
 * @param[in] kernel A valid kernel object. The OpenCL context associated with kernel and commandQueue
 * must be the same.
 *
 * @param[in] workDim The number of dimensions used to specify the global work-items and work-items in the
 * work-group. work_dim must be greater than zero and less than or equal to three.
 * get_global_id(X) where the highest X is the workDim
 *
 * @param[in] workSize Points to an array of work_dim unsigned values that describe the number of global work-items
 * in workDim dimensions that will execute the kernel function. The total number of global
 * work-items is computed as global_work_size[0] *...* global_work_size[workDim - 1].
 * The values specified in global_work_size cannot exceed the range given by the sizeof(size_t)
 * for the device on which the kernel execution will be enqueued. The sizeof(size_t) for a
 * device can be determined using CL_DEVICE_ADDRESS_BITS in the table of OpenCL Device Queries
 * for clGetDeviceInfo. If, for example, CL_DEVICE_ADDRESS_BITS = 32, i.e. the device uses a
 * 32-bit address space, size_t is a 32-bit unsigned integer and global_work_size values must
 * be in the range 1 .. 2^32 - 1. Values outside this range return a CL_OUT_OF_RESOURCES error.
 */
bool kernelRun(Id kernel, const glm::ivec3& workSize, int workDim, bool blocking) {
	// TODO: check contraints/limits of the hardware for workSize and workDim
	if (kernel == InvalidId) {
		Log::error("Given kernel handle is invalid");
		return false;
	}
	core_assert_always(workDim > 0);
	core_assert_always(workDim <= 3);
	/**
	 * Returns an event object that identifies this particular kernel execution instance.
	 * Event objects are unique and can be used to identify a particular kernel execution
	 * instance later on. If event is NULL, no event will be created for this kernel
	 * execution instance and therefore it will not be possible for the application to
	 * query or queue a wait for this particular kernel execution instance.
	 */
	cl_event event = nullptr;
	cl_kernel clKernel = (cl_kernel) kernel;
	/**
	 * Must currently be a NULL value. In a future revision of OpenCL, global_work_offset can
	 * be used to specify an array of work_dim unsigned values that describe the offset used
	 * to calculate the global ID of a work-item instead of having the global IDs always start
	 * at offset (0, 0,... 0).
	 */
	const size_t *globalWorkOffset = nullptr;
	const size_t globalWorkSize[] = {(size_t)workSize.x, (size_t)workSize.y, (size_t)workSize.z};
	/**
	 * Points to an array of work_dim unsigned values that describe the number of work-items that
	 * make up a work-group (also referred to as the size of the work-group) that will execute the
	 * kernel specified by kernel. The total number of work-items in a work-group is computed as
	 * @c localWorkSize[0] *... * localWorkSize[work_dim - 1]. The total number of work-items in
	 * the work-group must be less than or equal to the CL_DEVICE_MAX_WORK_GROUP_SIZE value specified
	 * in table of OpenCL Device Queries for clGetDeviceInfo and the number of work-items specified
	 * in local_work_size[0],... local_work_size[work_dim - 1] must be less than or equal to the
	 * corresponding values specified
	 * by CL_DEVICE_MAX_WORK_ITEM_SIZES[0],.... CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1].
	 * The explicitly specified local_work_size will be used to determine how to break the global
	 * work-items specified by global_work_size into appropriate work-group instances. If
	 * local_work_size is specified, the values specified in
	 * global_work_size[0],... global_work_size[work_dim - 1] must be evenly divisible by the
	 * corresponding values specified in local_work_size[0],... local_work_size[work_dim - 1].
	 *
	 * The work-group size to be used for kernel can also be specified in the program source using
	 * the __attribute__((reqd_work_group_size(X, Y, Z)))qualifier. In this case the size of work
	 * group specified by local_work_size must match the value specified by the
	 * reqd_work_group_size __attribute__ qualifier.
	 *
	 * @c localWorkSize can also be a NULL value in which case the OpenCL implementation will
	 * determine how to be break the global work-items into appropriate work-group instances.
	 */
	const size_t *localWorkSize = nullptr;
	/**
	 * Specify events that need to complete before this particular command can be executed. If
	 * event_wait_list is NULL, then this particular command does not wait on any event to complete.
	 * If event_wait_list is NULL, num_events_in_wait_list must be 0. If event_wait_list is not NULL,
	 * the list of events pointed to by event_wait_list must be valid and num_events_in_wait_list
	 * must be greater than 0. The events specified in event_wait_list act as synchronization points.
	 * The context associated with events in event_wait_list and command_queue must be the same.
	 */
	const cl_uint numEventsInWaitList = 0;
	const cl_event* eventWaitList = nullptr;

	core_assert(_priv::_ctx.commandQueue != nullptr);
	const cl_int error = clEnqueueNDRangeKernel(_priv::_ctx.commandQueue,
			clKernel, workDim, globalWorkOffset, globalWorkSize,
			localWorkSize, numEventsInWaitList, eventWaitList, &event);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		if (blocking) {
			return finish();
		}
		return true;
	}
	return false;
}

Id createKernel(Id program, const char *name) {
	if (program == InvalidId) {
		return InvalidId;
	}
	core_assert(name != nullptr);
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateKernel.html
	cl_int error = CL_SUCCESS;
	cl_kernel kernel = clCreateKernel((cl_program)program, name, &error);
	_priv::checkError(error);
	if (error == CL_SUCCESS) {
		return kernel;
	}
	return InvalidId;
}

bool finish() {
	core_assert(_priv::_ctx.context != nullptr);
	core_assert(_priv::_ctx.commandQueue != nullptr);
	cl_int error = clFlush(_priv::_ctx.commandQueue);
	_priv::checkError(error);
	if (error != CL_SUCCESS) {
		return false;
	}
	error = clFinish(_priv::_ctx.commandQueue);
	_priv::checkError(error);
	return error == CL_SUCCESS;
}

bool supported() {
	return _priv::_ctx.context != nullptr;
}

template<typename T>
auto getActualDeviceInfo(int info) {
	T val = (T)0;
	const cl_int error = clGetDeviceInfo(_priv::_ctx.deviceId, (cl_device_info)info, sizeof(T), &val, nullptr);
	_priv::checkError(error);
	return val;
}

static bool extensionSupported(const char *extensions, const char *extension) {
	const char *where = SDL_strchr(extension, ' ');
	if (where || *extension == '\0') {
		return false;
	}

	if (extensions == nullptr) {
		return false;
	}

	const char *start = extensions;

	for (;;) {
		where = SDL_strstr(start, extension);
		if (where == nullptr) {
			break;
		}

		const char *terminator = where + SDL_strlen(extension);
		if (where == extensions || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				Log::info("Detected feature: %s", extension);
				return true;
			}
		}

		start = terminator;
	}
	return false;
}

bool init() {
	core_assert(_priv::_ctx.context == nullptr);
	if (computeCLInit() == -1) {
		Log::debug("Could not init opencl library");
		return false;
	}

	cl_int error;
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetPlatformIDs.html
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clGetDeviceIDs.html
	// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/clCreateContext.html
	error = clGetPlatformIDs(0, nullptr, &_priv::_ctx.platformIdCount);
#if cl_khr_icd
	if (error != CL_PLATFORM_NOT_FOUND_KHR)
#endif
	{
		_priv::checkError(error);
	}

	if (_priv::_ctx.platformIdCount == 0u) {
		Log::debug("No OpenCL platform found. Is the native runtime or driver installed?");
		return false;
	}

	_priv::_ctx.platformIds.reserve(_priv::_ctx.platformIdCount);
	error = clGetPlatformIDs(_priv::_ctx.platformIdCount,
			_priv::_ctx.platformIds.data(), nullptr);
	_priv::checkError(error);
	if (_priv::_ctx.platformIdCount == 0u) {
		Log::debug("Didn't find any OpenCL platforms");
		return false;
	}

	cl_uint platformIndex = 0;
	for (; platformIndex < _priv::_ctx.platformIdCount; ++platformIndex) {
		const std::string& platform = getPlatformName(_priv::_ctx.platformIds[platformIndex]);
		error = clGetDeviceIDs(_priv::_ctx.platformIds[platformIndex], CL_DEVICE_TYPE_ALL, 0,
				nullptr, &_priv::_ctx.deviceIdCount);
		if (error != CL_DEVICE_NOT_FOUND) {
			_priv::checkError(error);
		}

		Log::info("Found %u device(s) on platform %s", _priv::_ctx.deviceIdCount, platform.c_str());
		if (_priv::_ctx.deviceIdCount == 0u) {
			continue;
		}

		_priv::_ctx.deviceIds.reserve(_priv::_ctx.deviceIdCount);
		error = clGetDeviceIDs(_priv::_ctx.platformIds[platformIndex], CL_DEVICE_TYPE_ALL,
				_priv::_ctx.deviceIdCount, _priv::_ctx.deviceIds.data(), nullptr);
		_priv::checkError(error);

		error = clGetDeviceIDs(_priv::_ctx.platformIds[platformIndex], CL_DEVICE_TYPE_GPU, 1, &_priv::_ctx.deviceId, nullptr);
		if (error == CL_DEVICE_NOT_FOUND) {
			error = clGetDeviceIDs(_priv::_ctx.platformIds[platformIndex], CL_DEVICE_TYPE_DEFAULT, 1, &_priv::_ctx.deviceId, nullptr);
		}
		_priv::checkError(error);
		if (error != CL_SUCCESS) {
			Log::error("Failed to query the device on platform %s", platform.c_str());
			continue;
		}
		Log::info("Use platform %s (%i/%i)", platform.c_str(), platformIndex, _priv::_ctx.platformIdCount);
		break;
	}
	if (platformIndex >= _priv::_ctx.platformIdCount) {
		Log::debug("No valid OpenCL devices found");
		return false;
	}

	std::vector<cl_context_properties> contextProperties;
	contextProperties.reserve(3 + _priv::_ctx.externalProperties.size());
	// e.g. contributed properties by the computevideo submodule if you want to have gl-cl sharing
	core_assert(_priv::_ctx.externalProperties.size() % 2 == 0);
	for (auto& v : _priv::_ctx.externalProperties) {
		contextProperties.push_back(v);
	}
	contextProperties.push_back(CL_CONTEXT_PLATFORM);
	contextProperties.push_back((cl_context_properties)_priv::_ctx.platformIds[platformIndex]);
	contextProperties.push_back(0);
#ifdef cl_khr_gl_sharing
	if (_priv::_ctx.useGL && clGetGLContextInfoKHR) {
		cl_device_id interopDevice = nullptr;
		error = clGetGLContextInfoKHR(contextProperties.data(), CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &interopDevice, nullptr);
		_priv::checkError(error);
		if (error == CL_SUCCESS && _priv::_ctx.deviceId != interopDevice && interopDevice != nullptr) {
			Log::info("Use new device as gl/cl interop");
			_priv::_ctx.deviceId = interopDevice;
		}
	}
#endif

	const std::string& device = getDeviceInfo(_priv::_ctx.deviceId, CL_DEVICE_NAME);
	const std::string& vendor = getDeviceInfo(_priv::_ctx.deviceId, CL_DEVICE_VENDOR);
	const std::string& version = getDeviceInfo(_priv::_ctx.deviceId, CL_DRIVER_VERSION);
	const std::string& devVersion = getDeviceInfo(_priv::_ctx.deviceId, CL_DEVICE_VERSION);
	Log::info("CL_DEVICE_VENDOR: %s", vendor.c_str());
	Log::info("CL_DEVICE_NAME: %s", device.c_str());
	Log::info("CL_DEVICE_VERSION: %s", devVersion.c_str());
	Log::info("CL_DRIVER_VERSION: %s", version.c_str());
	size_t extensionSize;
	error = clGetDeviceInfo(_priv::_ctx.deviceId, CL_DEVICE_EXTENSIONS, 0, nullptr, &extensionSize);
	_priv::checkError(error);
	if (extensionSize > 0) {
		std::unique_ptr<char[]> extensions(new char[extensionSize + 1]);
		error = clGetDeviceInfo(_priv::_ctx.deviceId, CL_DEVICE_EXTENSIONS,	extensionSize, (void*)extensions.get(), &extensionSize);
		_priv::checkError(error);
		std::string extensionsStr = extensions.get();

		Log::info("OpenCL device extensions:");
		std::vector<std::string> extensionsVec;
		core::string::splitString(extensionsStr, extensionsVec, " ");
		for (const auto& e : extensionsVec) {
			Log::info("ext: %s", e.c_str());
		}

#if defined (__APPLE__)
		_priv::_ctx.features[std::enum_value(Feature::VideoSharingEvent)] = extensionSupported(extensions.get(), "cl_APPLE_gl_sharing");
#else
		_priv::_ctx.features[std::enum_value(Feature::VideoSharingEvent)] = extensionSupported(extensions.get(), "cl_khr_gl_event");
#endif
		_priv::_ctx.features[std::enum_value(Feature::VideoSharing)] = extensionSupported(extensions.get(), "cl_khr_gl_sharing");
		_priv::_ctx.features[std::enum_value(Feature::Write3dTextures)] = extensionSupported(extensions.get(), "cl_khr_3d_image_writes");
	}

	if (_priv::_ctx.useGL && !hasFeature(Feature::VideoSharing)) {
		Log::error("Missing video context sharing");
		return false;
	}

	_priv::_ctx.imageSupport = getActualDeviceInfo<cl_bool>(CL_DEVICE_IMAGE_SUPPORT);
	_priv::_ctx.image1DSize = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE_MAX_BUFFER_SIZE);
	_priv::_ctx.image2DSize[0] = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE2D_MAX_WIDTH);
	_priv::_ctx.image2DSize[1] = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE2D_MAX_HEIGHT);
	_priv::_ctx.image3DSize[0] = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE3D_MAX_WIDTH);
	_priv::_ctx.image3DSize[1] = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE3D_MAX_HEIGHT);
	_priv::_ctx.image3DSize[2] = getActualDeviceInfo<size_t>(CL_DEVICE_IMAGE3D_MAX_DEPTH);
	_priv::_ctx.maxComputeUnits = getActualDeviceInfo<size_t>(CL_DEVICE_MAX_COMPUTE_UNITS);
	Log::debug("Max compute units: %i", (int)_priv::_ctx.maxComputeUnits);

	error = clGetDeviceInfo(_priv::_ctx.deviceId,
			CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(_priv::_ctx.alignment), &_priv::_ctx.alignment, 0);
	_priv::checkError(error);
	if (error != CL_SUCCESS) {
		_priv::_ctx.alignment = 4096;
	} else {
		_priv::_ctx.alignment = core_max(sizeof(void*), size_t(_priv::_ctx.alignment));
	}
	Log::debug("Device memory alignment: %u", _priv::_ctx.alignment);

	error = CL_SUCCESS;

	const cl_context_properties* properties = contextProperties.data();
	_priv::_ctx.context = clCreateContext(properties, 1, &_priv::_ctx.deviceId, nullptr, nullptr, &error);
	if (_priv::_ctx.useGL && error != CL_SUCCESS) {
		_priv::_ctx.context = clCreateContextFromType(properties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, &error);
	}
	if (!_priv::_ctx.context) {
		Log::error("Failed to create the context");
		_priv::checkError(error);
		return false;
	}

	error = CL_SUCCESS;
#if 0
	// TODO: this segfaults on nvidia/linux
//#ifdef CL_VERSION_2_0
	// TODO: CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE??
	// TODO: CL_QUEUE_PROFILING_ENABLE
	cl_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
	/**
	 * Must be a device associated with context. It can either be in the list of devices specified when context
	 * is created using clCreateContext or have the same device type as the device type specified when the
	 * context is created using clCreateContextFromType.
	 */
	_priv::_ctx.commandQueue = clCreateCommandQueueWithProperties(
			_priv::_ctx.context, _priv::_ctx.deviceId, properties, &error);
#else
	_priv::_ctx.commandQueue = clCreateCommandQueue(
			_priv::_ctx.context, _priv::_ctx.deviceId, 0, &error);
#endif
	_priv::checkError(error);

	Log::info("OpenCL Context created");
	return true;
}

void shutdown() {
	if (_priv::_ctx.commandQueue != nullptr) {
		const cl_int error = clReleaseCommandQueue(_priv::_ctx.commandQueue);
		_priv::checkError(error);
	}
	if (_priv::_ctx.context != nullptr) {
		const cl_int error = clReleaseContext(_priv::_ctx.context);
		_priv::checkError(error);
	}
	_priv::_ctx = _priv::Context();
	computeCLShutdown();
}

bool hasFeature(Feature f) {
	return _priv::_ctx.supports(f);
}

}
