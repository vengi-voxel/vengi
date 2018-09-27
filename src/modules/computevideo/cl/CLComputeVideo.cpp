/**
 * @file
 *
 * @ingroup Compute
 */
#include "compute/cl/CLCompute.h"
#include "video/Buffer.h"
#include "video/Texture.h"
#include <CL/cl_gl.h>
#include <SDL.h>
#include <SDL_platform.h>

namespace computevideo {

bool init() {
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

compute::Id createTexture(compute::BufferFlag flags, video::Texture& texture) {
	// TODO: implement me
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
