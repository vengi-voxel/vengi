/**
 * @file
 */
#pragma once

#include "CL.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#define WIN32_LEAN_AND_MEAN 1
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#define APIENTRY WINAPI
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef CLAPI
#define CLAPI extern
#endif

#if !cl_khr_icd
#define CL_PLATFORM_NOT_FOUND_KHR -1001
#endif

extern int computeCLInit();
extern void computeCLShutdown();

typedef cl_int (APIENTRY PFNCLGetPlatformIDs_PROC)(cl_uint, cl_platform_id *, cl_uint *);
typedef cl_int (APIENTRY PFNCLGetPlatformInfo_PROC)(cl_platform_id, cl_platform_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLGetDeviceIDs_PROC)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef cl_int (APIENTRY PFNCLGetDeviceInfo_PROC)(cl_device_id, cl_device_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLCreateSubDevices_PROC)(cl_device_id, const cl_device_partition_property *, cl_uint, cl_device_id *, cl_uint *);
typedef cl_int (APIENTRY PFNCLRetainDevice_PROC)(cl_device_id);
typedef cl_int (APIENTRY PFNCLReleaseDevice_PROC)(cl_device_id);
typedef cl_context (APIENTRY PFNCLCreateContext_PROC)(const cl_context_properties *, cl_uint, const cl_device_id *, void*, void *, cl_int *);
typedef cl_context (APIENTRY PFNCLCreateContextFromType_PROC)(const cl_context_properties *, cl_device_type, void*, void *, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainContext_PROC)(cl_context);
typedef cl_int (APIENTRY PFNCLReleaseContext_PROC)(cl_context);
typedef cl_int (APIENTRY PFNCLGetContextInfo_PROC)(cl_context, cl_context_info, size_t, void *, size_t *);
typedef cl_command_queue (APIENTRY PFNCLCreateCommandQueue_PROC)(cl_context, cl_device_id, cl_command_queue_properties, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainCommandQueue_PROC)(cl_command_queue);
typedef cl_int (APIENTRY PFNCLReleaseCommandQueue_PROC)(cl_command_queue);
typedef cl_int (APIENTRY PFNCLGetCommandQueueInfo_PROC)(cl_command_queue, cl_command_queue_info, size_t, void *, size_t *);
typedef cl_mem (APIENTRY PFNCLCreateBuffer_PROC)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef cl_mem (APIENTRY PFNCLCreateSubBuffer_PROC)(cl_mem, cl_mem_flags, cl_buffer_create_type, const void *, cl_int *);
typedef cl_mem (APIENTRY PFNCLCreateImage_PROC)(cl_context, cl_mem_flags, const cl_image_format *, const cl_image_desc *, void *, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainMemObject_PROC)(cl_mem);
typedef cl_int (APIENTRY PFNCLReleaseMemObject_PROC)(cl_mem);
typedef cl_int (APIENTRY PFNCLGetMemObjectInfo_PROC)(cl_mem, cl_mem_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLGetImageInfo_PROC)(cl_mem, cl_image_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLSetMemObjectDestructorCallback_PROC)(cl_mem, void (*pfn_notify)(cl_mem memobj, void* user_data), void *);
typedef cl_int (APIENTRY PFNCLGetSupportedImageFormats_PROC)(cl_context, cl_mem_flags, cl_mem_object_type, cl_uint, cl_image_format *, cl_uint *);
typedef cl_sampler (APIENTRY PFNCLCreateSampler_PROC)(cl_context, cl_bool, cl_addressing_mode, cl_filter_mode, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainSampler_PROC)(cl_sampler);
typedef cl_int (APIENTRY PFNCLReleaseSampler_PROC)(cl_sampler);
typedef cl_int (APIENTRY PFNCLGetSamplerInfo_PROC)(cl_sampler, cl_sampler_info, size_t, void *, size_t *);
typedef cl_program (APIENTRY PFNCLCreateProgramWithSource_PROC)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef cl_program (APIENTRY PFNCLCreateProgramWithBinary_PROC)(cl_context, cl_uint, const cl_device_id *, const size_t *, const unsigned char **, cl_int *,
		cl_int *);
typedef cl_program (APIENTRY PFNCLCreateProgramWithBuiltInKernels_PROC)(cl_context, cl_uint, const cl_device_id *, const char *, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainProgram_PROC)(cl_program);
typedef cl_int (APIENTRY PFNCLReleaseProgram_PROC)(cl_program);
typedef cl_int (APIENTRY PFNCLBuildProgram_PROC)(cl_program, cl_uint, const cl_device_id *, const char *, void (*pfn_notify)(cl_program program, void * user_data),
		void *);
typedef cl_int (APIENTRY PFNCLCompileProgram_PROC)(cl_program, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *, const char **,
		void (*pfn_notify)(cl_program program, void * user_data), void *);
typedef cl_program (APIENTRY PFNCLLinkProgram_PROC)(cl_context, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *,
		void (*pfn_notify)(cl_program program, void * user_data), void *, cl_int *);
typedef cl_int (APIENTRY PFNCLUnloadPlatformCompiler_PROC)(cl_platform_id);
typedef cl_int (APIENTRY PFNCLGetProgramInfo_PROC)(cl_program, cl_program_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLGetProgramBuildInfo_PROC)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
typedef cl_kernel (APIENTRY PFNCLCreateKernel_PROC)(cl_program, const char *, cl_int *);
typedef cl_int (APIENTRY PFNCLCreateKernelsInProgram_PROC)(cl_program, cl_uint, cl_kernel *, cl_uint *);
typedef cl_int (APIENTRY PFNCLRetainKernel_PROC)(cl_kernel);
typedef cl_int (APIENTRY PFNCLReleaseKernel_PROC)(cl_kernel);
typedef cl_int (APIENTRY PFNCLSetKernelArg_PROC)(cl_kernel, cl_uint, size_t, const void *);
typedef cl_int (APIENTRY PFNCLGetKernelInfo_PROC)(cl_kernel, cl_kernel_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLGetKernelArgInfo_PROC)(cl_kernel, cl_uint, cl_kernel_arg_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLGetKernelWorkGroupInfo_PROC)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLWaitForEvents_PROC)(cl_uint, const cl_event *);
typedef cl_int (APIENTRY PFNCLGetEventInfo_PROC)(cl_event, cl_event_info, size_t, void *, size_t *);
typedef cl_event (APIENTRY PFNCLCreateUserEvent_PROC)(cl_context, cl_int *);
typedef cl_int (APIENTRY PFNCLRetainEvent_PROC)(cl_event);
typedef cl_int (APIENTRY PFNCLReleaseEvent_PROC)(cl_event);
typedef cl_int (APIENTRY PFNCLSetUserEventStatus_PROC)(cl_event, cl_int);
typedef cl_int (APIENTRY PFNCLSetEventCallback_PROC)(cl_event, cl_int, void (*pfn_notify)(cl_event, cl_int, void *), void *);
typedef cl_int (APIENTRY PFNCLGetEventProfilingInfo_PROC)(cl_event, cl_profiling_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLFlush_PROC)(cl_command_queue);
typedef cl_int (APIENTRY PFNCLFinish_PROC)(cl_command_queue);
typedef cl_int (APIENTRY PFNCLEnqueueReadBuffer_PROC)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueReadBufferRect_PROC)(cl_command_queue, cl_mem, cl_bool, const size_t *, const size_t *, const size_t *, size_t, size_t, size_t,
		size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueWriteBuffer_PROC)(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueWriteBufferRect_PROC)(cl_command_queue, cl_mem, cl_bool, const size_t *, const size_t *, const size_t *, size_t, size_t, size_t,
		size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueFillBuffer_PROC)(cl_command_queue, cl_mem, const void *, size_t, size_t, size_t, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueCopyBuffer_PROC)(cl_command_queue, cl_mem, cl_mem, size_t, size_t, size_t, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueCopyBufferRect_PROC)(cl_command_queue, cl_mem, cl_mem, const size_t *, const size_t *, const size_t *, size_t, size_t, size_t,
		size_t, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueReadImage_PROC)(cl_command_queue, cl_mem, cl_bool, const size_t *, const size_t *, size_t, size_t, void *, cl_uint,
		const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueWriteImage_PROC)(cl_command_queue, cl_mem, cl_bool, const size_t *, const size_t *, size_t, size_t, const void *, cl_uint,
		const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueFillImage_PROC)(cl_command_queue, cl_mem, const void *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueCopyImage_PROC)(cl_command_queue, cl_mem, cl_mem, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *,
		cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueCopyImageToBuffer_PROC)(cl_command_queue, cl_mem, cl_mem, const size_t *, const size_t *, size_t, cl_uint, const cl_event *,
		cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueCopyBufferToImage_PROC)(cl_command_queue, cl_mem, cl_mem, size_t, const size_t *, const size_t *, cl_uint, const cl_event *,
		cl_event *);
typedef void * (APIENTRY PFNCLEnqueueMapBuffer_PROC)(cl_command_queue, cl_mem, cl_bool, cl_map_flags, size_t, size_t, cl_uint, const cl_event *, cl_event *,
		cl_int *);
typedef void * (APIENTRY PFNCLEnqueueMapImage_PROC)(cl_command_queue, cl_mem, cl_bool, cl_map_flags, const size_t *, const size_t *, size_t *, size_t *, cl_uint,
		const cl_event *, cl_event *, cl_int *);
typedef cl_int (APIENTRY PFNCLEnqueueUnmapMemObject_PROC)(cl_command_queue, cl_mem, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueMigrateMemObjects_PROC)(cl_command_queue, cl_uint, const cl_mem *, cl_mem_migration_flags, cl_uint, const cl_event *,
		cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueNDRangeKernel_PROC)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint,
		const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueTask_PROC)(cl_command_queue, cl_kernel, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueNativeKernel_PROC)(cl_command_queue, void (*user_func)(void *), void *, size_t, cl_uint, const cl_mem *, const void **, cl_uint,
		const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueMarkerWithWaitList_PROC)(cl_command_queue, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueBarrierWithWaitList_PROC)(cl_command_queue, cl_uint, const cl_event *, cl_event *);
typedef void * (APIENTRY PFNCLGetExtensionFunctionAddressForPlatform_PROC)(cl_platform_id, const char *);
typedef cl_mem (APIENTRY PFNCLCreateImage2D_PROC)(cl_context, cl_mem_flags, const cl_image_format *, size_t, size_t, size_t, void *, cl_int *);
typedef cl_mem (APIENTRY PFNCLCreateImage3D_PROC)(cl_context, cl_mem_flags, const cl_image_format *, size_t, size_t, size_t, size_t, size_t, void *, cl_int *);
typedef cl_int (APIENTRY PFNCLEnqueueMarker_PROC)(cl_command_queue, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueWaitForEvents_PROC)(cl_command_queue, cl_uint, const cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueBarrier_PROC)(cl_command_queue);
typedef cl_int (APIENTRY PFNCLUnloadCompiler_PROC)(void);
typedef void * (APIENTRY PFNCLGetExtensionFunctionAddress_PROC)(const char *);
typedef cl_int (APIENTRY PFNCLIcdGetPlatformIDs_PROC)(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms);
typedef cl_mem (APIENTRY PFNCLCreateFromGLBuffer_PROC)(cl_context, cl_mem_flags, cl_GLuint, int *);
typedef cl_mem (APIENTRY PFNCLCreateFromGLTexture_PROC)(cl_context, cl_mem_flags, cl_GLenum, cl_GLint, cl_GLuint, cl_int *);
typedef cl_mem (APIENTRY PFNCLCreateFromGLRenderbuffer_PROC)(cl_context, cl_mem_flags, cl_GLuint, cl_int *);
typedef cl_int (APIENTRY PFNCLGetGLObjectInfo_PROC)(cl_mem memobj, cl_gl_object_type *, cl_GLuint *);
typedef cl_int (APIENTRY PFNCLGetGLTextureInfo_PROC)(cl_mem, cl_gl_texture_info, size_t, void *, size_t *);
typedef cl_int (APIENTRY PFNCLEnqueueAcquireGLObjects_PROC)(cl_command_queue, cl_uint, const cl_mem *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (APIENTRY PFNCLEnqueueReleaseGLObjects_PROC)(cl_command_queue, cl_uint, const cl_mem *, cl_uint, const cl_event *, cl_event *);
typedef cl_mem (APIENTRY PFNCLCreateFromGLTexture2D_PROC)(cl_context, cl_mem_flags, cl_GLenum, cl_GLint, cl_GLuint, cl_int *);
typedef cl_mem (APIENTRY PFNCLCreateFromGLTexture3D_PROC)(cl_context, cl_mem_flags, cl_GLenum, cl_GLint, cl_GLuint, cl_int *);
#if cl_khr_gl_sharing
typedef cl_int (APIENTRY PFNCLGetGLContextInfoKHR_PROC)(const cl_context_properties *, cl_gl_context_info, size_t, void *, size_t *);
#endif

CLAPI PFNCLGetPlatformIDs_PROC* clpfGetPlatformIDs;
CLAPI PFNCLGetPlatformInfo_PROC* clpfGetPlatformInfo;
CLAPI PFNCLGetDeviceIDs_PROC* clpfGetDeviceIDs;
CLAPI PFNCLGetDeviceInfo_PROC* clpfGetDeviceInfo;
CLAPI PFNCLCreateSubDevices_PROC* clpfCreateSubDevices;
CLAPI PFNCLRetainDevice_PROC* clpfRetainDevice;
CLAPI PFNCLReleaseDevice_PROC* clpfReleaseDevice;
CLAPI PFNCLCreateContext_PROC* clpfCreateContext;
CLAPI PFNCLCreateContextFromType_PROC* clpfCreateContextFromType;
CLAPI PFNCLRetainContext_PROC* clpfRetainContext;
CLAPI PFNCLReleaseContext_PROC* clpfReleaseContext;
CLAPI PFNCLGetContextInfo_PROC* clpfGetContextInfo;
CLAPI PFNCLCreateCommandQueue_PROC* clpfCreateCommandQueue;
CLAPI PFNCLRetainCommandQueue_PROC* clpfRetainCommandQueue;
CLAPI PFNCLReleaseCommandQueue_PROC* clpfReleaseCommandQueue;
CLAPI PFNCLGetCommandQueueInfo_PROC* clpfGetCommandQueueInfo;
CLAPI PFNCLCreateBuffer_PROC* clpfCreateBuffer;
CLAPI PFNCLCreateSubBuffer_PROC* clpfCreateSubBuffer;
CLAPI PFNCLCreateImage_PROC* clpfCreateImage;
CLAPI PFNCLRetainMemObject_PROC* clpfRetainMemObject;
CLAPI PFNCLReleaseMemObject_PROC* clpfReleaseMemObject;
CLAPI PFNCLGetMemObjectInfo_PROC* clpfGetMemObjectInfo;
CLAPI PFNCLGetImageInfo_PROC* clpfGetImageInfo;
CLAPI PFNCLSetMemObjectDestructorCallback_PROC* clpfSetMemObjectDestructorCallback;
CLAPI PFNCLGetSupportedImageFormats_PROC* clpfGetSupportedImageFormats;
CLAPI PFNCLCreateSampler_PROC* clpfCreateSampler;
CLAPI PFNCLRetainSampler_PROC* clpfRetainSampler;
CLAPI PFNCLReleaseSampler_PROC* clpfReleaseSampler;
CLAPI PFNCLGetSamplerInfo_PROC* clpfGetSamplerInfo;
CLAPI PFNCLCreateProgramWithSource_PROC* clpfCreateProgramWithSource;
CLAPI PFNCLCreateProgramWithBinary_PROC* clpfCreateProgramWithBinary;
CLAPI PFNCLCreateProgramWithBuiltInKernels_PROC* clpfCreateProgramWithBuiltInKernels;
CLAPI PFNCLRetainProgram_PROC* clpfRetainProgram;
CLAPI PFNCLReleaseProgram_PROC* clpfReleaseProgram;
CLAPI PFNCLBuildProgram_PROC* clpfBuildProgram;
CLAPI PFNCLCompileProgram_PROC* clpfCompileProgram;
CLAPI PFNCLLinkProgram_PROC* clpfLinkProgram;
CLAPI PFNCLUnloadPlatformCompiler_PROC* clpfUnloadPlatformCompiler;
CLAPI PFNCLGetProgramInfo_PROC* clpfGetProgramInfo;
CLAPI PFNCLGetProgramBuildInfo_PROC* clpfGetProgramBuildInfo;
CLAPI PFNCLCreateKernel_PROC* clpfCreateKernel;
CLAPI PFNCLCreateKernelsInProgram_PROC* clpfCreateKernelsInProgram;
CLAPI PFNCLRetainKernel_PROC* clpfRetainKernel;
CLAPI PFNCLReleaseKernel_PROC* clpfReleaseKernel;
CLAPI PFNCLSetKernelArg_PROC* clpfSetKernelArg;
CLAPI PFNCLGetKernelInfo_PROC* clpfGetKernelInfo;
CLAPI PFNCLGetKernelArgInfo_PROC* clpfGetKernelArgInfo;
CLAPI PFNCLGetKernelWorkGroupInfo_PROC* clpfGetKernelWorkGroupInfo;
CLAPI PFNCLWaitForEvents_PROC* clpfWaitForEvents;
CLAPI PFNCLGetEventInfo_PROC* clpfGetEventInfo;
CLAPI PFNCLCreateUserEvent_PROC* clpfCreateUserEvent;
CLAPI PFNCLRetainEvent_PROC* clpfRetainEvent;
CLAPI PFNCLReleaseEvent_PROC* clpfReleaseEvent;
CLAPI PFNCLSetUserEventStatus_PROC* clpfSetUserEventStatus;
CLAPI PFNCLSetEventCallback_PROC* clpfSetEventCallback;
CLAPI PFNCLGetEventProfilingInfo_PROC* clpfGetEventProfilingInfo;
CLAPI PFNCLFlush_PROC* clpfFlush;
CLAPI PFNCLFinish_PROC* clpfFinish;
CLAPI PFNCLEnqueueReadBuffer_PROC* clpfEnqueueReadBuffer;
CLAPI PFNCLEnqueueReadBufferRect_PROC* clpfEnqueueReadBufferRect;
CLAPI PFNCLEnqueueWriteBuffer_PROC* clpfEnqueueWriteBuffer;
CLAPI PFNCLEnqueueWriteBufferRect_PROC* clpfEnqueueWriteBufferRect;
CLAPI PFNCLEnqueueFillBuffer_PROC* clpfEnqueueFillBuffer;
CLAPI PFNCLEnqueueCopyBuffer_PROC* clpfEnqueueCopyBuffer;
CLAPI PFNCLEnqueueCopyBufferRect_PROC* clpfEnqueueCopyBufferRect;
CLAPI PFNCLEnqueueReadImage_PROC* clpfEnqueueReadImage;
CLAPI PFNCLEnqueueWriteImage_PROC* clpfEnqueueWriteImage;
CLAPI PFNCLEnqueueFillImage_PROC* clpfEnqueueFillImage;
CLAPI PFNCLEnqueueCopyImage_PROC* clpfEnqueueCopyImage;
CLAPI PFNCLEnqueueCopyImageToBuffer_PROC* clpfEnqueueCopyImageToBuffer;
CLAPI PFNCLEnqueueCopyBufferToImage_PROC* clpfEnqueueCopyBufferToImage;
CLAPI PFNCLEnqueueMapBuffer_PROC* clpfEnqueueMapBuffer;
CLAPI PFNCLEnqueueMapImage_PROC* clpfEnqueueMapImage;
CLAPI PFNCLEnqueueUnmapMemObject_PROC* clpfEnqueueUnmapMemObject;
CLAPI PFNCLEnqueueMigrateMemObjects_PROC* clpfEnqueueMigrateMemObjects;
CLAPI PFNCLEnqueueNDRangeKernel_PROC* clpfEnqueueNDRangeKernel;
CLAPI PFNCLEnqueueTask_PROC* clpfEnqueueTask;
CLAPI PFNCLEnqueueNativeKernel_PROC* clpfEnqueueNativeKernel;
CLAPI PFNCLEnqueueMarkerWithWaitList_PROC* clpfEnqueueMarkerWithWaitList;
CLAPI PFNCLEnqueueBarrierWithWaitList_PROC* clpfEnqueueBarrierWithWaitList;
CLAPI PFNCLGetExtensionFunctionAddressForPlatform_PROC* clpfGetExtensionFunctionAddressForPlatform;
CLAPI PFNCLCreateImage2D_PROC* clpfCreateImage2D;
CLAPI PFNCLCreateImage3D_PROC* clpfCreateImage3D;
CLAPI PFNCLEnqueueMarker_PROC* clpfEnqueueMarker;
CLAPI PFNCLEnqueueWaitForEvents_PROC* clpfEnqueueWaitForEvents;
CLAPI PFNCLEnqueueBarrier_PROC* clpfEnqueueBarrier;
CLAPI PFNCLUnloadCompiler_PROC* clpfUnloadCompiler;
CLAPI PFNCLGetExtensionFunctionAddress_PROC* clpfGetExtensionFunctionAddress;
CLAPI PFNCLIcdGetPlatformIDs_PROC* clpfIcdGetPlatformIDs;
CLAPI PFNCLCreateFromGLBuffer_PROC* clpfCreateFromGLBuffer;
CLAPI PFNCLCreateFromGLTexture_PROC* clpfCreateFromGLTexture;
CLAPI PFNCLCreateFromGLRenderbuffer_PROC* clpfCreateFromGLRenderbuffer;
CLAPI PFNCLGetGLObjectInfo_PROC* clpfGetGLObjectInfo;
CLAPI PFNCLGetGLTextureInfo_PROC* clpfGetGLTextureInfo;
CLAPI PFNCLEnqueueAcquireGLObjects_PROC* clpfEnqueueAcquireGLObjects;
CLAPI PFNCLEnqueueReleaseGLObjects_PROC* clpfEnqueueReleaseGLObjects;
CLAPI PFNCLCreateFromGLTexture2D_PROC* clpfCreateFromGLTexture2D;
CLAPI PFNCLCreateFromGLTexture3D_PROC* clpfCreateFromGLTexture3D;
#if cl_khr_gl_sharing
CLAPI PFNCLGetGLContextInfoKHR_PROC* clpfGetGLContextInfoKHR;
#endif

#define clGetPlatformIDs clpfGetPlatformIDs
#define clGetPlatformInfo clpfGetPlatformInfo
#define clGetDeviceIDs clpfGetDeviceIDs
#define clGetDeviceInfo clpfGetDeviceInfo
#define clCreateSubDevices clpfCreateSubDevices
#define clRetainDevice clpfRetainDevice
#define clReleaseDevice clpfReleaseDevice
#define clCreateContext clpfCreateContext
#define clCreateContextFromType clpfCreateContextFromType
#define clRetainContext clpfRetainContext
#define clReleaseContext clpfReleaseContext
#define clGetContextInfo clpfGetContextInfo
#define clCreateCommandQueue clpfCreateCommandQueue
#define clRetainCommandQueue clpfRetainCommandQueue
#define clReleaseCommandQueue clpfReleaseCommandQueue
#define clGetCommandQueueInfo clpfGetCommandQueueInfo
#define clCreateBuffer clpfCreateBuffer
#define clCreateSubBuffer clpfCreateSubBuffer
#define clCreateImage clpfCreateImage
#define clRetainMemObject clpfRetainMemObject
#define clReleaseMemObject clpfReleaseMemObject
#define clGetMemObjectInfo clpfGetMemObjectInfo
#define clGetImageInfo clpfGetImageInfo
#define clSetMemObjectDestructorCallback clpfSetMemObjectDestructorCallback
#define clGetSupportedImageFormats clpfGetSupportedImageFormats
#define clCreateSampler clpfCreateSampler
#define clRetainSampler clpfRetainSampler
#define clReleaseSampler clpfReleaseSampler
#define clGetSamplerInfo clpfGetSamplerInfo
#define clCreateProgramWithSource clpfCreateProgramWithSource
#define clCreateProgramWithBinary clpfCreateProgramWithBinary
#define clCreateProgramWithBuiltInKernels clpfCreateProgramWithBuiltInKernels
#define clRetainProgram clpfRetainProgram
#define clReleaseProgram clpfReleaseProgram
#define clBuildProgram clpfBuildProgram
#define clCompileProgram clpfCompileProgram
#define clLinkProgram clpfLinkProgram
#define clUnloadPlatformCompiler clpfUnloadPlatformCompiler
#define clGetProgramInfo clpfGetProgramInfo
#define clGetProgramBuildInfo clpfGetProgramBuildInfo
#define clCreateKernel clpfCreateKernel
#define clCreateKernelsInProgram clpfCreateKernelsInProgram
#define clRetainKernel clpfRetainKernel
#define clReleaseKernel clpfReleaseKernel
#define clSetKernelArg clpfSetKernelArg
#define clGetKernelInfo clpfGetKernelInfo
#define clGetKernelArgInfo clpfGetKernelArgInfo
#define clGetKernelWorkGroupInfo clpfGetKernelWorkGroupInfo
#define clWaitForEvents clpfWaitForEvents
#define clGetEventInfo clpfGetEventInfo
#define clCreateUserEvent clpfCreateUserEvent
#define clRetainEvent clpfRetainEvent
#define clReleaseEvent clpfReleaseEvent
#define clSetUserEventStatus clpfSetUserEventStatus
#define clSetEventCallback clpfSetEventCallback
#define clGetEventProfilingInfo clpfGetEventProfilingInfo
#define clFlush clpfFlush
#define clFinish clpfFinish
#define clEnqueueReadBuffer clpfEnqueueReadBuffer
#define clEnqueueReadBufferRect clpfEnqueueReadBufferRect
#define clEnqueueWriteBuffer clpfEnqueueWriteBuffer
#define clEnqueueWriteBufferRect clpfEnqueueWriteBufferRect
#define clEnqueueFillBuffer clpfEnqueueFillBuffer
#define clEnqueueCopyBuffer clpfEnqueueCopyBuffer
#define clEnqueueCopyBufferRect clpfEnqueueCopyBufferRect
#define clEnqueueReadImage clpfEnqueueReadImage
#define clEnqueueWriteImage clpfEnqueueWriteImage
#define clEnqueueFillImage clpfEnqueueFillImage
#define clEnqueueCopyImage clpfEnqueueCopyImage
#define clEnqueueCopyImageToBuffer clpfEnqueueCopyImageToBuffer
#define clEnqueueCopyBufferToImage clpfEnqueueCopyBufferToImage
#define clEnqueueMapBuffer clpfEnqueueMapBuffer
#define clEnqueueMapImage clpfEnqueueMapImage
#define clEnqueueUnmapMemObject clpfEnqueueUnmapMemObject
#define clEnqueueMigrateMemObjects clpfEnqueueMigrateMemObjects
#define clEnqueueNDRangeKernel clpfEnqueueNDRangeKernel
#define clEnqueueTask clpfEnqueueTask
#define clEnqueueNativeKernel clpfEnqueueNativeKernel
#define clEnqueueMarkerWithWaitList clpfEnqueueMarkerWithWaitList
#define clEnqueueBarrierWithWaitList clpfEnqueueBarrierWithWaitList
#define clGetExtensionFunctionAddressForPlatform clpfGetExtensionFunctionAddressForPlatform
#define clCreateImage2D clpfCreateImage2D
#define clCreateImage3D clpfCreateImage3D
#define clEnqueueMarker clpfEnqueueMarker
#define clEnqueueWaitForEvents clpfEnqueueWaitForEvents
#define clEnqueueBarrier clpfEnqueueBarrier
#define clUnloadCompiler clpfUnloadCompiler
#define clGetExtensionFunctionAddress clpfGetExtensionFunctionAddress
#define clCreateFromGLBuffer clpfCreateFromGLBuffer
#define clCreateFromGLTexture clpfCreateFromGLTexture
#define clCreateFromGLRenderbuffer clpfCreateFromGLRenderbuffer
#define clGetGLObjectInfo clpfGetGLObjectInfo
#define clGetGLTextureInfo clpfGetGLTextureInfo
#define clEnqueueAcquireGLObjects clpfEnqueueAcquireGLObjects
#define clEnqueueReleaseGLObjects clpfEnqueueReleaseGLObjects
#define clCreateFromGLTexture2D clpfCreateFromGLTexture2D
#define clCreateFromGLTexture3D clpfCreateFromGLTexture3D
#define clGetGLContextInfoKHR clpfGetGLContextInfoKHR

#ifdef __cplusplus
}
#endif
