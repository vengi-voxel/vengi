/**
 * @file
 */
#include "CLSymbol.h"
#include <SDL.h>
#include <SDL_platform.h>
#include <stdio.h>

#if defined(__LINUX__) || defined(__ANDROID__)
#include <dirent.h>
#endif
#include "engine-config.h"

static void* obj = NULL;

#ifdef __cplusplus
extern "C" {
#endif

void computeCLShutdown() {
	SDL_UnloadObject(obj);

	clpfGetPlatformIDs = NULL;
	clpfGetPlatformInfo = NULL;
	clpfGetDeviceIDs = NULL;
	clpfGetDeviceInfo = NULL;
	clpfCreateSubDevices = NULL;
	clpfRetainDevice = NULL;
	clpfReleaseDevice = NULL;
	clpfCreateContext = NULL;
	clpfCreateContextFromType = NULL;
	clpfRetainContext = NULL;
	clpfReleaseContext = NULL;
	clpfGetContextInfo = NULL;
	clpfCreateCommandQueue = NULL;
	clpfRetainCommandQueue = NULL;
	clpfReleaseCommandQueue = NULL;
	clpfGetCommandQueueInfo = NULL;
	clpfCreateBuffer = NULL;
	clpfCreateSubBuffer = NULL;
	clpfCreateImage = NULL;
	clpfRetainMemObject = NULL;
	clpfReleaseMemObject = NULL;
	clpfGetMemObjectInfo = NULL;
	clpfGetImageInfo = NULL;
	clpfSetMemObjectDestructorCallback = NULL;
	clpfGetSupportedImageFormats = NULL;
	clpfCreateSampler = NULL;
	clpfRetainSampler = NULL;
	clpfReleaseSampler = NULL;
	clpfGetSamplerInfo = NULL;
	clpfCreateProgramWithSource = NULL;
	clpfCreateProgramWithBinary = NULL;
	clpfCreateProgramWithBuiltInKernels = NULL;
	clpfRetainProgram = NULL;
	clpfReleaseProgram = NULL;
	clpfBuildProgram = NULL;
	clpfCompileProgram = NULL;
	clpfLinkProgram = NULL;
	clpfUnloadPlatformCompiler = NULL;
	clpfGetProgramInfo = NULL;
	clpfGetProgramBuildInfo = NULL;
	clpfCreateKernel = NULL;
	clpfCreateKernelsInProgram = NULL;
	clpfRetainKernel = NULL;
	clpfReleaseKernel = NULL;
	clpfSetKernelArg = NULL;
	clpfGetKernelInfo = NULL;
	clpfGetKernelArgInfo = NULL;
	clpfGetKernelWorkGroupInfo = NULL;
	clpfWaitForEvents = NULL;
	clpfGetEventInfo = NULL;
	clpfCreateUserEvent = NULL;
	clpfRetainEvent = NULL;
	clpfReleaseEvent = NULL;
	clpfSetUserEventStatus = NULL;
	clpfSetEventCallback = NULL;
	clpfGetEventProfilingInfo = NULL;
	clpfFlush = NULL;
	clpfFinish = NULL;
	clpfEnqueueReadBuffer = NULL;
	clpfEnqueueReadBufferRect = NULL;
	clpfEnqueueWriteBuffer = NULL;
	clpfEnqueueWriteBufferRect = NULL;
	clpfEnqueueFillBuffer = NULL;
	clpfEnqueueCopyBuffer = NULL;
	clpfEnqueueCopyBufferRect = NULL;
	clpfEnqueueReadImage = NULL;
	clpfEnqueueWriteImage = NULL;
	clpfEnqueueFillImage = NULL;
	clpfEnqueueCopyImage = NULL;
	clpfEnqueueCopyImageToBuffer = NULL;
	clpfEnqueueCopyBufferToImage = NULL;
	clpfEnqueueMapBuffer = NULL;
	clpfEnqueueMapImage = NULL;
	clpfEnqueueUnmapMemObject = NULL;
	clpfEnqueueMigrateMemObjects = NULL;
	clpfEnqueueNDRangeKernel = NULL;
	clpfEnqueueTask = NULL;
	clpfEnqueueNativeKernel = NULL;
	clpfEnqueueMarkerWithWaitList = NULL;
	clpfEnqueueBarrierWithWaitList = NULL;
	clpfGetExtensionFunctionAddressForPlatform = NULL;
	clpfCreateImage2D = NULL;
	clpfCreateImage3D = NULL;
	clpfEnqueueMarker = NULL;
	clpfEnqueueWaitForEvents = NULL;
	clpfEnqueueBarrier = NULL;
	clpfUnloadCompiler = NULL;
	clpfGetExtensionFunctionAddress = NULL;
	clpfIcdGetPlatformIDs = NULL;
	clpfCreateFromGLBuffer = NULL;
	clpfCreateFromGLTexture = NULL;
	clpfCreateFromGLRenderbuffer = NULL;
	clpfGetGLObjectInfo = NULL;
	clpfGetGLTextureInfo = NULL;
	clpfEnqueueAcquireGLObjects = NULL;
	clpfEnqueueReleaseGLObjects = NULL;
	clpfCreateFromGLTexture2D = NULL;
	clpfCreateFromGLTexture3D = NULL;
#if cl_khr_gl_sharing
	clpfGetGLContextInfoKHR = NULL;
#endif
}

#if defined(__MACOSX__)
static const char *default_so_paths[] = {
	"/System/Library/Frameworks/OpenCL.framework/OpenCL",
	"libOpenCL.so"
};
#elif defined(__ANDROID__)
static const char *default_so_paths[] = {
	"libOpenCL.so",
	"/system/lib/libOpenCL.so",
	"/system/vendor/lib/libOpenCL.so",
	"/system/vendor/lib/egl/libGLES_mali.so",
	"/system/vendor/lib/libPVROCL.so",
	"/data/data/org.pocl.libs/files/lib/libpocl.so"
};
#elif defined(__WINDOWS__)
static const char *default_so_paths[] = {
	"OpenCL.dll"
};
#elif defined(__LINUX__)
static const char *default_so_paths[] = {
	"libOpenCL.so",
	"libOpenCL.so.0",
	"libOpenCL.so.1",
	"libOpenCL.so.2"
};
#else
#error "Unsupported platform"
#endif

#if defined(__ANDROID__) || defined(__LINUX__)
static void* loadICDLinux() {
#ifdef __ANDROID__
	const char *vendorPath = "/system/vendor/Khronos/OpenCL/vendors/";
#else
	const char *vendorPath = "/etc/OpenCL/vendors/";
#endif
	struct dirent *entry = NULL;
	DIR *dir = opendir(vendorPath);
	if (NULL == dir) {
		return NULL;
	}
	while ((entry = readdir(dir)) != NULL) {
		switch (entry->d_type) {
		case DT_UNKNOWN:
		case DT_REG:
		case DT_LNK: {
			const char *ext = SDL_strrchr(entry->d_name, '.');
			if (ext == NULL) {
				continue;
			}
			if (SDL_strcmp(ext, ".icd") != 0) {
				continue;
			}
			char buf[512];
			SDL_snprintf(buf, sizeof(buf), "%s%s", vendorPath, entry->d_name);
			buf[511] = '\0';

			FILE *icd = fopen(buf, "r");
			if (icd == NULL) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Can't open file %s", buf);
				break;
			}
			fseek(icd, 0, SEEK_END);
			const long icdSize = ftell(icd);
			if ((size_t)icdSize >= sizeof(buf) - 1) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "File content of '%s' doesn't fit buffer", buf);
				fclose(icd);
				break;
			}
			fseek(icd, 0, SEEK_SET);
			if (icdSize != (long) fread(buf, 1, icdSize, icd)) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Could not read the whole icd file");
				fclose(icd);
				break;
			}
			fclose(icd);
			if (buf[icdSize - 1] == '\n') {
				buf[icdSize - 1] = '\0';
			}
			void* lib = SDL_LoadObject(buf);
			if (lib != NULL) {
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Loaded OpenCL library '%s'", buf);
				closedir(dir);
				return lib;
			}
			SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Could not load the specified library '%s'", buf);
			break;
		}
		default:
			break;
		}
	}

	closedir(dir);
	return NULL;
}
#endif

static void* loadICD() {
#if defined(__ANDROID__) || defined(__LINUX__)
	return loadICDLinux();
#else
	return NULL;
#endif
}

int computeCLInit() {
	const int n = SDL_arraysize(default_so_paths);
	int i;

#ifdef OPENCL_LIBRARY
	obj = SDL_LoadObject(OPENCL_LIBRARY);
#else
	obj = NULL;
#endif
	if (obj == NULL) {
		for (i = 0; i < n; ++i) {
			obj = SDL_LoadObject(default_so_paths[i]);
			if (obj != NULL) {
				break;
			}
		}
	}

	if (obj == NULL) {
		// some vendors also exports all symbols - try that as a last resort
		obj = loadICD();
	}

	if (obj == NULL) {
		return -1;
	}

	clpfGetExtensionFunctionAddress = (PFNCLGetExtensionFunctionAddress_PROC*)SDL_LoadFunction(obj, "clGetExtensionFunctionAddress");
	if (clpfGetExtensionFunctionAddress == NULL) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Given OpenCL library doesn't contain needed symbol clGetExtensionFunctionAddress");
		return -1;
	}

#if 0
	clpfIcdGetPlatformIDs = (PFNCLIcdGetPlatformIDs_PROC*)clpfGetExtensionFunctionAddress("clIcdGetPlatformIDsKHR");
	if (clpfIcdGetPlatformIDs == NULL) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Given OpenCL library doesn't contain needed symbol clIcdGetPlatformIDsKHR");
		return -1;
	}

	cl_uint platformCount;
	cl_int result = clpfIcdGetPlatformIDs(0, NULL, &platformCount);
	if (result != CL_SUCCESS) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Failed to execute clIcdGetPlatformIDs");
		return -1;
	}
	cl_platform_id* platforms = (cl_platform_id *)SDL_malloc(platformCount * sizeof(cl_platform_id));
	if (platforms == NULL) {
		return SDL_OutOfMemory();
	}
	SDL_memset(platforms, 0, platformCount * sizeof(*platforms));
	result = clpfIcdGetPlatformIDs(platformCount, platforms, NULL);
	if (result != CL_SUCCESS) {
		SDL_free(platforms);
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Failed to execute clIcdGetPlatformIDs to get the data");
		return -1;
	}
	SDL_free(platforms);
#endif

	clpfGetPlatformIDs = (PFNCLGetPlatformIDs_PROC*)SDL_LoadFunction(obj, "clGetPlatformIDs");
	clpfGetPlatformInfo = (PFNCLGetPlatformInfo_PROC*)SDL_LoadFunction(obj, "clGetPlatformInfo");
	clpfGetDeviceIDs = (PFNCLGetDeviceIDs_PROC*)SDL_LoadFunction(obj, "clGetDeviceIDs");
	clpfGetDeviceInfo = (PFNCLGetDeviceInfo_PROC*)SDL_LoadFunction(obj, "clGetDeviceInfo");
	clpfCreateSubDevices = (PFNCLCreateSubDevices_PROC*)SDL_LoadFunction(obj, "clCreateSubDevices");
	clpfRetainDevice = (PFNCLRetainDevice_PROC*)SDL_LoadFunction(obj, "clRetainDevice");
	clpfReleaseDevice = (PFNCLReleaseDevice_PROC*)SDL_LoadFunction(obj, "clReleaseDevice");
	clpfCreateContext = (PFNCLCreateContext_PROC*)SDL_LoadFunction(obj, "clCreateContext");
	clpfCreateContextFromType = (PFNCLCreateContextFromType_PROC*)SDL_LoadFunction(obj, "clCreateContextFromType");
	clpfRetainContext = (PFNCLRetainContext_PROC*)SDL_LoadFunction(obj, "clRetainContext");
	clpfReleaseContext = (PFNCLReleaseContext_PROC*)SDL_LoadFunction(obj, "clReleaseContext");
	clpfGetContextInfo = (PFNCLGetContextInfo_PROC*)SDL_LoadFunction(obj, "clGetContextInfo");
	clpfCreateCommandQueue = (PFNCLCreateCommandQueue_PROC*)SDL_LoadFunction(obj, "clCreateCommandQueue");
	clpfRetainCommandQueue = (PFNCLRetainCommandQueue_PROC*)SDL_LoadFunction(obj, "clRetainCommandQueue");
	clpfReleaseCommandQueue = (PFNCLReleaseCommandQueue_PROC*)SDL_LoadFunction(obj, "clReleaseCommandQueue");
	clpfGetCommandQueueInfo = (PFNCLGetCommandQueueInfo_PROC*)SDL_LoadFunction(obj, "clGetCommandQueueInfo");
	clpfCreateBuffer = (PFNCLCreateBuffer_PROC*)SDL_LoadFunction(obj, "clCreateBuffer");
	clpfCreateSubBuffer = (PFNCLCreateSubBuffer_PROC*)SDL_LoadFunction(obj, "clCreateSubBuffer");
	clpfCreateImage = (PFNCLCreateImage_PROC*)SDL_LoadFunction(obj, "clCreateImage");
	clpfRetainMemObject = (PFNCLRetainMemObject_PROC*)SDL_LoadFunction(obj, "clRetainMemObject");
	clpfReleaseMemObject = (PFNCLReleaseMemObject_PROC*)SDL_LoadFunction(obj, "clReleaseMemObject");
	clpfGetMemObjectInfo = (PFNCLGetMemObjectInfo_PROC*)SDL_LoadFunction(obj, "clGetMemObjectInfo");
	clpfGetImageInfo = (PFNCLGetImageInfo_PROC*)SDL_LoadFunction(obj, "clGetImageInfo");
	clpfSetMemObjectDestructorCallback = (PFNCLSetMemObjectDestructorCallback_PROC*)SDL_LoadFunction(obj, "clSetMemObjectDestructorCallback");
	clpfGetSupportedImageFormats = (PFNCLGetSupportedImageFormats_PROC*)SDL_LoadFunction(obj, "clGetSupportedImageFormats");
	clpfCreateSampler = (PFNCLCreateSampler_PROC*)SDL_LoadFunction(obj, "clCreateSampler");
	clpfRetainSampler = (PFNCLRetainSampler_PROC*)SDL_LoadFunction(obj, "clRetainSampler");
	clpfReleaseSampler = (PFNCLReleaseSampler_PROC*)SDL_LoadFunction(obj, "clReleaseSampler");
	clpfGetSamplerInfo = (PFNCLGetSamplerInfo_PROC*)SDL_LoadFunction(obj, "clGetSamplerInfo");
	clpfCreateProgramWithSource = (PFNCLCreateProgramWithSource_PROC*)SDL_LoadFunction(obj, "clCreateProgramWithSource");
	clpfCreateProgramWithBinary = (PFNCLCreateProgramWithBinary_PROC*)SDL_LoadFunction(obj, "clCreateProgramWithBinary");
	clpfCreateProgramWithBuiltInKernels = (PFNCLCreateProgramWithBuiltInKernels_PROC*)SDL_LoadFunction(obj, "clCreateProgramWithBuiltInKernels");
	clpfRetainProgram = (PFNCLRetainProgram_PROC*)SDL_LoadFunction(obj, "clRetainProgram");
	clpfReleaseProgram = (PFNCLReleaseProgram_PROC*)SDL_LoadFunction(obj, "clReleaseProgram");
	clpfBuildProgram = (PFNCLBuildProgram_PROC*)SDL_LoadFunction(obj, "clBuildProgram");
	clpfCompileProgram = (PFNCLCompileProgram_PROC*)SDL_LoadFunction(obj, "clCompileProgram");
	clpfLinkProgram = (PFNCLLinkProgram_PROC*)SDL_LoadFunction(obj, "clLinkProgram");
	clpfUnloadPlatformCompiler = (PFNCLUnloadPlatformCompiler_PROC*)SDL_LoadFunction(obj, "clUnloadPlatformCompiler");
	clpfGetProgramInfo = (PFNCLGetProgramInfo_PROC*)SDL_LoadFunction(obj, "clGetProgramInfo");
	clpfGetProgramBuildInfo = (PFNCLGetProgramBuildInfo_PROC*)SDL_LoadFunction(obj, "clGetProgramBuildInfo");
	clpfCreateKernel = (PFNCLCreateKernel_PROC*)SDL_LoadFunction(obj, "clCreateKernel");
	clpfCreateKernelsInProgram = (PFNCLCreateKernelsInProgram_PROC*)SDL_LoadFunction(obj, "clCreateKernelsInProgram");
	clpfRetainKernel = (PFNCLRetainKernel_PROC*)SDL_LoadFunction(obj, "clRetainKernel");
	clpfReleaseKernel = (PFNCLReleaseKernel_PROC*)SDL_LoadFunction(obj, "clReleaseKernel");
	clpfSetKernelArg = (PFNCLSetKernelArg_PROC*)SDL_LoadFunction(obj, "clSetKernelArg");
	clpfGetKernelInfo = (PFNCLGetKernelInfo_PROC*)SDL_LoadFunction(obj, "clGetKernelInfo");
	clpfGetKernelArgInfo = (PFNCLGetKernelArgInfo_PROC*)SDL_LoadFunction(obj, "clGetKernelArgInfo");
	clpfGetKernelWorkGroupInfo = (PFNCLGetKernelWorkGroupInfo_PROC*)SDL_LoadFunction(obj, "clGetKernelWorkGroupInfo");
	clpfWaitForEvents = (PFNCLWaitForEvents_PROC*)SDL_LoadFunction(obj, "clWaitForEvents");
	clpfGetEventInfo = (PFNCLGetEventInfo_PROC*)SDL_LoadFunction(obj, "clGetEventInfo");
	clpfCreateUserEvent = (PFNCLCreateUserEvent_PROC*)SDL_LoadFunction(obj, "clCreateUserEvent");
	clpfRetainEvent = (PFNCLRetainEvent_PROC*)SDL_LoadFunction(obj, "clRetainEvent");
	clpfReleaseEvent = (PFNCLReleaseEvent_PROC*)SDL_LoadFunction(obj, "clReleaseEvent");
	clpfSetUserEventStatus = (PFNCLSetUserEventStatus_PROC*)SDL_LoadFunction(obj, "clSetUserEventStatus");
	clpfSetEventCallback = (PFNCLSetEventCallback_PROC*)SDL_LoadFunction(obj, "clSetEventCallback");
	clpfGetEventProfilingInfo = (PFNCLGetEventProfilingInfo_PROC*)SDL_LoadFunction(obj, "clGetEventProfilingInfo");
	clpfFlush = (PFNCLFlush_PROC*)SDL_LoadFunction(obj, "clFlush");
	clpfFinish = (PFNCLFinish_PROC*)SDL_LoadFunction(obj, "clFinish");
	clpfEnqueueReadBuffer = (PFNCLEnqueueReadBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueReadBuffer");
	clpfEnqueueReadBufferRect = (PFNCLEnqueueReadBufferRect_PROC*)SDL_LoadFunction(obj, "clEnqueueReadBufferRect");
	clpfEnqueueWriteBuffer = (PFNCLEnqueueWriteBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueWriteBuffer");
	clpfEnqueueWriteBufferRect = (PFNCLEnqueueWriteBufferRect_PROC*)SDL_LoadFunction(obj, "clEnqueueWriteBufferRect");
	clpfEnqueueFillBuffer = (PFNCLEnqueueFillBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueFillBuffer");
	clpfEnqueueCopyBuffer = (PFNCLEnqueueCopyBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueCopyBuffer");
	clpfEnqueueCopyBufferRect = (PFNCLEnqueueCopyBufferRect_PROC*)SDL_LoadFunction(obj, "clEnqueueCopyBufferRect");
	clpfEnqueueReadImage = (PFNCLEnqueueReadImage_PROC*)SDL_LoadFunction(obj, "clEnqueueReadImage");
	clpfEnqueueWriteImage = (PFNCLEnqueueWriteImage_PROC*)SDL_LoadFunction(obj, "clEnqueueWriteImage");
	clpfEnqueueFillImage = (PFNCLEnqueueFillImage_PROC*)SDL_LoadFunction(obj, "clEnqueueFillImage");
	clpfEnqueueCopyImage = (PFNCLEnqueueCopyImage_PROC*)SDL_LoadFunction(obj, "clEnqueueCopyImage");
	clpfEnqueueCopyImageToBuffer = (PFNCLEnqueueCopyImageToBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueCopyImageToBuffer");
	clpfEnqueueCopyBufferToImage = (PFNCLEnqueueCopyBufferToImage_PROC*)SDL_LoadFunction(obj, "clEnqueueCopyBufferToImage");
	clpfEnqueueMapBuffer = (PFNCLEnqueueMapBuffer_PROC*)SDL_LoadFunction(obj, "clEnqueueMapBuffer");
	clpfEnqueueMapImage = (PFNCLEnqueueMapImage_PROC*)SDL_LoadFunction(obj, "clEnqueueMapImage");
	clpfEnqueueUnmapMemObject = (PFNCLEnqueueUnmapMemObject_PROC*)SDL_LoadFunction(obj, "clEnqueueUnmapMemObject");
	clpfEnqueueMigrateMemObjects = (PFNCLEnqueueMigrateMemObjects_PROC*)SDL_LoadFunction(obj, "clEnqueueMigrateMemObjects");
	clpfEnqueueNDRangeKernel = (PFNCLEnqueueNDRangeKernel_PROC*)SDL_LoadFunction(obj, "clEnqueueNDRangeKernel");
	clpfEnqueueTask = (PFNCLEnqueueTask_PROC*)SDL_LoadFunction(obj, "clEnqueueTask");
	clpfEnqueueNativeKernel = (PFNCLEnqueueNativeKernel_PROC*)SDL_LoadFunction(obj, "clEnqueueNativeKernel");
	clpfEnqueueMarkerWithWaitList = (PFNCLEnqueueMarkerWithWaitList_PROC*)SDL_LoadFunction(obj, "clEnqueueMarkerWithWaitList");
	clpfEnqueueBarrierWithWaitList = (PFNCLEnqueueBarrierWithWaitList_PROC*)SDL_LoadFunction(obj, "clEnqueueBarrierWithWaitList");
	clpfGetExtensionFunctionAddressForPlatform = (PFNCLGetExtensionFunctionAddressForPlatform_PROC*)SDL_LoadFunction(obj, "clGetExtensionFunctionAddressForPlatform");
	// deprecated
	clpfCreateImage2D = (PFNCLCreateImage2D_PROC*)SDL_LoadFunction(obj, "clCreateImage2D");
	// deprecated
	clpfCreateImage3D = (PFNCLCreateImage3D_PROC*)SDL_LoadFunction(obj, "clCreateImage3D");
	clpfEnqueueMarker = (PFNCLEnqueueMarker_PROC*)SDL_LoadFunction(obj, "clEnqueueMarker");
	clpfEnqueueWaitForEvents = (PFNCLEnqueueWaitForEvents_PROC*)SDL_LoadFunction(obj, "clEnqueueWaitForEvents");
	clpfEnqueueBarrier = (PFNCLEnqueueBarrier_PROC*)SDL_LoadFunction(obj, "clEnqueueBarrier");
	clpfUnloadCompiler = (PFNCLUnloadCompiler_PROC*)SDL_LoadFunction(obj, "clUnloadCompiler");
	clpfCreateFromGLBuffer = (PFNCLCreateFromGLBuffer_PROC*)SDL_LoadFunction(obj, "clCreateFromGLBuffer");
	clpfCreateFromGLTexture = (PFNCLCreateFromGLTexture_PROC*)SDL_LoadFunction(obj, "clCreateFromGLTexture");
	clpfCreateFromGLRenderbuffer = (PFNCLCreateFromGLRenderbuffer_PROC*)SDL_LoadFunction(obj, "clCreateFromGLRenderbuffer");
	clpfGetGLObjectInfo = (PFNCLGetGLObjectInfo_PROC*)SDL_LoadFunction(obj, "clGetGLObjectInfo");
	clpfGetGLTextureInfo = (PFNCLGetGLTextureInfo_PROC*)SDL_LoadFunction(obj, "clGetGLTextureInfo");
	clpfEnqueueAcquireGLObjects = (PFNCLEnqueueAcquireGLObjects_PROC*)SDL_LoadFunction(obj, "clEnqueueAcquireGLObjects");
	clpfEnqueueReleaseGLObjects = (PFNCLEnqueueReleaseGLObjects_PROC*)SDL_LoadFunction(obj, "clEnqueueReleaseGLObjects");
	// deprecated
	clpfCreateFromGLTexture2D = (PFNCLCreateFromGLTexture2D_PROC*)SDL_LoadFunction(obj, "clCreateFromGLTexture2D");
	// deprecated
	clpfCreateFromGLTexture3D = (PFNCLCreateFromGLTexture3D_PROC*)SDL_LoadFunction(obj, "clCreateFromGLTexture3D");
#if cl_khr_gl_sharing
	clpfGetGLContextInfoKHR = (PFNCLGetGLContextInfoKHR_PROC*)SDL_LoadFunction(obj, "clGetGLContextInfoKHR");
#endif
	if (clGetPlatformIDs == NULL) {
		SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Given OpenCL library doesn't contain needed symbols");
		return -1;
	}

	return 0;
}

PFNCLGetPlatformIDs_PROC* clpfGetPlatformIDs = NULL;
PFNCLGetPlatformInfo_PROC* clpfGetPlatformInfo = NULL;
PFNCLGetDeviceIDs_PROC* clpfGetDeviceIDs = NULL;
PFNCLGetDeviceInfo_PROC* clpfGetDeviceInfo = NULL;
PFNCLCreateSubDevices_PROC* clpfCreateSubDevices = NULL;
PFNCLRetainDevice_PROC* clpfRetainDevice = NULL;
PFNCLReleaseDevice_PROC* clpfReleaseDevice = NULL;
PFNCLCreateContext_PROC* clpfCreateContext = NULL;
PFNCLCreateContextFromType_PROC* clpfCreateContextFromType = NULL;
PFNCLRetainContext_PROC* clpfRetainContext = NULL;
PFNCLReleaseContext_PROC* clpfReleaseContext = NULL;
PFNCLGetContextInfo_PROC* clpfGetContextInfo = NULL;
PFNCLCreateCommandQueue_PROC* clpfCreateCommandQueue = NULL;
PFNCLRetainCommandQueue_PROC* clpfRetainCommandQueue = NULL;
PFNCLReleaseCommandQueue_PROC* clpfReleaseCommandQueue = NULL;
PFNCLGetCommandQueueInfo_PROC* clpfGetCommandQueueInfo = NULL;
PFNCLCreateBuffer_PROC* clpfCreateBuffer = NULL;
PFNCLCreateSubBuffer_PROC* clpfCreateSubBuffer = NULL;
PFNCLCreateImage_PROC* clpfCreateImage = NULL;
PFNCLRetainMemObject_PROC* clpfRetainMemObject = NULL;
PFNCLReleaseMemObject_PROC* clpfReleaseMemObject = NULL;
PFNCLGetMemObjectInfo_PROC* clpfGetMemObjectInfo = NULL;
PFNCLGetImageInfo_PROC* clpfGetImageInfo = NULL;
PFNCLSetMemObjectDestructorCallback_PROC* clpfSetMemObjectDestructorCallback = NULL;
PFNCLGetSupportedImageFormats_PROC* clpfGetSupportedImageFormats = NULL;
PFNCLCreateSampler_PROC* clpfCreateSampler = NULL;
PFNCLRetainSampler_PROC* clpfRetainSampler = NULL;
PFNCLReleaseSampler_PROC* clpfReleaseSampler = NULL;
PFNCLGetSamplerInfo_PROC* clpfGetSamplerInfo = NULL;
PFNCLCreateProgramWithSource_PROC* clpfCreateProgramWithSource = NULL;
PFNCLCreateProgramWithBinary_PROC* clpfCreateProgramWithBinary = NULL;
PFNCLCreateProgramWithBuiltInKernels_PROC* clpfCreateProgramWithBuiltInKernels = NULL;
PFNCLRetainProgram_PROC* clpfRetainProgram = NULL;
PFNCLReleaseProgram_PROC* clpfReleaseProgram = NULL;
PFNCLBuildProgram_PROC* clpfBuildProgram = NULL;
PFNCLCompileProgram_PROC* clpfCompileProgram = NULL;
PFNCLLinkProgram_PROC* clpfLinkProgram = NULL;
PFNCLUnloadPlatformCompiler_PROC* clpfUnloadPlatformCompiler = NULL;
PFNCLGetProgramInfo_PROC* clpfGetProgramInfo = NULL;
PFNCLGetProgramBuildInfo_PROC* clpfGetProgramBuildInfo = NULL;
PFNCLCreateKernel_PROC* clpfCreateKernel = NULL;
PFNCLCreateKernelsInProgram_PROC* clpfCreateKernelsInProgram = NULL;
PFNCLRetainKernel_PROC* clpfRetainKernel = NULL;
PFNCLReleaseKernel_PROC* clpfReleaseKernel = NULL;
PFNCLSetKernelArg_PROC* clpfSetKernelArg = NULL;
PFNCLGetKernelInfo_PROC* clpfGetKernelInfo = NULL;
PFNCLGetKernelArgInfo_PROC* clpfGetKernelArgInfo = NULL;
PFNCLGetKernelWorkGroupInfo_PROC* clpfGetKernelWorkGroupInfo = NULL;
PFNCLWaitForEvents_PROC* clpfWaitForEvents = NULL;
PFNCLGetEventInfo_PROC* clpfGetEventInfo = NULL;
PFNCLCreateUserEvent_PROC* clpfCreateUserEvent = NULL;
PFNCLRetainEvent_PROC* clpfRetainEvent = NULL;
PFNCLReleaseEvent_PROC* clpfReleaseEvent = NULL;
PFNCLSetUserEventStatus_PROC* clpfSetUserEventStatus = NULL;
PFNCLSetEventCallback_PROC* clpfSetEventCallback = NULL;
PFNCLGetEventProfilingInfo_PROC* clpfGetEventProfilingInfo = NULL;
PFNCLFlush_PROC* clpfFlush = NULL;
PFNCLFinish_PROC* clpfFinish = NULL;
PFNCLEnqueueReadBuffer_PROC* clpfEnqueueReadBuffer = NULL;
PFNCLEnqueueReadBufferRect_PROC* clpfEnqueueReadBufferRect = NULL;
PFNCLEnqueueWriteBuffer_PROC* clpfEnqueueWriteBuffer = NULL;
PFNCLEnqueueWriteBufferRect_PROC* clpfEnqueueWriteBufferRect = NULL;
PFNCLEnqueueFillBuffer_PROC* clpfEnqueueFillBuffer = NULL;
PFNCLEnqueueCopyBuffer_PROC* clpfEnqueueCopyBuffer = NULL;
PFNCLEnqueueCopyBufferRect_PROC* clpfEnqueueCopyBufferRect = NULL;
PFNCLEnqueueReadImage_PROC* clpfEnqueueReadImage = NULL;
PFNCLEnqueueWriteImage_PROC* clpfEnqueueWriteImage = NULL;
PFNCLEnqueueFillImage_PROC* clpfEnqueueFillImage = NULL;
PFNCLEnqueueCopyImage_PROC* clpfEnqueueCopyImage = NULL;
PFNCLEnqueueCopyImageToBuffer_PROC* clpfEnqueueCopyImageToBuffer = NULL;
PFNCLEnqueueCopyBufferToImage_PROC* clpfEnqueueCopyBufferToImage = NULL;
PFNCLEnqueueMapBuffer_PROC* clpfEnqueueMapBuffer = NULL;
PFNCLEnqueueMapImage_PROC* clpfEnqueueMapImage = NULL;
PFNCLEnqueueUnmapMemObject_PROC* clpfEnqueueUnmapMemObject = NULL;
PFNCLEnqueueMigrateMemObjects_PROC* clpfEnqueueMigrateMemObjects = NULL;
PFNCLEnqueueNDRangeKernel_PROC* clpfEnqueueNDRangeKernel = NULL;
PFNCLEnqueueTask_PROC* clpfEnqueueTask = NULL;
PFNCLEnqueueNativeKernel_PROC* clpfEnqueueNativeKernel = NULL;
PFNCLEnqueueMarkerWithWaitList_PROC* clpfEnqueueMarkerWithWaitList = NULL;
PFNCLEnqueueBarrierWithWaitList_PROC* clpfEnqueueBarrierWithWaitList = NULL;
PFNCLGetExtensionFunctionAddressForPlatform_PROC* clpfGetExtensionFunctionAddressForPlatform = NULL;
PFNCLCreateImage2D_PROC* clpfCreateImage2D = NULL;
PFNCLCreateImage3D_PROC* clpfCreateImage3D = NULL;
PFNCLEnqueueMarker_PROC* clpfEnqueueMarker = NULL;
PFNCLEnqueueWaitForEvents_PROC* clpfEnqueueWaitForEvents = NULL;
PFNCLEnqueueBarrier_PROC* clpfEnqueueBarrier = NULL;
PFNCLUnloadCompiler_PROC* clpfUnloadCompiler = NULL;
PFNCLGetExtensionFunctionAddress_PROC* clpfGetExtensionFunctionAddress = NULL;
PFNCLIcdGetPlatformIDs_PROC* clpfIcdGetPlatformIDs = NULL;
PFNCLCreateFromGLBuffer_PROC* clpfCreateFromGLBuffer = NULL;
PFNCLCreateFromGLTexture_PROC* clpfCreateFromGLTexture = NULL;
PFNCLCreateFromGLRenderbuffer_PROC* clpfCreateFromGLRenderbuffer = NULL;
PFNCLGetGLObjectInfo_PROC* clpfGetGLObjectInfo = NULL;
PFNCLGetGLTextureInfo_PROC* clpfGetGLTextureInfo = NULL;
PFNCLEnqueueAcquireGLObjects_PROC* clpfEnqueueAcquireGLObjects = NULL;
PFNCLEnqueueReleaseGLObjects_PROC* clpfEnqueueReleaseGLObjects = NULL;
PFNCLCreateFromGLTexture2D_PROC* clpfCreateFromGLTexture2D = NULL;
PFNCLCreateFromGLTexture3D_PROC* clpfCreateFromGLTexture3D = NULL;
#if cl_khr_gl_sharing
PFNCLGetGLContextInfoKHR_PROC* clpfGetGLContextInfoKHR = NULL;
#endif

#ifdef __cplusplus
}
#endif
